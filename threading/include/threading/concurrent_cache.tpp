#pragma once

#include <functional>
#include <algorithm>

namespace mtfs::threading {

template<typename Key, typename Value>
ConcurrentCacheManager<Key, Value>::ConcurrentCacheManager(
    size_t capacity, 
    mtfs::cache::CachePolicy policy,
    size_t numShards)
    : numShards(numShards), threadPool(GlobalThreadPool::getInstance()) {
    
    // Ensure at least 1 shard
    numShards = std::max(size_t(1), numShards);
    
    // Distribute capacity among shards
    size_t capacityPerShard = std::max(size_t(1), capacity / numShards);
    
    // Create shards
    shards.reserve(numShards);
    for (size_t i = 0; i < numShards; ++i) {
        shards.push_back(std::make_unique<CacheShard>(capacityPerShard, policy));
    }
}

template<typename Key, typename Value>
size_t ConcurrentCacheManager<Key, Value>::getShardIndex(const Key& key) const {
    std::hash<Key> hasher;
    return hasher(key) % numShards;
}

template<typename Key, typename Value>
typename ConcurrentCacheManager<Key, Value>::CacheShard& 
ConcurrentCacheManager<Key, Value>::getShard(const Key& key) {
    return *shards[getShardIndex(key)];
}

template<typename Key, typename Value>
const typename ConcurrentCacheManager<Key, Value>::CacheShard& 
ConcurrentCacheManager<Key, Value>::getShard(const Key& key) const {
    return *shards[getShardIndex(key)];
}

template<typename Key, typename Value>
std::future<void> ConcurrentCacheManager<Key, Value>::putAsync(const Key& key, const Value& value) {
    return threadPool.enqueue([this, key, value]() {
        auto start = std::chrono::steady_clock::now();
        try {
            auto& shard = getShard(key);
            std::unique_lock<std::shared_mutex> lock(shard.mutex);
            shard.cache->put(key, value);
            
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - start
            );
            trackAsyncOperation(true, duration);
        } catch (...) {
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - start
            );
            trackAsyncOperation(false, duration);
            throw;
        }
    });
}

template<typename Key, typename Value>
std::future<Value> ConcurrentCacheManager<Key, Value>::getAsync(const Key& key) {
    return threadPool.enqueue([this, key]() -> Value {
        auto start = std::chrono::steady_clock::now();
        try {
            auto& shard = getShard(key);
            std::shared_lock<std::shared_mutex> lock(shard.mutex);
            auto result = shard.cache->get(key);
            
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - start
            );
            trackAsyncOperation(true, duration);
            return result;
        } catch (...) {
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - start
            );
            trackAsyncOperation(false, duration);
            throw;
        }
    });
}

template<typename Key, typename Value>
void ConcurrentCacheManager<Key, Value>::put(const Key& key, const Value& value) {
    auto& shard = getShard(key);
    std::unique_lock<std::shared_mutex> lock(shard.mutex);
    shard.cache->put(key, value);
}

template<typename Key, typename Value>
Value ConcurrentCacheManager<Key, Value>::get(const Key& key) {
    auto& shard = getShard(key);
    std::shared_lock<std::shared_mutex> lock(shard.mutex);
    return shard.cache->get(key);
}

template<typename Key, typename Value>
bool ConcurrentCacheManager<Key, Value>::contains(const Key& key) const {
    auto& shard = getShard(key);
    std::shared_lock<std::shared_mutex> lock(shard.mutex);
    return shard.cache->contains(key);
}

template<typename Key, typename Value>
void ConcurrentCacheManager<Key, Value>::remove(const Key& key) {
    auto& shard = getShard(key);
    std::unique_lock<std::shared_mutex> lock(shard.mutex);
    shard.cache->remove(key);
}

template<typename Key, typename Value>
void ConcurrentCacheManager<Key, Value>::clear() {
    for (auto& shard : shards) {
        std::unique_lock<std::shared_mutex> lock(shard->mutex);
        shard->cache->clear();
    }
}

template<typename Key, typename Value>
std::future<void> ConcurrentCacheManager<Key, Value>::putBatchAsync(
    const std::vector<std::pair<Key, Value>>& items) {
    
    return threadPool.enqueue([this, items]() {
        for (const auto& item : items) {
            put(item.first, item.second);
        }
    });
}

template<typename Key, typename Value>
typename ConcurrentCacheManager<Key, Value>::ConcurrentStats 
ConcurrentCacheManager<Key, Value>::getConcurrentStats() const {
    return concurrentStats;
}

template<typename Key, typename Value>
void ConcurrentCacheManager<Key, Value>::resetConcurrentStats() {
    concurrentStats = ConcurrentStats{};
}

template<typename Key, typename Value>
void ConcurrentCacheManager<Key, Value>::trackAsyncOperation(bool success, std::chrono::milliseconds duration) {
    concurrentStats.totalAsyncOperations++;
    if (success) {
        concurrentStats.completedAsyncOperations++;
    } else {
        concurrentStats.failedAsyncOperations++;
    }
    
    // Update average response time
    double newAvg = (concurrentStats.averageResponseTime.load() + duration.count()) / 2.0;
    concurrentStats.averageResponseTime = newAvg;
}

} // namespace mtfs::threading

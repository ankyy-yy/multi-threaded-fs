#pragma once

#include <memory>
#include <vector>
#include <future>
#include <atomic>
#include <shared_mutex>
#include "cache/enhanced_cache.hpp"
#include "threading/thread_pool.hpp"

namespace mtfs::threading {

template<typename Key, typename Value>
class ConcurrentCacheManager {
public:
    explicit ConcurrentCacheManager(
        size_t capacity, 
        mtfs::cache::CachePolicy policy = mtfs::cache::CachePolicy::LRU,
        size_t numShards = 16
    );
    
    ~ConcurrentCacheManager() = default;

    // Concurrent cache operations
    std::future<void> putAsync(const Key& key, const Value& value);
    std::future<Value> getAsync(const Key& key);
    std::future<bool> containsAsync(const Key& key);
    std::future<void> removeAsync(const Key& key);
    
    // Batch operations
    std::future<void> putBatchAsync(const std::vector<std::pair<Key, Value>>& items);
    std::future<std::vector<Value>> getBatchAsync(const std::vector<Key>& keys);
    std::future<void> removeBatchAsync(const std::vector<Key>& keys);
    
    // Synchronous operations (thread-safe)
    void put(const Key& key, const Value& value);
    Value get(const Key& key);
    bool contains(const Key& key) const;
    void remove(const Key& key);
    void clear();
    
    // Cache management
    void setPolicy(mtfs::cache::CachePolicy policy);
    mtfs::cache::CachePolicy getPolicy() const;
    void resize(size_t newCapacity);
    
    // Enhanced features
    std::future<void> pinAsync(const Key& key);
    std::future<void> unpinAsync(const Key& key);
    std::future<bool> isPinnedAsync(const Key& key);
    std::future<void> prefetchAsync(const Key& key, const Value& value);
    
    // Analytics and monitoring
    std::future<mtfs::cache::CacheStatistics> getStatisticsAsync();
    std::future<std::vector<Key>> getHotKeysAsync(size_t count = 10);
    std::future<void> resetStatisticsAsync();
    
    // Background operations
    void startBackgroundOptimization();
    void stopBackgroundOptimization();
    void schedulePeriodicCleanup(std::chrono::seconds interval);
    
    // Performance monitoring
    struct ConcurrentStats {
        std::atomic<size_t> concurrentReads{0};
        std::atomic<size_t> concurrentWrites{0};
        std::atomic<size_t> totalAsyncOperations{0};
        std::atomic<size_t> completedAsyncOperations{0};
        std::atomic<size_t> failedAsyncOperations{0};
        std::atomic<double> averageResponseTime{0.0};
        
        double getCompletionRate() const {
            return totalAsyncOperations > 0 ? 
                (static_cast<double>(completedAsyncOperations) / totalAsyncOperations) * 100.0 : 0.0;
        }
        
        double getFailureRate() const {
            return totalAsyncOperations > 0 ?
                (static_cast<double>(failedAsyncOperations) / totalAsyncOperations) * 100.0 : 0.0;
        }
    };
    
    ConcurrentStats getConcurrentStats() const;
    void resetConcurrentStats();
    
    // Cache warming
    std::future<void> warmupCacheAsync(const std::vector<std::pair<Key, Value>>& preloadData);
    
    // Cache eviction strategies
    std::future<void> forceEvictionAsync(size_t targetSize);
    std::future<size_t> getOptimalSizeAsync();

private:
    // Sharded cache implementation for better concurrency
    struct CacheShard {
        std::unique_ptr<mtfs::cache::CacheManager<Key, Value>> cache;
        mutable std::shared_mutex mutex;
        
        CacheShard(size_t capacity, mtfs::cache::CachePolicy policy)
            : cache(std::make_unique<mtfs::cache::CacheManager<Key, Value>>(capacity, policy)) {}
    };
    
    std::vector<std::unique_ptr<CacheShard>> shards;
    size_t numShards;
    ThreadPool& threadPool;
    
    // Background processing
    std::atomic<bool> backgroundOptimizationEnabled{false};
    std::atomic<bool> periodicCleanupEnabled{false};
    std::unique_ptr<std::thread> optimizationThread;
    std::unique_ptr<std::thread> cleanupThread;
    
    // Statistics
    mutable ConcurrentStats concurrentStats;
    
    // Hash function for sharding
    size_t getShardIndex(const Key& key) const;
    CacheShard& getShard(const Key& key);
    const CacheShard& getShard(const Key& key) const;
    
    // Background operations
    void backgroundOptimizationLoop();
    void periodicCleanupLoop(std::chrono::seconds interval);
    
    // Statistics tracking
    void trackAsyncOperation(bool success, std::chrono::milliseconds duration);
    
    // Batch operation helpers
    template<typename Operation>
    auto executeBatchOperation(const std::vector<Key>& keys, Operation op) 
        -> std::future<std::vector<decltype(op(keys[0]))>>;
};

} // namespace mtfs::threading

// Template implementation
#include "concurrent_cache.tpp"

#pragma once

#include <memory>
#include <mutex>
#include <list>
#include <unordered_map>
#include <unordered_set>
#include <queue>
#include <stack>
#include <chrono>
#include "common/error.hpp"

namespace mtfs::cache {

// Cache policy types
enum class CachePolicy {
    LRU,  // Least Recently Used
    LFU,  // Least Frequently Used
    FIFO, // First In First Out
    LIFO  // Last In First Out
};

// Cache entry with metadata
template<typename Key, typename Value>
struct CacheEntry {
    Key key;
    Value value;
    size_t accessCount{0};
    std::chrono::system_clock::time_point lastAccessed;
    std::chrono::system_clock::time_point createdAt;
    bool isPinned{false};
    
    CacheEntry() : lastAccessed(std::chrono::system_clock::now()),
                   createdAt(std::chrono::system_clock::now()) {}
    
    CacheEntry(const Key& k, const Value& v) 
        : key(k), value(v), 
          lastAccessed(std::chrono::system_clock::now()),
          createdAt(std::chrono::system_clock::now()) {}
};

// Hot file information structure
template<typename Key, typename Value>
struct HotFileInfo {
    Key key;
    size_t accessCount;
    std::chrono::system_clock::time_point lastAccessed;
    std::chrono::duration<double> ageInCache;
    bool isPinned;
    double accessFrequency;  // accesses per second
};

// Cache statistics
struct CacheStatistics {
    size_t hits{0};
    size_t misses{0};
    size_t evictions{0};
    size_t totalAccesses{0};
    size_t pinnedItems{0};
    size_t prefetchedItems{0};
    double hitRate{0.0};
    std::chrono::system_clock::time_point lastResetTime;
    
    CacheStatistics() : lastResetTime(std::chrono::system_clock::now()) {}
    
    void updateHitRate() {
        totalAccesses = hits + misses;
        hitRate = totalAccesses > 0 ? (static_cast<double>(hits) / totalAccesses) * 100.0 : 0.0;
    }
};

// Base cache interface
template<typename Key, typename Value>
class CacheInterface {
public:
    virtual ~CacheInterface() = default;
    
    virtual void put(const Key& key, const Value& value) = 0;
    virtual Value get(const Key& key) = 0;
    virtual bool contains(const Key& key) const = 0;
    virtual void remove(const Key& key) = 0;
    virtual void clear() = 0;
    virtual size_t size() const = 0;
    virtual size_t capacity() const = 0;
    virtual CacheStatistics getStatistics() const = 0;
    virtual void resetStatistics() = 0;
    
    // Enhanced features
    virtual void pin(const Key& key) = 0;
    virtual void unpin(const Key& key) = 0;
    virtual bool isPinned(const Key& key) const = 0;
    virtual void prefetch(const Key& key, const Value& value) = 0;
    virtual std::vector<Key> getKeys() const = 0;
};

// Enhanced LRU Cache
template<typename Key, typename Value>
class EnhancedLRUCache : public CacheInterface<Key, Value> {
public:
    explicit EnhancedLRUCache(size_t capacity);
    
    void put(const Key& key, const Value& value) override;
    Value get(const Key& key) override;
    bool contains(const Key& key) const override;
    void remove(const Key& key) override;
    void clear() override;
    size_t size() const override;
    size_t capacity() const override;
    CacheStatistics getStatistics() const override;
    void resetStatistics() override;
    
    // Enhanced features
    void pin(const Key& key) override;
    void unpin(const Key& key) override;
    bool isPinned(const Key& key) const override;
    void prefetch(const Key& key, const Value& value) override;
    std::vector<Key> getKeys() const override;

private:
    using EntryType = CacheEntry<Key, Value>;
    using EntryList = std::list<EntryType>;
    using EntryMap = std::unordered_map<Key, typename EntryList::iterator>;
    
    void evict();
    void moveToFront(typename EntryList::iterator it);
    
    const size_t maxCapacity;
    EntryList entries;
    EntryMap lookup;
    std::unordered_set<Key> pinnedKeys;
    mutable std::mutex cacheMutex;
    mutable CacheStatistics stats;
};

// LFU Cache implementation
template<typename Key, typename Value>
class LFUCache : public CacheInterface<Key, Value> {
public:
    explicit LFUCache(size_t capacity);
    
    void put(const Key& key, const Value& value) override;
    Value get(const Key& key) override;
    bool contains(const Key& key) const override;
    void remove(const Key& key) override;
    void clear() override;
    size_t size() const override;
    size_t capacity() const override;
    CacheStatistics getStatistics() const override;
    void resetStatistics() override;
    
    // Enhanced features
    void pin(const Key& key) override;
    void unpin(const Key& key) override;
    bool isPinned(const Key& key) const override;
    void prefetch(const Key& key, const Value& value) override;
    std::vector<Key> getKeys() const override;

private:
    using EntryType = CacheEntry<Key, Value>;
    using FrequencyMap = std::unordered_map<size_t, std::list<Key>>;
    using KeyMap = std::unordered_map<Key, EntryType>;
    using KeyFreqMap = std::unordered_map<Key, size_t>;
    
    void evict();
    void updateFrequency(const Key& key);
    
    const size_t maxCapacity;
    size_t minFrequency{1};
    FrequencyMap frequencies;
    KeyMap keyToEntry;
    KeyFreqMap keyToFreq;
    std::unordered_set<Key> pinnedKeys;
    mutable std::mutex cacheMutex;
    mutable CacheStatistics stats;
};

// FIFO Cache implementation
template<typename Key, typename Value>
class FIFOCache : public CacheInterface<Key, Value> {
public:
    explicit FIFOCache(size_t capacity);
    
    void put(const Key& key, const Value& value) override;
    Value get(const Key& key) override;
    bool contains(const Key& key) const override;
    void remove(const Key& key) override;
    void clear() override;
    size_t size() const override;
    size_t capacity() const override;
    CacheStatistics getStatistics() const override;
    void resetStatistics() override;
    
    // Enhanced features
    void pin(const Key& key) override;
    void unpin(const Key& key) override;
    bool isPinned(const Key& key) const override;
    void prefetch(const Key& key, const Value& value) override;
    std::vector<Key> getKeys() const override;

private:
    using EntryType = CacheEntry<Key, Value>;
    using EntryQueue = std::queue<Key>;
    using EntryMap = std::unordered_map<Key, EntryType>;
    
    void evict();
    
    const size_t maxCapacity;
    EntryQueue insertionOrder;
    EntryMap entries;
    std::unordered_set<Key> pinnedKeys;
    mutable std::mutex cacheMutex;
    mutable CacheStatistics stats;
};

// LIFO Cache implementation
template<typename Key, typename Value>
class LIFOCache : public CacheInterface<Key, Value> {
public:
    explicit LIFOCache(size_t capacity);
    
    void put(const Key& key, const Value& value) override;
    Value get(const Key& key) override;
    bool contains(const Key& key) const override;
    void remove(const Key& key) override;
    void clear() override;
    size_t size() const override;
    size_t capacity() const override;
    CacheStatistics getStatistics() const override;
    void resetStatistics() override;
    
    // Enhanced features
    void pin(const Key& key) override;
    void unpin(const Key& key) override;
    bool isPinned(const Key& key) const override;
    void prefetch(const Key& key, const Value& value) override;
    std::vector<Key> getKeys() const override;

private:
    using EntryType = CacheEntry<Key, Value>;
    using EntryStack = std::stack<Key>;
    using EntryMap = std::unordered_map<Key, EntryType>;
    
    void evict();
    
    const size_t maxCapacity;
    EntryStack insertionOrder;
    EntryMap entries;
    std::unordered_set<Key> pinnedKeys;
    mutable std::mutex cacheMutex;
    mutable CacheStatistics stats;
};

// Cache manager to handle different policies
template<typename Key, typename Value>
class CacheManager {
public:
    explicit CacheManager(size_t capacity, CachePolicy policy = CachePolicy::LRU);
    
    // Cache operations
    void put(const Key& key, const Value& value);
    Value get(const Key& key);
    bool contains(const Key& key) const;
    void remove(const Key& key);
    void clear();
    
    // Cache management
    void setPolicy(CachePolicy policy);
    CachePolicy getPolicy() const;
    void resize(size_t newCapacity);
    
    // Enhanced features
    void pin(const Key& key);
    void unpin(const Key& key);
    bool isPinned(const Key& key) const;
    void prefetch(const Key& key, const Value& value);
      // Analytics
    CacheStatistics getStatistics() const;
    void resetStatistics();
    void showCacheAnalytics() const;
    std::vector<Key> getHotKeys(size_t count = 10) const;
    
    // Hot file analytics
    void showHotFileAnalytics(size_t topCount = 10) const;
    std::vector<HotFileInfo<Key, Value>> getHotFileDetails(size_t count = 10) const;
    void trackAccessPattern(const Key& key);
    void monitorPerformance();
    
    // Maintenance
    void warmup(const std::vector<std::pair<Key, Value>>& data);
    void optimizeForWorkload();

private:
    void recreateCache();
    
    size_t cacheCapacity;
    CachePolicy currentPolicy;
    std::unique_ptr<CacheInterface<Key, Value>> cache;
    mutable std::mutex managerMutex;
};

} // namespace mtfs::cache

// Include implementation files
#include "enhanced_cache.tpp"

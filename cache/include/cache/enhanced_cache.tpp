#pragma once

#include <algorithm>
#include <iostream>
#include <iomanip>
#include <sstream>

namespace mtfs::cache {

// ===== EnhancedLRUCache Implementation =====

template<typename Key, typename Value>
EnhancedLRUCache<Key, Value>::EnhancedLRUCache(size_t capacity) 
    : maxCapacity(capacity) {}

template<typename Key, typename Value>
void EnhancedLRUCache<Key, Value>::put(const Key& key, const Value& value) {
    std::lock_guard<std::mutex> lock(cacheMutex);
    
    auto it = lookup.find(key);
    if (it != lookup.end()) {
        // Update existing entry
        it->second->value = value;
        it->second->lastAccessed = std::chrono::system_clock::now();
        moveToFront(it->second);
        return;
    }
    
    // Add new entry
    if (entries.size() >= maxCapacity) {
        evict();
    }
    
    entries.emplace_front(key, value);
    lookup[key] = entries.begin();
}

template<typename Key, typename Value>
Value EnhancedLRUCache<Key, Value>::get(const Key& key) {
    std::lock_guard<std::mutex> lock(cacheMutex);
    
    auto it = lookup.find(key);
    if (it == lookup.end()) {
        stats.misses++;
        stats.updateHitRate();
        throw std::runtime_error("Key not found in cache");
    }
    
    // Update access info
    it->second->accessCount++;
    it->second->lastAccessed = std::chrono::system_clock::now();
    moveToFront(it->second);
    
    stats.hits++;
    stats.updateHitRate();
    return it->second->value;
}

template<typename Key, typename Value>
bool EnhancedLRUCache<Key, Value>::contains(const Key& key) const {
    std::lock_guard<std::mutex> lock(cacheMutex);
    return lookup.find(key) != lookup.end();
}

template<typename Key, typename Value>
void EnhancedLRUCache<Key, Value>::remove(const Key& key) {
    std::lock_guard<std::mutex> lock(cacheMutex);
    
    auto it = lookup.find(key);
    if (it != lookup.end()) {
        entries.erase(it->second);
        lookup.erase(it);
        pinnedKeys.erase(key);
    }
}

template<typename Key, typename Value>
void EnhancedLRUCache<Key, Value>::clear() {
    std::lock_guard<std::mutex> lock(cacheMutex);
    entries.clear();
    lookup.clear();
    pinnedKeys.clear();
}

template<typename Key, typename Value>
size_t EnhancedLRUCache<Key, Value>::size() const {
    std::lock_guard<std::mutex> lock(cacheMutex);
    return entries.size();
}

template<typename Key, typename Value>
size_t EnhancedLRUCache<Key, Value>::capacity() const {
    return maxCapacity;
}

template<typename Key, typename Value>
CacheStatistics EnhancedLRUCache<Key, Value>::getStatistics() const {
    std::lock_guard<std::mutex> lock(cacheMutex);
    auto statsCopy = stats;
    statsCopy.pinnedItems = pinnedKeys.size();
    return statsCopy;
}

template<typename Key, typename Value>
void EnhancedLRUCache<Key, Value>::resetStatistics() {
    std::lock_guard<std::mutex> lock(cacheMutex);
    stats = CacheStatistics();
}

template<typename Key, typename Value>
void EnhancedLRUCache<Key, Value>::pin(const Key& key) {
    std::lock_guard<std::mutex> lock(cacheMutex);
    if (lookup.find(key) != lookup.end()) {
        pinnedKeys.insert(key);
    }
}

template<typename Key, typename Value>
void EnhancedLRUCache<Key, Value>::unpin(const Key& key) {
    std::lock_guard<std::mutex> lock(cacheMutex);
    pinnedKeys.erase(key);
}

template<typename Key, typename Value>
bool EnhancedLRUCache<Key, Value>::isPinned(const Key& key) const {
    std::lock_guard<std::mutex> lock(cacheMutex);
    return pinnedKeys.find(key) != pinnedKeys.end();
}

template<typename Key, typename Value>
void EnhancedLRUCache<Key, Value>::prefetch(const Key& key, const Value& value) {
    std::lock_guard<std::mutex> lock(cacheMutex);
    
    if (lookup.find(key) == lookup.end()) {
        // Key doesn't exist, add it if there's space
        if (entries.size() < maxCapacity) {
            entries.emplace_front(key, value);
            lookup[key] = entries.begin();
            stats.prefetchedItems++;
        } else {
            // Need to evict, but treat as prefetch
            evict();
            entries.emplace_front(key, value);
            lookup[key] = entries.begin();
            stats.prefetchedItems++;
        }
    } else {
        // Key exists, update value and count as prefetch
        auto it = lookup.find(key);
        it->second->value = value;
        it->second->lastAccessed = std::chrono::system_clock::now();
        moveToFront(it->second);
        stats.prefetchedItems++;
    }
}

template<typename Key, typename Value>
std::vector<Key> EnhancedLRUCache<Key, Value>::getKeys() const {
    std::lock_guard<std::mutex> lock(cacheMutex);
    std::vector<Key> keys;
    for (const auto& entry : entries) {
        keys.push_back(entry.key);
    }
    return keys;
}

template<typename Key, typename Value>
void EnhancedLRUCache<Key, Value>::evict() {
    while (!entries.empty()) {
        auto lastIt = std::prev(entries.end());
        if (pinnedKeys.find(lastIt->key) == pinnedKeys.end()) {
            lookup.erase(lastIt->key);
            entries.erase(lastIt);
            stats.evictions++;
            break;
        }
        
        // Move pinned item to front and try next
        moveToFront(lastIt);
        if (entries.size() <= 1) break; // Prevent infinite loop
    }
}

template<typename Key, typename Value>
void EnhancedLRUCache<Key, Value>::moveToFront(typename EntryList::iterator it) {
    if (it != entries.begin()) {
        entries.splice(entries.begin(), entries, it);
    }
}

// ===== LFUCache Implementation =====

template<typename Key, typename Value>
LFUCache<Key, Value>::LFUCache(size_t capacity) : maxCapacity(capacity) {}

template<typename Key, typename Value>
void LFUCache<Key, Value>::put(const Key& key, const Value& value) {
    std::lock_guard<std::mutex> lock(cacheMutex);
    
    if (maxCapacity == 0) return;
    
    auto it = keyToEntry.find(key);
    if (it != keyToEntry.end()) {
        // Update existing entry
        it->second.value = value;
        it->second.lastAccessed = std::chrono::system_clock::now();
        updateFrequency(key);
        return;
    }
    
    // Add new entry
    if (keyToEntry.size() >= maxCapacity) {
        evict();
    }
    
    keyToEntry[key] = EntryType(key, value);
    keyToFreq[key] = 1;
    frequencies[1].push_back(key);
    minFrequency = 1;
}

template<typename Key, typename Value>
Value LFUCache<Key, Value>::get(const Key& key) {
    std::lock_guard<std::mutex> lock(cacheMutex);
    
    auto it = keyToEntry.find(key);
    if (it == keyToEntry.end()) {
        stats.misses++;
        stats.updateHitRate();
        throw std::runtime_error("Key not found in cache");
    }
    
    // Update access info
    it->second.accessCount++;
    it->second.lastAccessed = std::chrono::system_clock::now();
    updateFrequency(key);
    
    stats.hits++;
    stats.updateHitRate();
    return it->second.value;
}

template<typename Key, typename Value>
bool LFUCache<Key, Value>::contains(const Key& key) const {
    std::lock_guard<std::mutex> lock(cacheMutex);
    return keyToEntry.find(key) != keyToEntry.end();
}

template<typename Key, typename Value>
void LFUCache<Key, Value>::remove(const Key& key) {
    std::lock_guard<std::mutex> lock(cacheMutex);
    
    auto it = keyToEntry.find(key);
    if (it != keyToEntry.end()) {
        size_t freq = keyToFreq[key];
        auto& freqList = frequencies[freq];
        freqList.remove(key);
        if (freqList.empty() && freq == minFrequency) {
            minFrequency++;
        }
        
        keyToEntry.erase(key);
        keyToFreq.erase(key);
        pinnedKeys.erase(key);
    }
}

template<typename Key, typename Value>
void LFUCache<Key, Value>::clear() {
    std::lock_guard<std::mutex> lock(cacheMutex);
    keyToEntry.clear();
    keyToFreq.clear();
    frequencies.clear();
    pinnedKeys.clear();
    minFrequency = 1;
}

template<typename Key, typename Value>
size_t LFUCache<Key, Value>::size() const {
    std::lock_guard<std::mutex> lock(cacheMutex);
    return keyToEntry.size();
}

template<typename Key, typename Value>
size_t LFUCache<Key, Value>::capacity() const {
    return maxCapacity;
}

template<typename Key, typename Value>
CacheStatistics LFUCache<Key, Value>::getStatistics() const {
    std::lock_guard<std::mutex> lock(cacheMutex);
    auto statsCopy = stats;
    statsCopy.pinnedItems = pinnedKeys.size();
    return statsCopy;
}

template<typename Key, typename Value>
void LFUCache<Key, Value>::resetStatistics() {
    std::lock_guard<std::mutex> lock(cacheMutex);
    stats = CacheStatistics();
}

template<typename Key, typename Value>
void LFUCache<Key, Value>::pin(const Key& key) {
    std::lock_guard<std::mutex> lock(cacheMutex);
    if (keyToEntry.find(key) != keyToEntry.end()) {
        pinnedKeys.insert(key);
    }
}

template<typename Key, typename Value>
void LFUCache<Key, Value>::unpin(const Key& key) {
    std::lock_guard<std::mutex> lock(cacheMutex);
    pinnedKeys.erase(key);
}

template<typename Key, typename Value>
bool LFUCache<Key, Value>::isPinned(const Key& key) const {
    std::lock_guard<std::mutex> lock(cacheMutex);
    return pinnedKeys.find(key) != pinnedKeys.end();
}

template<typename Key, typename Value>
void LFUCache<Key, Value>::prefetch(const Key& key, const Value& value) {
    std::lock_guard<std::mutex> lock(cacheMutex);
    
    auto it = keyToEntry.find(key);
    if (it == keyToEntry.end()) {
        // Key doesn't exist, add it
        if (keyToEntry.size() >= maxCapacity) {
            evict();
        }
        keyToEntry[key] = EntryType(key, value);
        keyToFreq[key] = 1;
        frequencies[1].push_back(key);
        minFrequency = 1;
        stats.prefetchedItems++;
    } else {
        // Key exists, update value and count as prefetch
        it->second.value = value;
        it->second.lastAccessed = std::chrono::system_clock::now();
        stats.prefetchedItems++;
    }
}

template<typename Key, typename Value>
std::vector<Key> LFUCache<Key, Value>::getKeys() const {
    std::lock_guard<std::mutex> lock(cacheMutex);
    std::vector<Key> keys;
    for (const auto& pair : keyToEntry) {
        keys.push_back(pair.first);
    }
    return keys;
}

template<typename Key, typename Value>
void LFUCache<Key, Value>::evict() {
    // Find least frequent unpinned key
    for (auto freqIt = frequencies.find(minFrequency); 
         freqIt != frequencies.end(); ++freqIt) {
        auto& keyList = freqIt->second;
        
        for (auto keyIt = keyList.begin(); keyIt != keyList.end(); ++keyIt) {
            if (pinnedKeys.find(*keyIt) == pinnedKeys.end()) {
                Key keyToRemove = *keyIt;
                keyList.erase(keyIt);
                keyToEntry.erase(keyToRemove);
                keyToFreq.erase(keyToRemove);
                stats.evictions++;
                
                if (keyList.empty() && freqIt->first == minFrequency) {
                    minFrequency++;
                }
                return;
            }
        }
    }
}

template<typename Key, typename Value>
void LFUCache<Key, Value>::updateFrequency(const Key& key) {
    size_t oldFreq = keyToFreq[key];
    size_t newFreq = oldFreq + 1;
    
    // Remove from old frequency list
    frequencies[oldFreq].remove(key);
    if (frequencies[oldFreq].empty() && oldFreq == minFrequency) {
        minFrequency++;
    }
    
    // Add to new frequency list
    keyToFreq[key] = newFreq;
    frequencies[newFreq].push_back(key);
}

// ===== FIFOCache Implementation =====

template<typename Key, typename Value>
FIFOCache<Key, Value>::FIFOCache(size_t capacity) : maxCapacity(capacity) {}

template<typename Key, typename Value>
void FIFOCache<Key, Value>::put(const Key& key, const Value& value) {
    std::lock_guard<std::mutex> lock(cacheMutex);
    
    auto it = entries.find(key);
    if (it != entries.end()) {
        // Update existing entry
        it->second.value = value;
        it->second.lastAccessed = std::chrono::system_clock::now();
        return;
    }
    
    // Add new entry
    if (entries.size() >= maxCapacity) {
        evict();
    }
    
    entries[key] = EntryType(key, value);
    insertionOrder.push(key);
}

template<typename Key, typename Value>
Value FIFOCache<Key, Value>::get(const Key& key) {
    std::lock_guard<std::mutex> lock(cacheMutex);
    
    auto it = entries.find(key);
    if (it == entries.end()) {
        stats.misses++;
        stats.updateHitRate();
        throw std::runtime_error("Key not found in cache");
    }
    
    // Update access info
    it->second.accessCount++;
    it->second.lastAccessed = std::chrono::system_clock::now();
    
    stats.hits++;
    stats.updateHitRate();
    return it->second.value;
}

template<typename Key, typename Value>
bool FIFOCache<Key, Value>::contains(const Key& key) const {
    std::lock_guard<std::mutex> lock(cacheMutex);
    return entries.find(key) != entries.end();
}

template<typename Key, typename Value>
void FIFOCache<Key, Value>::remove(const Key& key) {
    std::lock_guard<std::mutex> lock(cacheMutex);
    
    auto it = entries.find(key);
    if (it != entries.end()) {
        entries.erase(it);
        pinnedKeys.erase(key);
        // Note: Can't efficiently remove from queue, will be handled in evict
    }
}

template<typename Key, typename Value>
void FIFOCache<Key, Value>::clear() {
    std::lock_guard<std::mutex> lock(cacheMutex);
    entries.clear();
    pinnedKeys.clear();
    insertionOrder = EntryQueue(); // Clear queue
}

template<typename Key, typename Value>
size_t FIFOCache<Key, Value>::size() const {
    std::lock_guard<std::mutex> lock(cacheMutex);
    return entries.size();
}

template<typename Key, typename Value>
size_t FIFOCache<Key, Value>::capacity() const {
    return maxCapacity;
}

template<typename Key, typename Value>
CacheStatistics FIFOCache<Key, Value>::getStatistics() const {
    std::lock_guard<std::mutex> lock(cacheMutex);
    auto statsCopy = stats;
    statsCopy.pinnedItems = pinnedKeys.size();
    return statsCopy;
}

template<typename Key, typename Value>
void FIFOCache<Key, Value>::resetStatistics() {
    std::lock_guard<std::mutex> lock(cacheMutex);
    stats = CacheStatistics();
}

template<typename Key, typename Value>
void FIFOCache<Key, Value>::pin(const Key& key) {
    std::lock_guard<std::mutex> lock(cacheMutex);
    if (entries.find(key) != entries.end()) {
        pinnedKeys.insert(key);
    }
}

template<typename Key, typename Value>
void FIFOCache<Key, Value>::unpin(const Key& key) {
    std::lock_guard<std::mutex> lock(cacheMutex);
    pinnedKeys.erase(key);
}

template<typename Key, typename Value>
bool FIFOCache<Key, Value>::isPinned(const Key& key) const {
    std::lock_guard<std::mutex> lock(cacheMutex);
    return pinnedKeys.find(key) != pinnedKeys.end();
}

template<typename Key, typename Value>
void FIFOCache<Key, Value>::prefetch(const Key& key, const Value& value) {
    std::lock_guard<std::mutex> lock(cacheMutex);
    
    auto it = entries.find(key);
    if (it == entries.end()) {
        // Key doesn't exist, add it
        if (entries.size() >= maxCapacity) {
            evict();
        }
        entries[key] = EntryType(key, value);
        insertionOrder.push(key);
        stats.prefetchedItems++;
    } else {
        // Key exists, update value and count as prefetch
        it->second.value = value;
        it->second.lastAccessed = std::chrono::system_clock::now();
        stats.prefetchedItems++;
    }
}

template<typename Key, typename Value>
std::vector<Key> FIFOCache<Key, Value>::getKeys() const {
    std::lock_guard<std::mutex> lock(cacheMutex);
    std::vector<Key> keys;
    for (const auto& pair : entries) {
        keys.push_back(pair.first);
    }
    return keys;
}

template<typename Key, typename Value>
void FIFOCache<Key, Value>::evict() {
    while (!insertionOrder.empty()) {
        Key oldestKey = insertionOrder.front();
        insertionOrder.pop();
        
        // Check if key still exists and is not pinned
        auto it = entries.find(oldestKey);
        if (it != entries.end() && pinnedKeys.find(oldestKey) == pinnedKeys.end()) {
            entries.erase(it);
            stats.evictions++;
            break;
        }
    }
}

// ===== LIFOCache Implementation =====

template<typename Key, typename Value>
LIFOCache<Key, Value>::LIFOCache(size_t capacity) : maxCapacity(capacity) {}

template<typename Key, typename Value>
void LIFOCache<Key, Value>::put(const Key& key, const Value& value) {
    std::lock_guard<std::mutex> lock(cacheMutex);
    
    auto it = entries.find(key);
    if (it != entries.end()) {
        // Update existing entry
        it->second.value = value;
        it->second.lastAccessed = std::chrono::system_clock::now();
        
        // Remove from current position in stack
        auto& stack = insertionOrder;
        // Create new stack without this key
        EntryStack newStack;
        while (!stack.empty()) {
            if (stack.top() != key) {
                newStack.push(stack.top());
            }
            stack.pop();
        }
        // Reverse back to original order (except removed key)
        while (!newStack.empty()) {
            stack.push(newStack.top());
            newStack.pop();
        }
        // Add updated key to top (most recent)
        stack.push(key);
        return;
    }
    
    // Add new entry
    if (entries.size() >= maxCapacity) {
        evict();
    }
    
    entries[key] = EntryType(key, value);
    insertionOrder.push(key);  // Push to top of stack
}

template<typename Key, typename Value>
Value LIFOCache<Key, Value>::get(const Key& key) {
    std::lock_guard<std::mutex> lock(cacheMutex);
    
    auto it = entries.find(key);
    if (it == entries.end()) {
        stats.misses++;
        stats.updateHitRate();
        throw std::runtime_error("Key not found in cache");
    }
    
    // Update access info (but don't change LIFO order)
    it->second.accessCount++;
    it->second.lastAccessed = std::chrono::system_clock::now();
    
    stats.hits++;
    stats.updateHitRate();
    return it->second.value;
}

template<typename Key, typename Value>
bool LIFOCache<Key, Value>::contains(const Key& key) const {
    std::lock_guard<std::mutex> lock(cacheMutex);
    return entries.find(key) != entries.end();
}

template<typename Key, typename Value>
void LIFOCache<Key, Value>::remove(const Key& key) {
    std::lock_guard<std::mutex> lock(cacheMutex);
    
    auto it = entries.find(key);
    if (it != entries.end()) {
        entries.erase(it);
        pinnedKeys.erase(key);
        
        // Remove from stack (similar to put method)
        auto& stack = insertionOrder;
        EntryStack newStack;
        while (!stack.empty()) {
            if (stack.top() != key) {
                newStack.push(stack.top());
            }
            stack.pop();
        }
        while (!newStack.empty()) {
            stack.push(newStack.top());
            newStack.pop();
        }
    }
}

template<typename Key, typename Value>
void LIFOCache<Key, Value>::clear() {
    std::lock_guard<std::mutex> lock(cacheMutex);
    entries.clear();
    pinnedKeys.clear();
    insertionOrder = EntryStack(); // Clear stack
}

template<typename Key, typename Value>
size_t LIFOCache<Key, Value>::size() const {
    std::lock_guard<std::mutex> lock(cacheMutex);
    return entries.size();
}

template<typename Key, typename Value>
size_t LIFOCache<Key, Value>::capacity() const {
    return maxCapacity;
}

template<typename Key, typename Value>
CacheStatistics LIFOCache<Key, Value>::getStatistics() const {
    std::lock_guard<std::mutex> lock(cacheMutex);
    auto statsCopy = stats;
    statsCopy.pinnedItems = pinnedKeys.size();
    return statsCopy;
}

template<typename Key, typename Value>
void LIFOCache<Key, Value>::resetStatistics() {
    std::lock_guard<std::mutex> lock(cacheMutex);
    stats = CacheStatistics();
}

template<typename Key, typename Value>
void LIFOCache<Key, Value>::pin(const Key& key) {
    std::lock_guard<std::mutex> lock(cacheMutex);
    if (entries.find(key) != entries.end()) {
        pinnedKeys.insert(key);
    }
}

template<typename Key, typename Value>
void LIFOCache<Key, Value>::unpin(const Key& key) {
    std::lock_guard<std::mutex> lock(cacheMutex);
    pinnedKeys.erase(key);
}

template<typename Key, typename Value>
bool LIFOCache<Key, Value>::isPinned(const Key& key) const {
    std::lock_guard<std::mutex> lock(cacheMutex);
    return pinnedKeys.find(key) != pinnedKeys.end();
}

template<typename Key, typename Value>
void LIFOCache<Key, Value>::prefetch(const Key& key, const Value& value) {
    std::lock_guard<std::mutex> lock(cacheMutex);
    
    auto it = entries.find(key);
    if (it == entries.end()) {
        // Key doesn't exist, add it
        if (entries.size() >= maxCapacity) {
            evict();
        }
        entries[key] = EntryType(key, value);
        insertionOrder.push(key);  // Add to top of stack
        stats.prefetchedItems++;
    } else {
        // Key exists, update value and count as prefetch
        it->second.value = value;
        it->second.lastAccessed = std::chrono::system_clock::now();
        stats.prefetchedItems++;
    }
}

template<typename Key, typename Value>
std::vector<Key> LIFOCache<Key, Value>::getKeys() const {
    std::lock_guard<std::mutex> lock(cacheMutex);
    std::vector<Key> keys;
    for (const auto& pair : entries) {
        keys.push_back(pair.first);
    }
    return keys;
}

template<typename Key, typename Value>
void LIFOCache<Key, Value>::evict() {
    // LIFO: Remove the most recently added unpinned item (top of stack)
    EntryStack tempStack;
    Key keyToRemove;
    bool found = false;
    
    // Find the most recent unpinned item
    while (!insertionOrder.empty()) {
        Key topKey = insertionOrder.top();
        insertionOrder.pop();
        
        auto it = entries.find(topKey);
        if (it != entries.end() && pinnedKeys.find(topKey) == pinnedKeys.end()) {
            // Found unpinned item to remove
            keyToRemove = topKey;
            found = true;
            break;
        } else {
            // Keep this item, store temporarily
            tempStack.push(topKey);
        }
    }
    
    // Restore the items we temporarily removed
    while (!tempStack.empty()) {
        insertionOrder.push(tempStack.top());
        tempStack.pop();
    }
    
    // Remove the found item
    if (found) {
        entries.erase(keyToRemove);
        stats.evictions++;
    }
}

// ===== CacheManager Implementation =====

template<typename Key, typename Value>
CacheManager<Key, Value>::CacheManager(size_t capacity, CachePolicy policy) 
    : cacheCapacity(capacity), currentPolicy(policy) {
    recreateCache();
}

template<typename Key, typename Value>
void CacheManager<Key, Value>::put(const Key& key, const Value& value) {
    std::lock_guard<std::mutex> lock(managerMutex);
    cache->put(key, value);
}

template<typename Key, typename Value>
Value CacheManager<Key, Value>::get(const Key& key) {
    std::lock_guard<std::mutex> lock(managerMutex);
    return cache->get(key);
}

template<typename Key, typename Value>
bool CacheManager<Key, Value>::contains(const Key& key) const {
    std::lock_guard<std::mutex> lock(managerMutex);
    return cache->contains(key);
}

template<typename Key, typename Value>
void CacheManager<Key, Value>::remove(const Key& key) {
    std::lock_guard<std::mutex> lock(managerMutex);
    cache->remove(key);
}

template<typename Key, typename Value>
void CacheManager<Key, Value>::clear() {
    std::lock_guard<std::mutex> lock(managerMutex);
    cache->clear();
}

template<typename Key, typename Value>
void CacheManager<Key, Value>::setPolicy(CachePolicy policy) {
    std::lock_guard<std::mutex> lock(managerMutex);
    if (currentPolicy != policy) {
        currentPolicy = policy;
        recreateCache();
    }
}

template<typename Key, typename Value>
CachePolicy CacheManager<Key, Value>::getPolicy() const {
    std::lock_guard<std::mutex> lock(managerMutex);
    return currentPolicy;
}

template<typename Key, typename Value>
void CacheManager<Key, Value>::resize(size_t newCapacity) {
    std::lock_guard<std::mutex> lock(managerMutex);
    if (cacheCapacity != newCapacity) {
        cacheCapacity = newCapacity;
        recreateCache();
    }
}

template<typename Key, typename Value>
void CacheManager<Key, Value>::pin(const Key& key) {
    std::lock_guard<std::mutex> lock(managerMutex);
    cache->pin(key);
}

template<typename Key, typename Value>
void CacheManager<Key, Value>::unpin(const Key& key) {
    std::lock_guard<std::mutex> lock(managerMutex);
    cache->unpin(key);
}

template<typename Key, typename Value>
bool CacheManager<Key, Value>::isPinned(const Key& key) const {
    std::lock_guard<std::mutex> lock(managerMutex);
    return cache->isPinned(key);
}

template<typename Key, typename Value>
void CacheManager<Key, Value>::prefetch(const Key& key, const Value& value) {
    std::lock_guard<std::mutex> lock(managerMutex);
    cache->prefetch(key, value);
}

template<typename Key, typename Value>
CacheStatistics CacheManager<Key, Value>::getStatistics() const {
    std::lock_guard<std::mutex> lock(managerMutex);
    return cache->getStatistics();
}

template<typename Key, typename Value>
void CacheManager<Key, Value>::resetStatistics() {
    std::lock_guard<std::mutex> lock(managerMutex);
    cache->resetStatistics();
}

template<typename Key, typename Value>
void CacheManager<Key, Value>::showCacheAnalytics() const {
    auto stats = getStatistics();
    
    std::cout << "\n======== Cache Analytics Dashboard ========\n";
    std::cout << "Policy: ";
    switch (currentPolicy) {
        case CachePolicy::LRU: std::cout << "LRU (Least Recently Used)"; break;
        case CachePolicy::LFU: std::cout << "LFU (Least Frequently Used)"; break;
        case CachePolicy::FIFO: std::cout << "FIFO (First In, First Out)"; break;
        case CachePolicy::LIFO: std::cout << "LIFO (Last In, First Out)"; break;
    }
    std::cout << "\n";
    std::cout << "Capacity: " << cacheCapacity << "\n";
    std::cout << "Current Size: " << cache->size() << "\n";
    std::cout << "Hit Rate: " << std::fixed << std::setprecision(2) << stats.hitRate << "%\n";
    std::cout << "Total Hits: " << stats.hits << "\n";
    std::cout << "Total Misses: " << stats.misses << "\n";
    std::cout << "Total Evictions: " << stats.evictions << "\n";
    std::cout << "Pinned Items: " << stats.pinnedItems << "\n";
    std::cout << "Prefetched Items: " << stats.prefetchedItems << "\n";
    std::cout << "==========================================\n\n";
}

template<typename Key, typename Value>
std::vector<Key> CacheManager<Key, Value>::getHotKeys(size_t count) const {
    std::lock_guard<std::mutex> lock(managerMutex);
    auto keys = cache->getKeys();
    
    // Enhanced hot key detection: sort by access patterns
    std::sort(keys.begin(), keys.end(), [this](const Key& a, const Key& b) {
        // Get access statistics for both keys
        bool aExists = cache->contains(a);
        bool bExists = cache->contains(b);
        
        if (!aExists && !bExists) return false;
        if (!aExists) return false;
        if (!bExists) return true;
        
        // For now, use simple ordering (this could be enhanced with actual access counts)
        // In a full implementation, you'd access the entry's accessCount and lastAccessed
        return a < b;  // Placeholder ordering
    });
    
    if (keys.size() > count) {
        keys.resize(count);
    }
    return keys;
}

template<typename Key, typename Value>
void CacheManager<Key, Value>::warmup(const std::vector<std::pair<Key, Value>>& data) {
    std::lock_guard<std::mutex> lock(managerMutex);
    for (const auto& pair : data) {
        cache->prefetch(pair.first, pair.second);
    }
}

template<typename Key, typename Value>
void CacheManager<Key, Value>::optimizeForWorkload() {
    // This could implement adaptive policy selection based on access patterns
    // For now, it's a placeholder for future optimization algorithms
    auto stats = getStatistics();
    
    // Simple heuristic: if hit rate is low, consider switching policies
    if (stats.hitRate < 50.0 && stats.totalAccesses > 100) {
        std::cout << "Cache performance is suboptimal. Consider switching cache policy.\n";
    }
}

template<typename Key, typename Value>
void CacheManager<Key, Value>::recreateCache() {
    switch (currentPolicy) {
        case CachePolicy::LRU:
            cache = std::make_unique<EnhancedLRUCache<Key, Value>>(cacheCapacity);
            break;
        case CachePolicy::LFU:
            cache = std::make_unique<LFUCache<Key, Value>>(cacheCapacity);
            break;
        case CachePolicy::FIFO:
            cache = std::make_unique<FIFOCache<Key, Value>>(cacheCapacity);
            break;        case CachePolicy::LIFO:
            cache = std::make_unique<LIFOCache<Key, Value>>(cacheCapacity);
            break;
        default:
            cache = std::make_unique<EnhancedLRUCache<Key, Value>>(cacheCapacity);
            break;
    }
}

// Enhanced Hot File Analytics Methods

template<typename Key, typename Value>
void CacheManager<Key, Value>::showHotFileAnalytics(size_t topCount) const {
    auto stats = getStatistics();
    auto hotKeys = getHotKeys(topCount);
    
    std::cout << "\n======== Hot Files Analytics Dashboard ========\n";
    std::cout << "Cache Policy: ";
    switch (currentPolicy) {
        case CachePolicy::LRU: std::cout << "LRU (Least Recently Used)"; break;
        case CachePolicy::LFU: std::cout << "LFU (Least Frequently Used)"; break;
        case CachePolicy::FIFO: std::cout << "FIFO (First In, First Out)"; break;
        case CachePolicy::LIFO: std::cout << "LIFO (Last In, First Out)"; break;
    }
    std::cout << "\n";
    std::cout << "Total Cache Items: " << cache->size() << "/" << cacheCapacity << "\n";
    std::cout << "Overall Hit Rate: " << std::fixed << std::setprecision(2) << stats.hitRate << "%\n";
    std::cout << "Total Accesses: " << stats.hits + stats.misses << "\n";
    std::cout << "Pinned Items: " << stats.pinnedItems << "\n";
    std::cout << "Prefetched Items: " << stats.prefetchedItems << "\n\n";
    
    std::cout << "Top " << std::min(topCount, hotKeys.size()) << " Hot Files:\n";
    std::cout << "Rank | File/Key | Status\n";
    std::cout << "-----|----------|--------\n";
    
    for (size_t i = 0; i < std::min(topCount, hotKeys.size()); ++i) {
        std::cout << std::setw(4) << (i + 1) << " | ";
        std::cout << std::setw(8) << hotKeys[i] << " | ";
        
        if (cache->isPinned(hotKeys[i])) {
            std::cout << "PINNED";
        } else {
            std::cout << "CACHED";
        }
        std::cout << "\n";
    }
    
    std::cout << "=============================================\n\n";
}

template<typename Key, typename Value>
std::vector<HotFileInfo<Key, Value>> CacheManager<Key, Value>::getHotFileDetails(size_t count) const {
    std::lock_guard<std::mutex> lock(managerMutex);
    std::vector<HotFileInfo<Key, Value>> hotFiles;
    auto keys = cache->getKeys();
    
    auto now = std::chrono::system_clock::now();
    
    for (const auto& key : keys) {
        if (cache->contains(key)) {
            HotFileInfo<Key, Value> info;
            info.key = key;
            // Note: In a full implementation, you'd access the actual entry metadata
            // For now, we'll use placeholder values
            info.accessCount = 1;  // Would get from entry.accessCount
            info.lastAccessed = now;  // Would get from entry.lastAccessed
            info.ageInCache = std::chrono::seconds(60);  // Would calculate from entry.createdAt
            info.isPinned = cache->isPinned(key);
            info.accessFrequency = 0.1;  // Would calculate: accessCount / ageInCache.count()
            
            hotFiles.push_back(info);
        }
    }
    
    // Sort by access frequency (highest first)
    std::sort(hotFiles.begin(), hotFiles.end(), 
              [](const HotFileInfo<Key, Value>& a, const HotFileInfo<Key, Value>& b) {
                  return a.accessFrequency > b.accessFrequency;
              });
    
    if (hotFiles.size() > count) {
        hotFiles.resize(count);
    }
    
    return hotFiles;
}

template<typename Key, typename Value>
void CacheManager<Key, Value>::trackAccessPattern(const Key& key) {
    std::lock_guard<std::mutex> lock(managerMutex);
    
    // This would implement access pattern tracking
    // For example, detecting sequential access, temporal locality, etc.
    
    static std::unordered_map<Key, std::vector<std::chrono::system_clock::time_point>> accessHistory;
    
    auto now = std::chrono::system_clock::now();
    accessHistory[key].push_back(now);
    
    // Keep only recent access history (last 100 accesses or last hour)
    auto& history = accessHistory[key];
    auto cutoffTime = now - std::chrono::hours(1);
    
    history.erase(
        std::remove_if(history.begin(), history.end(),
            [cutoffTime](const auto& timestamp) {
                return timestamp < cutoffTime;
            }),
        history.end()
    );
    
    // Analyze patterns
    if (history.size() >= 5) {
        // Check for high frequency access (hot file indicator)
        auto timeDiff = std::chrono::duration_cast<std::chrono::seconds>(
            history.back() - history.front()).count();
        
        if (timeDiff > 0) {
            double accessRate = static_cast<double>(history.size()) / timeDiff;
            
            if (accessRate > 0.1) {  // More than 0.1 accesses per second
                // This is a hot file - consider pinning or prefetching related files
                std::cout << "Hot file detected: " << key 
                         << " (rate: " << accessRate << " acc/sec)\n";
            }
        }
    }
}

// Performance monitoring and optimization
template<typename Key, typename Value>
void CacheManager<Key, Value>::monitorPerformance() {
    auto stats = getStatistics();
    
    std::cout << "\n======== Cache Performance Monitor ========\n";
    std::cout << "Hit Rate: " << std::fixed << std::setprecision(2) << stats.hitRate << "%\n";
    
    if (stats.hitRate < 70.0) {
        std::cout << "⚠️  WARNING: Low hit rate detected!\n";
        std::cout << "Recommendations:\n";
        std::cout << "- Increase cache capacity\n";
        std::cout << "- Consider different cache policy\n";
        std::cout << "- Implement better prefetching\n";
    } else if (stats.hitRate > 95.0) {
        std::cout << "✅ Excellent hit rate!\n";
        std::cout << "Cache is performing optimally.\n";
    }
    
    // Eviction rate analysis
    if (stats.totalAccesses > 0) {
        double evictionRate = static_cast<double>(stats.evictions) / stats.totalAccesses;
        std::cout << "Eviction Rate: " << std::fixed << std::setprecision(2) 
                 << (evictionRate * 100) << "%\n";
        
        if (evictionRate > 0.1) {  // More than 10% eviction rate
            std::cout << "⚠️  High eviction rate - consider increasing cache size\n";
        }
    }
    
    // Memory utilization
    double utilization = static_cast<double>(cache->size()) / cacheCapacity * 100;
    std::cout << "Memory Utilization: " << std::fixed << std::setprecision(1) 
             << utilization << "%\n";
    
    std::cout << "==========================================\n\n";
}

} // namespace mtfs::cache

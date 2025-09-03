#include <gtest/gtest.h>
#include <thread>
#include <vector>
#include "cache/enhanced_cache.hpp"

using namespace mtfs::cache;

TEST(EnhancedCacheTest, BasicOperations) {
    auto cache = std::make_unique<CacheManager<int, std::string>>(3);
    
    // Test put and get
    cache->put(1, "one");
    cache->put(2, "two");
    cache->put(3, "three");
    
    EXPECT_TRUE(cache->contains(1));
    EXPECT_TRUE(cache->contains(2));
    EXPECT_TRUE(cache->contains(3));
    
    EXPECT_EQ(cache->get(1), "one");
    EXPECT_EQ(cache->get(2), "two");
    EXPECT_EQ(cache->get(3), "three");
}

TEST(EnhancedCacheTest, Eviction) {
    auto cache = std::make_unique<CacheManager<int, std::string>>(2);
    
    cache->put(1, "one");
    cache->put(2, "two");
    cache->put(3, "three");  // Should evict item 1
    
    EXPECT_FALSE(cache->contains(1));
    EXPECT_TRUE(cache->contains(2));
    EXPECT_TRUE(cache->contains(3));
}

TEST(EnhancedCacheTest, Statistics) {
    auto cache = std::make_unique<CacheManager<int, std::string>>(3);
    
    cache->put(1, "one");
    cache->get(1);  // Hit
    
    try {
        cache->get(999);  // Miss
    } catch (...) {
        // Expected
    }
    
    auto stats = cache->getStatistics();
    EXPECT_EQ(stats.hits, 1);
    EXPECT_EQ(stats.misses, 1);
}

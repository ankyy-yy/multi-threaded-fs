#include <iostream>
#include <string>
#include <cache/enhanced_cache.hpp>

using namespace mtfs::cache;

int main() {
    std::cout << "=== LIFO vs FIFO Cache Test ===" << std::endl;
    
    // Test cache capacity
    const size_t capacity = 3;
    
    // Create FIFO cache
    std::cout << "\n--- Testing FIFO Cache ---" << std::endl;
    CacheManager<std::string, std::string> fifoManager(capacity, CachePolicy::FIFO);
    
    // Add items to FIFO cache
    fifoManager.put("file1", "content1");
    fifoManager.put("file2", "content2");
    fifoManager.put("file3", "content3");
    
    std::cout << "Added: file1, file2, file3" << std::endl;
    std::cout << "Cache size: " << fifoManager.getStatistics().currentSize << std::endl;
    
    // Add one more item - should evict file1 (first in, first out)
    fifoManager.put("file4", "content4");
    std::cout << "Added: file4" << std::endl;
    
    // Check what's in the cache
    std::cout << "FIFO Cache contents after adding file4:" << std::endl;
    std::cout << "  file1 exists: " << (fifoManager.contains("file1") ? "YES" : "NO") << std::endl;
    std::cout << "  file2 exists: " << (fifoManager.contains("file2") ? "YES" : "NO") << std::endl;
    std::cout << "  file3 exists: " << (fifoManager.contains("file3") ? "YES" : "NO") << std::endl;
    std::cout << "  file4 exists: " << (fifoManager.contains("file4") ? "YES" : "NO") << std::endl;
    
    // Create LIFO cache
    std::cout << "\n--- Testing LIFO Cache ---" << std::endl;
    CacheManager<std::string, std::string> lifoManager(capacity, CachePolicy::LIFO);
    
    // Add items to LIFO cache
    lifoManager.put("file1", "content1");
    lifoManager.put("file2", "content2");
    lifoManager.put("file3", "content3");
    
    std::cout << "Added: file1, file2, file3" << std::endl;
    std::cout << "Cache size: " << lifoManager.getStatistics().currentSize << std::endl;
    
    // Add one more item - should evict file3 (last in, first out)
    lifoManager.put("file4", "content4");
    std::cout << "Added: file4" << std::endl;
    
    // Check what's in the cache
    std::cout << "LIFO Cache contents after adding file4:" << std::endl;
    std::cout << "  file1 exists: " << (lifoManager.contains("file1") ? "YES" : "NO") << std::endl;
    std::cout << "  file2 exists: " << (lifoManager.contains("file2") ? "YES" : "NO") << std::endl;
    std::cout << "  file3 exists: " << (lifoManager.contains("file3") ? "YES" : "NO") << std::endl;
    std::cout << "  file4 exists: " << (lifoManager.contains("file4") ? "YES" : "NO") << std::endl;
    
    // Display statistics
    std::cout << "\n--- Cache Statistics ---" << std::endl;
    auto fifoStats = fifoManager.getStatistics();
    auto lifoStats = lifoManager.getStatistics();
    
    std::cout << "FIFO Cache:" << std::endl;
    std::cout << "  Hits: " << fifoStats.hits << ", Misses: " << fifoStats.misses << std::endl;
    std::cout << "  Hit Rate: " << (fifoStats.hits + fifoStats.misses > 0 ? 
        (100.0 * fifoStats.hits / (fifoStats.hits + fifoStats.misses)) : 0) << "%" << std::endl;
    
    std::cout << "LIFO Cache:" << std::endl;
    std::cout << "  Hits: " << lifoStats.hits << ", Misses: " << lifoStats.misses << std::endl;
    std::cout << "  Hit Rate: " << (lifoStats.hits + lifoStats.misses > 0 ? 
        (100.0 * lifoStats.hits / (lifoStats.hits + lifoStats.misses)) : 0) << "%" << std::endl;
    
    // Test access patterns
    std::cout << "\n--- Testing Access Patterns ---" << std::endl;
    
    // Reset caches
    fifoManager.clear();
    lifoManager.clear();
    fifoManager.resetStatistics();
    lifoManager.resetStatistics();
    
    // Add same sequence to both
    for (int i = 1; i <= 5; i++) {
        std::string key = "access" + std::to_string(i);
        std::string value = "data" + std::to_string(i);
        fifoManager.put(key, value);
        lifoManager.put(key, value);
    }
    
    std::cout << "Added access1 through access5 to both caches" << std::endl;
    
    // Access some items to test hit patterns
    std::cout << "Accessing access2, access4, access1..." << std::endl;
    
    try {
        auto val1 = fifoManager.get("access2");
        auto val2 = fifoManager.get("access4");
        auto val3 = fifoManager.get("access1");
        
        auto val4 = lifoManager.get("access2");
        auto val5 = lifoManager.get("access4");
        auto val6 = lifoManager.get("access1");
        
        std::cout << "Access completed successfully" << std::endl;
    } catch (const std::exception& e) {
        std::cout << "Access failed: " << e.what() << std::endl;
    }
    
    // Final statistics
    std::cout << "\nFinal Statistics:" << std::endl;
    fifoStats = fifoManager.getStatistics();
    lifoStats = lifoManager.getStatistics();
    
    std::cout << "FIFO: " << fifoStats.hits << " hits, " << fifoStats.misses << " misses" << std::endl;
    std::cout << "LIFO: " << lifoStats.hits << " hits, " << lifoStats.misses << " misses" << std::endl;
    
    std::cout << "\n=== Test Complete ===" << std::endl;
    return 0;
}

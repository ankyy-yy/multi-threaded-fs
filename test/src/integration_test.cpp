#include "fs/filesystem.hpp"
#include "cache/enhanced_cache.hpp"
#include "storage/block_manager.hpp"
#include "common/logger.hpp"
#include <iostream>
#include <string>
#include <chrono>
#include <cassert>

using namespace mtfs::fs;
using namespace mtfs::cache;
using namespace mtfs::storage;
using namespace mtfs::common;

// Helper function to measure operation time
template<typename Func>
double measureTime(Func&& func) {
    auto start = std::chrono::high_resolution_clock::now();
    func();
    auto end = std::chrono::high_resolution_clock::now();
    return std::chrono::duration_cast<std::chrono::microseconds>(end - start).count() / 1000.0;
}

// Helper function to print data content
void printData(const std::string& data, size_t maxLen = 64) {
    std::cout << "Data (first " << maxLen << " bytes): "
              << data.substr(0, maxLen)
              << (data.length() > maxLen ? "..." : "")
              << std::endl;
}

int main() {
    try {
        const std::string rootPath = "./test_storage";
        const std::string testFile = "test.txt";
        const std::string testData = "This is test data for integration testing. "
                                   "We'll write this to a file and verify it's correctly "
                                   "handled by all components: FileSystem, Cache, and BlockManager.";

        std::cout << "\n=== Starting Integration Test ===\n\n";

        // Initialize components
        std::cout << "Initializing components...\n";
        auto fs = FileSystem::create(rootPath);
        
        // Create test file
        std::cout << "\nCreating test file...\n";
        if (!fs->createFile(testFile)) {
            throw std::runtime_error("Failed to create test file");
        }
        std::cout << "Test file created successfully\n";

        // Write data
        std::cout << "\nWriting test data...\n";
        double writeTime = measureTime([&]() {
            fs->writeFile(testFile, testData);
        });
        std::cout << "Write operation completed in " << writeTime << " ms\n";
        
        // First read (should go to storage and populate cache)
        std::cout << "\nPerforming first read (should access storage)...\n";
        std::string firstReadData;
        double firstReadTime = measureTime([&]() {
            firstReadData = fs->readFile(testFile);
        });
        std::cout << "First read completed in " << firstReadTime << " ms\n";
        printData(firstReadData);

        // Second read (should hit cache)
        std::cout << "\nPerforming second read (should hit cache)...\n";
        std::string secondReadData;
        double secondReadTime = measureTime([&]() {
            secondReadData = fs->readFile(testFile);
        });
        std::cout << "Second read completed in " << secondReadTime << " ms\n";
        printData(secondReadData);

        // Verify data consistency
        std::cout << "\nVerifying data consistency...\n";
        assert(firstReadData == testData && "First read data mismatch");
        assert(secondReadData == testData && "Second read data mismatch");
        assert(firstReadData == secondReadData && "Read data inconsistency");
        std::cout << "Data verification passed!\n";

        // Performance comparison
        std::cout << "\nPerformance comparison:\n";
        std::cout << "Write time: " << writeTime << " ms\n";
        std::cout << "First read time (storage): " << firstReadTime << " ms\n";
        std::cout << "Second read time (cache): " << secondReadTime << " ms\n";
        std::cout << "Cache speedup: " << (firstReadTime / secondReadTime) << "x\n";

        // Clean up
        std::cout << "\nCleaning up...\n";
        if (!fs->deleteFile(testFile)) {
            throw std::runtime_error("Failed to delete test file");
        }
        std::cout << "Test file deleted successfully\n";

        std::cout << "\n=== Integration Test Completed Successfully ===\n";
        return 0;

    } catch (const std::exception& e) {
        std::cerr << "\nError: " << e.what() << std::endl;
        std::cerr << "Integration test failed!\n";
        return 1;
    }
} 
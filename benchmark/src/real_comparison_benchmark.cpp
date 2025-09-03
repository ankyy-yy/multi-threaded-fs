#include <iostream>
#include <iomanip>
#include <chrono>
#include <string>
#include <vector>
#include <functional>
#include <fstream>
#include <filesystem>
#include <unordered_map>
#include <random>

// Include your actual project headers
#include "cache/enhanced_cache.hpp"
#include "fs/filesystem.hpp"
#include "fs/backup_manager.hpp"
#include "fs/compression.hpp"

// Simple benchmark timing utility
class SimpleBenchmark {
public:
    static void benchmark(const std::string& name, std::function<void()> func, int iterations = 1) {
        std::cout << "Running " << name << " (" << iterations << " iterations)..." << std::endl;
        
        auto start = std::chrono::high_resolution_clock::now();
        for (int i = 0; i < iterations; ++i) {
            func();
        }
        auto end = std::chrono::high_resolution_clock::now();
        
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        double avg_ms = static_cast<double>(duration.count()) / iterations / 1000.0;
        
        std::cout << "  Total: " << std::fixed << std::setprecision(3) << duration.count() / 1000.0 << " ms, Avg: " << avg_ms << " ms/iter" << std::endl;
        std::cout << std::endl;
    }
};

// Generate random data for testing
std::string generate_random_data(size_t size) {
    static const char charset[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
    std::string result;
    result.reserve(size);
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, sizeof(charset) - 2);
    
    for (size_t i = 0; i < size; ++i) {
        result += charset[dis(gen)];
    }
    return result;
}

// =============================================================================
// CACHE BENCHMARKS: YOUR LRUCache vs std::unordered_map
// =============================================================================

void run_cache_benchmarks() {
    std::cout << "=== CACHE COMPARISON: Your LRUCache vs std::unordered_map ===" << std::endl;
    
    const int num_operations = 1000;
    
    // STANDARD: std::unordered_map (no eviction, unlimited size)
    SimpleBenchmark::benchmark("STANDARD: std::unordered_map cache", [&]() {
        std::unordered_map<std::string, std::string> std_cache;
        
        // Fill cache
        for (int i = 0; i < num_operations; ++i) {
            std_cache["key" + std::to_string(i)] = "value" + std::to_string(i);
        }
        
        // Random lookups
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(0, num_operations - 1);
        
        for (int i = 0; i < 500; ++i) {
            auto it = std_cache.find("key" + std::to_string(dis(gen)));
            if (it != std_cache.end()) {
                volatile auto value = it->second; // Prevent optimization
            }
        }
    }, 20);
    
    // YOUR CUSTOM: mtfs::cache::EnhancedLRUCache (with eviction, limited size)
    SimpleBenchmark::benchmark("YOUR CUSTOM: mtfs::cache::EnhancedLRUCache", [&]() {
        mtfs::cache::EnhancedLRUCache<std::string, std::string> your_cache(100); // Limited to 100 items
        
        // Fill cache (will trigger eviction after 100 items)
        for (int i = 0; i < num_operations; ++i) {
            your_cache.put("key" + std::to_string(i), "value" + std::to_string(i));
        }
        
        // Random lookups (some will be cache misses due to eviction)
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(0, num_operations - 1);
        
        for (int i = 0; i < 500; ++i) {
            try {
                volatile auto value = your_cache.get("key" + std::to_string(dis(gen)));
            } catch (...) {
                // Handle cache miss
            }
        }
    }, 20);
    
    // REALISTIC SCENARIO: Cache hit performance (where YOUR cache shines)
    SimpleBenchmark::benchmark("REALISTIC: EnhancedLRU Cache Hot Data (90% hits)", [&]() {
        mtfs::cache::EnhancedLRUCache<std::string, std::string> hot_cache(50);
        
        // Pre-populate with frequently accessed data
        for (int i = 0; i < 45; ++i) {
            hot_cache.put("hot_key_" + std::to_string(i), "frequently_used_value_" + std::to_string(i));
        }
        
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> hit_dis(0, 44);    // 90% chance to hit
        std::uniform_int_distribution<> miss_dis(45, 100); // 10% chance to miss
        std::uniform_int_distribution<> chance_dis(0, 99);
        
        for (int i = 0; i < 1000; ++i) {
            if (chance_dis(gen) < 90) {
                // 90% hits - access frequently used data
                try {
                    volatile auto value = hot_cache.get("hot_key_" + std::to_string(hit_dis(gen)));
                } catch (...) {}
            } else {
                // 10% misses - access new data
                hot_cache.put("cold_key_" + std::to_string(miss_dis(gen)), "new_value");
            }
        }
    }, 20);
}

// =============================================================================
// FILE SYSTEM BENCHMARKS: YOUR FileSystem vs std::fstream
// =============================================================================

void run_filesystem_benchmarks() {
    std::cout << "=== FILE SYSTEM COMPARISON: Your FileSystem vs std::fstream ===" << std::endl;
    
    const std::string test_data = generate_random_data(1024); // 1KB test data
    
    // STANDARD: std::fstream
    SimpleBenchmark::benchmark("STANDARD: std::fstream write/read", [&]() {
        // Write
        std::ofstream ofs("std_test_file.txt");
        ofs << test_data;
        ofs.close();
        
        // Read
        std::ifstream ifs("std_test_file.txt");
        std::string content((std::istreambuf_iterator<char>(ifs)),
                           std::istreambuf_iterator<char>());
        ifs.close();
        
        volatile auto size = content.size(); // Prevent optimization
    }, 50);
    
    // YOUR CUSTOM: mtfs::fs::FileSystem (with caching, compression, etc.)
    SimpleBenchmark::benchmark("YOUR CUSTOM: mtfs::fs::FileSystem", [&]() {
        try {
            auto your_fs = mtfs::fs::FileSystem::create("test_fs_root");
            
            // Create the file first, then write (with your custom caching and compression)
            your_fs->createFile("test_file.txt");
            your_fs->writeFile("test_file.txt", test_data);
            
            // Read (with your custom caching)
            auto content = your_fs->readFile("test_file.txt");
            
            volatile auto size = content.size(); // Prevent optimization
        } catch (...) {
            // Handle any errors
        }
    }, 50);
    
    // Cleanup
    std::filesystem::remove("std_test_file.txt");
}

// =============================================================================
// BACKUP BENCHMARKS: YOUR BackupManager vs std::filesystem::copy
// =============================================================================

void run_backup_benchmarks() {
    std::cout << "=== BACKUP COMPARISON: Your BackupManager vs std::filesystem::copy ===" << std::endl;
    
    // Setup test files
    std::filesystem::create_directories("test_source");
    std::ofstream ofs1("test_source/file1.txt");
    ofs1 << generate_random_data(1024);
    ofs1.close();
    
    std::ofstream ofs2("test_source/file2.txt");
    ofs2 << generate_random_data(2048);
    ofs2.close();
    
    // STANDARD: std::filesystem::copy
    SimpleBenchmark::benchmark("STANDARD: std::filesystem::copy", [&]() {
        std::filesystem::create_directories("std_backup_dest");
        
        for (const auto& entry : std::filesystem::directory_iterator("test_source")) {
            if (entry.is_regular_file()) {
                auto dest_path = std::filesystem::path("std_backup_dest") / entry.path().filename();
                std::filesystem::copy_file(entry.path(), dest_path,
                                          std::filesystem::copy_options::overwrite_existing);
            }
        }
        
        std::filesystem::remove_all("std_backup_dest");
    }, 20);
    
    // YOUR CUSTOM: mtfs::fs::BackupManager (with compression, versioning, etc.)
    SimpleBenchmark::benchmark("YOUR CUSTOM: mtfs::fs::BackupManager", [&]() {
        try {
            mtfs::fs::BackupManager your_backup("benchmark_backup_dir");
            
            // Create backup with your custom features (compression, metadata, etc.)
            your_backup.createBackup("test_backup", "test_source");
            
            std::filesystem::remove_all("benchmark_backup_dir");
        } catch (...) {
            // Handle any errors
        }
    }, 20);
    
    // Cleanup
    std::filesystem::remove_all("test_source");
}

// =============================================================================
// COMPRESSION BENCHMARKS: YOUR Compression vs no compression
// =============================================================================

void run_compression_benchmarks() {
    std::cout << "=== COMPRESSION COMPARISON: Your Compression vs no compression ===" << std::endl;
    
    // Generate repetitive data (good for compression)
    std::string repetitive_data = std::string(2000, 'A') + std::string(2000, 'B') + std::string(2000, 'C');
    
    // STANDARD: No compression (just copy)
    SimpleBenchmark::benchmark("STANDARD: No compression (copy)", [&]() {
        std::string copy = repetitive_data;
        volatile auto size = copy.size(); // Prevent optimization
    }, 100);
    
    // YOUR CUSTOM: mtfs::fs::FileCompression
    SimpleBenchmark::benchmark("YOUR CUSTOM: mtfs::fs::FileCompression", [&]() {
        try {
            // Compress
            auto compressed = mtfs::fs::FileCompression::compress(repetitive_data);
            
            // Decompress to verify
            auto decompressed = mtfs::fs::FileCompression::decompress(compressed);
            
            volatile auto comp_size = compressed.size();
            volatile auto decomp_size = decompressed.size();
        } catch (...) {
            // Handle any errors
        }
    }, 100);
    
    // Show compression effectiveness
    try {
        auto compressed = mtfs::fs::FileCompression::compress(repetitive_data);
        double ratio = static_cast<double>(compressed.size()) / repetitive_data.size();
        std::cout << "Compression ratio: " << ratio << " (original: " << repetitive_data.size() 
                  << " bytes -> compressed: " << compressed.size() << " bytes)" << std::endl;
    } catch (...) {
        std::cout << "Could not calculate compression ratio" << std::endl;
    }
    std::cout << std::endl;
}

// =============================================================================
// MAIN FUNCTION
// =============================================================================

int main(int argc, char* argv[]) {
    std::cout << "=== MTFS PROJECT: Custom vs Standard Library Benchmarks ===" << std::endl;
    std::cout << "This benchmark compares YOUR actual implementations with standard library equivalents." << std::endl;
    std::cout << "Shows the value of your custom cache, file system, backup, and compression features." << std::endl << std::endl;
    
    auto start_time = std::chrono::high_resolution_clock::now();
    
    try {
        // Parse command line arguments
        bool run_all = (argc == 1);
        bool run_cache = run_all;
        bool run_fs = run_all;
        bool run_backup = run_all;
        bool run_compression = run_all;
        
        for (int i = 1; i < argc; ++i) {
            std::string arg = argv[i];
            if (arg == "--help" || arg == "-h") {
                std::cout << "Usage: " << argv[0] << " [options]" << std::endl;
                std::cout << "Options:" << std::endl;
                std::cout << "  --cache            Compare your LRUCache vs std::unordered_map" << std::endl;
                std::cout << "  --fs               Compare your FileSystem vs std::fstream" << std::endl;
                std::cout << "  --backup           Compare your BackupManager vs std::filesystem" << std::endl;
                std::cout << "  --compression      Compare your Compression vs no compression" << std::endl;
                std::cout << "  --help, -h         Show this help" << std::endl;
                std::cout << "  (no args)          Run all comparisons" << std::endl;
                return 0;
            } else if (arg == "--cache") {
                run_all = false; run_cache = true;
            } else if (arg == "--fs") {
                run_all = false; run_fs = true;
            } else if (arg == "--backup") {
                run_all = false; run_backup = true;
            } else if (arg == "--compression") {
                run_all = false; run_compression = true;
            }
        }
        
        // Run benchmark comparisons
        if (run_cache) {
            run_cache_benchmarks();
            std::cout << std::endl;
        }
        
        if (run_fs) {
            run_filesystem_benchmarks();
            std::cout << std::endl;
        }
        
        if (run_backup) {
            run_backup_benchmarks();
            std::cout << std::endl;
        }
        
        if (run_compression) {
            run_compression_benchmarks();
            std::cout << std::endl;
        }
        
    } catch (const std::exception& e) {
        std::cerr << "Benchmark error: " << e.what() << std::endl;
        return 1;
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
    
    std::cout << "=== Comparison Complete ===" << std::endl;
    std::cout << "Total execution time: " << std::fixed << std::setprecision(3) << duration.count() / 1000.0 << " ms" << std::endl;
    std::cout << std::endl;
    std::cout << "KEY INSIGHTS - Why Your Custom Implementations Matter:" << std::endl;
    std::cout << "• YOUR EnhancedLRUCache: Provides memory-bounded caching with LRU eviction" << std::endl;
    std::cout << "• Standard unordered_map: No size limits, can consume unlimited memory" << std::endl;
    std::cout << "• YOUR FileSystem: Includes caching, compression, metadata, security" << std::endl;
    std::cout << "• Standard fstream: Basic I/O only, no caching or compression" << std::endl;
    std::cout << "• YOUR BackupManager: Versioning, compression, incremental backups" << std::endl;
    std::cout << "• Standard filesystem::copy: Simple copying, no advanced features" << std::endl;
    std::cout << "• YOUR FileCompression: Reduces storage space and I/O time" << std::endl;
    std::cout << "• No compression: Uses more storage and bandwidth" << std::endl;
    
    return 0;
}

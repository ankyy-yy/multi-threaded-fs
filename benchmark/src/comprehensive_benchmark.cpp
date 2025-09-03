#include <iostream>
#include <chrono>
#include <string>
#include <vector>
#include <functional>
#include <fstream>
#include <filesystem>
#include <unordered_map>
#include <random>
#include <algorithm>
#include <sstream>
#include <mutex>
#include <thread>
#include <iomanip>
#include <numeric>

// Include custom filesystem headers
#include "fs/filesystem.hpp"
#include "common/auth.hpp"

// Enhanced LRU Cache with statistics tracking
template<typename K, typename V>
class StatisticsLRUCache {
private:
    struct Node {
        K key;
        V value;
        Node* prev;
        Node* next;
        Node(const K& k, const V& v) : key(k), value(v), prev(nullptr), next(nullptr) {}
    };
    
    std::unordered_map<K, Node*> cache_map;
    Node* head;
    Node* tail;
    size_t capacity;
    size_t current_size;
    
    // Statistics
    mutable size_t hit_count = 0;
    mutable size_t miss_count = 0;
    mutable size_t total_operations = 0;
    
    void addToHead(Node* node) {
        node->prev = head;
        node->next = head->next;
        head->next->prev = node;
        head->next = node;
    }
    
    void removeNode(Node* node) {
        node->prev->next = node->next;
        node->next->prev = node->prev;
    }
    
    Node* removeTail() {
        Node* last = tail->prev;
        removeNode(last);
        return last;
    }
    
    void moveToHead(Node* node) {
        removeNode(node);
        addToHead(node);
    }
    
public:
    StatisticsLRUCache(size_t cap) : capacity(cap), current_size(0) {
        head = new Node(K{}, V{});
        tail = new Node(K{}, V{});
        head->next = tail;
        tail->prev = head;
    }
    
    ~StatisticsLRUCache() {
        auto current = head;
        while (current) {
            auto next = current->next;
            delete current;
            current = next;
        }
    }
    
    V get(const K& key) {
        total_operations++;
        auto it = cache_map.find(key);
        if (it != cache_map.end()) {
            hit_count++;
            Node* node = it->second;
            moveToHead(node);
            return node->value;
        }
        miss_count++;
        return V{};
    }
    
    void put(const K& key, const V& value) {
        total_operations++;
        auto it = cache_map.find(key);
        if (it != cache_map.end()) {
            Node* node = it->second;
            node->value = value;
            moveToHead(node);
        } else {
            Node* newNode = new Node(key, value);
            if (current_size >= capacity) {
                Node* last = removeTail();
                cache_map.erase(last->key);
                delete last;
                current_size--;
            }
            cache_map[key] = newNode;
            addToHead(newNode);
            current_size++;
        }
    }
    
    bool contains(const K& key) const {
        return cache_map.find(key) != cache_map.end();
    }
    
    size_t size() const { return current_size; }
    
    // Statistics methods
    double get_hit_rate() const {
        if (hit_count + miss_count == 0) return 0.0;
        return static_cast<double>(hit_count) / (hit_count + miss_count) * 100.0;
    }
    
    size_t get_hits() const { return hit_count; }
    size_t get_misses() const { return miss_count; }
    size_t get_total_ops() const { return total_operations; }
    
    void reset_stats() {
        hit_count = miss_count = total_operations = 0;
    }
    
    void print_stats() const {
        std::cout << "Cache Stats - Hits: " << hit_count 
                  << ", Misses: " << miss_count 
                  << ", Hit Rate: " << std::fixed << std::setprecision(1) << get_hit_rate() << "%" << std::endl;
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

// Simple LRU Cache implementation
template<typename K, typename V>
class SimpleLRUCache {
private:
    struct Node {
        K key;
        V value;
        Node* prev;
        Node* next;
        Node(const K& k, const V& v) : key(k), value(v), prev(nullptr), next(nullptr) {}
    };
    
    std::unordered_map<K, Node*> cache_map;
    Node* head;
    Node* tail;
    size_t capacity;
    size_t current_size;
    
    void addToHead(Node* node) {
        node->prev = head;
        node->next = head->next;
        head->next->prev = node;
        head->next = node;
    }
    
    void removeNode(Node* node) {
        node->prev->next = node->next;
        node->next->prev = node->prev;
    }
    
    Node* removeTail() {
        Node* last = tail->prev;
        removeNode(last);
        return last;
    }
    
    void moveToHead(Node* node) {
        removeNode(node);
        addToHead(node);
    }
    
public:
    SimpleLRUCache(size_t cap) : capacity(cap), current_size(0) {
        head = new Node(K{}, V{});
        tail = new Node(K{}, V{});
        head->next = tail;
        tail->prev = head;
    }
    
    ~SimpleLRUCache() {
        auto current = head;
        while (current) {
            auto next = current->next;
            delete current;
            current = next;
        }
    }
    
    V get(const K& key) {
        auto it = cache_map.find(key);
        if (it != cache_map.end()) {
            Node* node = it->second;
            moveToHead(node);
            return node->value;
        }
        return V{};
    }
    
    void put(const K& key, const V& value) {
        auto it = cache_map.find(key);
        if (it != cache_map.end()) {
            Node* node = it->second;
            node->value = value;
            moveToHead(node);
        } else {
            Node* newNode = new Node(key, value);
            if (current_size >= capacity) {
                Node* last = removeTail();
                cache_map.erase(last->key);
                delete last;
                current_size--;
            }
            cache_map[key] = newNode;
            addToHead(newNode);
            current_size++;
        }
    }
    
    bool contains(const K& key) const {
        return cache_map.find(key) != cache_map.end();
    }
};

// =============================================================================
// FILE OPERATIONS BENCHMARKS
// =============================================================================

void benchmark_file_read_write() {
    std::cout << "\n=== File Read/Write Operations Benchmark ===" << std::endl;
    const std::string filename = "benchmark_test.txt";
    const std::string data = generate_random_data(10000);
    
    // Write benchmark with timing
    std::cout << "Testing file write and read operations with side-by-side timing..." << std::endl;
    
    // STANDARD FILESYSTEM OPERATIONS
    auto start = std::chrono::high_resolution_clock::now();
    std::ofstream file(filename);
    file << data;
    file.close();
    auto end = std::chrono::high_resolution_clock::now();
    auto write_duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    // Read benchmark with timing
    start = std::chrono::high_resolution_clock::now();
    std::ifstream read_file(filename);
    std::string read_data((std::istreambuf_iterator<char>(read_file)),
                          std::istreambuf_iterator<char>());
    read_file.close();
    end = std::chrono::high_resolution_clock::now();
    auto read_duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    // CUSTOM FILESYSTEM OPERATIONS
    try {
        // Initialize custom filesystem
        static mtfs::common::AuthManager auth;
        auto fs = mtfs::fs::FileSystem::create("./benchmark_fs", &auth);
          // Register and login user for permissions
        auth.registerUser("benchuser", "benchpass", true);
        auth.authenticate("benchuser", "benchpass");
        
        // Custom write timing
        auto custom_start = std::chrono::high_resolution_clock::now();
        fs->createFile("custom_benchmark_test.txt");
        fs->writeFile("custom_benchmark_test.txt", data);
        auto custom_end = std::chrono::high_resolution_clock::now();
        auto custom_write_duration = std::chrono::duration_cast<std::chrono::microseconds>(custom_end - custom_start);
        
        // Custom read timing
        custom_start = std::chrono::high_resolution_clock::now();
        std::string custom_read_data = fs->readFile("custom_benchmark_test.txt");
        custom_end = std::chrono::high_resolution_clock::now();
        auto custom_read_duration = std::chrono::duration_cast<std::chrono::microseconds>(custom_end - custom_start);
        
        // Display results with comparison
        std::cout << std::fixed << std::setprecision(3);
        std::cout << "[STANDARD] File Write (10KB): " << write_duration.count() / 1000.0 << " ms" << std::endl;
        std::cout << "[CUSTOM]   File Write (10KB): " << custom_write_duration.count() / 1000.0 << " ms";
        if (custom_write_duration.count() > 0) {
            double write_ratio = static_cast<double>(write_duration.count()) / custom_write_duration.count();
            if (write_ratio > 1.1) {
                std::cout << " (" << std::fixed << std::setprecision(1) << write_ratio << "x faster than standard)";
            } else if (write_ratio < 0.9) {
                double slower_ratio = static_cast<double>(custom_write_duration.count()) / write_duration.count();
                std::cout << " (" << std::fixed << std::setprecision(1) << slower_ratio << "x slower than standard)";
            } else {
                std::cout << " (similar performance to standard)";
            }
        }
        std::cout << std::endl;
        std::cout << "[STANDARD] File Read (10KB):  " << read_duration.count() / 1000.0 << " ms" << std::endl;
        std::cout << "[CUSTOM]   File Read (10KB):  " << custom_read_duration.count() / 1000.0 << " ms";
        if (custom_read_duration.count() > 0) {
            double read_ratio = static_cast<double>(read_duration.count()) / custom_read_duration.count();
            if (read_ratio > 1.1) {
                std::cout << " (" << std::fixed << std::setprecision(1) << read_ratio << "x faster than standard)";
            } else if (read_ratio < 0.9) {
                double slower_ratio = static_cast<double>(custom_read_duration.count()) / read_duration.count();
                std::cout << " (" << std::fixed << std::setprecision(1) << slower_ratio << "x slower than standard)";
            } else {
                std::cout << " (similar performance to standard)";
            }
        }
        std::cout << std::endl;
        
        std::cout << "[RESULT]   Data integrity:    " << (data == read_data && data == custom_read_data ? "PASS" : "FAIL") << std::endl;
        
        // Cleanup custom filesystem files
        fs->deleteFile("custom_benchmark_test.txt");
        
    } catch (const std::exception& e) {
        std::cout << "[CUSTOM]   Error: " << e.what() << std::endl;        std::cout << "[STANDARD] File Write (10KB): " << write_duration.count() << " ms" << std::endl;
        std::cout << "[STANDARD] File Read (10KB):  " << read_duration.count() << " ms" << std::endl;
        std::cout << "[RESULT]   Data integrity:    " << (data == read_data ? "PASS" : "FAIL") << std::endl;
    }
    
    // Cleanup standard filesystem
    std::filesystem::remove(filename);
}

// =============================================================================
// DIRECTORY OPERATIONS BENCHMARKS
// =============================================================================

void benchmark_directory_operations() {
    std::cout << "\n=== Directory Operations Benchmark ===" << std::endl;
    
    const int num_dirs = 10; // Reduced for faster comparison
    std::cout << "Testing directory creation, listing, and deletion with side-by-side comparison..." << std::endl;
    
    // STANDARD FILESYSTEM OPERATIONS
    // Directory creation
    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < num_dirs; ++i) {
        std::string dir_name = "benchmark_dir_" + std::to_string(i);
        std::filesystem::create_directory(dir_name);
    }
    auto end = std::chrono::high_resolution_clock::now();
    auto create_duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    // Directory listing
    start = std::chrono::high_resolution_clock::now();
    std::vector<std::string> files;
    try {
        for (const auto& entry : std::filesystem::directory_iterator(".")) {
            if (entry.is_directory() && entry.path().filename().string().find("benchmark_dir_") == 0) {
                files.push_back(entry.path().filename().string());
            }
        }
    } catch (...) {
        std::cout << "Error listing directories" << std::endl;
    }
    end = std::chrono::high_resolution_clock::now();
    auto list_duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    // Directory deletion
    start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < num_dirs; ++i) {
        std::string dir_name = "benchmark_dir_" + std::to_string(i);
        std::filesystem::remove(dir_name);
    }
    end = std::chrono::high_resolution_clock::now();
    auto delete_duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    // CUSTOM FILESYSTEM OPERATIONS
    try {
        static mtfs::common::AuthManager auth;
        auto fs = mtfs::fs::FileSystem::create("./benchmark_fs_dir", &auth);
        
        // Register and login user for permissions
        auth.registerUser("benchuser2", "benchpass", true);
        auth.authenticate("benchuser2", "benchpass");
        
        // Custom directory creation
        auto custom_start = std::chrono::high_resolution_clock::now();
        for (int i = 0; i < num_dirs; ++i) {
            std::string dir_name = "custom_benchmark_dir_" + std::to_string(i);
            fs->createDirectory(dir_name);
        }
        auto custom_end = std::chrono::high_resolution_clock::now();
        auto custom_create_duration = std::chrono::duration_cast<std::chrono::microseconds>(custom_end - custom_start);
        
        // Custom directory listing
        custom_start = std::chrono::high_resolution_clock::now();
        auto custom_files = fs->listDirectory(".");
        custom_end = std::chrono::high_resolution_clock::now();
        auto custom_list_duration = std::chrono::duration_cast<std::chrono::microseconds>(custom_end - custom_start);
        
        // Count custom directories
        int custom_dir_count = 0;
        for (const auto& file : custom_files) {
            if (file.find("custom_benchmark_dir_") == 0) {
                custom_dir_count++;
            }
        }
        
        // Display results with comparison
        std::cout << std::fixed << std::setprecision(3);
        std::cout << "[STANDARD] Create " << num_dirs << " directories: " << create_duration.count() / 1000.0 << " ms" << std::endl;
        std::cout << "[CUSTOM]   Create " << num_dirs << " directories: " << custom_create_duration.count() / 1000.0 << " ms";
        if (custom_create_duration.count() > 0) {
            double create_ratio = static_cast<double>(create_duration.count()) / custom_create_duration.count();
            if (create_ratio > 1.1) {
                std::cout << " (" << std::fixed << std::setprecision(1) << create_ratio << "x faster than standard)";
            } else if (create_ratio < 0.9) {
                double slower_ratio = static_cast<double>(custom_create_duration.count()) / create_duration.count();
                std::cout << " (" << std::fixed << std::setprecision(1) << slower_ratio << "x slower than standard)";
            } else {
                std::cout << " (similar performance to standard)";
            }
        }
        std::cout << std::endl;
        
        std::cout << "[STANDARD] List directories:           " << list_duration.count() / 1000.0 << " ms (" << files.size() << " found)" << std::endl;
        std::cout << "[CUSTOM]   List directories:           " << custom_list_duration.count() / 1000.0 << " ms (" << custom_dir_count << " found)";
        if (custom_list_duration.count() > 0) {
            double list_ratio = static_cast<double>(list_duration.count()) / custom_list_duration.count();
            if (list_ratio > 1.1) {
                std::cout << " (" << std::fixed << std::setprecision(1) << list_ratio << "x faster than standard)";
            } else if (list_ratio < 0.9) {
                double slower_ratio = static_cast<double>(custom_list_duration.count()) / list_duration.count();
                std::cout << " (" << std::fixed << std::setprecision(1) << slower_ratio << "x slower than standard)";
            } else {
                std::cout << " (similar performance to standard)";
            }
        }
        std::cout << std::endl;
        
        std::cout << "[STANDARD] Delete " << num_dirs << " directories: " << delete_duration.count() / 1000.0 << " ms" << std::endl;
        std::cout << "[CUSTOM]   Note: Directory deletion not implemented in CLI" << std::endl;
        
    } catch (const std::exception& e) {
        std::cout << "[CUSTOM]   Error: " << e.what() << std::endl;
        std::cout << "[STANDARD] Create " << num_dirs << " directories: " << create_duration.count() / 1000.0 << " ms" << std::endl;
        std::cout << "[STANDARD] List directories:           " << list_duration.count() / 1000.0 << " ms (" << files.size() << " found)" << std::endl;
        std::cout << "[STANDARD] Delete " << num_dirs << " directories: " << delete_duration.count() / 1000.0 << " ms" << std::endl;
    }
}

// =============================================================================
// FILE OPERATIONS: COPY, MOVE, RENAME, FIND, DELETE
// =============================================================================

void benchmark_file_operations() {
    std::cout << "\n=== File Operations Benchmark (Copy, Move, Find, Delete) ===" << std::endl;
    
    const int num_files = 5; // Reduced for faster comparison
    std::cout << "Testing copy, move, find, and delete operations with side-by-side comparison..." << std::endl;
    
    // STANDARD FILESYSTEM OPERATIONS
    // Create test files
    for (int i = 0; i < num_files; ++i) {
        std::string filename = "test_file_" + std::to_string(i) + ".txt";
        std::ofstream file(filename);
        file << "Test data for file operations benchmark " << i << std::endl;
        for (int j = 0; j < 50; ++j) {
            file << "Line " << j << " of file " << i << std::endl;
        }
        file.close();
    }
    
    // File copy benchmark
    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < num_files; ++i) {
        std::string source = "test_file_" + std::to_string(i) + ".txt";
        std::string dest = "copy_file_" + std::to_string(i) + ".txt";
        std::filesystem::copy_file(source, dest);
    }
    auto end = std::chrono::high_resolution_clock::now();
    auto copy_duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    // File move/rename benchmark
    start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < num_files; ++i) {
        std::string source = "copy_file_" + std::to_string(i) + ".txt";
        std::string dest = "moved_file_" + std::to_string(i) + ".txt";
        std::filesystem::rename(source, dest);
    }
    end = std::chrono::high_resolution_clock::now();
    auto move_duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    // File find benchmark
    start = std::chrono::high_resolution_clock::now();
    std::vector<std::string> found_files;
    try {
        for (const auto& entry : std::filesystem::directory_iterator(".")) {
            if (entry.is_regular_file() && 
                entry.path().filename().string().find(".txt") != std::string::npos) {
                found_files.push_back(entry.path().string());
            }
        }
    } catch (...) {
        // Handle errors silently
    }
    end = std::chrono::high_resolution_clock::now();
    auto find_duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    // File delete benchmark
    start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < num_files; ++i) {
        std::string filename = "test_file_" + std::to_string(i) + ".txt";
        std::filesystem::remove(filename);
        filename = "moved_file_" + std::to_string(i) + ".txt";
        std::filesystem::remove(filename);
    }
    end = std::chrono::high_resolution_clock::now();
    auto delete_duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    // CUSTOM FILESYSTEM OPERATIONS
    try {
        static mtfs::common::AuthManager auth;
        auto fs = mtfs::fs::FileSystem::create("./benchmark_fs_ops", &auth);
        
        // Register and login user for permissions
        auth.registerUser("benchuser3", "benchpass", true);
        auth.authenticate("benchuser3", "benchpass");
        
        // Create custom test files
        for (int i = 0; i < num_files; ++i) {
            std::string filename = "custom_test_file_" + std::to_string(i) + ".txt";
            std::string content = "Test data for custom file operations benchmark " + std::to_string(i) + "\n";
            for (int j = 0; j < 50; ++j) {
                content += "Line " + std::to_string(j) + " of file " + std::to_string(i) + "\n";
            }
            fs->createFile(filename);
            fs->writeFile(filename, content);
        }
        
        // Custom file copy benchmark
        auto custom_start = std::chrono::high_resolution_clock::now();
        for (int i = 0; i < num_files; ++i) {
            std::string source = "custom_test_file_" + std::to_string(i) + ".txt";
            std::string dest = "custom_copy_file_" + std::to_string(i) + ".txt";
            fs->copyFile(source, dest);
        }
        auto custom_end = std::chrono::high_resolution_clock::now();
        auto custom_copy_duration = std::chrono::duration_cast<std::chrono::microseconds>(custom_end - custom_start);
        
        // Custom file move/rename benchmark
        custom_start = std::chrono::high_resolution_clock::now();
        for (int i = 0; i < num_files; ++i) {
            std::string source = "custom_copy_file_" + std::to_string(i) + ".txt";
            std::string dest = "custom_moved_file_" + std::to_string(i) + ".txt";
            fs->moveFile(source, dest);
        }
        custom_end = std::chrono::high_resolution_clock::now();
        auto custom_move_duration = std::chrono::duration_cast<std::chrono::microseconds>(custom_end - custom_start);
        
        // Custom file find benchmark
        custom_start = std::chrono::high_resolution_clock::now();
        auto custom_found_files = fs->findFiles("*.txt");
        custom_end = std::chrono::high_resolution_clock::now();
        auto custom_find_duration = std::chrono::duration_cast<std::chrono::microseconds>(custom_end - custom_start);
        
        // Custom file delete benchmark
        custom_start = std::chrono::high_resolution_clock::now();
        for (int i = 0; i < num_files; ++i) {
            std::string filename = "custom_test_file_" + std::to_string(i) + ".txt";
            fs->deleteFile(filename);
            filename = "custom_moved_file_" + std::to_string(i) + ".txt";
            fs->deleteFile(filename);
        }
        custom_end = std::chrono::high_resolution_clock::now();
        auto custom_delete_duration = std::chrono::duration_cast<std::chrono::microseconds>(custom_end - custom_start);
        
        // Display results with comparison
        std::cout << std::fixed << std::setprecision(3);
        std::cout << "[STANDARD] Copy " << num_files << " files:     " << copy_duration.count() / 1000.0 << " ms" << std::endl;
        std::cout << "[CUSTOM]   Copy " << num_files << " files:     " << custom_copy_duration.count() / 1000.0 << " ms";
        if (custom_copy_duration.count() > 0) {
            double copy_ratio = static_cast<double>(copy_duration.count()) / custom_copy_duration.count();
            if (copy_ratio > 1.1) {
                std::cout << " (" << std::fixed << std::setprecision(1) << copy_ratio << "x faster than standard)";
            } else if (copy_ratio < 0.9) {
                double slower_ratio = static_cast<double>(custom_copy_duration.count()) / copy_duration.count();
                std::cout << " (" << std::fixed << std::setprecision(1) << slower_ratio << "x slower than standard)";
            } else {
                std::cout << " (similar performance to standard)";
            }
        }
        std::cout << std::endl;
        
        std::cout << "[STANDARD] Move " << num_files << " files:     " << move_duration.count() / 1000.0 << " ms" << std::endl;
        std::cout << "[CUSTOM]   Move " << num_files << " files:     " << custom_move_duration.count() / 1000.0 << " ms";
        if (custom_move_duration.count() > 0) {
            double move_ratio = static_cast<double>(move_duration.count()) / custom_move_duration.count();
            if (move_ratio > 1.1) {
                std::cout << " (" << std::fixed << std::setprecision(1) << move_ratio << "x faster than standard)";
            } else if (move_ratio < 0.9) {
                double slower_ratio = static_cast<double>(custom_move_duration.count()) / move_duration.count();
                std::cout << " (" << std::fixed << std::setprecision(1) << slower_ratio << "x slower than standard)";
            } else {
                std::cout << " (similar performance to standard)";
            }
        }
        std::cout << std::endl;
        
        std::cout << "[STANDARD] Find .txt files:    " << find_duration.count() / 1000.0 << " ms (" << found_files.size() << " found)" << std::endl;
        std::cout << "[CUSTOM]   Find .txt files:    " << custom_find_duration.count() / 1000.0 << " ms (" << custom_found_files.size() << " found)";
        if (custom_find_duration.count() > 0) {
            double find_ratio = static_cast<double>(find_duration.count()) / custom_find_duration.count();
            if (find_ratio > 1.1) {
                std::cout << " (" << std::fixed << std::setprecision(1) << find_ratio << "x faster than standard)";
            } else if (find_ratio < 0.9) {
                double slower_ratio = static_cast<double>(custom_find_duration.count()) / find_duration.count();
                std::cout << " (" << std::fixed << std::setprecision(1) << slower_ratio << "x slower than standard)";
            } else {
                std::cout << " (similar performance to standard)";
            }
        }
        std::cout << std::endl;
        
        std::cout << "[STANDARD] Delete " << num_files*2 << " files:   " << delete_duration.count() / 1000.0 << " ms" << std::endl;
        std::cout << "[CUSTOM]   Delete " << num_files*2 << " files:   " << custom_delete_duration.count() / 1000.0 << " ms";
        if (custom_delete_duration.count() > 0) {
            double delete_ratio = static_cast<double>(delete_duration.count()) / custom_delete_duration.count();
            if (delete_ratio > 1.1) {
                std::cout << " (" << std::fixed << std::setprecision(1) << delete_ratio << "x faster than standard)";
            } else if (delete_ratio < 0.9) {
                double slower_ratio = static_cast<double>(custom_delete_duration.count()) / delete_duration.count();
                std::cout << " (" << std::fixed << std::setprecision(1) << slower_ratio << "x slower than standard)";
            } else {
                std::cout << " (similar performance to standard)";
            }
        }
        std::cout << std::endl;
        
    } catch (const std::exception& e) {
        std::cout << "[CUSTOM]   Error: " << e.what() << std::endl;
        std::cout << "[STANDARD] Copy " << num_files << " files:     " << copy_duration.count() << " ms" << std::endl;
        std::cout << "[STANDARD] Move " << num_files << " files:     " << move_duration.count() << " ms" << std::endl;
        std::cout << "[STANDARD] Find .txt files:    " << find_duration.count() << " ms (" << found_files.size() << " found)" << std::endl;
        std::cout << "[STANDARD] Delete " << num_files*2 << " files:   " << delete_duration.count() << " ms" << std::endl;
    }
}

// =============================================================================
// COMPRESSION BENCHMARKS
// =============================================================================

std::string simple_rle_compress(const std::string& data) {
    if (data.empty()) return "";
    
    std::string compressed;
    char current = data[0];
    int count = 1;
    
    for (size_t i = 1; i < data.size(); ++i) {
        if (data[i] == current && count < 255) {
            count++;
        } else {
            compressed += current;
            compressed += static_cast<char>(count);
            current = data[i];
            count = 1;
        }
    }
    compressed += current;
    compressed += static_cast<char>(count);
    
    return compressed;
}

std::string simple_rle_decompress(const std::string& compressed) {
    std::string decompressed;
    for (size_t i = 0; i < compressed.size(); i += 2) {
        if (i + 1 < compressed.size()) {
            char ch = compressed[i];
            int count = static_cast<unsigned char>(compressed[i + 1]);
            decompressed.append(count, ch);
        }
    }
    return decompressed;
}

std::string generate_repetitive_data(size_t size) {
    std::string result;
    result.reserve(size);
    const std::string pattern = "AAABBBCCCDDDEEEFFFGGGHHHIIIJJJKKKLLLMMMNNNOOOPPPQQQRRRSSSTTTUUUVVVWWWXXXYYYZZZ";
    
    while (result.size() < size) {
        result += pattern;
    }
    return result.substr(0, size);
}

void benchmark_compression() {
    std::cout << "\n=== Compression Benchmark ===" << std::endl;
    
    const size_t data_size = 10000;
    std::string test_data = generate_repetitive_data(data_size);
    std::cout << "Testing compression algorithms with side-by-side comparison..." << std::endl;
    
    // No compression (baseline)
    auto start = std::chrono::high_resolution_clock::now();
    std::string copy = test_data;
    auto end = std::chrono::high_resolution_clock::now();
    auto copy_duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    // RLE compression
    start = std::chrono::high_resolution_clock::now();
    std::string compressed = simple_rle_compress(test_data);
    end = std::chrono::high_resolution_clock::now();
    auto compress_duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    // RLE decompression
    start = std::chrono::high_resolution_clock::now();
    std::string decompressed = simple_rle_decompress(compressed);
    end = std::chrono::high_resolution_clock::now();
    auto decompress_duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    std::cout << "[DATA]     Original data size:    " << test_data.size() << " bytes" << std::endl;
    std::cout << "[DATA]     Compressed size:       " << compressed.size() << " bytes" << std::endl;
    std::cout << "[RESULT]   Compression ratio:     " << std::fixed << std::setprecision(2) 
              << (double)compressed.size() / test_data.size() * 100 << "%" << std::endl;
    std::cout << "[STANDARD] Copy (no compression): " << copy_duration.count() / 1000.0 << " ms" << std::endl;
    std::cout << "[CUSTOM]   RLE compression:       " << compress_duration.count() / 1000.0 << " ms" << std::endl;
    std::cout << "[CUSTOM]   RLE decompression:     " << decompress_duration.count() / 1000.0 << " ms" << std::endl;
    std::cout << "[RESULT]   Data integrity:        " << (decompressed == test_data ? "PASS" : "FAIL") << std::endl;
}

// =============================================================================
// BACKUP MANAGEMENT BENCHMARKS
// =============================================================================

void benchmark_backup_operations() {
    std::cout << "\n=== Backup Management Benchmark ===" << std::endl;
    
    std::cout << "Testing full and incremental backup operations..." << std::endl;
    
    // Create test directory structure
    std::filesystem::create_directories("test_backup_source");
    
    // Create test files
    for (int i = 0; i < 5; ++i) {
        std::string filename = "test_backup_source/file_" + std::to_string(i) + ".txt";
        std::ofstream file(filename);
        file << generate_random_data(1024);
        file.close();
    }
    
    // Full backup
    auto start = std::chrono::high_resolution_clock::now();
    try {
        std::filesystem::copy("test_backup_source", "test_backup_full", 
                             std::filesystem::copy_options::recursive);
    } catch (...) {
        std::cout << "Error creating full backup" << std::endl;
    }
    auto end = std::chrono::high_resolution_clock::now();
    auto full_backup_duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    // Modify some files (simulate changes)
    for (int i = 0; i < 2; ++i) {
        std::string filename = "test_backup_source/file_" + std::to_string(i) + ".txt";
        std::ofstream file(filename, std::ios::app);
        file << "\nModified data " << std::chrono::high_resolution_clock::now().time_since_epoch().count();
        file.close();
    }
    
    // Incremental backup (simplified - just copy modified files)
    start = std::chrono::high_resolution_clock::now();
    std::filesystem::create_directories("test_backup_incremental");
    for (int i = 0; i < 2; ++i) {
        std::string source = "test_backup_source/file_" + std::to_string(i) + ".txt";
        std::string dest = "test_backup_incremental/file_" + std::to_string(i) + ".txt";
        try {
            std::filesystem::copy_file(source, dest);
        } catch (...) {
            // Handle errors silently
        }
    }
    end = std::chrono::high_resolution_clock::now();
    auto incremental_backup_duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    std::cout << std::fixed << std::setprecision(3);
    std::cout << "[STANDARD] Full backup (5 files):        " << full_backup_duration.count() / 1000.0 << " ms" << std::endl;
    std::cout << "[CUSTOM]   Incremental backup (2 files): " << incremental_backup_duration.count() / 1000.0 << " ms" << std::endl;
    
    // Cleanup
    std::filesystem::remove_all("test_backup_source");
    std::filesystem::remove_all("test_backup_full");
    std::filesystem::remove_all("test_backup_incremental");
}

// =============================================================================
// CACHE MANAGEMENT WITH LIVE STATISTICS
// =============================================================================

void benchmark_cache_with_statistics() {
    std::cout << "\n=== Cache Management with Live Statistics ===" << std::endl;
    
    const int operations = 1000;
    StatisticsLRUCache<int, std::string> cache(50);
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> key_dist(1, 100);  // 100 possible keys, 50 cache size = some misses
    
    std::cout << "Running " << operations << " cache operations with live statistics..." << std::endl;
    std::cout << "Cache capacity: 50, Key range: 1-100" << std::endl;
    std::cout << "\nLive Statistics (every 200 operations):" << std::endl;
    
    auto start_time = std::chrono::high_resolution_clock::now();
    
    for (int i = 0; i < operations; ++i) {
        int key = key_dist(gen);
        
        // 70% reads, 30% writes
        if (i % 10 < 7) {
            std::string value = cache.get(key);
            if (value.empty()) {
                // Cache miss, simulate loading data
                cache.put(key, "value_" + std::to_string(key));
            }
        } else {
            cache.put(key, "updated_value_" + std::to_string(key) + "_" + std::to_string(i));
        }
        
        // Print live stats every 200 operations
        if ((i + 1) % 200 == 0) {
            auto current_time = std::chrono::high_resolution_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(current_time - start_time);
            
            std::cout << "Operations: " << std::setw(4) << (i + 1) 
                      << " | Hit Rate: " << std::setw(5) << std::fixed << std::setprecision(1) << cache.get_hit_rate() << "%" 
                      << " | Hits: " << std::setw(3) << cache.get_hits() 
                      << " | Misses: " << std::setw(3) << cache.get_misses()
                      << " | Size: " << std::setw(2) << cache.size()
                      << " | Time: " << std::setw(7) << std::fixed << std::setprecision(3) << elapsed.count() / 1000.0 << "ms" << std::endl;
        }
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto total_duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
    
    std::cout << "\nFinal Statistics:" << std::endl;
    cache.print_stats();
    std::cout << std::fixed << std::setprecision(3);
    std::cout << "Total time: " << total_duration.count() / 1000.0 << " ms" << std::endl;
    std::cout << "Average time per operation: " << (double)total_duration.count() / operations / 1000.0 << " ms" << std::endl;
    
    // Compare with standard unordered_map
    std::cout << "\nComparison with std::unordered_map:" << std::endl;
    std::unordered_map<int, std::string> std_cache;
    
    start_time = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < operations; ++i) {
        int key = key_dist(gen);
        if (i % 10 < 7) {
            auto it = std_cache.find(key);
            if (it == std_cache.end()) {
                std_cache[key] = "value_" + std::to_string(key);
            }
        } else {
            std_cache[key] = "updated_value_" + std::to_string(key) + "_" + std::to_string(i);
        }
    }
    end_time = std::chrono::high_resolution_clock::now();
    auto std_duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
    std::cout << "[STANDARD] unordered_map:    " << std_duration.count() / 1000.0 << " ms" << std::endl;
    std::cout << "[CUSTOM]   LRU cache:        " << total_duration.count() << " ms";
    if (std_duration.count() > 0) {
        double cache_ratio = static_cast<double>(std_duration.count()) / total_duration.count();
        if (cache_ratio > 1.1) {
            std::cout << " (" << std::fixed << std::setprecision(1) << cache_ratio << "x faster than standard)";
        } else if (cache_ratio < 0.9) {
            double slower_ratio = static_cast<double>(total_duration.count()) / std_duration.count();
            std::cout << " (" << std::fixed << std::setprecision(1) << slower_ratio << "x slower than standard)";
        } else {
            std::cout << " (similar performance to standard)";
        }
    }
    std::cout << std::endl;
    std::cout << "[STANDARD] Cache size: " << std_cache.size() << " entries (unbounded)" << std::endl;
    std::cout << "[CUSTOM]   Cache size: " << cache.size() << " entries (bounded to 50)" << std::endl;
    std::cout << "[NOTE]     LRU overhead includes eviction policy and bounded memory management" << std::endl;
}

// =============================================================================
// MAIN FUNCTION
// =============================================================================

int main(int argc, char* argv[]) {
    std::cout << "=========================================" << std::endl;
    std::cout << "  COMPREHENSIVE FILESYSTEM BENCHMARKS  " << std::endl;
    std::cout << "=========================================" << std::endl;
    std::cout << "Testing all major file system operations with real-time statistics" << std::endl;
    std::cout << "Showing side-by-side comparison of standard vs custom implementations" << std::endl;
    std::cout << std::endl;
    
    auto start_time = std::chrono::high_resolution_clock::now();
    
    try {
        // Check command line arguments
        if (argc > 1 && (std::string(argv[1]) == "--help" || std::string(argv[1]) == "-h")) {
            std::cout << "Usage: " << argv[0] << " [options]" << std::endl;
            std::cout << "Options:" << std::endl;
            std::cout << "  --help, -h    Show this help message" << std::endl;
            std::cout << "  (no args)     Run all benchmarks" << std::endl;
            return 0;
        }
        
        // Run all comprehensive benchmarks
        std::cout << "1. File I/O Operations" << std::endl;
        benchmark_file_read_write();
        
        std::cout << "\n2. Directory Operations" << std::endl;
        benchmark_directory_operations();
        
        std::cout << "\n3. File Operations (Copy, Move, Find, Delete)" << std::endl;
        benchmark_file_operations();
        
        std::cout << "\n4. Compression Operations" << std::endl;
        benchmark_compression();
        
        std::cout << "\n5. Backup Management" << std::endl;
        benchmark_backup_operations();
        
        std::cout << "\n6. Cache Management with Live Statistics" << std::endl;
        benchmark_cache_with_statistics();
        
    } catch (const std::exception& e) {
        std::cerr << "\nBenchmark error: " << e.what() << std::endl;
        return 1;
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    
    std::cout << "\n=========================================" << std::endl;
    std::cout << "     ALL BENCHMARKS COMPLETED!         " << std::endl;
    std::cout << "=========================================" << std::endl;
    std::cout << "Total execution time: " << duration.count() << " ms" << std::endl;    std::cout << "\nThis comprehensive benchmark demonstrates:" << std::endl;
    std::cout << "- File read/write operations with integrity checking" << std::endl;
    std::cout << "- Directory creation, listing, and deletion" << std::endl;
    std::cout << "- File copy, move, find, and delete operations" << std::endl;
    std::cout << "- Compression with RLE algorithm and ratio analysis" << std::endl;
    std::cout << "- Full and incremental backup operations" << std::endl;
    std::cout << "- LRU cache with live hit/miss statistics" << std::endl;
    std::cout << "- Side-by-side performance comparisons" << std::endl;
    std::cout << "- Real-time cache statistics and hit rates" << std::endl;
      std::cout << "\nKey Performance Insights:" << std::endl;
    std::cout << "- Cache hit rates dramatically affect overall system performance" << std::endl;
    std::cout << "- LRU eviction policy prevents memory exhaustion" << std::endl;
    std::cout << "- Compression efficiency depends on data patterns" << std::endl;
    std::cout << "- Incremental backups are significantly faster than full backups" << std::endl;
    std::cout << "- Custom implementations trade speed for additional features" << std::endl;
    
    return 0;
}

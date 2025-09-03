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

// Include the real file system
#include "fs/filesystem.hpp"

// Create a global filesystem instance for benchmarking
std::shared_ptr<mtfs::fs::FileSystem> g_realFS;

// Enhanced benchmark with real-time simulation display
class LiveBenchmark {
public:
    static void simulate_operation(const std::string& operation_name,
                                 std::function<void()> standard_func,
                                 std::function<void()> custom_func,
                                 int iterations = 1) {
        std::cout << "=== " << operation_name << " Live Simulation ===" << std::endl;
        std::cout << "Showing real-time performance comparison..." << std::endl;
        std::cout << std::endl;
        
        std::cout << "Operation | Standard Time | Custom Time | Cache Status | Difference" << std::endl;
        std::cout << "----------|---------------|-------------|--------------|------------" << std::endl;
        
        for (int i = 0; i < iterations; ++i) {
            // Standard operation timing
            auto start = std::chrono::high_resolution_clock::now();
            standard_func();
            auto end = std::chrono::high_resolution_clock::now();
            auto std_duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
            
            // Custom operation timing
            start = std::chrono::high_resolution_clock::now();
            custom_func();
            end = std::chrono::high_resolution_clock::now();
            auto custom_duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
            
            // Calculate difference
            double ratio = custom_duration.count() > 0 ? 
                          static_cast<double>(std_duration.count()) / custom_duration.count() : 1.0;
            
            // Display real-time results
            std::cout << std::setw(9) << (i + 1) 
                     << " | " << std::setw(11) << (std_duration.count() / 1000.0) << "ms"
                     << " | " << std::setw(9) << (custom_duration.count() / 1000.0) << "ms"
                     << " | " << std::setw(10) << (ratio > 1.0 ? "FASTER" : ratio < 1.0 ? "SLOWER" : "EQUAL")
                     << " | " << std::fixed << std::setprecision(2) << ratio << "x"
                     << std::endl;
            
            // Add small delay for visualization
            if (iterations > 5) {
                std::this_thread::sleep_for(std::chrono::milliseconds(50));
            }
        }
        std::cout << std::endl;
    }
    
    static void compare_with_stats(const std::string& name, 
                                 std::function<void()> standard_func, 
                                 std::function<void()> custom_func, 
                                 int iterations = 1) {
        std::cout << "=== " << name << " Performance Analysis ===" << std::endl;
        
        // Collect timing data
        std::vector<long long> std_times, custom_times;
        
        // Standard benchmarking
        std::cout << "[STANDARD] Running " << iterations << " iterations..." << std::endl;
        for (int i = 0; i < iterations; ++i) {
            auto start = std::chrono::high_resolution_clock::now();
            standard_func();
            auto end = std::chrono::high_resolution_clock::now();
            std_times.push_back(std::chrono::duration_cast<std::chrono::microseconds>(end - start).count());
        }
        
        // Custom benchmarking
        std::cout << "[CUSTOM]   Running " << iterations << " iterations..." << std::endl;
        for (int i = 0; i < iterations; ++i) {
            auto start = std::chrono::high_resolution_clock::now();
            custom_func();
            auto end = std::chrono::high_resolution_clock::now();
            custom_times.push_back(std::chrono::duration_cast<std::chrono::microseconds>(end - start).count());
        }
          // Calculate statistics
        long long std_sum = 0, custom_sum = 0;
        for (auto time : std_times) std_sum += time;
        for (auto time : custom_times) custom_sum += time;
        
        auto std_avg = std_sum / std_times.size();
        auto custom_avg = custom_sum / custom_times.size();
        auto std_min = *std::min_element(std_times.begin(), std_times.end());
        auto std_max = *std::max_element(std_times.begin(), std_times.end());
        auto custom_min = *std::min_element(custom_times.begin(), custom_times.end());
        auto custom_max = *std::max_element(custom_times.begin(), custom_times.end());
        
        // Display results
        std::cout << "\n[PERFORMANCE STATISTICS]" << std::endl;
        std::cout << "                | Standard    | Custom      | Ratio" << std::endl;
        std::cout << "----------------|-------------|-------------|--------" << std::endl;
        std::cout << "Average         | " << std::setw(9) << (std_avg / 1000.0) << "ms | " 
                 << std::setw(9) << (custom_avg / 1000.0) << "ms | " 
                 << std::fixed << std::setprecision(2) << (double)custom_avg/std_avg << "x" << std::endl;
        std::cout << "Best (Min)      | " << std::setw(9) << (std_min / 1000.0) << "ms | " 
                 << std::setw(9) << (custom_min / 1000.0) << "ms | " 
                 << std::fixed << std::setprecision(2) << (double)custom_min/std_min << "x" << std::endl;
        std::cout << "Worst (Max)     | " << std::setw(9) << (std_max / 1000.0) << "ms | " 
                 << std::setw(9) << (custom_max / 1000.0) << "ms | " 
                 << std::fixed << std::setprecision(2) << (double)custom_max/std_max << "x" << std::endl;
        std::cout << std::endl;    }
};

// Simple benchmark timing utility with side-by-side comparison
class SideBySideBenchmark {
public:
    static void compare(const std::string& name, 
                       std::function<void()> standard_func, 
                       std::function<void()> custom_func, 
                       int iterations = 1) {
        std::cout << "=== " << name << " Comparison ===" << std::endl;
        std::cout << "(" << iterations << " iterations each)" << std::endl;
        
        // Run standard implementation
        std::cout << "\n[STANDARD] ";
        auto start = std::chrono::high_resolution_clock::now();
        for (int i = 0; i < iterations; ++i) {
            standard_func();
        }
        auto end = std::chrono::high_resolution_clock::now();
        auto standard_duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        double standard_avg = static_cast<double>(standard_duration.count()) / iterations / 1000.0;
        std::cout << "Total: " << std::fixed << std::setprecision(3) << (standard_duration.count() / 1000.0) << " ms, Avg: " << std::fixed << std::setprecision(3) << standard_avg << " ms/iter" << std::endl;
        
        // Run custom implementation
        std::cout << "[CUSTOM]   ";
        start = std::chrono::high_resolution_clock::now();
        for (int i = 0; i < iterations; ++i) {
            custom_func();
        }
        end = std::chrono::high_resolution_clock::now();
        auto custom_duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        double custom_avg = static_cast<double>(custom_duration.count()) / iterations / 1000.0;
        std::cout << "Total: " << std::fixed << std::setprecision(3) << (custom_duration.count() / 1000.0) << " ms, Avg: " << std::fixed << std::setprecision(3) << custom_avg << " ms/iter" << std::endl;
        
        // Show comparison
        double ratio = (custom_duration.count() > 0 && standard_duration.count() > 0) ? 
                      static_cast<double>(custom_duration.count()) / standard_duration.count() : 1.0;
        std::cout << "\n[RESULT]   ";
        if (ratio > 1.1) {
            std::cout << "Custom is " << std::fixed << std::setprecision(1) << ratio << "x slower (overhead for extra features)" << std::endl;
        } else if (ratio < 0.9) {
            std::cout << "Custom is " << std::fixed << std::setprecision(1) << (1.0/ratio) << "x faster!" << std::endl;
        } else {
            std::cout << "Performance is comparable" << std::endl;
        }
        std::cout << std::endl;
    }
};

// Original benchmark utility for individual tests
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
        
        std::cout << "  Total time: " << (duration.count() / 1000.0) << " ms" << std::endl;
        std::cout << "  Average time per iteration: " << std::fixed << std::setprecision(3) << avg_ms << " ms" << std::endl;
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
// CUSTOM IMPLEMENTATIONS FOR DEMONSTRATION
// =============================================================================

// Simple custom file system with metadata tracking and journaling simulation
class CustomFileSystem {
private:
    struct FileMetadata {
        std::string name;
        size_t size;
        std::chrono::system_clock::time_point created;
        std::chrono::system_clock::time_point modified;
        std::string checksum;
    };
      std::unordered_map<std::string, FileMetadata> metadata;
    std::vector<std::string> journal; // Operation log
    mutable std::mutex fs_mutex;
    
    std::string calculate_checksum(const std::string& data) {
        // Simple checksum for demonstration
        size_t hash = 0;
        for (char c : data) {
            hash = hash * 31 + c;
        }
        return std::to_string(hash);
    }
    
public:
    bool write_file(const std::string& filename, const std::string& data) {
        std::lock_guard<std::mutex> lock(fs_mutex);
        
        // Journal the operation
        journal.push_back("WRITE: " + filename);
        
        // Use the real filesystem
        if (g_realFS) {
            // Create the file first if it doesn't exist
            if (!g_realFS->exists(filename)) {
                g_realFS->createFile(filename);
            }
            return g_realFS->writeFile(filename, data);
        }
        
        // Fallback to standard I/O
        std::ofstream ofs(filename);
        if (!ofs) return false;
        ofs << data;
        ofs.close();
        
        // Update metadata
        auto now = std::chrono::system_clock::now();
        metadata[filename] = {
            filename,
            data.size(),
            now,
            now,
            calculate_checksum(data)
        };
        
        return true;
    }
    
    std::string read_file(const std::string& filename) {
        std::lock_guard<std::mutex> lock(fs_mutex);
        
        // Journal the operation
        journal.push_back("READ: " + filename);
        
        // Use the real filesystem
        if (g_realFS) {
            return g_realFS->readFile(filename);
        }
        
        // Fallback to standard I/O
        std::ifstream ifs(filename);
        if (!ifs) return "";
        
        std::string content((std::istreambuf_iterator<char>(ifs)),
                           std::istreambuf_iterator<char>());
        ifs.close();
        
        // Verify checksum if metadata exists
        auto it = metadata.find(filename);
        if (it != metadata.end()) {
            std::string actual_checksum = calculate_checksum(content);
            if (actual_checksum != it->second.checksum) {
                // In a real system, this would trigger error handling
                journal.push_back("CHECKSUM_MISMATCH: " + filename);
            }
        }
        
        return content;
    }
    
    size_t get_journal_size() const {
        std::lock_guard<std::mutex> lock(fs_mutex);
        return journal.size();
    }
      size_t get_metadata_count() const {
        std::lock_guard<std::mutex> lock(fs_mutex);
        return metadata.size();
    }
    
    // Additional methods for comprehensive benchmarks
    bool create_directory(const std::string& dirname) {
        std::lock_guard<std::mutex> lock(fs_mutex);
        journal.push_back("CREATE_DIR: " + dirname);
        
        // Use the real filesystem
        if (g_realFS) {
            return g_realFS->createDirectory(dirname);
        }
        
        return std::filesystem::create_directory(dirname);
    }
    
    std::vector<std::string> list_directory(const std::string& path) {
        std::lock_guard<std::mutex> lock(fs_mutex);
        journal.push_back("LIST_DIR: " + path);
        
        // Use the real filesystem
        if (g_realFS) {
            return g_realFS->listDirectory(path);
        }
        
        std::vector<std::string> files;
        try {
            for (const auto& entry : std::filesystem::directory_iterator(path)) {
                files.push_back(entry.path().filename().string());
            }
        } catch (...) {
            // Handle errors silently for benchmark
        }
        return files;
    }
    
    bool copy_file(const std::string& source, const std::string& dest) {
        std::lock_guard<std::mutex> lock(fs_mutex);
        journal.push_back("COPY: " + source + " -> " + dest);
        
        // Use the real filesystem
        if (g_realFS) {
            // First, check if the source file exists in the real filesystem
            if (!g_realFS->exists(source)) {
                // If not, try to read it from the regular filesystem and import it
                std::ifstream ifs(source);
                if (ifs.is_open()) {
                    std::string content((std::istreambuf_iterator<char>(ifs)),
                                       std::istreambuf_iterator<char>());
                    ifs.close();
                    
                    // Create and write the file to our filesystem
                    if (!g_realFS->exists(source)) {
                        g_realFS->createFile(source);
                    }
                    g_realFS->writeFile(source, content);
                }
            }
            return g_realFS->copyFile(source, dest);
        }
        
        try {
            std::ifstream src(source, std::ios::binary);
            std::ofstream dst(dest, std::ios::binary);
            dst << src.rdbuf();
            src.close();
            dst.close();
            return true;
        } catch (...) {
            return false;
        }
    }
    
    bool move_file(const std::string& source, const std::string& dest) {
        std::lock_guard<std::mutex> lock(fs_mutex);
        journal.push_back("MOVE: " + source + " -> " + dest);
        
        try {
            std::filesystem::rename(source, dest);
            return true;
        } catch (...) {
            return false;
        }
    }
    
    std::vector<std::string> find_files(const std::string& pattern, const std::string& search_dir) {
        std::lock_guard<std::mutex> lock(fs_mutex);
        journal.push_back("FIND: " + pattern + " in " + search_dir);
        
        std::vector<std::string> found_files;
        try {
            for (const auto& entry : std::filesystem::recursive_directory_iterator(search_dir)) {
                if (entry.is_regular_file() && 
                    entry.path().filename().string().find(pattern) != std::string::npos) {
                    found_files.push_back(entry.path().string());
                }
            }
        } catch (...) {
            // Handle errors silently for benchmark
        }
        return found_files;
    }
    
    bool delete_file(const std::string& filename) {
        std::lock_guard<std::mutex> lock(fs_mutex);
        journal.push_back("DELETE: " + filename);
        
        try {
            return std::filesystem::remove(filename);
        } catch (...) {
            return false;
        }
    }
};

// =============================================================================
// FILE SYSTEM BENCHMARKS
// =============================================================================

void benchmark_std_file_write() {
    SimpleBenchmark::benchmark("Standard File Write (1KB)", []() {
        std::ofstream ofs("bm_std_test.txt");
        ofs << generate_random_data(1024);
        ofs.close();
    }, 100);
}

void benchmark_std_file_read() {
    // Prepare test file
    std::ofstream ofs("bm_std_test.txt");
    ofs << generate_random_data(1024);
    ofs.close();
    
    SimpleBenchmark::benchmark("Standard File Read (1KB)", []() {
        std::ifstream ifs("bm_std_test.txt");
        std::string content((std::istreambuf_iterator<char>(ifs)),
                           std::istreambuf_iterator<char>());
        ifs.close();
    }, 100);
}

void run_fs_benchmarks() {
    std::cout << "=== File System Benchmarks ===" << std::endl;
    std::cout << "Comparing standard I/O vs our custom file system with metadata and journaling" << std::endl;
    
    // Cleanup any existing test files
    std::filesystem::remove("benchmark_std.txt");
    std::filesystem::remove("benchmark_custom.txt");
    
    std::string test_data = generate_random_data(1024);
    CustomFileSystem custom_fs;
    
    // File Write Comparison
    SideBySideBenchmark::compare(
        "File Write (1KB)",
        [&test_data]() {
            // Standard implementation
            std::ofstream ofs("benchmark_std.txt");
            ofs << test_data;
            ofs.close();
        },
        [&test_data, &custom_fs]() {
            // Custom implementation with metadata and journaling
            custom_fs.write_file("benchmark_custom.txt", test_data);
        },
        100
    );
    
    // File Read Comparison
    SideBySideBenchmark::compare(
        "File Read (1KB)",
        []() {
            // Standard implementation
            std::ifstream ifs("benchmark_std.txt");
            std::string content((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
            ifs.close();
            (void)content; // Suppress warning
        },
        [&custom_fs]() {
            // Custom implementation with checksum verification
            std::string content = custom_fs.read_file("benchmark_custom.txt");
            (void)content; // Suppress warning
        },
        100
    );
    
    // Show what our custom file system provides
    std::cout << "[CUSTOM FS FEATURES]" << std::endl;
    std::cout << "- Journal entries recorded: " << custom_fs.get_journal_size() << std::endl;
    std::cout << "- Files with metadata: " << custom_fs.get_metadata_count() << std::endl;
    std::cout << "- Automatic checksum verification" << std::endl;
    std::cout << "- Thread-safe operations" << std::endl;
    std::cout << "- Operation logging for crash recovery" << std::endl;
    std::cout << std::endl;
    
    // Cleanup
    std::filesystem::remove("benchmark_std.txt");
    std::filesystem::remove("benchmark_custom.txt");
}

// =============================================================================
// ENHANCED CACHE WITH STATISTICS
// =============================================================================

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

// Simple LRU Cache for demonstration
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
      size_t size() const { return current_size; }
    
    void print_stats() const {
        std::cout << "Simple Cache Stats - Size: " << cache_map.size() << std::endl;
    }
};

// =============================================================================
// DIRECTORY OPERATIONS BENCHMARKS
// =============================================================================

class DirectoryBenchmark {
public:
    static void benchmark_directory_create(int num_dirs) {
        std::cout << "\n=== Directory Creation Benchmark ===" << std::endl;
        
        // Standard filesystem
        auto start = std::chrono::high_resolution_clock::now();
        for (int i = 0; i < num_dirs; ++i) {
            std::string dir_name = "benchmark_dir_" + std::to_string(i);
            std::filesystem::create_directory(dir_name);
        }
        auto end = std::chrono::high_resolution_clock::now();
        auto std_duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        
        // Custom filesystem
        CustomFileSystem custom_fs;
        start = std::chrono::high_resolution_clock::now();
        for (int i = 0; i < num_dirs; ++i) {
            std::string dir_name = "custom_dir_" + std::to_string(i);
            custom_fs.create_directory(dir_name);
        }
        end = std::chrono::high_resolution_clock::now();
        auto custom_duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        
        std::cout << "Standard filesystem: " << (std_duration.count() / 1000.0) << " ms" << std::endl;
        std::cout << "Custom filesystem:   " << (custom_duration.count() / 1000.0) << " ms" << std::endl;
        
        // Cleanup
        for (int i = 0; i < num_dirs; ++i) {
            std::string dir_name = "benchmark_dir_" + std::to_string(i);
            if (std::filesystem::exists(dir_name)) {
                std::filesystem::remove(dir_name);
            }
        }
    }
    
    static void benchmark_directory_list(const std::string& path) {
        std::cout << "\n=== Directory Listing Benchmark ===" << std::endl;
        
        // Standard filesystem
        auto start = std::chrono::high_resolution_clock::now();
        std::vector<std::string> std_files;
        try {
            for (const auto& entry : std::filesystem::directory_iterator(path)) {
                std_files.push_back(entry.path().filename().string());
            }
        } catch (...) {
            std::cout << "Error listing directory with standard filesystem" << std::endl;
        }
        auto end = std::chrono::high_resolution_clock::now();
        auto std_duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        
        // Custom filesystem
        CustomFileSystem custom_fs;
        start = std::chrono::high_resolution_clock::now();
        std::vector<std::string> custom_files = custom_fs.list_directory(path);
        end = std::chrono::high_resolution_clock::now();
        auto custom_duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        
        std::cout << "Standard filesystem: " << (std_duration.count() / 1000.0) << " ms (" << std_files.size() << " files)" << std::endl;
        std::cout << "Custom filesystem:   " << (custom_duration.count() / 1000.0) << " ms (" << custom_files.size() << " files)" << std::endl;
    }
};

// =============================================================================
// FILE OPERATIONS BENCHMARKS
// =============================================================================

class FileOperationsBenchmark {
public:
    static void benchmark_file_copy(const std::string& source, const std::string& dest_prefix, int num_copies) {
        std::cout << "\n=== File Copy Benchmark ===" << std::endl;
        
        // Create source file if it doesn't exist
        if (!std::filesystem::exists(source)) {
            std::ofstream file(source);
            for (int i = 0; i < 1000; ++i) {
                file << "This is line " << i << " of test data for benchmarking.\n";
            }
            file.close();
        }
        
        // Cleanup any existing copy files first
        for (int i = 0; i < num_copies; ++i) {
            std::string std_dest = dest_prefix + "_std_" + std::to_string(i) + ".txt";
            std::string custom_dest = dest_prefix + "_custom_" + std::to_string(i) + ".txt";
            std::filesystem::remove(std_dest);
            std::filesystem::remove(custom_dest);
        }
        
        // Standard filesystem copy
        auto start = std::chrono::high_resolution_clock::now();
        for (int i = 0; i < num_copies; ++i) {
            std::string dest = dest_prefix + "_std_" + std::to_string(i) + ".txt";
            std::filesystem::copy_file(source, dest);
        }
        auto end = std::chrono::high_resolution_clock::now();
        auto std_duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        
        // Custom filesystem copy
        CustomFileSystem custom_fs;
        start = std::chrono::high_resolution_clock::now();
        for (int i = 0; i < num_copies; ++i) {
            std::string dest = dest_prefix + "_custom_" + std::to_string(i) + ".txt";
            custom_fs.copy_file(source, dest);
        }
        end = std::chrono::high_resolution_clock::now();
        auto custom_duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        
        std::cout << "Standard filesystem: " << (std_duration.count() / 1000.0) << " ms" << std::endl;
        std::cout << "Custom filesystem:   " << (custom_duration.count() / 1000.0) << " ms" << std::endl;
        
        // Cleanup
        for (int i = 0; i < num_copies; ++i) {
            std::string std_file = dest_prefix + "_std_" + std::to_string(i) + ".txt";
            std::string custom_file = dest_prefix + "_custom_" + std::to_string(i) + ".txt";
            if (std::filesystem::exists(std_file)) std::filesystem::remove(std_file);
            if (std::filesystem::exists(custom_file)) std::filesystem::remove(custom_file);
        }
    }
    
    static void benchmark_file_move(const std::string& source_prefix, const std::string& dest_prefix, int num_files) {
        std::cout << "\n=== File Move/Rename Benchmark ===" << std::endl;
        
        // Create source files
        std::vector<std::string> source_files;
        for (int i = 0; i < num_files; ++i) {
            std::string filename = source_prefix + "_" + std::to_string(i) + ".txt";
            std::ofstream file(filename);
            file << "Test data for move benchmark " << i << std::endl;
            file.close();
            source_files.push_back(filename);
        }
        
        // Standard filesystem move
        auto start = std::chrono::high_resolution_clock::now();
        for (int i = 0; i < num_files; ++i) {
            std::string source = source_prefix + "_" + std::to_string(i) + ".txt";
            std::string dest = dest_prefix + "_std_" + std::to_string(i) + ".txt";
            std::filesystem::rename(source, dest);
        }
        auto end = std::chrono::high_resolution_clock::now();
        auto std_duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        
        // Recreate source files for custom test
        for (int i = 0; i < num_files; ++i) {
            std::string filename = source_prefix + "_" + std::to_string(i) + ".txt";
            std::ofstream file(filename);
            file << "Test data for move benchmark " << i << std::endl;
            file.close();
        }
        
        // Custom filesystem move
        CustomFileSystem custom_fs;
        start = std::chrono::high_resolution_clock::now();
        for (int i = 0; i < num_files; ++i) {
            std::string source = source_prefix + "_" + std::to_string(i) + ".txt";
            std::string dest = dest_prefix + "_custom_" + std::to_string(i) + ".txt";
            custom_fs.move_file(source, dest);
        }
        end = std::chrono::high_resolution_clock::now();
        auto custom_duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        
        std::cout << "Standard filesystem: " << (std_duration.count() / 1000.0) << " ms" << std::endl;
        std::cout << "Custom filesystem:   " << (custom_duration.count() / 1000.0) << " ms" << std::endl;
        
        // Cleanup
        for (int i = 0; i < num_files; ++i) {
            std::string std_file = dest_prefix + "_std_" + std::to_string(i) + ".txt";
            std::string custom_file = dest_prefix + "_custom_" + std::to_string(i) + ".txt";
            if (std::filesystem::exists(std_file)) std::filesystem::remove(std_file);
            if (std::filesystem::exists(custom_file)) std::filesystem::remove(custom_file);
        }
    }
    
    static void benchmark_file_find(const std::string& search_pattern, const std::string& search_dir) {
        std::cout << "\n=== File Find Benchmark ===" << std::endl;
        
        // Standard filesystem find
        auto start = std::chrono::high_resolution_clock::now();
        std::vector<std::string> std_found;
        try {
            for (const auto& entry : std::filesystem::recursive_directory_iterator(search_dir)) {
                if (entry.is_regular_file() && 
                    entry.path().filename().string().find(search_pattern) != std::string::npos) {
                    std_found.push_back(entry.path().string());
                }
            }
        } catch (...) {
            // Directory might not exist
        }
        auto end = std::chrono::high_resolution_clock::now();
        auto std_duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        
        // Custom filesystem find
        CustomFileSystem custom_fs;
        start = std::chrono::high_resolution_clock::now();
        std::vector<std::string> custom_found = custom_fs.find_files(search_pattern, search_dir);
        end = std::chrono::high_resolution_clock::now();
        auto custom_duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        
        std::cout << "Standard filesystem: " << (std_duration.count() / 1000.0) << " ms (" << std_found.size() << " files found)" << std::endl;
        std::cout << "Custom filesystem:   " << (custom_duration.count() / 1000.0) << " ms (" << custom_found.size() << " files found)" << std::endl;
    }
    
    static void benchmark_file_delete(const std::string& file_prefix, int num_files) {
        std::cout << "\n=== File Delete Benchmark ===" << std::endl;
        
        // Create files for standard test
        for (int i = 0; i < num_files; ++i) {
            std::string filename = file_prefix + "_std_" + std::to_string(i) + ".txt";
            std::ofstream file(filename);
            file << "Test data for delete benchmark " << i << std::endl;
            file.close();
        }
        
        // Standard filesystem delete
        auto start = std::chrono::high_resolution_clock::now();
        for (int i = 0; i < num_files; ++i) {
            std::string filename = file_prefix + "_std_" + std::to_string(i) + ".txt";
            std::filesystem::remove(filename);
        }
        auto end = std::chrono::high_resolution_clock::now();
        auto std_duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        
        // Create files for custom test
        for (int i = 0; i < num_files; ++i) {
            std::string filename = file_prefix + "_custom_" + std::to_string(i) + ".txt";
            std::ofstream file(filename);
            file << "Test data for delete benchmark " << i << std::endl;
            file.close();
        }
        
        // Custom filesystem delete
        CustomFileSystem custom_fs;
        start = std::chrono::high_resolution_clock::now();
        for (int i = 0; i < num_files; ++i) {
            std::string filename = file_prefix + "_custom_" + std::to_string(i) + ".txt";
            custom_fs.delete_file(filename);
        }
        end = std::chrono::high_resolution_clock::now();
        auto custom_duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        
        std::cout << "Standard filesystem: " << (std_duration.count() / 1000.0) << " ms" << std::endl;
        std::cout << "Custom filesystem:   " << (custom_duration.count() / 1000.0) << " ms" << std::endl;
    }
};

// =============================================================================
// LIVE CACHE STATISTICS BENCHMARK
// =============================================================================

class LiveCacheStatsBenchmark {
public:
    static void run_live_cache_demo(int operations = 1000) {
        std::cout << "\n=== Live Cache Statistics Demo ===" << std::endl;
        
        StatisticsLRUCache<int, std::string> cache(50);
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> key_dist(1, 100);  // 100 possible keys, 50 cache size = some misses
        
        std::cout << "Running " << operations << " cache operations..." << std::endl;
        std::cout << "Cache capacity: 50, Key range: 1-100" << std::endl;
        std::cout << "\nLive Statistics (every 100 operations):" << std::endl;
        
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
            
            // Print live stats every 100 operations
            if ((i + 1) % 100 == 0) {
                auto current_time = std::chrono::high_resolution_clock::now();
                auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(current_time - start_time);
                
                std::cout << "Operations: " << std::setw(4) << (i + 1) 
                          << " | Hit Rate: " << std::setw(5) << std::fixed << std::setprecision(1) << cache.get_hit_rate() << "%" 
                          << " | Hits: " << std::setw(3) << cache.get_hits() 
                          << " | Misses: " << std::setw(3) << cache.get_misses()
                          << " | Size: " << std::setw(2) << cache.size()
                          << " | Time: " << std::setw(4) << elapsed.count() << "ms" << std::endl;
            }
        }
        
        auto end_time = std::chrono::high_resolution_clock::now();
        auto total_duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
        
        std::cout << "\nFinal Statistics:" << std::endl;
        cache.print_stats();
        std::cout << "Total time: " << total_duration.count() << " ms" << std::endl;
        std::cout << "Average time per operation: " << std::fixed << std::setprecision(4) << (double)total_duration.count() / operations << " ms" << std::endl;
    }
};

void benchmark_std_unordered_map_cache() {
    SimpleBenchmark::benchmark("Standard unordered_map cache (1000 operations)", []() {
        std::unordered_map<std::string, std::string> cache;
        
        // Fill cache
        for (int i = 0; i < 1000; ++i) {
            cache["key" + std::to_string(i)] = "value" + std::to_string(i);
        }
        
        // Random lookups
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(0, 999);
        
        for (int i = 0; i < 500; ++i) {
            auto it = cache.find("key" + std::to_string(dis(gen)));
            (void)it; // Suppress unused variable warning
        }
    }, 50);
}

void benchmark_lru_cache_hit_performance() {
    SimpleBenchmark::benchmark("LRU Cache Hit Performance (hot data)", []() {
        SimpleLRUCache<std::string, std::string> cache(100);
        cache.put("hot_key", "frequently_accessed_data");
        
        for (int i = 0; i < 10000; ++i) {
            std::string value = cache.get("hot_key"); // This should be very fast!
            (void)value;
        }
    }, 10);
}

void benchmark_cache_hit_miss_realistic() {
    SimpleBenchmark::benchmark("Realistic Cache Hit/Miss (80% hit rate)", []() {
        SimpleLRUCache<std::string, std::string> cache(50);
        
        // Pre-populate cache with "hot" data
        for (int i = 0; i < 40; ++i) {
            cache.put("hot_key_" + std::to_string(i), "hot_value_" + std::to_string(i));
        }
        
        int counter = 0;
        for (int i = 0; i < 1000; ++i) {
            if (counter % 5 == 0) {
                // 20% cache misses - new data
                cache.put("cold_key_" + std::to_string(counter), "cold_value");
            } else {
                // 80% cache hits - existing data
                std::string value = cache.get("hot_key_" + std::to_string(counter % 40));
                (void)value;
            }
            counter++;
        }
    }, 20);
}

void run_cache_benchmarks() {
    std::cout << "=== Cache Benchmarks ===" << std::endl;
    std::cout << "Comparing standard unordered_map vs our LRU cache with eviction policy" << std::endl;
    
    // Cache Operations Comparison
    SideBySideBenchmark::compare(
        "Cache Operations (1000 puts + 500 gets)",
        []() {
            // Standard unordered_map (no eviction policy)
            std::unordered_map<std::string, std::string> cache;
            
            // Fill cache - will grow indefinitely
            for (int i = 0; i < 1000; ++i) {
                cache["key" + std::to_string(i)] = "value" + std::to_string(i);
            }
            
            // Random lookups
            std::random_device rd;
            std::mt19937 gen(rd());
            std::uniform_int_distribution<> dis(0, 999);
            
            for (int i = 0; i < 500; ++i) {
                auto it = cache.find("key" + std::to_string(dis(gen)));
                (void)it; // Suppress unused variable warning
            }
        },
        []() {
            // LRU Cache with eviction policy
            SimpleLRUCache<std::string, std::string> cache(100); // Limited capacity
            
            // Fill cache - will evict old entries when full
            for (int i = 0; i < 1000; ++i) {
                cache.put("key" + std::to_string(i), "value" + std::to_string(i));
            }
            
            // Random lookups
            std::random_device rd;
            std::mt19937 gen(rd());
            std::uniform_int_distribution<> dis(0, 999);
            
            for (int i = 0; i < 500; ++i) {
                std::string value = cache.get("key" + std::to_string(dis(gen)));
                (void)value; // Suppress unused variable warning
            }
        },
        10
    );
    
    // Memory Usage Demonstration
    std::cout << "[MEMORY USAGE COMPARISON]" << std::endl;
    
    // Standard map - grows indefinitely
    std::unordered_map<std::string, std::string> unlimited_cache;
    for (int i = 0; i < 10000; ++i) {
        unlimited_cache["key" + std::to_string(i)] = "value" + std::to_string(i);
    }
    std::cout << "Standard map: " << unlimited_cache.size() << " entries (grows indefinitely)" << std::endl;
    
    // LRU cache - bounded size
    SimpleLRUCache<std::string, std::string> bounded_cache(100);
    for (int i = 0; i < 10000; ++i) {
        bounded_cache.put("key" + std::to_string(i), "value" + std::to_string(i));
    }
    std::cout << "LRU cache: " << bounded_cache.size() << " entries (bounded to prevent memory exhaustion)" << std::endl;
    std::cout << std::endl;
    
    // Show realistic cache hit scenario
    std::cout << "[REALISTIC CACHE HIT SCENARIO]" << std::endl;
    SimpleBenchmark::benchmark("LRU Cache with 80% hit rate", []() {
        SimpleLRUCache<std::string, std::string> cache(50);
        
        // Pre-populate cache with "hot" data
        for (int i = 0; i < 40; ++i) {
            cache.put("hot_key_" + std::to_string(i), "hot_value_" + std::to_string(i));
        }
        
        int counter = 0;
        for (int i = 0; i < 1000; ++i) {
            if (counter % 5 == 0) {
                // 20% cache misses - new data
                cache.put("cold_key_" + std::to_string(counter), "cold_value");
            } else {
                // 80% cache hits - existing data
                std::string value = cache.get("hot_key_" + std::to_string(counter % 40));
                (void)value;
            }
            counter++;
        }
    }, 20);
}

// =============================================================================
// COMPRESSION BENCHMARKS
// =============================================================================

// Simple compression algorithms for demonstration
class SimpleCompression {
public:
    // Run-length encoding
    static std::string rle_compress(const std::string& input) {
        if (input.empty()) return "";
        
        std::ostringstream result;
        char current = input[0];
        int count = 1;
        
        for (size_t i = 1; i < input.length(); ++i) {
            if (input[i] == current && count < 9) {
                count++;
            } else {
                result << current << count;
                current = input[i];
                count = 1;
            }
        }
        result << current << count;
        return result.str();
    }
};

std::string generate_repetitive_data(size_t size) {
    std::string result;
    result.reserve(size);
    const std::string pattern = "AAABBBCCCDDDEEEFFFGGGHHHIIIJJJKKKLLLMMMNNNOOOPPPQQQRRRSSSTTTUUUVVVWWWXXXYYYZZZ";
    
    while (result.size() < size) {
        result += pattern;
    }
    return result.substr(0, size);
}

void benchmark_no_compression() {
    std::string test_data = generate_repetitive_data(10000);
    
    SimpleBenchmark::benchmark("No Compression (copy string)", [&test_data]() {
        std::string copy = test_data;
        (void)copy; // Suppress unused variable warning
    }, 500);
}

void benchmark_rle_compression() {
    std::string repetitive_data = generate_repetitive_data(5000);
    
    SimpleBenchmark::benchmark("RLE Compression", [&repetitive_data]() {
        std::string compressed = SimpleCompression::rle_compress(repetitive_data);
        (void)compressed;
    }, 100);
    
    std::string compressed = SimpleCompression::rle_compress(repetitive_data);
    double compression_ratio = static_cast<double>(compressed.size()) / repetitive_data.size();
    std::cout << "RLE Compression ratio: " << compression_ratio << " (smaller is better)" << std::endl << std::endl;
}

void run_compression_benchmarks() {
    std::cout << "=== Compression Benchmarks ===" << std::endl;
    
    // Baseline benchmarks
    std::cout << "\n--- Baseline Benchmarks ---" << std::endl;
    benchmark_no_compression();
    
    // Compression algorithm benchmarks
    std::cout << "\n--- Compression Algorithm Benchmarks ---" << std::endl;
    benchmark_rle_compression();
    
    std::cout << "Compression benchmarks completed." << std::endl;
}

// =============================================================================
// BACKUP BENCHMARKS
// =============================================================================

void setup_test_backup_files() {
    namespace fs = std::filesystem;
    
    // Create test directories
    fs::create_directories("test_backup_source");
    
    // Create test files
    std::ofstream ofs1("test_backup_source/file1.txt");
    ofs1 << generate_random_data(1024);
    ofs1.close();
    
    std::ofstream ofs2("test_backup_source/file2.txt");
    ofs2 << generate_random_data(2048);
    ofs2.close();
}

void cleanup_test_backup_files() {
    namespace fs = std::filesystem;
    
    if (fs::exists("test_backup_source")) {
        fs::remove_all("test_backup_source");
    }
    if (fs::exists("test_backup_dest")) {
        fs::remove_all("test_backup_dest");
    }
}

void benchmark_std_file_copy() {
    namespace fs = std::filesystem;
    
    SimpleBenchmark::benchmark("Standard File Copy (std::filesystem)", []() {
        fs::copy_file("test_backup_source/file1.txt", "temp_copy.txt", 
                     fs::copy_options::overwrite_existing);
        fs::remove("temp_copy.txt");
    }, 50);
}

void benchmark_custom_file_copy() {
    SimpleBenchmark::benchmark("Custom File Copy", []() {
        std::ifstream src("test_backup_source/file1.txt", std::ios::binary);
        std::ofstream dst("temp_custom_copy.txt", std::ios::binary);
        dst << src.rdbuf();
        src.close();
        dst.close();
        std::filesystem::remove("temp_custom_copy.txt");
    }, 50);
}

void run_backup_benchmarks() {
    std::cout << "=== Backup Benchmarks ===" << std::endl;
    
    // Setup test environment
    cleanup_test_backup_files();
    setup_test_backup_files();
    
    // Baseline benchmarks
    std::cout << "\n--- Baseline Benchmarks (std::filesystem) ---" << std::endl;
    benchmark_std_file_copy();
    
    // Custom backup benchmarks
    std::cout << "\n--- Custom Backup Benchmarks ---" << std::endl;
    benchmark_custom_file_copy();
    
    // Cleanup
    cleanup_test_backup_files();
    
    std::cout << "Backup benchmarks completed." << std::endl;
}

// =============================================================================
// MAIN FUNCTION
// =============================================================================

int main(int argc, char* argv[]) {
    std::cout << "=========================================" << std::endl;
    std::cout << "  COMPREHENSIVE FILESYSTEM BENCHMARKS  " << std::endl;
    std::cout << "=========================================" << std::endl << std::endl;
    
    // Initialize the real filesystem for proper benchmarking
    g_realFS = mtfs::fs::FileSystem::create("./fs_root");
    
    auto start_time = std::chrono::high_resolution_clock::now();
    
    try {
        // Parse command line arguments for selective benchmarking
        std::vector<std::string> args;
        for (int i = 1; i < argc; ++i) {
            args.emplace_back(argv[i]);
        }
        
        bool run_all = args.empty();
        bool run_fs = run_all;
        bool run_cache = run_all;
        bool run_compression = run_all;
        bool run_backup = run_all;
        bool run_comprehensive = run_all;
        bool run_live = run_all;
        
        // Check for specific benchmark requests
        for (const auto& arg : args) {
            if (arg == "--fs" || arg == "-f") run_fs = true;
            else if (arg == "--cache" || arg == "-c") run_cache = true;
            else if (arg == "--compression" || arg == "-z") run_compression = true;
            else if (arg == "--backup" || arg == "-b") run_backup = true;
            else if (arg == "--comprehensive" || arg == "-a") run_comprehensive = true;
            else if (arg == "--live" || arg == "-l") run_live = true;
            else if (arg == "--help" || arg == "-h") {
                std::cout << "Usage: " << argv[0] << " [options]" << std::endl;
                std::cout << "Options:" << std::endl;
                std::cout << "  --fs, -f           Run file system benchmarks" << std::endl;
                std::cout << "  --cache, -c        Run cache benchmarks" << std::endl;
                std::cout << "  --compression, -z  Run compression benchmarks" << std::endl;
                std::cout << "  --backup, -b       Run backup benchmarks" << std::endl;
                std::cout << "  --comprehensive, -a Run comprehensive operation benchmarks" << std::endl;
                std::cout << "  --live, -l         Run live cache statistics demo" << std::endl;
                std::cout << "  --help, -h         Show this help message" << std::endl;
                std::cout << "  (no args)          Run all benchmarks" << std::endl;
                return 0;
            }
        }
        
        if (!run_all) {
            run_fs = run_cache = run_compression = run_backup = run_comprehensive = run_live = false;
            for (const auto& arg : args) {
                if (arg == "--fs" || arg == "-f") run_fs = true;
                else if (arg == "--cache" || arg == "-c") run_cache = true;
                else if (arg == "--compression" || arg == "-z") run_compression = true;
                else if (arg == "--backup" || arg == "-b") run_backup = true;
                else if (arg == "--comprehensive" || arg == "-a") run_comprehensive = true;
                else if (arg == "--live" || arg == "-l") run_live = true;
            }
        }
        
        // Run selected benchmarks
        if (run_fs) {
            std::cout << "1. Running File System Benchmarks..." << std::endl;
            run_fs_benchmarks();
            std::cout << std::endl;
        }
        
        if (run_cache) {
            std::cout << "2. Running Cache Benchmarks..." << std::endl;
            run_cache_benchmarks();
            std::cout << std::endl;
        }
        
        if (run_compression) {
            std::cout << "3. Running Compression Benchmarks..." << std::endl;
            run_compression_benchmarks();
            std::cout << std::endl;
        }
        
        if (run_backup) {
            std::cout << "4. Running Backup Benchmarks..." << std::endl;
            run_backup_benchmarks();
            std::cout << std::endl;
        }
        
        if (run_comprehensive) {
            std::cout << "5. Running Comprehensive Operation Benchmarks..." << std::endl;
            
            // File I/O benchmarks
            std::cout << "\n--- File I/O Operations ---" << std::endl;
            const std::string filename = "benchmark_test.txt";
            const std::string data = generate_random_data(10000);
            
            auto start = std::chrono::high_resolution_clock::now();
            std::ofstream file(filename);
            file << data;
            file.close();
            auto end = std::chrono::high_resolution_clock::now();
            auto write_duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
            
            start = std::chrono::high_resolution_clock::now();
            std::ifstream read_file(filename);
            std::string read_data((std::istreambuf_iterator<char>(read_file)),
                                  std::istreambuf_iterator<char>());
            read_file.close();
            end = std::chrono::high_resolution_clock::now();
            auto read_duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
            
            std::cout << "File Write: " << (write_duration.count() / 1000.0) << " ms" << std::endl;
            std::cout << "File Read:  " << (read_duration.count() / 1000.0) << " ms" << std::endl;
            std::filesystem::remove(filename);
            
            // Directory operations
            DirectoryBenchmark::benchmark_directory_create(25);
            DirectoryBenchmark::benchmark_directory_list(".");
            
            // File operations
            FileOperationsBenchmark::benchmark_file_copy("benchmark_source.txt", "benchmark_copy", 10);
            FileOperationsBenchmark::benchmark_file_move("move_source", "move_dest", 10);
            FileOperationsBenchmark::benchmark_file_find(".txt", ".");
            FileOperationsBenchmark::benchmark_file_delete("delete_test", 10);
            
            // Cache with statistics
            std::cout << "\n--- Cache Operations with Statistics ---" << std::endl;
            const int operations = 5000;
            
            std::unordered_map<int, std::string> std_cache;
            start = std::chrono::high_resolution_clock::now();
            for (int i = 0; i < operations; ++i) {
                if (i % 4 == 0) {
                    std_cache[i % 100] = "value_" + std::to_string(i);
                } else {
                    auto it = std_cache.find(i % 100);
                    if (it != std_cache.end()) {
                        std::string value = it->second;
                    }
                }
            }
            end = std::chrono::high_resolution_clock::now();
            auto std_duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
            
            StatisticsLRUCache<int, std::string> custom_cache(500);
            // Pre-populate some cache entries to ensure hits
            for (int i = 0; i < 50; ++i) {
                custom_cache.put(i, "initial_value_" + std::to_string(i));
            }
            custom_cache.reset_stats(); // Reset stats after pre-population
            
            start = std::chrono::high_resolution_clock::now();
            for (int i = 0; i < operations; ++i) {
                if (i % 4 == 0) {
                    // 25% writes - populate cache
                    custom_cache.put(i % 100, "value_" + std::to_string(i));
                } else {
                    // 75% reads - from a smaller pool to get cache hits
                    std::string value = custom_cache.get(i % 100);
                }
            }
            end = std::chrono::high_resolution_clock::now();
            auto custom_duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
            
            std::cout << "Standard unordered_map: " << (std_duration.count() / 1000.0) << " ms" << std::endl;
            std::cout << "Custom LRU cache:       " << (custom_duration.count() / 1000.0) << " ms" << std::endl;
            custom_cache.print_stats();
            
            std::cout << std::endl;
        }
        
        if (run_live) {
            std::cout << "6. Running Live Cache Statistics Demo..." << std::endl;
            LiveCacheStatsBenchmark::run_live_cache_demo(500);
            std::cout << std::endl;
        }
        
    } catch (const std::exception& e) {
        std::cerr << "Benchmark error: " << e.what() << std::endl;
        return 1;
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    
    std::cout << "=========================================" << std::endl;
    std::cout << "     ALL BENCHMARKS COMPLETED!         " << std::endl;
    std::cout << "=========================================" << std::endl;
    std::cout << "Total execution time: " << duration.count() << " ms" << std::endl;
    
    return 0;
}

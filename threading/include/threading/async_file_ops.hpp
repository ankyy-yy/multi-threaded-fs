#pragma once

#include <string>
#include <vector>
#include <future>
#include <memory>
#include <functional>
#include "threading/thread_pool.hpp"

// Forward declaration
namespace mtfs::fs {
    class FileSystem;
}

namespace mtfs::threading {

class AsyncFileOperations {
public:
    explicit AsyncFileOperations(mtfs::fs::FileSystem* fs, ThreadPool& pool);
    ~AsyncFileOperations() = default;

    // Async file operations
    std::future<std::string> readFileAsync(const std::string& path);
    std::future<bool> writeFileAsync(const std::string& path, const std::string& content);
    std::future<bool> copyFileAsync(const std::string& source, const std::string& destination);
    std::future<bool> moveFileAsync(const std::string& source, const std::string& destination);
    std::future<bool> deleteFileAsync(const std::string& path);
    
    // Async directory operations
    std::future<bool> createDirectoryAsync(const std::string& path);
    std::future<std::vector<std::string>> listDirectoryAsync(const std::string& path);
    std::future<std::vector<std::string>> listFilesAsync(const std::string& pattern);
    std::future<bool> deleteDirectoryAsync(const std::string& path);
    
    // Async compression operations
    std::future<bool> compressFileAsync(const std::string& path);
    std::future<bool> decompressFileAsync(const std::string& path);
    
    // Async backup operations
    std::future<bool> createBackupAsync(const std::string& backupName, const std::string& sourcePath);
    std::future<bool> restoreBackupAsync(const std::string& backupName, const std::string& targetPath);
    
    // Batch operations
    std::future<std::vector<bool>> batchCopyAsync(const std::vector<std::pair<std::string, std::string>>& operations);
    std::future<std::vector<bool>> batchDeleteAsync(const std::vector<std::string>& paths);
    
    // Progress tracking
    struct OperationProgress {
        size_t totalOperations{0};
        size_t completedOperations{0};
        size_t failedOperations{0};
        std::chrono::steady_clock::time_point startTime;
        bool isComplete{false};
        
        double getProgressPercentage() const {
            return totalOperations > 0 ? (static_cast<double>(completedOperations) / totalOperations) * 100.0 : 0.0;
        }
        
        std::chrono::milliseconds getElapsedTime() const {
            return std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - startTime
            );
        }
    };
    
    // Progress callback type
    using ProgressCallback = std::function<void(const OperationProgress&)>;
    
    // Async operations with progress tracking
    std::future<bool> batchCopyWithProgressAsync(
        const std::vector<std::pair<std::string, std::string>>& operations,
        ProgressCallback callback = nullptr
    );
    
    // Get operation statistics
    struct OperationStats {
        size_t totalOperationsStarted{0};
        size_t totalOperationsCompleted{0};
        size_t totalOperationsFailed{0};
        std::chrono::milliseconds totalExecutionTime{0};
        size_t activeOperations{0};
    };
    
    OperationStats getStats() const;
    void resetStats();

private:
    mtfs::fs::FileSystem* filesystem;
    ThreadPool& threadPool;
    
    // Statistics tracking
    mutable std::mutex statsMutex;
    mutable OperationStats stats;
    
    void updateStats(bool success, std::chrono::milliseconds duration);
    
    // Helper for batch operations
    template<typename Operation>
    std::future<std::vector<bool>> executeBatchOperation(
        const std::vector<typename Operation::InputType>& inputs,
        Operation operation,
        ProgressCallback callback = nullptr
    );
};

} // namespace mtfs::threading

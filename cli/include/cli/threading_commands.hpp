#pragma once

#include <string>
#include <vector>
#include <future>
#include <memory>
#include "fs/filesystem.hpp"
#include "threading/thread_pool.hpp"
#include "threading/async_file_ops.hpp"
#include "threading/parallel_backup.hpp"

namespace mtfs::cli {

class ThreadingCommands {
public:
    explicit ThreadingCommands(mtfs::fs::FileSystem* fs);
    ~ThreadingCommands() = default;

    // Thread pool management commands
    void showThreadPoolStatus();
    void setThreadPoolSize(size_t numThreads);
    void showThreadingHelp();
    
    // Async file operation commands
    void asyncReadFile(const std::string& path);
    void asyncWriteFile(const std::string& path, const std::string& content);
    void asyncCopyFile(const std::string& source, const std::string& destination);
    void asyncMoveFile(const std::string& source, const std::string& destination);
    void asyncDeleteFile(const std::string& path);
    
    // Batch operation commands
    void batchCopyFiles(const std::vector<std::pair<std::string, std::string>>& operations);
    void batchDeleteFiles(const std::vector<std::string>& paths);
    
    // Parallel backup commands
    void createParallelBackup(const std::string& backupName, const std::vector<std::string>& sourcePaths);
    void verifyBackupAsync(const std::string& backupName);
    void showBackupProgress(const std::string& backupName);
    
    // Performance monitoring
    void showAsyncOperationStats();
    void resetAsyncOperationStats();
    void showConcurrentCacheStats();
    
    // Background task management
    void listActiveBackgroundTasks();
    void cancelBackgroundTask(const std::string& taskId);
    void waitForAllTasks();

private:
    mtfs::fs::FileSystem* filesystem;
    std::unique_ptr<mtfs::threading::AsyncFileOperations> asyncOps;
    std::unique_ptr<mtfs::threading::ParallelBackupManager> parallelBackup;
    
    // Active task tracking
    struct ActiveTask {
        std::string id;
        std::string description;
        std::chrono::steady_clock::time_point startTime;
        std::future<void> future;
        bool isCompleted{false};
    };
    
    std::vector<std::shared_ptr<ActiveTask>> activeTasks;
    std::mutex tasksMutex;
    
    // Helper methods
    std::string generateTaskId();
    void cleanupCompletedTasks();
    void displayProgressBar(double percentage, const std::string& label = "");
    
    // Progress callbacks
    static void backupProgressCallback(const mtfs::threading::ParallelBackupManager::BackupProgress& progress);
};

} // namespace mtfs::cli

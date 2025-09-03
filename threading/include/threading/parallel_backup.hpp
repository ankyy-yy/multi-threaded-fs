#pragma once

#include <string>
#include <vector>
#include <future>
#include <memory>
#include <atomic>
#include <mutex>
#include <functional>
#include "threading/thread_pool.hpp"

namespace mtfs::threading {

class ParallelBackupManager {
public:
    explicit ParallelBackupManager(size_t numThreads = std::thread::hardware_concurrency());
    ~ParallelBackupManager() = default;

    // Parallel backup operations
    struct BackupTask {
        std::string sourcePath;
        std::string backupPath;
        bool compress{true};
        bool verify{true};
    };
    
    struct BackupProgress {
        std::atomic<size_t> filesProcessed{0};
        std::atomic<size_t> totalFiles{0};
        std::atomic<size_t> bytesProcessed{0};
        std::atomic<size_t> totalBytes{0};
        std::atomic<size_t> filesCompressed{0};
        std::atomic<size_t> compressionSaved{0};
        std::atomic<bool> isComplete{false};
        std::atomic<bool> hasErrors{false};
        std::chrono::steady_clock::time_point startTime;
        
        double getFileProgress() const {
            return totalFiles > 0 ? (static_cast<double>(filesProcessed) / totalFiles) * 100.0 : 0.0;
        }
        
        double getByteProgress() const {
            return totalBytes > 0 ? (static_cast<double>(bytesProcessed) / totalBytes) * 100.0 : 0.0;
        }
        
        double getCompressionRatio() const {
            return bytesProcessed > 0 ? (static_cast<double>(compressionSaved) / bytesProcessed) : 0.0;
        }
    };
    
    using ProgressCallback = std::function<void(const BackupProgress&)>;
    
    // Main backup operations
    std::future<bool> createParallelBackup(
        const std::string& backupName,
        const std::vector<std::string>& sourcePaths,
        ProgressCallback callback = nullptr
    );
    
    std::future<bool> restoreParallelBackup(
        const std::string& backupName,
        const std::string& targetPath,
        ProgressCallback callback = nullptr
    );
    
    // Incremental backup
    std::future<bool> createIncrementalBackup(
        const std::string& backupName,
        const std::string& baseBackup,
        const std::vector<std::string>& sourcePaths,
        ProgressCallback callback = nullptr
    );
    
    // Verification operations
    std::future<bool> verifyBackupIntegrity(
        const std::string& backupName,
        ProgressCallback callback = nullptr
    );
    
    // Backup comparison
    std::future<std::vector<std::string>> compareBackups(
        const std::string& backup1,
        const std::string& backup2
    );
    
    // Cleanup operations
    std::future<bool> cleanupOldBackups(
        int maxBackupsToKeep,
        ProgressCallback callback = nullptr
    );
    
    // Statistics
    struct BackupStats {
        size_t totalBackupsCreated{0};
        size_t totalBackupsRestored{0};
        size_t totalFilesBackedUp{0};
        size_t totalBytesBackedUp{0};
        size_t totalCompressionSaved{0};
        std::chrono::milliseconds totalBackupTime{0};
        std::chrono::milliseconds totalRestoreTime{0};
        double averageCompressionRatio{0.0};
    };
    
    BackupStats getStats() const;
    void resetStats();
    
    // Pool management
    void setThreadCount(size_t numThreads);
    size_t getThreadCount() const;
    bool isBusy() const;

private:
    std::unique_ptr<ThreadPool> backupThreadPool;
    
    // Statistics
    mutable std::mutex statsMutex;
    mutable BackupStats stats;
    
    // Internal backup operations
    bool backupFile(const std::string& sourcePath, const std::string& backupPath, bool compress);
    bool restoreFile(const std::string& backupPath, const std::string& targetPath);
    bool verifyFile(const std::string& backupPath, const std::string& originalPath);
    
    // Directory scanning
    std::vector<std::string> scanDirectory(const std::string& path, bool recursive = true);
    size_t calculateDirectorySize(const std::string& path);
    
    // Utility functions
    void updateStats(const BackupStats& increment);
    std::string generateBackupMetadata(const BackupTask& task);
    bool saveBackupMetadata(const std::string& backupName, const std::string& metadata);
    std::string loadBackupMetadata(const std::string& backupName);
};

} // namespace mtfs::threading

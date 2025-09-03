#include "threading/parallel_backup.hpp"
#include <filesystem>
#include <algorithm>
#include <fstream>

namespace mtfs::threading {

ParallelBackupManager::ParallelBackupManager(size_t numThreads) 
    : backupThreadPool(std::make_unique<ThreadPool>(numThreads)) {
}

std::future<bool> ParallelBackupManager::createParallelBackup(
    const std::string& backupName,
    const std::vector<std::string>& sourcePaths,
    ProgressCallback callback) {
    
    return backupThreadPool->enqueue([this, backupName, sourcePaths, callback]() -> bool {
        BackupProgress progress;
        progress.startTime = std::chrono::steady_clock::now();
        
        // Calculate total work
        for (const auto& path : sourcePaths) {
            auto files = scanDirectory(path);
            progress.totalFiles += files.size();
            progress.totalBytes += calculateDirectorySize(path);
        }
        
        if (callback) {
            callback(progress);
        }
        
        // Create backup directory
        std::string backupDir = "backups/" + backupName;
        std::filesystem::create_directories(backupDir);
        
        // Process files in parallel
        std::vector<std::future<bool>> futures;
        
        for (const auto& sourcePath : sourcePaths) {
            auto files = scanDirectory(sourcePath);
            
            for (const auto& file : files) {
                std::string relativePath = std::filesystem::relative(file, sourcePath).string();
                std::string backupPath = backupDir + "/" + relativePath;
                
                futures.push_back(backupThreadPool->enqueue([this, file, backupPath, &progress, callback]() -> bool {
                    bool success = backupFile(file, backupPath, true);
                    
                    progress.filesProcessed++;
                    try {
                        auto fileSize = std::filesystem::file_size(file);
                        progress.bytesProcessed += fileSize;
                        
                        if (success && std::filesystem::exists(backupPath)) {
                            auto backupSize = std::filesystem::file_size(backupPath);
                            if (backupSize < fileSize) {
                                progress.filesCompressed++;
                                progress.compressionSaved += (fileSize - backupSize);
                            }
                        }
                    } catch (...) {
                        // File size calculation failed, continue
                    }
                    
                    if (!success) {
                        progress.hasErrors = true;
                    }
                    
                    if (callback) {
                        callback(progress);
                    }
                    
                    return success;
                }));
            }
        }
        
        // Wait for all backup operations to complete
        bool allSuccess = true;
        for (auto& future : futures) {
            try {
                if (!future.get()) {
                    allSuccess = false;
                }
            } catch (...) {
                allSuccess = false;
                progress.hasErrors = true;
            }
        }
        
        progress.isComplete = true;
        if (callback) {
            callback(progress);
        }
        
        // Update statistics
        BackupStats increment;
        increment.totalBackupsCreated = 1;
        increment.totalFilesBackedUp = progress.filesProcessed;
        increment.totalBytesBackedUp = progress.bytesProcessed;
        increment.totalCompressionSaved = progress.compressionSaved;
        increment.totalBackupTime = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - progress.startTime
        );
        increment.averageCompressionRatio = progress.getCompressionRatio();
        
        updateStats(increment);
        
        return allSuccess && !progress.hasErrors;
    });
}

std::future<bool> ParallelBackupManager::verifyBackupIntegrity(
    const std::string& backupName,
    ProgressCallback callback) {
    
    return backupThreadPool->enqueue([this, backupName, callback]() -> bool {
        BackupProgress progress;
        progress.startTime = std::chrono::steady_clock::now();
        
        std::string backupDir = "backups/" + backupName;
        if (!std::filesystem::exists(backupDir)) {
            return false;
        }
        
        auto files = scanDirectory(backupDir);
        progress.totalFiles = files.size();
        
        if (callback) {
            callback(progress);
        }
        
        std::vector<std::future<bool>> futures;
        
        for (const auto& file : files) {
            futures.push_back(backupThreadPool->enqueue([this, file, &progress, callback]() -> bool {
                // Simple file existence and size check
                bool isValid = std::filesystem::exists(file) && std::filesystem::file_size(file) > 0;
                
                progress.filesProcessed++;
                if (!isValid) {
                    progress.hasErrors = true;
                }
                
                if (callback) {
                    callback(progress);
                }
                
                return isValid;
            }));
        }
        
        bool allValid = true;
        for (auto& future : futures) {
            try {
                if (!future.get()) {
                    allValid = false;
                }
            } catch (...) {
                allValid = false;
                progress.hasErrors = true;
            }
        }
        
        progress.isComplete = true;
        if (callback) {
            callback(progress);
        }
        
        return allValid && !progress.hasErrors;
    });
}

std::vector<std::string> ParallelBackupManager::scanDirectory(const std::string& path, bool recursive) {
    std::vector<std::string> files;
    
    try {
        if (recursive) {
            for (const auto& entry : std::filesystem::recursive_directory_iterator(path)) {
                if (entry.is_regular_file()) {
                    files.push_back(entry.path().string());
                }
            }
        } else {
            for (const auto& entry : std::filesystem::directory_iterator(path)) {
                if (entry.is_regular_file()) {
                    files.push_back(entry.path().string());
                }
            }
        }
    } catch (...) {
        // Directory scanning failed
    }
    
    return files;
}

size_t ParallelBackupManager::calculateDirectorySize(const std::string& path) {
    size_t totalSize = 0;
    
    try {
        for (const auto& entry : std::filesystem::recursive_directory_iterator(path)) {
            if (entry.is_regular_file()) {
                totalSize += std::filesystem::file_size(entry);
            }
        }
    } catch (...) {
        // Size calculation failed
    }
    
    return totalSize;
}

bool ParallelBackupManager::backupFile(const std::string& sourcePath, const std::string& backupPath, bool compress) {
    try {
        // Ensure backup directory exists
        std::filesystem::create_directories(std::filesystem::path(backupPath).parent_path());
        
        // Simple file copy for now (compression would be implemented here)
        std::filesystem::copy_file(sourcePath, backupPath, std::filesystem::copy_options::overwrite_existing);
        
        return true;
    } catch (...) {
        return false;
    }
}

ParallelBackupManager::BackupStats ParallelBackupManager::getStats() const {
    std::lock_guard<std::mutex> lock(statsMutex);
    return stats;
}

void ParallelBackupManager::resetStats() {
    std::lock_guard<std::mutex> lock(statsMutex);
    stats = BackupStats{};
}

void ParallelBackupManager::updateStats(const BackupStats& increment) {
    std::lock_guard<std::mutex> lock(statsMutex);
    stats.totalBackupsCreated += increment.totalBackupsCreated;
    stats.totalBackupsRestored += increment.totalBackupsRestored;
    stats.totalFilesBackedUp += increment.totalFilesBackedUp;
    stats.totalBytesBackedUp += increment.totalBytesBackedUp;
    stats.totalCompressionSaved += increment.totalCompressionSaved;
    stats.totalBackupTime += increment.totalBackupTime;
    stats.totalRestoreTime += increment.totalRestoreTime;
    
    // Update average compression ratio
    if (stats.totalBytesBackedUp > 0) {
        stats.averageCompressionRatio = static_cast<double>(stats.totalCompressionSaved) / stats.totalBytesBackedUp;
    }
}

bool ParallelBackupManager::isBusy() const {
    return backupThreadPool->isBusy();
}

void ParallelBackupManager::setThreadCount(size_t numThreads) {
    backupThreadPool->resize(numThreads);
}

size_t ParallelBackupManager::getThreadCount() const {
    return backupThreadPool->getThreadCount();
}

} // namespace mtfs::threading

#include "threading/async_file_ops.hpp"
#include "fs/filesystem.hpp"
#include <chrono>

namespace mtfs::threading {

AsyncFileOperations::AsyncFileOperations(mtfs::fs::FileSystem* fs, ThreadPool& pool) 
    : filesystem(fs), threadPool(pool) {
}

std::future<std::vector<std::string>> AsyncFileOperations::listFilesAsync(const std::string& pattern) {
    return threadPool.enqueue([this, pattern]() -> std::vector<std::string> {
        auto start = std::chrono::steady_clock::now();
        try {
            auto result = this->filesystem->findFiles(pattern);
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - start
            );
            updateStats(true, duration);
            return result;
        } catch (...) {
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - start
            );
            updateStats(false, duration);
            throw;
        }
    });
}

std::future<std::string> AsyncFileOperations::readFileAsync(const std::string& path) {
    return threadPool.enqueue([this, path]() -> std::string {
        auto start = std::chrono::steady_clock::now();
        try {
            auto result = this->filesystem->readFile(path);
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - start
            );
            updateStats(true, duration);
            return result;
        } catch (...) {
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - start
            );
            updateStats(false, duration);
            throw;
        }
    });
}

std::future<bool> AsyncFileOperations::writeFileAsync(const std::string& path, const std::string& content) {
    return threadPool.enqueue([this, path, content]() -> bool {
        auto start = std::chrono::steady_clock::now();
        try {
            this->filesystem->writeFile(path, content);
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - start
            );
            updateStats(true, duration);
            return true;
        } catch (...) {
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - start
            );
            updateStats(false, duration);
            return false;
        }
    });
}

std::future<bool> AsyncFileOperations::copyFileAsync(const std::string& source, const std::string& destination) {
    return threadPool.enqueue([this, source, destination]() -> bool {
        auto start = std::chrono::steady_clock::now();
        try {
            auto result = this->filesystem->copyFile(source, destination);
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - start
            );
            updateStats(result, duration);
            return result;
        } catch (...) {
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - start
            );
            updateStats(false, duration);
            return false;
        }
    });
}

std::future<bool> AsyncFileOperations::moveFileAsync(const std::string& source, const std::string& destination) {
    return threadPool.enqueue([this, source, destination]() -> bool {
        auto start = std::chrono::steady_clock::now();
        try {
            auto result = this->filesystem->moveFile(source, destination);
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - start
            );
            updateStats(result, duration);
            return result;
        } catch (...) {
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - start
            );
            updateStats(false, duration);
            return false;
        }
    });
}

std::future<bool> AsyncFileOperations::deleteFileAsync(const std::string& path) {
    return threadPool.enqueue([this, path]() -> bool {
        auto start = std::chrono::steady_clock::now();
        try {
            auto result = this->filesystem->deleteFile(path);
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - start
            );
            updateStats(result, duration);
            return result;
        } catch (...) {
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - start
            );
            updateStats(false, duration);
            return false;
        }
    });
}

std::future<bool> AsyncFileOperations::createDirectoryAsync(const std::string& path) {
    return threadPool.enqueue([this, path]() -> bool {
        auto start = std::chrono::steady_clock::now();
        try {
            auto result = this->filesystem->createDirectory(path);
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - start
            );
            updateStats(result, duration);
            return result;
        } catch (...) {
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - start
            );
            updateStats(false, duration);
            return false;
        }
    });
}

std::future<std::vector<std::string>> AsyncFileOperations::listDirectoryAsync(const std::string& path) {
    return threadPool.enqueue([this, path]() -> std::vector<std::string> {
        auto start = std::chrono::steady_clock::now();
        try {
            auto result = this->filesystem->listDirectory(path);
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - start
            );
            updateStats(true, duration);
            return result;
        } catch (...) {
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - start
            );
            updateStats(false, duration);
            throw;
        }
    });
}

std::future<std::vector<bool>> AsyncFileOperations::batchCopyAsync(
    const std::vector<std::pair<std::string, std::string>>& operations) {
    
    return threadPool.enqueue([this, operations]() -> std::vector<bool> {
        std::vector<std::future<bool>> futures;
        
        for (const auto& op : operations) {
            futures.push_back(copyFileAsync(op.first, op.second));
        }
        
        std::vector<bool> results;
        for (auto& future : futures) {
            try {
                results.push_back(future.get());
            } catch (...) {
                results.push_back(false);
            }
        }
        
        return results;
    });
}

std::future<bool> AsyncFileOperations::batchCopyWithProgressAsync(
    const std::vector<std::pair<std::string, std::string>>& operations,
    ProgressCallback callback) {
    
    return threadPool.enqueue([this, operations, callback]() -> bool {
        OperationProgress progress;
        progress.totalOperations = operations.size();
        progress.startTime = std::chrono::steady_clock::now();
        
        if (callback) {
            callback(progress);
        }
        
        std::vector<std::future<bool>> futures;
        for (const auto& op : operations) {
            futures.push_back(copyFileAsync(op.first, op.second));
        }
        
        bool allSuccess = true;
        for (auto& future : futures) {
            try {
                bool success = future.get();
                if (success) {
                    progress.completedOperations++;
                } else {
                    progress.failedOperations++;
                    allSuccess = false;
                }
            } catch (...) {
                progress.failedOperations++;
                allSuccess = false;
            }
            
            if (callback) {
                callback(progress);
            }
        }
        
        progress.isComplete = true;
        if (callback) {
            callback(progress);
        }
        
        return allSuccess;
    });
}

AsyncFileOperations::OperationStats AsyncFileOperations::getStats() const {
    std::lock_guard<std::mutex> lock(statsMutex);
    return stats;
}

void AsyncFileOperations::resetStats() {
    std::lock_guard<std::mutex> lock(statsMutex);
    stats = OperationStats{};
}

void AsyncFileOperations::updateStats(bool success, std::chrono::milliseconds duration) {
    std::lock_guard<std::mutex> lock(statsMutex);
    stats.totalOperationsStarted++;
    
    if (success) {
        stats.totalOperationsCompleted++;
    } else {
        stats.totalOperationsFailed++;
    }
    
    stats.totalExecutionTime += duration;
}

} // namespace mtfs::threading

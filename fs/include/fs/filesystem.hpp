#pragma once

#include <string>
#include <vector>
#include <memory>
#include <cstdint>
#include <chrono>
#include <unordered_map>
#include <future>
#include "common/error.hpp"
#include "common/auth.hpp"
#include "cache/enhanced_cache.hpp"
#include "fs/compression.hpp"
#include "fs/backup_manager.hpp"

namespace mtfs::fs {

struct FileMetadata {
    std::string name;
    std::size_t size{0};
    bool isDirectory{false};
    std::chrono::system_clock::time_point createdAt;
    std::chrono::system_clock::time_point modifiedAt;
    uint32_t permissions{0644};  // Default Unix-style permissions
    std::string owner;           // Username of file owner
    std::string group;           // Group (optional, for future use)
};

struct PerformanceStats {
    size_t cacheHits{0};
    size_t cacheMisses{0};
    size_t totalReads{0};
    size_t totalWrites{0};
    size_t totalFileOperations{0};
    double avgReadTime{0.0};
    double avgWriteTime{0.0};
    std::chrono::system_clock::time_point lastResetTime;
    
    PerformanceStats() : lastResetTime(std::chrono::system_clock::now()) {}
    
    double getCacheHitRate() const {
        size_t total = cacheHits + cacheMisses;
        return total > 0 ? (static_cast<double>(cacheHits) / total) * 100.0 : 0.0;
    }
};

class FileSystem {
public:
    // Factory method
    static std::shared_ptr<FileSystem> create(const std::string& rootPath, mtfs::common::AuthManager* auth = nullptr);

    // Basic file operations
    bool createFile(const std::string& path);
    bool writeFile(const std::string& path, const std::string& data);
    std::string readFile(const std::string& path);
    bool deleteFile(const std::string& path);
    
    // Directory operations
    bool createDirectory(const std::string& path);
    std::vector<std::string> listDirectory(const std::string& path);
    
    // Advanced file operations
    bool copyFile(const std::string& source, const std::string& destination);
    bool moveFile(const std::string& source, const std::string& destination);
    bool renameFile(const std::string& oldName, const std::string& newName);
    std::vector<std::string> findFiles(const std::string& pattern, const std::string& directory = ".");
    FileMetadata getFileInfo(const std::string& path);
    
    // Advanced operations
    std::size_t write(const std::string& path, const void* buffer, std::size_t size, std::size_t offset);
    std::size_t read(const std::string& path, void* buffer, std::size_t size, std::size_t offset);
    void setPermissions(const std::string& path, uint32_t permissions);
    FileMetadata getMetadata(const std::string& path);
    
    // System operations
    void sync();
    void mount();
    void unmount();
    bool exists(const std::string& path);
    
    // Async file operations (multi-threaded)
    std::future<std::string> readFileAsync(const std::string& path);
    std::future<bool> writeFileAsync(const std::string& path, const std::string& content);
    std::future<bool> copyFileAsync(const std::string& source, const std::string& destination);
    std::future<bool> moveFileAsync(const std::string& source, const std::string& destination);
    std::future<bool> deleteFileAsync(const std::string& path);
    std::future<bool> createDirectoryAsync(const std::string& path);
    std::future<std::vector<std::string>> listDirectoryAsync(const std::string& path);
    
    // Batch async operations
    std::future<std::vector<bool>> batchCopyAsync(const std::vector<std::pair<std::string, std::string>>& operations);
    std::future<std::vector<bool>> batchDeleteAsync(const std::vector<std::string>& paths);
    
      // Cache control
    void clearCache();
    size_t getCacheSize() const;
    
    // Enhanced cache management
    void setCachePolicy(cache::CachePolicy policy);
    cache::CachePolicy getCachePolicy() const;
    void resizeCache(size_t newCapacity);
    void pinFile(const std::string& path);
    void unpinFile(const std::string& path);
    bool isFilePinned(const std::string& path) const;
    void prefetchFile(const std::string& path);
    cache::CacheStatistics getCacheStatistics() const;
    void resetCacheStatistics();
    void showCacheAnalytics() const;
    std::vector<std::string> getHotFiles(size_t count = 10) const;
    
    // Performance monitoring
    PerformanceStats getStats() const;
    void resetStats();
    void showPerformanceDashboard() const;
    
    // File compression
    bool compressFile(const std::string& filePath);
    bool decompressFile(const std::string& filePath);
    CompressionStats getCompressionStats() const;
    void resetCompressionStats();
    
    // Backup system
    bool createBackup(const std::string& backupName);
    bool restoreBackup(const std::string& backupName, const std::string& targetDirectory = "");
    bool deleteBackup(const std::string& backupName);
    std::vector<std::string> listBackups() const;
    void showBackupDashboard() const;
    BackupStats getBackupStats() const;
    
    virtual ~FileSystem() = default;

protected:
    explicit FileSystem(const std::string& rootPath, mtfs::common::AuthManager* auth = nullptr);

private:
    FileMetadata resolvePath(const std::string& path);
    
    // Helper function for glob pattern matching
    bool matchesPattern(const std::string& filename, const std::string& pattern);
    
    std::string rootPath;
    
    // Enhanced cache for file contents
    static constexpr size_t CACHE_CAPACITY = 1000;
    std::unique_ptr<cache::CacheManager<std::string, std::string>> enhancedCache;
    
    // Performance statistics
    mutable PerformanceStats stats;
    
    // Compression statistics
    mutable CompressionStats compressionStats;
    
    // Backup manager
    std::unique_ptr<BackupManager> backupManager;

    // Auth manager for permission checks
    mtfs::common::AuthManager* authManager{nullptr};

    // Metadata persistence
    std::unordered_map<std::string, FileMetadata> fileMetadataMap;
    std::string metadataFilePath;
    bool saveMetadata() const;
    bool loadMetadata();
};

} // namespace mtfs::fs
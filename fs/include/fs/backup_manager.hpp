#pragma once

#include <string>
#include <vector>
#include <map>
#include <chrono>
#include <memory>
#include <stdexcept>
#include "common/error.hpp"

namespace mtfs::fs {

// Forward declaration
class FileSystem;

struct BackupMetadata {
    std::string backupName;
    std::string backupPath;
    std::chrono::system_clock::time_point createdAt;
    std::chrono::system_clock::time_point lastModified;
    size_t totalFiles{0};
    size_t totalSize{0};
    bool isIncremental{false};
    std::string parentBackup; // For incremental backups
    std::vector<std::string> includedFiles;
    
    BackupMetadata() : createdAt(std::chrono::system_clock::now()), 
                      lastModified(std::chrono::system_clock::now()) {}
};

struct BackupStats {
    size_t totalBackups{0};
    size_t totalBackupSize{0};
    size_t filesBackedUp{0};
    std::chrono::system_clock::time_point lastBackupTime;
    double compressionRatio{0.0};
    
    BackupStats() : lastBackupTime(std::chrono::system_clock::now()) {}
};

class BackupManager {
public:
    explicit BackupManager(const std::string& backupDirectory);
    ~BackupManager() = default;

    // Core backup operations
    bool createBackup(const std::string& backupName, const std::string& sourceDirectory);
    bool createIncrementalBackup(const std::string& backupName, const std::string& parentBackup, 
                                const std::string& sourceDirectory);
    bool restoreBackup(const std::string& backupName, const std::string& targetDirectory);
    bool deleteBackup(const std::string& backupName);
    
    // Backup management
    std::vector<BackupMetadata> listBackups() const;
    BackupMetadata getBackupInfo(const std::string& backupName) const;
    bool backupExists(const std::string& backupName) const;
    
    // Verification and validation
    bool verifyBackup(const std::string& backupName) const;
    std::vector<std::string> getChangedFiles(const std::string& sourceDirectory, 
                                           const std::string& lastBackupName) const;
    
    // Statistics and monitoring
    BackupStats getBackupStats() const;
    void resetStats();
    void showBackupDashboard() const;
    
    // Cleanup operations
    bool cleanupOldBackups(int maxBackupsToKeep);
    size_t calculateBackupSize(const std::string& backupName) const;

private:
    std::string backupDirectory;
    std::string metadataFile;
    mutable BackupStats stats;
    
    // Internal helper methods
    bool initializeBackupDirectory();
    std::string getBackupPath(const std::string& backupName) const;
    std::string getMetadataPath(const std::string& backupName) const;
    
    // Metadata management
    bool saveBackupMetadata(const BackupMetadata& metadata) const;
    BackupMetadata loadBackupMetadata(const std::string& backupName) const;
    void updateGlobalStats(const BackupMetadata& metadata);
    
    // File operations
    bool copyFileToBackup(const std::string& sourcePath, const std::string& backupPath) const;
    bool copyDirectoryToBackup(const std::string& sourceDir, const std::string& backupDir) const;
    bool restoreFileFromBackup(const std::string& backupPath, const std::string& targetPath) const;
    
    // Utility methods
    std::vector<std::string> getDirectoryFiles(const std::string& directory) const;
    std::string getFileHash(const std::string& filePath) const;
    bool isFileModified(const std::string& filePath, const std::chrono::system_clock::time_point& lastBackupTime) const;
    std::string formatFileSize(size_t bytes) const;
    std::string formatDuration(const std::chrono::system_clock::time_point& start, 
                              const std::chrono::system_clock::time_point& end) const;
};

// Backup exception classes
class BackupException : public std::runtime_error {
public:
    explicit BackupException(const std::string& message) : std::runtime_error("Backup Error: " + message) {}
};

class BackupNotFoundException : public BackupException {
public:
    explicit BackupNotFoundException(const std::string& backupName) 
        : BackupException("Backup not found: " + backupName) {}
};

class BackupAlreadyExistsException : public BackupException {
public:
    explicit BackupAlreadyExistsException(const std::string& backupName) 
        : BackupException("Backup already exists: " + backupName) {}
};

} // namespace mtfs::fs

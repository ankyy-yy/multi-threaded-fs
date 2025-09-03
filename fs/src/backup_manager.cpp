#include "fs/backup_manager.hpp"
#include "common/logger.hpp"
#include <fstream>
#include <sstream>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <algorithm>
#include <ctime>

namespace mtfs::fs {

using namespace mtfs::common;
namespace fs = std::filesystem;

BackupManager::BackupManager(const std::string& backupDirectory) 
    : backupDirectory(backupDirectory), metadataFile(backupDirectory + "/backup_metadata.txt") {
    
    if (!initializeBackupDirectory()) {
        throw BackupException("Failed to initialize backup directory: " + backupDirectory);
    }
    
    LOG_INFO("Backup manager initialized at: " + backupDirectory);
}

bool BackupManager::initializeBackupDirectory() {
    try {
        if (!fs::exists(backupDirectory)) {
            fs::create_directories(backupDirectory);
            LOG_INFO("Created backup directory: " + backupDirectory);
        }
        return true;
    } catch (const std::exception& e) {
        LOG_ERROR("Failed to initialize backup directory: " + std::string(e.what()));
        return false;
    }
}

bool BackupManager::createBackup(const std::string& backupName, const std::string& sourceDirectory) {
    try {
        LOG_INFO("Creating backup: " + backupName + " from " + sourceDirectory);
        
        if (backupExists(backupName)) {
            throw BackupAlreadyExistsException(backupName);
        }
        
        if (!fs::exists(sourceDirectory)) {
            throw BackupException("Source directory does not exist: " + sourceDirectory);
        }
        
        // Create backup metadata
        BackupMetadata metadata;
        metadata.backupName = backupName;
        metadata.backupPath = getBackupPath(backupName);
        metadata.isIncremental = false;
        
        // Create backup directory
        fs::create_directories(metadata.backupPath);
        
        // Copy files
        auto sourceFiles = getDirectoryFiles(sourceDirectory);
        metadata.includedFiles = sourceFiles;
        metadata.totalFiles = sourceFiles.size();
        
        size_t totalSize = 0;
        for (const auto& file : sourceFiles) {
            std::string sourcePath = sourceDirectory + "/" + file;
            std::string backupPath = metadata.backupPath + "/" + file;
            
            if (copyFileToBackup(sourcePath, backupPath)) {
                if (fs::exists(sourcePath)) {
                    totalSize += fs::file_size(sourcePath);
                }
            }
        }
        
        metadata.totalSize = totalSize;
        
        // Save metadata
        if (!saveBackupMetadata(metadata)) {
            throw BackupException("Failed to save backup metadata");
        }
        
        // Update statistics
        updateGlobalStats(metadata);
        
        LOG_INFO("Backup created successfully: " + backupName + 
                " (" + std::to_string(metadata.totalFiles) + " files, " + 
                formatFileSize(metadata.totalSize) + ")");
        
        return true;
    } catch (const std::exception& e) {
        LOG_ERROR("Failed to create backup: " + std::string(e.what()));
        throw;
    }
}

bool BackupManager::restoreBackup(const std::string& backupName, const std::string& targetDirectory) {
    try {
        LOG_INFO("Restoring backup: " + backupName + " to " + targetDirectory);
        
        if (!backupExists(backupName)) {
            throw BackupNotFoundException(backupName);
        }
        
        BackupMetadata metadata = loadBackupMetadata(backupName);
        
        // Create target directory
        fs::create_directories(targetDirectory);
        
        // Restore files
        size_t restoredFiles = 0;
        for (const auto& file : metadata.includedFiles) {
            std::string backupPath = metadata.backupPath + "/" + file;
            std::string targetPath = targetDirectory + "/" + file;
            
            if (restoreFileFromBackup(backupPath, targetPath)) {
                restoredFiles++;
            }
        }
        
        LOG_INFO("Backup restored successfully: " + backupName + 
                " (" + std::to_string(restoredFiles) + " files restored)");
        
        return true;
    } catch (const std::exception& e) {
        LOG_ERROR("Failed to restore backup: " + std::string(e.what()));
        throw;
    }
}

bool BackupManager::deleteBackup(const std::string& backupName) {
    try {
        LOG_INFO("Deleting backup: " + backupName);
        
        if (!backupExists(backupName)) {
            throw BackupNotFoundException(backupName);
        }
        
        std::string backupPath = getBackupPath(backupName);
        std::string metadataPath = getMetadataPath(backupName);
        
        // Remove backup directory
        fs::remove_all(backupPath);
        
        // Remove metadata file
        if (fs::exists(metadataPath)) {
            fs::remove(metadataPath);
        }
        
        LOG_INFO("Backup deleted successfully: " + backupName);
        return true;
    } catch (const std::exception& e) {
        LOG_ERROR("Failed to delete backup: " + std::string(e.what()));
        return false;
    }
}

std::vector<BackupMetadata> BackupManager::listBackups() const {
    std::vector<BackupMetadata> backups;
    
    try {
        for (const auto& entry : fs::directory_iterator(backupDirectory)) {
            if (entry.is_directory()) {
                std::string backupName = entry.path().filename().string();
                if (backupName != "." && backupName != ".." && 
                    fs::exists(getMetadataPath(backupName))) {
                    try {
                        BackupMetadata metadata = loadBackupMetadata(backupName);
                        backups.push_back(metadata);
                    } catch (const std::exception& e) {
                        LOG_ERROR("Failed to load metadata for backup: " + backupName);
                    }
                }
            }
        }
        
        // Sort by creation time (newest first)
        std::sort(backups.begin(), backups.end(), 
                 [](const BackupMetadata& a, const BackupMetadata& b) {
                     return a.createdAt > b.createdAt;
                 });
                 
    } catch (const std::exception& e) {
        LOG_ERROR("Failed to list backups: " + std::string(e.what()));
    }
    
    return backups;
}

bool BackupManager::backupExists(const std::string& backupName) const {
    return fs::exists(getBackupPath(backupName)) && fs::exists(getMetadataPath(backupName));
}

BackupStats BackupManager::getBackupStats() const {
    return stats;
}

void BackupManager::showBackupDashboard() const {
    auto backups = listBackups();
    
    std::cout << "\n================== BACKUP DASHBOARD ==================\n";
    std::cout << "Total Backups: " << backups.size() << "\n";
    std::cout << "Total Files Backed Up: " << stats.filesBackedUp << "\n";
    std::cout << "Total Backup Size: " << formatFileSize(stats.totalBackupSize) << "\n";
    
    if (!backups.empty()) {
        auto lastBackup_time = std::chrono::system_clock::to_time_t(stats.lastBackupTime);
        std::cout << "Last Backup: " << std::ctime(&lastBackup_time);
        
        std::cout << "\nRecent Backups:\n";
        std::cout << "---------------\n";
        
        int count = 0;
        for (const auto& backup : backups) {
            if (count >= 5) break; // Show only last 5 backups
            
            auto created_time = std::chrono::system_clock::to_time_t(backup.createdAt);
            std::cout << "  " << backup.backupName 
                      << " (" << backup.totalFiles << " files, " 
                      << formatFileSize(backup.totalSize) << ")"
                      << " - " << std::put_time(std::localtime(&created_time), "%Y-%m-%d %H:%M")
                      << (backup.isIncremental ? " [Incremental]" : " [Full]")
                      << "\n";
            count++;
        }
    }
    
    std::cout << "======================================================\n\n";
}

// Helper method implementations
std::string BackupManager::getBackupPath(const std::string& backupName) const {
    return backupDirectory + "/" + backupName;
}

std::string BackupManager::getMetadataPath(const std::string& backupName) const {
    return backupDirectory + "/" + backupName + "_metadata.txt";
}

bool BackupManager::saveBackupMetadata(const BackupMetadata& metadata) const {
    try {
        std::ofstream file(getMetadataPath(metadata.backupName));
        if (!file) return false;
        
        auto created = std::chrono::duration_cast<std::chrono::seconds>(
            metadata.createdAt.time_since_epoch()).count();
        auto modified = std::chrono::duration_cast<std::chrono::seconds>(
            metadata.lastModified.time_since_epoch()).count();
        
        file << "name=" << metadata.backupName << "\n";
        file << "path=" << metadata.backupPath << "\n";
        file << "created=" << created << "\n";
        file << "modified=" << modified << "\n";
        file << "files=" << metadata.totalFiles << "\n";
        file << "size=" << metadata.totalSize << "\n";
        file << "incremental=" << (metadata.isIncremental ? "1" : "0") << "\n";
        file << "parent=" << metadata.parentBackup << "\n";
        
        file << "filelist=";
        for (size_t i = 0; i < metadata.includedFiles.size(); ++i) {
            file << metadata.includedFiles[i];
            if (i < metadata.includedFiles.size() - 1) file << ",";
        }
        file << "\n";
        
        return true;
    } catch (const std::exception& e) {
        LOG_ERROR("Failed to save backup metadata: " + std::string(e.what()));
        return false;
    }
}

BackupMetadata BackupManager::loadBackupMetadata(const std::string& backupName) const {
    BackupMetadata metadata;
    
    try {
        std::ifstream file(getMetadataPath(backupName));
        if (!file) {
            throw BackupException("Cannot open metadata file for backup: " + backupName);
        }
        
        std::string line;
        while (std::getline(file, line)) {
            size_t pos = line.find('=');
            if (pos != std::string::npos) {
                std::string key = line.substr(0, pos);
                std::string value = line.substr(pos + 1);
                
                if (key == "name") metadata.backupName = value;
                else if (key == "path") metadata.backupPath = value;
                else if (key == "created") {
                    auto seconds = std::chrono::seconds(std::stoll(value));
                    metadata.createdAt = std::chrono::system_clock::time_point(seconds);
                }
                else if (key == "modified") {
                    auto seconds = std::chrono::seconds(std::stoll(value));
                    metadata.lastModified = std::chrono::system_clock::time_point(seconds);
                }
                else if (key == "files") metadata.totalFiles = std::stoull(value);
                else if (key == "size") metadata.totalSize = std::stoull(value);
                else if (key == "incremental") metadata.isIncremental = (value == "1");
                else if (key == "parent") metadata.parentBackup = value;
                else if (key == "filelist") {
                    std::stringstream ss(value);
                    std::string file;
                    while (std::getline(ss, file, ',')) {
                        metadata.includedFiles.push_back(file);
                    }
                }
            }
        }
        
        return metadata;
    } catch (const std::exception& e) {
        LOG_ERROR("Failed to load backup metadata: " + std::string(e.what()));
        throw;
    }
}

bool BackupManager::copyFileToBackup(const std::string& sourcePath, const std::string& backupPath) const {
    try {
        // Create directory structure if needed
        fs::path backupDir = fs::path(backupPath).parent_path();
        fs::create_directories(backupDir);
        
        // Copy file
        fs::copy_file(sourcePath, backupPath, fs::copy_options::overwrite_existing);
        return true;
    } catch (const std::exception& e) {
        LOG_ERROR("Failed to copy file to backup: " + sourcePath + " -> " + backupPath);
        return false;
    }
}

bool BackupManager::restoreFileFromBackup(const std::string& backupPath, const std::string& targetPath) const {
    try {
        // Create directory structure if needed
        fs::path targetDir = fs::path(targetPath).parent_path();
        fs::create_directories(targetDir);
        
        // Copy file
        fs::copy_file(backupPath, targetPath, fs::copy_options::overwrite_existing);
        return true;
    } catch (const std::exception& e) {
        LOG_ERROR("Failed to restore file from backup: " + backupPath + " -> " + targetPath);
        return false;
    }
}

std::vector<std::string> BackupManager::getDirectoryFiles(const std::string& directory) const {
    std::vector<std::string> files;
    
    try {
        for (const auto& entry : fs::recursive_directory_iterator(directory)) {
            if (entry.is_regular_file()) {
                fs::path relativePath = fs::relative(entry.path(), directory);
                files.push_back(relativePath.string());
            }
        }
    } catch (const std::exception& e) {
        LOG_ERROR("Failed to get directory files: " + std::string(e.what()));
    }
    
    return files;
}

void BackupManager::updateGlobalStats(const BackupMetadata& metadata) {
    stats.totalBackups++;
    stats.totalBackupSize += metadata.totalSize;
    stats.filesBackedUp += metadata.totalFiles;
    stats.lastBackupTime = metadata.createdAt;
}

std::string BackupManager::formatFileSize(size_t bytes) const {
    const char* units[] = {"B", "KB", "MB", "GB", "TB"};
    int unit = 0;
    double size = static_cast<double>(bytes);
    
    while (size >= 1024.0 && unit < 4) {
        size /= 1024.0;
        unit++;
    }
    
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(2) << size << " " << units[unit];
    return oss.str();
}

} // namespace mtfs::fs

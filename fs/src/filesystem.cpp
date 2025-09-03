#include "fs/filesystem.hpp"
#include "fs/compression.hpp"
#include "common/logger.hpp"
#include "common/error.hpp"
#include <direct.h>
#include <io.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <iostream>
#include <cstdio>

namespace mtfs::fs {

using namespace mtfs::common;  // Add this to use exceptions from common namespace

FileSystem::FileSystem(const std::string& rootPath, mtfs::common::AuthManager* auth)
    : rootPath(rootPath),
      enhancedCache(std::make_unique<cache::CacheManager<std::string, std::string>>(CACHE_CAPACITY)),
      authManager(auth),
      metadataFilePath(rootPath + "/.mtfs_metadata") {
    LOG_INFO("Initializing filesystem at: " + rootPath);
    _mkdir(rootPath.c_str());
    loadMetadata();
    
    // Initialize backup manager
    std::string backupDir = rootPath + "_backups";
    try {
        backupManager = std::make_unique<BackupManager>(backupDir);
        LOG_INFO("Backup manager initialized at: " + backupDir);
    } catch (const std::exception& e) {
        LOG_ERROR("Failed to initialize backup manager: " + std::string(e.what()));
    }
}

std::shared_ptr<FileSystem> FileSystem::create(const std::string& rootPath, mtfs::common::AuthManager* auth) {
    return std::shared_ptr<FileSystem>(new FileSystem(rootPath, auth));
}

bool FileSystem::saveMetadata() const {
    std::ofstream ofs(metadataFilePath, std::ios::trunc);
    if (!ofs) return false;
    for (const auto& [path, meta] : fileMetadataMap) {
        ofs << path << '\t' << meta.owner << '\t' << meta.permissions << '\t' << meta.size << '\t' << meta.isDirectory << '\n';
    }
    return true;
}

bool FileSystem::loadMetadata() {
    std::ifstream ifs(metadataFilePath);
    if (!ifs) return false;
    fileMetadataMap.clear();
    std::string path, owner;
    uint32_t permissions;
    std::size_t size;
    int isDir;
    while (ifs >> path >> owner >> permissions >> size >> isDir) {
        FileMetadata meta;
        meta.name = path;
        meta.owner = owner;
        meta.permissions = permissions;
        meta.size = size;
        meta.isDirectory = (isDir != 0);
        fileMetadataMap[path] = meta;
    }
    return true;
}

bool FileSystem::createFile(const std::string& path) {
    try {
        if (authManager && !authManager->isLoggedIn()) {
            throw FSException("Authentication required to create file");
        }
        std::string fullPath = rootPath + "/" + path;
        std::ofstream file(fullPath);
        if (!file) {
            LOG_ERROR("Failed to create file: " + path);
            throw FSException("Failed to create file: " + path);
        }
        file.close();
        // Set file owner and persist metadata
        FileMetadata meta;
        meta.name = path;
        meta.owner = authManager ? authManager->getCurrentUser() : "unknown";
        meta.permissions = 0644;
        meta.size = 0;
        meta.isDirectory = false;
        fileMetadataMap[path] = meta;
        saveMetadata();
        return true;
    } catch (const std::exception& e) {
        LOG_ERROR(std::string("Error creating file: ") + e.what());
        throw;
    }
}

bool FileSystem::writeFile(const std::string& path, const std::string& data) {
    try {
        auto startTime = std::chrono::high_resolution_clock::now();
        
        if (authManager && !authManager->isLoggedIn()) {
            throw FSException("Authentication required to write file");
        }
        // Permission check: only owner or admin can write
        if (authManager) {
            FileMetadata meta = getFileInfo(path);
            std::string user = authManager->getCurrentUser();
            if (meta.owner != user && !authManager->isAdmin(user)) {
                throw FSException("Permission denied: not owner or admin");
            }
        }
        std::string fullPath = rootPath + "/" + path;
        if (!exists(path)) {
            throw FileNotFoundException(path);
        }
        std::ofstream file(fullPath);
        if (!file) {
            throw FSException("Failed to open file for writing: " + path);
        }
        file << data;
        enhancedCache->put(path, data);
        stats.totalWrites++;
        stats.totalFileOperations++;

        // Update metadata
        FileMetadata& meta = fileMetadataMap[path];
        meta.size = data.size();
        meta.modifiedAt = std::chrono::system_clock::now();
        saveMetadata();
        
        auto endTime = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime);
        double writeTime = duration.count() / 1000.0; // Convert to milliseconds
        stats.avgWriteTime = (stats.avgWriteTime * (stats.totalWrites - 1) + writeTime) / stats.totalWrites;

        return true;
    } catch (const std::exception& e) {
        LOG_ERROR(std::string("Error writing file: ") + e.what());
        throw;
    }
}

std::string FileSystem::readFile(const std::string& path) {
    try {
        if (authManager && !authManager->isLoggedIn()) {
            throw FSException("Authentication required to read file");
        }
        // Permission check: only owner or admin can read (customize as needed)
        if (authManager) {
            FileMetadata meta = getFileInfo(path);
            std::string user = authManager->getCurrentUser();
            if (meta.owner != user && !authManager->isAdmin(user)) {
                throw FSException("Permission denied: not owner or admin");
            }
        }        // Try to get from cache first
        auto startTime = std::chrono::high_resolution_clock::now();
        try {
            std::string cachedData = enhancedCache->get(path);
            LOG_DEBUG("Cache hit for file: " + path);
            stats.cacheHits++;
            stats.totalReads++;
            stats.totalFileOperations++;
            
            auto endTime = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime);
            double readTime = duration.count() / 1000.0; // Convert to milliseconds
            stats.avgReadTime = (stats.avgReadTime * (stats.totalReads - 1) + readTime) / stats.totalReads;
            
            return cachedData;
        } catch (const std::runtime_error&) {
            // Cache miss, continue to read from disk
        }
        LOG_DEBUG("Cache miss for file: " + path);
        stats.cacheMisses++;
        stats.totalReads++;
        stats.totalFileOperations++;
        std::string fullPath = rootPath + "/" + path;
        if (!exists(path)) {
            throw FileNotFoundException(path);
        }
        std::ifstream file(fullPath);
        if (!file) {
            throw FSException("Failed to open file for reading: " + path);
        }        std::stringstream buffer;
        buffer << file.rdbuf();
        std::string data = buffer.str();
        enhancedCache->put(path, data);
        
        auto endTime = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime);
        double readTime = duration.count() / 1000.0; // Convert to milliseconds
        stats.avgReadTime = (stats.avgReadTime * (stats.totalReads - 1) + readTime) / stats.totalReads;
        
        return data;
    } catch (const std::exception& e) {
        LOG_ERROR(std::string("Error reading file: ") + e.what());
        throw;
    }
}

bool FileSystem::deleteFile(const std::string& path) {
    try {
        if (authManager && !authManager->isLoggedIn()) {
            throw FSException("Authentication required to delete file");
        }
        if (authManager) {
            FileMetadata meta = getFileInfo(path);
            std::string user = authManager->getCurrentUser();
            if (meta.owner != user && !authManager->isAdmin(user)) {
                throw FSException("Permission denied: not owner or admin");
            }
        }
        std::string fullPath = rootPath + "/" + path;
        if (!exists(path)) {
            throw FileNotFoundException(path);
        }
        enhancedCache->clear();
        fileMetadataMap.erase(path);
        saveMetadata();
        return remove(fullPath.c_str()) == 0;
    } catch (const std::exception& e) {
        LOG_ERROR(std::string("Error deleting file: ") + e.what());
        throw;
    }
}

bool FileSystem::createDirectory(const std::string& path) {
    try {
        std::string fullPath = rootPath + "/" + path;
        return _mkdir(fullPath.c_str()) == 0;
    } catch (const std::exception& e) {
        LOG_ERROR(std::string("Error creating directory: ") + e.what());
        throw;
    }
}

std::vector<std::string> FileSystem::listDirectory(const std::string& path) {
    try {
        std::string fullPath = rootPath + "/" + path;
        if (!exists(path)) {
            throw FileNotFoundException(path);
        }

        std::vector<std::string> entries;
        _finddata_t fileinfo;
        intptr_t handle = _findfirst((fullPath + "/*").c_str(), &fileinfo);
        
        if (handle != -1) {
            do {
                std::string name = fileinfo.name;
                if (name != "." && name != "..") {
                    entries.push_back(name);
                }
            } while (_findnext(handle, &fileinfo) == 0);
            _findclose(handle);
        }
        
        return entries;
    } catch (const std::exception& e) {
        LOG_ERROR(std::string("Error listing directory: ") + e.what());
        throw;
    }
}

FileMetadata FileSystem::getMetadata(const std::string& path) {
    try {
        std::string fullPath = rootPath + "/" + path;
        if (!exists(path)) {
            throw FileNotFoundException(path);
        }        struct stat fileStats;
        if (stat(fullPath.c_str(), &fileStats) != 0) {
            throw FSException("Failed to get file stats: " + path);
        }

        FileMetadata metadata;
        metadata.name = path.substr(path.find_last_of("/\\") + 1);
        metadata.size = fileStats.st_size;
        metadata.isDirectory = (fileStats.st_mode & S_IFDIR) != 0;
        metadata.permissions = fileStats.st_mode & 0777;
        metadata.modifiedAt = std::chrono::system_clock::from_time_t(fileStats.st_mtime);
        metadata.createdAt = std::chrono::system_clock::from_time_t(fileStats.st_ctime);

        return metadata;
    } catch (const std::exception& e) {
        LOG_ERROR(std::string("Error getting metadata: ") + e.what());
        throw;
    }
}

void FileSystem::setPermissions(const std::string& path, uint32_t permissions) {
    try {
        std::string fullPath = rootPath + "/" + path;
        if (!exists(path)) {
            throw FileNotFoundException(path);
        }

        if (_chmod(fullPath.c_str(), permissions) != 0) {
            throw FSException("Failed to set permissions: " + path);
        }

        // Update metadata
        fileMetadataMap[path].permissions = permissions;
        saveMetadata();
    } catch (const std::exception& e) {
        LOG_ERROR(std::string("Error setting permissions: ") + e.what());
        throw;
    }
}

bool FileSystem::exists(const std::string& path) {
    try {
        std::string fullPath = rootPath + "/" + path;        struct stat fileStats;
        return stat(fullPath.c_str(), &fileStats) == 0;
    } catch (const std::exception& e) {
        LOG_ERROR(std::string("Error checking existence: ") + e.what());
        throw;
    }
}

void FileSystem::sync() {
    // In a real implementation, this would flush all buffers to disk
    LOG_INFO("Syncing filesystem");
}

void FileSystem::mount() {
    LOG_INFO("Mounting filesystem at: " + rootPath);
    _mkdir(rootPath.c_str());
}

void FileSystem::unmount() {
    LOG_INFO("Unmounting filesystem from: " + rootPath);
    sync();
}

std::size_t FileSystem::write(const std::string& path, const void* buffer, std::size_t size, std::size_t offset) {
    try {
        std::string fullPath = rootPath + "/" + path;
        if (!exists(path)) {
            throw FileNotFoundException(path);
        }

        std::fstream file(fullPath, std::ios::binary | std::ios::in | std::ios::out);
        if (!file) {
            throw FSException("Failed to open file for writing: " + path);
        }

        file.seekp(offset);
        file.write(static_cast<const char*>(buffer), size);
        return size;
    } catch (const std::exception& e) {
        LOG_ERROR(std::string("Error in low-level write: ") + e.what());
        throw;
    }
}

std::size_t FileSystem::read(const std::string& path, void* buffer, std::size_t size, std::size_t offset) {
    try {
        std::string fullPath = rootPath + "/" + path;
        if (!exists(path)) {
            throw FileNotFoundException(path);
        }

        std::ifstream file(fullPath, std::ios::binary);
        if (!file) {
            throw FSException("Failed to open file for reading: " + path);
        }

        file.seekg(offset);
        file.read(static_cast<char*>(buffer), size);
        return file.gcount();
    } catch (const std::exception& e) {
        LOG_ERROR(std::string("Error in low-level read: ") + e.what());
        throw;
    }
}

FileMetadata FileSystem::resolvePath(const std::string& path) {
    LOG_DEBUG("Resolving path: " + path);
    return getMetadata(path);
}

// Cache control methods
void FileSystem::clearCache() {
    enhancedCache->clear();
    LOG_INFO("File system cache cleared");
}

size_t FileSystem::getCacheSize() const {
    return enhancedCache->getStatistics().hits + enhancedCache->getStatistics().misses;
}

// Enhanced cache management methods
void FileSystem::setCachePolicy(cache::CachePolicy policy) {
    enhancedCache->setPolicy(policy);
    LOG_INFO("Cache policy changed to: " + std::to_string(static_cast<int>(policy)));
}

cache::CachePolicy FileSystem::getCachePolicy() const {
    return enhancedCache->getPolicy();
}

void FileSystem::resizeCache(size_t newCapacity) {
    enhancedCache->resize(newCapacity);
    LOG_INFO("Cache resized to: " + std::to_string(newCapacity));
}

void FileSystem::pinFile(const std::string& path) {
    try {
        // Ensure file is in cache first
        if (!enhancedCache->contains(path)) {
            std::string data = readFile(path);
        }
        enhancedCache->pin(path);
        LOG_DEBUG("File pinned in cache: " + path);
    } catch (const std::exception& e) {
        LOG_ERROR("Failed to pin file: " + std::string(e.what()));
    }
}

void FileSystem::unpinFile(const std::string& path) {
    enhancedCache->unpin(path);
    LOG_DEBUG("File unpinned from cache: " + path);
}

bool FileSystem::isFilePinned(const std::string& path) const {
    return enhancedCache->isPinned(path);
}

void FileSystem::prefetchFile(const std::string& path) {
    try {        if (!exists(path)) {
            LOG_ERROR("Cannot prefetch non-existent file: " + path);
            return;
        }
        
        std::string data = readFile(path);
        enhancedCache->prefetch(path, data);
        LOG_DEBUG("File prefetched: " + path);
    } catch (const std::exception& e) {
        LOG_ERROR("Failed to prefetch file: " + std::string(e.what()));
    }
}

cache::CacheStatistics FileSystem::getCacheStatistics() const {
    return enhancedCache->getStatistics();
}

void FileSystem::resetCacheStatistics() {
    enhancedCache->resetStatistics();
    LOG_INFO("Cache statistics reset");
}

void FileSystem::showCacheAnalytics() const {
    std::cout << "\n======== File System Cache Analytics ========\n";
    enhancedCache->showCacheAnalytics();
    
    // Get enhanced cache statistics for consistent reporting
    auto cacheStats = enhancedCache->getStatistics();
    std::cout << "File Operations:\n";
    std::cout << "  Total Reads: " << this->stats.totalReads << "\n";
    std::cout << "  Total Writes: " << this->stats.totalWrites << "\n";
    std::cout << "  Enhanced Cache Hit Rate: " << std::fixed << std::setprecision(2) 
              << cacheStats.hitRate << "%\n";
    std::cout << "  Legacy Cache Hit Rate: " << std::fixed << std::setprecision(2) 
              << this->stats.getCacheHitRate() << "%\n";
    std::cout << "=============================================\n\n";
}

std::vector<std::string> FileSystem::getHotFiles(size_t count) const {
    return enhancedCache->getHotKeys(count);
}

// Advanced file operations
bool FileSystem::copyFile(const std::string& source, const std::string& destination) {
    try {
        LOG_INFO("Copying file: " + source + " -> " + destination);
        
        // Check if source file exists
        if (!exists(source)) {
            throw FileNotFoundException(source);
        }
        
        // Read source file content
        std::string content = readFile(source);
        
        // Create destination file and write content
        if (!createFile(destination)) {
            throw FSException("Failed to create destination file: " + destination);
        }
        
        if (!writeFile(destination, content)) {
            throw FSException("Failed to write to destination file: " + destination);
        }
        
        LOG_INFO("File copied successfully: " + source + " -> " + destination);
        return true;
    } catch (const std::exception& e) {
        LOG_ERROR("Error copying file: " + std::string(e.what()));
        throw;
    }
}

bool FileSystem::moveFile(const std::string& source, const std::string& destination) {
    try {
        LOG_INFO("Moving file: " + source + " -> " + destination);
        
        // Copy the file first
        if (!copyFile(source, destination)) {
            throw FSException("Failed to copy file during move operation");
        }
        
        // Delete the source file
        if (!deleteFile(source)) {
            // If delete fails, try to clean up the destination
            deleteFile(destination);
            throw FSException("Failed to delete source file during move operation");
        }
        
        LOG_INFO("File moved successfully: " + source + " -> " + destination);
        return true;
    } catch (const std::exception& e) {
        LOG_ERROR("Error moving file: " + std::string(e.what()));
        throw;
    }
}

bool FileSystem::renameFile(const std::string& oldName, const std::string& newName) {
    try {
        LOG_INFO("Renaming file: " + oldName + " -> " + newName);
        return moveFile(oldName, newName);
    } catch (const std::exception& e) {
        LOG_ERROR("Error renaming file: " + std::string(e.what()));
        throw;
    }
}

// Helper function for glob pattern matching
bool FileSystem::matchesPattern(const std::string& filename, const std::string& pattern) {
    // If pattern contains wildcards (* or ?), use glob matching
    if (pattern.find('*') != std::string::npos || pattern.find('?') != std::string::npos) {
        // Handle glob patterns with * and ?
        size_t patternPos = 0;
        size_t filenamePos = 0;
        size_t starIdx = std::string::npos;
        size_t match = 0;
        
        while (filenamePos < filename.length()) {
            // If we have a direct character match or a '?' wildcard
            if (patternPos < pattern.length() && 
                (pattern[patternPos] == filename[filenamePos] || pattern[patternPos] == '?')) {
                filenamePos++;
                patternPos++;
            }
            // If we encounter a '*' wildcard
            else if (patternPos < pattern.length() && pattern[patternPos] == '*') {
                starIdx = patternPos;
                match = filenamePos;
                patternPos++;
            }
            // If we had a previous '*' wildcard, backtrack to it
            else if (starIdx != std::string::npos) {
                patternPos = starIdx + 1;
                match++;
                filenamePos = match;
            }
            // No match
            else {
                return false;
            }
        }
        
        // Skip any remaining '*' at the end of pattern
        while (patternPos < pattern.length() && pattern[patternPos] == '*') {
            patternPos++;
        }
        
        return patternPos == pattern.length();
    } else {
        // For patterns without wildcards, do substring matching (like original behavior)
        return filename.find(pattern) != std::string::npos;
    }
}

std::vector<std::string> FileSystem::findFiles(const std::string& pattern, const std::string& directory) {
    try {
        LOG_INFO("Searching for files with pattern: " + pattern + " in directory: " + directory);
        
        std::vector<std::string> results;
        std::vector<std::string> files = listDirectory(directory);
        
        // Use glob pattern matching
        for (const auto& file : files) {
            if (matchesPattern(file, pattern)) {
                if (directory == ".") {
                    results.push_back(file);
                } else {
                    results.push_back(directory + "/" + file);
                }
            }
        }
        
        LOG_INFO("Found " + std::to_string(results.size()) + " files matching pattern: " + pattern);
        return results;
    } catch (const std::exception& e) {
        LOG_ERROR("Error searching files: " + std::string(e.what()));
        throw;
    }
}

FileMetadata FileSystem::getFileInfo(const std::string& path) {
    try {
        LOG_INFO("Getting file info for: " + path);
        return getMetadata(path);
    } catch (const std::exception& e) {
        LOG_ERROR("Error getting file info: " + std::string(e.what()));
        throw;
    }
}

// Performance monitoring methods
PerformanceStats FileSystem::getStats() const {
    return stats;
}

void FileSystem::resetStats() {
    stats = PerformanceStats();
    enhancedCache->resetStatistics();
    LOG_INFO("Performance statistics reset");
}

void FileSystem::showPerformanceDashboard() const {
    auto now = std::chrono::system_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(now - stats.lastResetTime);
    
    // Get enhanced cache statistics for accurate reporting
    auto cacheStats = enhancedCache->getStatistics();
    
    std::cout << "\n=================== PERFORMANCE DASHBOARD ===================\n";
    std::cout << "Monitoring Period: " << duration.count() << " ms (" 
              << std::fixed << std::setprecision(2) << duration.count() / 1000.0 << " seconds)\n";
    std::cout << "-----------------------------------------------------------\n";
    std::cout << "CACHE STATISTICS:\n";
    std::cout << "  Cache Hits: " << cacheStats.hits << "\n";
    std::cout << "  Cache Misses: " << cacheStats.misses << "\n";
    std::cout << "  Cache Hit Rate: " << std::fixed << std::setprecision(2) << cacheStats.hitRate << "%\n";
    std::cout << "  Cache Size: " << (cacheStats.hits + cacheStats.misses > 0 ? enhancedCache->getHotKeys(1000).size() : 0) << "/" << CACHE_CAPACITY << "\n";
    std::cout << "  Pinned Items: " << cacheStats.pinnedItems << "\n";
    std::cout << "  Prefetched Items: " << cacheStats.prefetchedItems << "\n";
    std::cout << "-----------------------------------------------------------\n";
    std::cout << "FILE OPERATIONS:\n";
    std::cout << "  Total Reads: " << stats.totalReads << "\n";
    std::cout << "  Total Writes: " << stats.totalWrites << "\n";
    std::cout << "  Total File Operations: " << stats.totalFileOperations << "\n";
    std::cout << "  Average Read Time: " << std::fixed << std::setprecision(3) << stats.avgReadTime << " ms\n";
    std::cout << "  Average Write Time: " << std::fixed << std::setprecision(3) << stats.avgWriteTime << " ms\n";
    std::cout << "==========================================================\n\n";
}

// File compression methods
bool FileSystem::compressFile(const std::string& filePath) {
    try {
        LOG_INFO("Compressing file: " + filePath);
        
        if (!exists(filePath)) {
            throw FileNotFoundException(filePath);
        }
        
        std::string fullPath = rootPath + "/" + filePath;
        std::string compressedPath = fullPath + ".mtfs";
        
        // Read original file size
        std::ifstream file(fullPath, std::ios::binary | std::ios::ate);
        size_t originalSize = file.tellg();
        file.close();
        
        // Compress the file
        if (!FileCompression::compressFile(fullPath, compressedPath)) {
            throw FSException("Failed to compress file: " + filePath);
        }
        
        // Get compressed file size
        std::ifstream compressedFile(compressedPath, std::ios::binary | std::ios::ate);
        size_t compressedSize = compressedFile.tellg();
        compressedFile.close();
        
        // Update statistics
        compressionStats.addCompressionOperation(originalSize, compressedSize);
        
        // Remove original file and rename compressed file
        std::remove(fullPath.c_str());
        std::rename(compressedPath.c_str(), fullPath.c_str());
        
        double ratio = FileCompression::calculateCompressionRatio(originalSize, compressedSize);
        LOG_INFO("File compressed successfully. Compression ratio: " + std::to_string(ratio) + "%");
        
        return true;
    } catch (const std::exception& e) {
        LOG_ERROR("Error compressing file: " + std::string(e.what()));
        throw;
    }
}

bool FileSystem::decompressFile(const std::string& filePath) {
    try {
        LOG_INFO("Decompressing file: " + filePath);
        
        if (!exists(filePath)) {
            throw FileNotFoundException(filePath);
        }
        
        std::string fullPath = rootPath + "/" + filePath;
        
        // Check if file is actually compressed
        if (!FileCompression::isCompressed(fullPath)) {
            throw FSException("File is not compressed: " + filePath);
        }
        
        std::string tempPath = fullPath + ".tmp";
        
        // Decompress the file
        if (!FileCompression::decompressFile(fullPath, tempPath)) {
            throw FSException("Failed to decompress file: " + filePath);
        }
        
        // Replace original with decompressed
        std::remove(fullPath.c_str());
        std::rename(tempPath.c_str(), fullPath.c_str());
        
        LOG_INFO("File decompressed successfully: " + filePath);
        return true;
    } catch (const std::exception& e) {
        LOG_ERROR("Error decompressing file: " + std::string(e.what()));
        throw;
    }
}

CompressionStats FileSystem::getCompressionStats() const {
    return compressionStats;
}

void FileSystem::resetCompressionStats() {
    compressionStats = CompressionStats();
    LOG_INFO("Compression statistics reset");
}

// Backup system methods
bool FileSystem::createBackup(const std::string& backupName) {
    try {
        if (!backupManager) {
            throw std::runtime_error("Backup manager not initialized");
        }
        
        LOG_INFO("Creating backup: " + backupName);
        return backupManager->createBackup(backupName, rootPath);
    } catch (const std::exception& e) {
        LOG_ERROR("Error creating backup: " + std::string(e.what()));
        throw;
    }
}

bool FileSystem::restoreBackup(const std::string& backupName, const std::string& targetDirectory) {
    try {
        if (!backupManager) {
            throw std::runtime_error("Backup manager not initialized");
        }
        
        std::string restoreDir = targetDirectory.empty() ? rootPath + "_restored" : targetDirectory;
        LOG_INFO("Restoring backup: " + backupName + " to " + restoreDir);
        return backupManager->restoreBackup(backupName, restoreDir);
    } catch (const std::exception& e) {
        LOG_ERROR("Error restoring backup: " + std::string(e.what()));
        throw;
    }
}

bool FileSystem::deleteBackup(const std::string& backupName) {
    try {
        if (!backupManager) {
            throw std::runtime_error("Backup manager not initialized");
        }
        
        LOG_INFO("Deleting backup: " + backupName);
        return backupManager->deleteBackup(backupName);
    } catch (const std::exception& e) {
        LOG_ERROR("Error deleting backup: " + std::string(e.what()));
        throw;
    }
}

std::vector<std::string> FileSystem::listBackups() const {
    try {
        if (!backupManager) {
            return {};
        }
        
        auto backups = backupManager->listBackups();
        std::vector<std::string> backupNames;
        
        for (const auto& backup : backups) {
            backupNames.push_back(backup.backupName);
        }
        
        return backupNames;
    } catch (const std::exception& e) {
        LOG_ERROR("Error listing backups: " + std::string(e.what()));
        return {};
    }
}

void FileSystem::showBackupDashboard() const {
    try {
        if (!backupManager) {
            std::cout << "Backup manager not available." << std::endl;
            return;
        }
        
        backupManager->showBackupDashboard();
    } catch (const std::exception& e) {
        LOG_ERROR("Error showing backup dashboard: " + std::string(e.what()));
        std::cout << "Error displaying backup dashboard: " << e.what() << std::endl;
    }
}

BackupStats FileSystem::getBackupStats() const {
    try {
        if (!backupManager) {
            return BackupStats();
        }
        
        return backupManager->getBackupStats();
    } catch (const std::exception& e) {
        LOG_ERROR("Error getting backup stats: " + std::string(e.what()));
        return BackupStats();
    }
}

} // namespace mtfs::fs
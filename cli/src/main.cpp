#include "fs/filesystem.hpp"
#include "cache/enhanced_cache.hpp"
#include "common/logger.hpp"
#include "storage/block_manager.hpp"
#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <memory>
#include <iomanip> // For std::setprecision

using namespace mtfs::fs;
using namespace mtfs::common;
using namespace mtfs::cache;

// Helper function to split command line into tokens, handling quoted strings
std::vector<std::string> splitCommand(const std::string& cmd) {
    std::vector<std::string> tokens;
    std::istringstream iss(cmd);
    std::string token;
    bool inQuotes = false;
    std::string currentToken;
    
    char c;
    while (iss.get(c)) {
        if (c == '"') {
            inQuotes = !inQuotes;
        } else if (c == ' ' && !inQuotes) {
            if (!currentToken.empty()) {
                tokens.push_back(currentToken);
                currentToken.clear();
            }
        } else {
            currentToken += c;
        }
    }
    
    if (!currentToken.empty()) {
        tokens.push_back(currentToken);
    }
    
    return tokens;
}

// Helper function to print command usage
void printUsage() {
    std::cout << "\nAvailable commands:\n"
              << "  login <username> <password>\n"
              << "  logout\n"
              << "  register <username> <password> [admin]\n"
              << "  remove-user <username>\n"
              << "  whoami\n"
              << "  create-file <filename>\n"
              << "  write-file <filename> <content>\n"
              << "  read-file <filename>\n"
              << "  delete-file <filename>\n"
              << "  create-dir <directoryname>\n"
              << "  list-dir <directoryname>\n"
              << "  copy-file <source> <destination>\n"
              << "  move-file <source> <destination>\n"
              << "  rename-file <oldname> <newname>\n"
              << "  find-file <pattern> [directory]\n"
              << "  file-info <filename>\n"
              << "  compress-file <filename>\n"
              << "  decompress-file <filename>\n"
              << "  compression-stats\n"
              << "  create-backup <backup_name>\n"
              << "  restore-backup <backup_name> [target_directory]\n"
              << "  delete-backup <backup_name>\n"
              << "  list-backups\n"              << "  backup-dashboard\n"
              << "  set-cache-policy <policy>    # LRU, LFU, FIFO, LIFO\n"
              << "  get-cache-policy\n"
              << "  resize-cache <size>\n"
              << "  pin-file <filename>\n"
              << "  unpin-file <filename>\n"
              << "  prefetch-file <filename>\n"
              << "  cache-analytics\n"
              << "  hot-files [count]\n"
              << "  show-stats\n"
              << "  reset-stats\n"
              << "  exit\n"
              << std::endl;
}

int main() {
    try {
        std::cout << "Multi-threaded File System CLI\n"
                  << "Type 'help' for available commands\n\n";

        // Initialize authentication manager
        static mtfs::common::AuthManager auth;

        // Initialize filesystem with a root directory and auth manager
        const std::string rootPath = "./fs_root";
        auto fs = FileSystem::create(rootPath, &auth);
        LOG_INFO("Filesystem initialized at: " + rootPath);

        std::string line;
        while (true) {
            std::cout << "> ";
            std::getline(std::cin, line);

            auto tokens = splitCommand(line);
            if (tokens.empty()) {
                continue;
            }

            const std::string& cmd = tokens[0];

            try {
                if (cmd == "help") {
                    printUsage();
                }
                else if (cmd == "exit") {
                    LOG_INFO("Shutting down filesystem");
                    break;
                }
                else if (cmd == "login") {
                    if (tokens.size() != 3) {
                        std::cout << "Usage: login <username> <password>" << std::endl;
                        continue;
                    }
                    if (auth.authenticate(tokens[1], tokens[2])) {
                        std::cout << "Login successful. Welcome, " << tokens[1] << "!" << std::endl;
                        LOG_INFO("User logged in: " + tokens[1]);
                    } else {
                        std::cout << "Login failed. Invalid credentials." << std::endl;
                    }
                }
                else if (cmd == "logout") {
                    auth.logout();
                    std::cout << "Logged out." << std::endl;
                }
                else if (cmd == "register") {
                    if (tokens.size() < 3) {
                        std::cout << "Usage: register <username> <password> [admin]" << std::endl;
                        continue;
                    }
                    bool isAdmin = (tokens.size() > 3 && tokens[3] == "admin");
                    if (auth.registerUser(tokens[1], tokens[2], isAdmin)) {
                        std::cout << "User registered: " << tokens[1] << (isAdmin ? " (admin)" : "") << std::endl;
                    } else {
                        std::cout << "User already exists: " << tokens[1] << std::endl;
                    }
                }
                else if (cmd == "remove-user") {
                    if (tokens.size() != 2) {
                        std::cout << "Usage: remove-user <username>" << std::endl;
                        continue;
                    }
                    if (auth.removeUser(tokens[1])) {
                        std::cout << "User removed: " << tokens[1] << std::endl;
                    } else {
                        std::cout << "User not found: " << tokens[1] << std::endl;
                    }
                }
                else if (cmd == "whoami") {
                    if (auth.isLoggedIn()) {
                        std::cout << "Logged in as: " << auth.getCurrentUser();
                        if (auth.isAdmin(auth.getCurrentUser())) std::cout << " (admin)";
                        std::cout << std::endl;
                    } else {
                        std::cout << "Not logged in." << std::endl;
                    }
                }
                else if (cmd == "create-file") {
                    if (tokens.size() != 2) {
                        std::cout << "Usage: create-file <filename>" << std::endl;
                        continue;
                    }
                    if (fs->createFile(tokens[1])) {
                        std::cout << "File created successfully: " << tokens[1] << std::endl;
                        LOG_INFO("Created file: " + tokens[1]);
                    }
                }
                else if (cmd == "write-file") {
                    if (tokens.size() < 3) {
                        std::cout << "Usage: write-file <filename> <content>" << std::endl;
                        continue;
                    }
                    // Combine all remaining tokens as content
                    std::string content;
                    for (size_t i = 2; i < tokens.size(); ++i) {
                        content += tokens[i] + (i < tokens.size() - 1 ? " " : "");
                    }
                    if (fs->writeFile(tokens[1], content)) {
                        std::cout << "Content written successfully to: " << tokens[1] << std::endl;
                        LOG_INFO("Wrote content to file: " + tokens[1]);
                    }
                }
                else if (cmd == "read-file") {
                    if (tokens.size() != 2) {
                        std::cout << "Usage: read-file <filename>" << std::endl;
                        continue;
                    }
                    std::string content = fs->readFile(tokens[1]);
                    std::cout << "Content of " << tokens[1] << ":\n" << content << std::endl;
                    LOG_INFO("Read file: " + tokens[1]);
                }
                else if (cmd == "delete-file") {
                    if (tokens.size() != 2) {
                        std::cout << "Usage: delete-file <filename>" << std::endl;
                        continue;
                    }
                    if (fs->deleteFile(tokens[1])) {
                        std::cout << "File deleted successfully: " << tokens[1] << std::endl;
                        LOG_INFO("Deleted file: " + tokens[1]);
                    }
                }
                else if (cmd == "create-dir") {
                    if (tokens.size() != 2) {
                        std::cout << "Usage: create-dir <directoryname>" << std::endl;
                        continue;
                    }
                    if (fs->createDirectory(tokens[1])) {
                        std::cout << "Directory created successfully: " << tokens[1] << std::endl;
                        LOG_INFO("Created directory: " + tokens[1]);
                    }
                }
                else if (cmd == "list-dir") {
                    if (tokens.size() != 2) {
                        std::cout << "Usage: list-dir <directoryname>" << std::endl;
                        continue;
                    }
                    auto files = fs->listDirectory(tokens[1]);
                    std::cout << "\nContents of directory " << tokens[1] << ":\n";
                    for (const auto& file : files) {
                        std::cout << "  " << file << std::endl;
                    }
                    LOG_INFO("Listed directory: " + tokens[1]);
                }
                else if (cmd == "copy-file") {
                    if (tokens.size() != 3) {
                        std::cout << "Usage: copy-file <source> <destination>" << std::endl;
                        continue;
                    }
                    if (fs->copyFile(tokens[1], tokens[2])) {
                        std::cout << "File copied successfully: " << tokens[1] << " -> " << tokens[2] << std::endl;
                        LOG_INFO("Copied file: " + tokens[1] + " -> " + tokens[2]);
                    }
                }
                else if (cmd == "move-file") {
                    if (tokens.size() != 3) {
                        std::cout << "Usage: move-file <source> <destination>" << std::endl;
                        continue;
                    }
                    if (fs->moveFile(tokens[1], tokens[2])) {
                        std::cout << "File moved successfully: " << tokens[1] << " -> " << tokens[2] << std::endl;
                        LOG_INFO("Moved file: " + tokens[1] + " -> " + tokens[2]);
                    }
                }
                else if (cmd == "rename-file") {
                    if (tokens.size() != 3) {
                        std::cout << "Usage: rename-file <oldname> <newname>" << std::endl;
                        continue;
                    }
                    if (fs->renameFile(tokens[1], tokens[2])) {
                        std::cout << "File renamed successfully: " << tokens[1] << " -> " << tokens[2] << std::endl;
                        LOG_INFO("Renamed file: " + tokens[1] + " -> " + tokens[2]);
                    }
                }
                else if (cmd == "find-file") {
                    if (tokens.size() < 2 || tokens.size() > 3) {
                        std::cout << "Usage: find-file <pattern> [directory]" << std::endl;
                        continue;
                    }
                    std::string directory = (tokens.size() == 3) ? tokens[2] : ".";
                    auto results = fs->findFiles(tokens[1], directory);
                    std::cout << "\nFiles matching pattern '" << tokens[1] << "':\n";
                    for (const auto& file : results) {
                        std::cout << "  " << file << std::endl;
                    }
                    std::cout << "Found " << results.size() << " files." << std::endl;
                    LOG_INFO("Found " + std::to_string(results.size()) + " files matching: " + tokens[1]);
                }
                else if (cmd == "file-info") {
                    if (tokens.size() != 2) {
                        std::cout << "Usage: file-info <filename>" << std::endl;
                        continue;
                    }
                    auto metadata = fs->getFileInfo(tokens[1]);
                    std::cout << "\nFile Information for: " << tokens[1] << "\n";
                    std::cout << "  Name: " << metadata.name << "\n";
                    std::cout << "  Size: " << metadata.size << " bytes\n";
                    std::cout << "  Type: " << (metadata.isDirectory ? "Directory" : "File") << "\n";
                    std::cout << "  Permissions: " << std::oct << metadata.permissions << std::dec << "\n";
                    
                    // Convert time to readable format
                    auto created_time = std::chrono::system_clock::to_time_t(metadata.createdAt);
                    auto modified_time = std::chrono::system_clock::to_time_t(metadata.modifiedAt);
                    std::cout << "  Created: " << std::ctime(&created_time);
                    std::cout << "  Modified: " << std::ctime(&modified_time);
                    LOG_INFO("Displayed file info: " + tokens[1]);
                }
                else if (cmd == "show-stats") {
                    fs->showPerformanceDashboard();
                    LOG_INFO("Displayed performance statistics");
                }
                else if (cmd == "reset-stats") {
                    fs->resetStats();
                    std::cout << "Performance statistics have been reset." << std::endl;
                    LOG_INFO("Reset performance statistics");
                }
                else if (cmd == "compress-file") {
                    if (tokens.size() != 2) {
                        std::cout << "Usage: compress-file <filename>" << std::endl;
                        continue;
                    }
                    if (fs->compressFile(tokens[1])) {
                        std::cout << "File compressed successfully: " << tokens[1] << std::endl;
                        LOG_INFO("Compressed file: " + tokens[1]);
                    }
                }
                else if (cmd == "decompress-file") {
                    if (tokens.size() != 2) {
                        std::cout << "Usage: decompress-file <filename>" << std::endl;
                        continue;
                    }
                    if (fs->decompressFile(tokens[1])) {
                        std::cout << "File decompressed successfully: " << tokens[1] << std::endl;
                        LOG_INFO("Decompressed file: " + tokens[1]);
                    }
                }
                else if (cmd == "compression-stats") {
                    auto compStats = fs->getCompressionStats();
                    std::cout << "\n============= COMPRESSION STATISTICS =============\n";
                    std::cout << "Total Files Compressed: " << compStats.totalFilesCompressed << "\n";
                    std::cout << "Total Original Bytes: " << compStats.totalOriginalBytes << "\n";
                    std::cout << "Total Compressed Bytes: " << compStats.totalCompressedBytes << "\n";
                    std::cout << "Overall Compression Ratio: " << std::fixed << std::setprecision(2) 
                              << compStats.getOverallCompressionRatio() << "%\n";
                    if (compStats.totalOriginalBytes > 0) {
                        size_t spacesSaved = compStats.totalOriginalBytes - compStats.totalCompressedBytes;
                        std::cout << "Space Saved: " << spacesSaved << " bytes\n";
                    }
                    std::cout << "==================================================\n";
                    LOG_INFO("Displayed compression statistics");
                }
                else if (cmd == "create-backup") {
                    if (tokens.size() != 2) {
                        std::cout << "Usage: create-backup <backup_name>" << std::endl;
                        continue;
                    }
                    if (fs->createBackup(tokens[1])) {
                        std::cout << "Backup created successfully: " << tokens[1] << std::endl;
                        LOG_INFO("Created backup: " + tokens[1]);
                    }
                }
                else if (cmd == "restore-backup") {
                    if (tokens.size() < 2 || tokens.size() > 3) {
                        std::cout << "Usage: restore-backup <backup_name> [target_directory]" << std::endl;
                        continue;
                    }
                    std::string targetDir = (tokens.size() == 3) ? tokens[2] : "";
                    if (fs->restoreBackup(tokens[1], targetDir)) {
                        std::cout << "Backup restored successfully: " << tokens[1];
                        if (!targetDir.empty()) {
                            std::cout << " to " << targetDir;
                        }
                        std::cout << std::endl;
                        LOG_INFO("Restored backup: " + tokens[1]);
                    }
                }
                else if (cmd == "delete-backup") {
                    if (tokens.size() != 2) {
                        std::cout << "Usage: delete-backup <backup_name>" << std::endl;
                        continue;
                    }
                    if (fs->deleteBackup(tokens[1])) {
                        std::cout << "Backup deleted successfully: " << tokens[1] << std::endl;
                        LOG_INFO("Deleted backup: " + tokens[1]);
                    }
                }
                else if (cmd == "list-backups") {
                    auto backups = fs->listBackups();
                    std::cout << "\nAvailable Backups:\n";
                    if (backups.empty()) {
                        std::cout << "  No backups found.\n";
                    } else {
                        for (const auto& backup : backups) {
                            std::cout << "  " << backup << "\n";
                        }
                    }
                    std::cout << "Total: " << backups.size() << " backup(s)\n";
                    LOG_INFO("Listed " + std::to_string(backups.size()) + " backups");
                }                else if (cmd == "backup-dashboard") {
                    fs->showBackupDashboard();
                    LOG_INFO("Displayed backup dashboard");
                }
                else if (cmd == "set-cache-policy") {
                    if (tokens.size() != 2) {
                        std::cout << "Usage: set-cache-policy <policy>  # LRU, LFU, FIFO, LIFO" << std::endl;
                        continue;
                    }                    std::string policyStr = tokens[1];
                    CachePolicy policy;
                    if (policyStr == "LRU") policy = CachePolicy::LRU;
                    else if (policyStr == "LFU") policy = CachePolicy::LFU;
                    else if (policyStr == "FIFO") policy = CachePolicy::FIFO;
                    else if (policyStr == "LIFO") policy = CachePolicy::LIFO;
                    else {
                        std::cout << "Invalid policy. Use: LRU, LFU, FIFO, or LIFO" << std::endl;
                        continue;
                    }
                    fs->setCachePolicy(policy);
                    std::cout << "Cache policy set to: " << policyStr << std::endl;
                    LOG_INFO("Set cache policy to: " + policyStr);
                }
                else if (cmd == "get-cache-policy") {
                    auto policy = fs->getCachePolicy();
                    std::string policyStr;                    switch (policy) {
                        case CachePolicy::LRU: policyStr = "LRU"; break;
                        case CachePolicy::LFU: policyStr = "LFU"; break;
                        case CachePolicy::FIFO: policyStr = "FIFO"; break;
                        case CachePolicy::LIFO: policyStr = "LIFO"; break;
                        default: policyStr = "Unknown"; break;
                    }
                    std::cout << "Current cache policy: " << policyStr << std::endl;
                }
                else if (cmd == "resize-cache") {
                    if (tokens.size() != 2) {
                        std::cout << "Usage: resize-cache <size>" << std::endl;
                        continue;
                    }
                    size_t newSize = std::stoull(tokens[1]);
                    fs->resizeCache(newSize);
                    std::cout << "Cache resized to: " << newSize << std::endl;
                    LOG_INFO("Resized cache to: " + std::to_string(newSize));
                }
                else if (cmd == "pin-file") {
                    if (tokens.size() != 2) {
                        std::cout << "Usage: pin-file <filename>" << std::endl;
                        continue;
                    }
                    fs->pinFile(tokens[1]);
                    std::cout << "File pinned in cache: " << tokens[1] << std::endl;
                    LOG_INFO("Pinned file: " + tokens[1]);
                }
                else if (cmd == "unpin-file") {
                    if (tokens.size() != 2) {
                        std::cout << "Usage: unpin-file <filename>" << std::endl;
                        continue;
                    }
                    fs->unpinFile(tokens[1]);
                    std::cout << "File unpinned from cache: " << tokens[1] << std::endl;
                    LOG_INFO("Unpinned file: " + tokens[1]);
                }
                else if (cmd == "prefetch-file") {
                    if (tokens.size() != 2) {
                        std::cout << "Usage: prefetch-file <filename>" << std::endl;
                        continue;
                    }
                    fs->prefetchFile(tokens[1]);
                    std::cout << "File prefetched: " << tokens[1] << std::endl;
                    LOG_INFO("Prefetched file: " + tokens[1]);
                }
                else if (cmd == "cache-analytics") {
                    fs->showCacheAnalytics();
                    LOG_INFO("Displayed cache analytics");
                }
                else if (cmd == "hot-files") {
                    size_t count = 10;
                    if (tokens.size() > 1) {
                        count = std::stoull(tokens[1]);
                    }
                    auto hotFiles = fs->getHotFiles(count);
                    std::cout << "\nHot Files (Top " << count << "):\n";
                    if (hotFiles.empty()) {
                        std::cout << "  No files in cache.\n";
                    } else {
                        for (size_t i = 0; i < hotFiles.size(); ++i) {
                            std::cout << "  " << (i+1) << ". " << hotFiles[i] << "\n";
                        }
                    }
                    LOG_INFO("Displayed hot files");
                }
                else {
                    std::cout << "Unknown command. Type 'help' for available commands." << std::endl;
                }
            }
            catch (const FSException& e) {
                std::cout << "Error: " << e.what() << std::endl;
                LOG_ERROR(e.what());
            }
            catch (const std::exception& e) {
                std::cout << "System error: " << e.what() << std::endl;
                LOG_ERROR(e.what());
            }
        }

        return 0;
    }
    catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        LOG_ERROR("Fatal error: " + std::string(e.what()));
        return 1;
    }
}
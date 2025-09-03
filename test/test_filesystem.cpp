#include <gtest/gtest.h>
#include "fs/filesystem.hpp"
#include "common/error.hpp"
#include <filesystem>
#include <memory>
#include <thread>
#include <chrono>

namespace mtfs::test {

class FileSystemTest : public ::testing::Test {
protected:
    void SetUp() override {
        testRootPath = std::filesystem::temp_directory_path() / "mtfs_test";
        std::filesystem::create_directories(testRootPath);
        fs = mtfs::fs::FileSystem::create(testRootPath.string());
    }

    void TearDown() override {
        std::filesystem::remove_all(testRootPath);
    }

    std::filesystem::path testRootPath;
    std::shared_ptr<mtfs::fs::FileSystem> fs;
};

// Test basic file operations
TEST_F(FileSystemTest, BasicFileOperations) {
    const std::string testFile = "test.txt";
    const std::string testData = "Hello, World!";

    // Test file creation
    ASSERT_TRUE(fs->createFile(testFile));
    ASSERT_TRUE(fs->exists(testFile));

    // Test file writing
    ASSERT_TRUE(fs->writeFile(testFile, testData));

    // Test file reading
    std::string readData = fs->readFile(testFile);
    ASSERT_EQ(readData, testData);

    // Test file deletion
    ASSERT_TRUE(fs->deleteFile(testFile));
    ASSERT_FALSE(fs->exists(testFile));
}

// Test directory operations
TEST_F(FileSystemTest, DirectoryOperations) {
    const std::string testDir = "test_dir";
    const std::string testFile1 = "test_dir/file1.txt";
    const std::string testFile2 = "test_dir/file2.txt";

    // Test directory creation
    ASSERT_TRUE(fs->createDirectory(testDir));
    ASSERT_TRUE(fs->exists(testDir));

    // Create test files in directory
    ASSERT_TRUE(fs->createFile(testFile1));
    ASSERT_TRUE(fs->createFile(testFile2));

    // Test directory listing
    auto files = fs->listDirectory(testDir);
    ASSERT_EQ(files.size(), 2);
    ASSERT_TRUE(std::find(files.begin(), files.end(), "file1.txt") != files.end());
    ASSERT_TRUE(std::find(files.begin(), files.end(), "file2.txt") != files.end());
}

// Test metadata operations
TEST_F(FileSystemTest, MetadataOperations) {
    const std::string testFile = "metadata_test.txt";
    const std::string testData = "Test content for metadata";

    // Create and write to test file
    ASSERT_TRUE(fs->createFile(testFile));
    ASSERT_TRUE(fs->writeFile(testFile, testData));

    // Test metadata retrieval
    auto metadata = fs->getMetadata(testFile);
    ASSERT_EQ(metadata.name, "metadata_test.txt");
    ASSERT_EQ(metadata.size, testData.length());
    ASSERT_FALSE(metadata.isDirectory);

    // Test permissions
    fs->setPermissions(testFile, 0444);  // Read-only
    metadata = fs->getMetadata(testFile);
    ASSERT_EQ(metadata.permissions & 0777, 0444);
}

// Test error conditions
TEST_F(FileSystemTest, ErrorConditions) {
    const std::string nonExistentFile = "nonexistent.txt";
    const std::string testFile = "test.txt";
    const std::string testDir = "test_dir";

    // Test reading non-existent file
    ASSERT_THROW(fs->readFile(nonExistentFile), mtfs::common::FileNotFoundException);

    // Test writing to non-existent file
    ASSERT_THROW(fs->writeFile(nonExistentFile, "data"), mtfs::common::FileNotFoundException);

    // Create file and directory with same name
    ASSERT_TRUE(fs->createFile(testFile));
    ASSERT_THROW(fs->createDirectory(testFile), mtfs::common::FSException);

    ASSERT_TRUE(fs->createDirectory(testDir));
    ASSERT_THROW(fs->createFile(testDir), mtfs::common::FSException);
}

// Test large file operations
TEST_F(FileSystemTest, LargeFileOperations) {
    const std::string largeFile = "large.txt";
    const size_t fileSize = 1024 * 1024;  // 1MB
    std::string largeData(fileSize, 'X');

    // Test large file creation and writing
    ASSERT_TRUE(fs->createFile(largeFile));
    ASSERT_TRUE(fs->writeFile(largeFile, largeData));

    // Test large file reading
    std::string readData = fs->readFile(largeFile);
    ASSERT_EQ(readData.size(), fileSize);
    ASSERT_EQ(readData, largeData);
}

// Test low-level operations
TEST_F(FileSystemTest, LowLevelOperations) {
    const std::string testFile = "binary.dat";
    const size_t dataSize = 1024;
    std::vector<char> writeData(dataSize);
    std::vector<char> readData(dataSize);

    // Fill write buffer with pattern
    for (size_t i = 0; i < dataSize; ++i) {
        writeData[i] = static_cast<char>(i % 256);
    }

    // Test file creation
    ASSERT_TRUE(fs->createFile(testFile));

    // Test writing at different offsets
    ASSERT_EQ(fs->write(testFile, writeData.data(), dataSize/2, 0), dataSize/2);
    ASSERT_EQ(fs->write(testFile, writeData.data() + dataSize/2, dataSize/2, dataSize/2), dataSize/2);

    // Test reading
    ASSERT_EQ(fs->read(testFile, readData.data(), dataSize, 0), dataSize);
    ASSERT_EQ(memcmp(writeData.data(), readData.data(), dataSize), 0);
}

// Test concurrent operations
TEST_F(FileSystemTest, ConcurrentOperations) {
    const std::string testFile = "concurrent.txt";
    const int numThreads = 4;
    const int opsPerThread = 100;

    ASSERT_TRUE(fs->createFile(testFile));

    std::vector<std::thread> threads;
    std::atomic<int> successCount{0};

    for (int i = 0; i < numThreads; ++i) {
        threads.emplace_back([&, i]() {
            for (int j = 0; j < opsPerThread; ++j) {
                try {
                    std::string data = "Thread " + std::to_string(i) + " Op " + std::to_string(j);
                    fs->writeFile(testFile, data);
                    std::string readBack = fs->readFile(testFile);
                    if (!readBack.empty()) {
                        successCount++;
                    }
                } catch (const std::exception&) {
                    // Expected some operations to fail due to concurrency
                }
            }
        });
    }

    for (auto& thread : threads) {
        thread.join();
    }

    // Verify that some operations succeeded
    ASSERT_GT(successCount, 0);
}

} // namespace mtfs::test 
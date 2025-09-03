#pragma once

#include <string>
#include <vector>
#include <fstream>
#include <memory>
#include <windows.h>
#include "common/error.hpp"

namespace mtfs::storage {

using BlockId = int;  // Type alias for block identifiers

class BlockManager {
public:
    static constexpr size_t BLOCK_SIZE = 4096;  // 4KB blocks
    static constexpr size_t MAX_BLOCKS = 1024;  // Initial maximum blocks
    static constexpr size_t BITMAP_BYTES = (MAX_BLOCKS + 7) / 8;  // Size of bitmap in bytes

    explicit BlockManager(const std::string& storagePath);
    ~BlockManager();

    // Block operations
    bool writeBlock(int blockId, const std::vector<char>& data);
    bool readBlock(int blockId, std::vector<char>& data);
    int allocateBlock();  // Returns new block ID or -1 on failure
    bool freeBlock(int blockId);
    void formatStorage();

    // Utility methods
    size_t getTotalBlocks() const { return MAX_BLOCKS; }
    size_t getFreeBlocks();  // Removed const as it modifies critical section
    bool isBlockFree(int blockId);  // Removed const as it needs to lock

private:
    std::string storagePath;
    std::fstream storageFile;
    std::vector<uint8_t> blockBitmap;  // 1 = used, 0 = free
    CRITICAL_SECTION cs;  // Windows critical section for thread safety

    // Internal helper methods
    bool initializeStorage();
    void loadBitmap();
    void saveBitmap();
    bool validateBlockId(int blockId) const;
    size_t getBlockOffset(int blockId) const;
    bool setBit(size_t index, bool value);
    bool getBit(size_t index);
};

} // namespace mtfs::storage 
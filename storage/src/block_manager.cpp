#include "storage/block_manager.hpp"
#include "common/logger.hpp"
#include <algorithm>
#include <cstring>

namespace mtfs::storage {

using namespace mtfs::common;

BlockManager::BlockManager(const std::string& storagePath) 
    : storagePath(storagePath), blockBitmap(BITMAP_BYTES, 0) {
    InitializeCriticalSection(&cs);
    if (!initializeStorage()) {
        throw std::runtime_error("Failed to initialize storage");
    }
    loadBitmap();
    LOG_INFO("Block manager initialized at: " + storagePath);
}

BlockManager::~BlockManager() {
    EnterCriticalSection(&cs);
    saveBitmap();
    storageFile.close();
    LeaveCriticalSection(&cs);
    DeleteCriticalSection(&cs);
}

bool BlockManager::writeBlock(int blockId, const std::vector<char>& data) {
    EnterCriticalSection(&cs);
    try {
        if (!validateBlockId(blockId) || isBlockFree(blockId)) {
            LeaveCriticalSection(&cs);
            LOG_ERROR("Invalid block ID or block is free: " + std::to_string(blockId));
            return false;
        }

        if (data.size() > BLOCK_SIZE) {
            LeaveCriticalSection(&cs);
            LOG_ERROR("Data size exceeds block size");
            return false;
        }

        std::vector<char> blockData(BLOCK_SIZE, 0);
        std::copy(data.begin(), data.end(), blockData.begin());

        size_t offset = getBlockOffset(blockId);
        storageFile.seekp(offset);
        storageFile.write(blockData.data(), BLOCK_SIZE);
        storageFile.flush();

        LOG_DEBUG("Written block: " + std::to_string(blockId));
        LeaveCriticalSection(&cs);
        return true;
    } catch (const std::exception& e) {
        LOG_ERROR("Failed to write block: " + std::string(e.what()));
        LeaveCriticalSection(&cs);
        return false;
    }
}

bool BlockManager::readBlock(int blockId, std::vector<char>& data) {
    EnterCriticalSection(&cs);
    try {
        if (!validateBlockId(blockId) || isBlockFree(blockId)) {
            LeaveCriticalSection(&cs);
            LOG_ERROR("Invalid block ID or block is free: " + std::to_string(blockId));
            return false;
        }

        data.resize(BLOCK_SIZE);
        size_t offset = getBlockOffset(blockId);
        storageFile.seekg(offset);
        storageFile.read(data.data(), BLOCK_SIZE);

        LOG_DEBUG("Read block: " + std::to_string(blockId));
        LeaveCriticalSection(&cs);
        return true;
    } catch (const std::exception& e) {
        LOG_ERROR("Failed to read block: " + std::string(e.what()));
        LeaveCriticalSection(&cs);
        return false;
    }
}

int BlockManager::allocateBlock() {
    EnterCriticalSection(&cs);
    for (size_t i = 0; i < MAX_BLOCKS; ++i) {
        if (!getBit(i)) {
            setBit(i, true);
            saveBitmap();
            LOG_DEBUG("Allocated block: " + std::to_string(i));
            LeaveCriticalSection(&cs);
            return static_cast<int>(i);
        }
    }
    LOG_ERROR("No free blocks available");
    LeaveCriticalSection(&cs);
    return -1;
}

bool BlockManager::freeBlock(int blockId) {
    EnterCriticalSection(&cs);
    if (!validateBlockId(blockId) || isBlockFree(blockId)) {
        LeaveCriticalSection(&cs);
        LOG_ERROR("Invalid block ID or block already free: " + std::to_string(blockId));
        return false;
    }

    setBit(blockId, false);
    saveBitmap();
    LOG_DEBUG("Freed block: " + std::to_string(blockId));
    LeaveCriticalSection(&cs);
    return true;
}

void BlockManager::formatStorage() {
    EnterCriticalSection(&cs);
    // Clear bitmap
    std::fill(blockBitmap.begin(), blockBitmap.end(), 0);
    
    // Reset storage file
    storageFile.close();
    storageFile.open(storagePath, std::ios::out | std::ios::binary | std::ios::trunc);
    
    // Write empty blocks
    std::vector<char> emptyBlock(BLOCK_SIZE, 0);
    for (size_t i = 0; i < MAX_BLOCKS; ++i) {
        storageFile.write(emptyBlock.data(), BLOCK_SIZE);
    }
    storageFile.flush();
    
    // Reopen in read/write mode
    storageFile.close();
    storageFile.open(storagePath, std::ios::in | std::ios::out | std::ios::binary);
    
    saveBitmap();
    LOG_INFO("Storage formatted");
    LeaveCriticalSection(&cs);
}

size_t BlockManager::getFreeBlocks() {
    EnterCriticalSection(&cs);
    size_t count = 0;
    for (size_t i = 0; i < MAX_BLOCKS; ++i) {
        if (!getBit(i)) ++count;
    }
    LeaveCriticalSection(&cs);
    return count;
}

bool BlockManager::isBlockFree(int blockId) {
    if (!validateBlockId(blockId)) return true;
    EnterCriticalSection(&cs);
    bool result = !getBit(blockId);
    LeaveCriticalSection(&cs);
    return result;
}

// Private helper methods
bool BlockManager::initializeStorage() {
    storageFile.open(storagePath, std::ios::in | std::ios::out | std::ios::binary);
    if (!storageFile) {
        // Create new storage file
        storageFile.clear();
        storageFile.open(storagePath, std::ios::out | std::ios::binary);
        if (!storageFile) return false;
        
        // Initialize with empty blocks
        std::vector<char> emptyBlock(BLOCK_SIZE, 0);
        for (size_t i = 0; i < MAX_BLOCKS; ++i) {
            storageFile.write(emptyBlock.data(), BLOCK_SIZE);
        }
        storageFile.close();
        
        // Reopen in read/write mode
        storageFile.open(storagePath, std::ios::in | std::ios::out | std::ios::binary);
        return storageFile.good();
    }
    return true;
}

void BlockManager::loadBitmap() {
    EnterCriticalSection(&cs);
    storageFile.seekg(0);
    storageFile.read(reinterpret_cast<char*>(blockBitmap.data()), BITMAP_BYTES);
    LeaveCriticalSection(&cs);
}

void BlockManager::saveBitmap() {
    storageFile.seekp(0);
    storageFile.write(reinterpret_cast<const char*>(blockBitmap.data()), BITMAP_BYTES);
    storageFile.flush();
}

bool BlockManager::validateBlockId(int blockId) const {
    return blockId >= 0 && static_cast<size_t>(blockId) < MAX_BLOCKS;
}

size_t BlockManager::getBlockOffset(int blockId) const {
    return static_cast<size_t>(blockId) * BLOCK_SIZE + BITMAP_BYTES;
}

bool BlockManager::setBit(size_t index, bool value) {
    if (index >= MAX_BLOCKS) return false;
    size_t byteIndex = index / 8;
    size_t bitIndex = index % 8;
    if (value) {
        blockBitmap[byteIndex] |= (1 << bitIndex);
    } else {
        blockBitmap[byteIndex] &= ~(1 << bitIndex);
    }
    return true;
}

bool BlockManager::getBit(size_t index) {
    if (index >= MAX_BLOCKS) return false;
    size_t byteIndex = index / 8;
    size_t bitIndex = index % 8;
    return (blockBitmap[byteIndex] & (1 << bitIndex)) != 0;
}

} // namespace mtfs::storage 
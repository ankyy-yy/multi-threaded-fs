#pragma once

#include <cstdint>
#include <vector>
#include <memory>
#include <chrono>
#include <string>
#include "common/error.hpp"
#include "storage/block_manager.hpp"

namespace mtfs::journal {

enum class JournalEntryType {
    CREATE_FILE,
    DELETE_FILE,
    WRITE_DATA,
    CREATE_DIR,
    DELETE_DIR,
    UPDATE_METADATA
};

struct JournalEntry {
    uint64_t sequenceNumber{0};
    JournalEntryType type;
    std::chrono::system_clock::time_point timestamp;
    std::vector<storage::BlockId> blocks;
    std::vector<uint8_t> metadata;
    
    JournalEntry() : timestamp(std::chrono::system_clock::now()) {}
};

class Journal {
public:
    // Factory method for future extensibility
    static std::shared_ptr<Journal> create(std::shared_ptr<storage::BlockManager> blockManager = nullptr);

    // Core journal operations
    void initialize();
    void logOperation(const std::string& operation);
    void recover();
    void clear();
    
    // Transaction support (basic implementation)
    void beginTransaction();
    void commitTransaction();
    void rollbackTransaction();

    // Entry management
    void logEntry(const JournalEntry& entry);
    std::vector<JournalEntry> getEntries(uint64_t fromSequence, uint64_t toSequence) const;
    
    // Status and management
    bool needsRecovery() const;
    void checkpoint();
    size_t size() const;
    uint64_t getLastSequenceNumber() const;

    Journal() = default;
    ~Journal() = default;

private:
    std::vector<JournalEntry> entries;
    uint64_t currentSequence{0};
    bool inTransaction{false};
    std::shared_ptr<storage::BlockManager> blockManager;
};

} // namespace mtfs::journal 
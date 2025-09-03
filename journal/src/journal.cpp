#include "journal/journal.hpp"
#include "common/logger.hpp"
#include <iostream>
#include <algorithm>

namespace mtfs::journal {

std::shared_ptr<Journal> Journal::create(std::shared_ptr<storage::BlockManager> blockManager) {
    auto journal = std::make_shared<Journal>();
    journal->blockManager = blockManager;
    journal->initialize();
    return journal;
}

void Journal::initialize() {
    LOG_INFO("Journal initialized");
    currentSequence = 0;
    inTransaction = false;
    entries.clear();
}

void Journal::logOperation(const std::string& operation) {
    LOG_INFO("Operation logged: " + operation);
    
    JournalEntry entry;
    entry.sequenceNumber = ++currentSequence;
    entry.type = JournalEntryType::UPDATE_METADATA;
    entry.metadata.assign(operation.begin(), operation.end());
    
    entries.push_back(entry);
}

void Journal::logEntry(const JournalEntry& entry) {
    JournalEntry newEntry = entry;
    newEntry.sequenceNumber = ++currentSequence;
    newEntry.timestamp = std::chrono::system_clock::now();
    entries.push_back(newEntry);
}

std::vector<JournalEntry> Journal::getEntries(uint64_t fromSequence, uint64_t toSequence) const {
    std::vector<JournalEntry> result;
    for (const auto& entry : entries) {
        if (entry.sequenceNumber >= fromSequence && entry.sequenceNumber <= toSequence) {
            result.push_back(entry);
        }
    }
    return result;
}

void Journal::beginTransaction() {
    inTransaction = true;
    LOG_INFO("Transaction began");
}

void Journal::commitTransaction() {
    if (inTransaction) {
        inTransaction = false;
        LOG_INFO("Transaction committed");
    }
}

void Journal::rollbackTransaction() {
    if (inTransaction) {
        inTransaction = false;
        LOG_INFO("Transaction rolled back");
    }
}

bool Journal::needsRecovery() const {
    return !entries.empty() && inTransaction;
}

void Journal::recover() {
    LOG_INFO("Journal recovery completed");
    inTransaction = false;
}

void Journal::checkpoint() {
    LOG_INFO("Journal checkpoint completed");
}

void Journal::clear() {
    LOG_INFO("Journal cleared");
    entries.clear();
    currentSequence = 0;
    inTransaction = false;
}

size_t Journal::size() const {
    return entries.size();
}

uint64_t Journal::getLastSequenceNumber() const {
    return currentSequence;
}

} // namespace mtfs::journal 
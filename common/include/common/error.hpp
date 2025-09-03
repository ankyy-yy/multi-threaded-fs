#pragma once

#include <stdexcept>
#include <string>

namespace mtfs::common {

class FSException : public std::runtime_error {
public:
    explicit FSException(const std::string& message) : std::runtime_error(message) {}
};

class FileNotFoundException : public FSException {
public:
    explicit FileNotFoundException(const std::string& path)
        : FSException("File not found: " + path) {}
};

class PermissionDeniedException : public FSException {
public:
    explicit PermissionDeniedException(const std::string& path)
        : FSException("Permission denied: " + path) {}
};

class DiskFullException : public FSException {
public:
    explicit DiskFullException()
        : FSException("Disk is full") {}
};

class ConcurrencyException : public FSException {
public:
    explicit ConcurrencyException(const std::string& message)
        : FSException("Concurrency error: " + message) {}
};

class CacheException : public FSException {
public:
    explicit CacheException(const std::string& message)
        : FSException("Cache error: " + message) {}
};

class JournalException : public FSException {
public:
    explicit JournalException(const std::string& message)
        : FSException("Journal error: " + message) {}
};

} // namespace mtfs::common 
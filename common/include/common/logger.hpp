#pragma once
#include <string>

namespace mtfs::common {

void log_info(const std::string& message);
void log_error(const std::string& message);
void log_debug(const std::string& message);

// Simple macros for logging
#define LOG_INFO(msg) mtfs::common::log_info(msg)
#define LOG_ERROR(msg) mtfs::common::log_error(msg)
#define LOG_DEBUG(msg) mtfs::common::log_debug(msg)

} // namespace mtfs::common 
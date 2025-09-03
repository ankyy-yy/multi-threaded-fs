#include "common/logger.hpp"
#include <iostream>
#include <ctime>
#include <iomanip>
#include <sstream>

namespace mtfs::common {

static std::string getCurrentTime() {
    auto now = std::time(nullptr);
    auto tm = *std::localtime(&now);
    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
    return oss.str();
}

void log_debug(const std::string& message) {
    std::cout << "[" << getCurrentTime() << "] [DEBUG] " << message << std::endl;
}

void log_info(const std::string& message) {
    std::cout << "[" << getCurrentTime() << "] [INFO] " << message << std::endl;
}

void log_error(const std::string& message) {
    std::cerr << "[" << getCurrentTime() << "] [ERROR] " << message << std::endl;
}

} // namespace mtfs::common 
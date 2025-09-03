#include "common/auth.hpp"
#include <functional>
#include <fstream>

namespace mtfs::common {

AuthManager::AuthManager() {
    // Add a default admin user for bootstrapping
    registerUser("admin", "admin", true);
}

bool AuthManager::registerUser(const std::string& username, const std::string& password, bool isAdmin) {
    std::lock_guard<std::mutex> lock(authMutex);
    if (users.count(username)) return false;
    users[username] = User{username, hashPassword(password), isAdmin};
    return true;
}

bool AuthManager::authenticate(const std::string& username, const std::string& password) {
    std::lock_guard<std::mutex> lock(authMutex);
    auto it = users.find(username);
    if (it == users.end()) return false;
    if (it->second.passwordHash == hashPassword(password)) {
        currentUser = username;
        return true;
    }
    return false;
}

bool AuthManager::removeUser(const std::string& username) {
    std::lock_guard<std::mutex> lock(authMutex);
    return users.erase(username) > 0;
}

bool AuthManager::userExists(const std::string& username) const {
    std::lock_guard<std::mutex> lock(authMutex);
    return users.count(username) > 0;
}

bool AuthManager::isAdmin(const std::string& username) const {
    std::lock_guard<std::mutex> lock(authMutex);
    auto it = users.find(username);
    return it != users.end() && it->second.isAdmin;
}

void AuthManager::logout() {
    std::lock_guard<std::mutex> lock(authMutex);
    currentUser.clear();
}

bool AuthManager::isLoggedIn() const {
    std::lock_guard<std::mutex> lock(authMutex);
    return !currentUser.empty();
}

std::string AuthManager::getCurrentUser() const {
    std::lock_guard<std::mutex> lock(authMutex);
    return currentUser;
}

std::string AuthManager::hashPassword(const std::string& password) const {
    // Simple hash for demonstration (not secure!)
    std::hash<std::string> hasher;
    return std::to_string(hasher(password));
}

bool AuthManager::saveToFile(const std::string& filename) const {
    std::lock_guard<std::mutex> lock(authMutex);
    std::ofstream ofs(filename, std::ios::trunc);
    if (!ofs) return false;
    for (const auto& [username, user] : users) {
        ofs << username << '\t' << user.passwordHash << '\t' << user.isAdmin << '\n';
    }
    return true;
}

bool AuthManager::loadFromFile(const std::string& filename) {
    std::lock_guard<std::mutex> lock(authMutex);
    std::ifstream ifs(filename);
    if (!ifs) return false;
    users.clear();
    std::string username, passwordHash;
    int isAdminInt;
    while (ifs >> username >> passwordHash >> isAdminInt) {
        users[username] = User{username, passwordHash, isAdminInt != 0};
    }
    return true;
}

} // namespace mtfs::common

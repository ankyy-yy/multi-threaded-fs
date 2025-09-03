#pragma once

#include <string>
#include <unordered_map>
#include <mutex>

namespace mtfs::common {

struct User {
    std::string username;
    std::string passwordHash;
    bool isAdmin{false};
};

class AuthManager {
public:
    AuthManager();
    ~AuthManager() = default;

    bool registerUser(const std::string& username, const std::string& password, bool isAdmin = false);
    bool authenticate(const std::string& username, const std::string& password);
    bool removeUser(const std::string& username);
    bool userExists(const std::string& username) const;
    bool isAdmin(const std::string& username) const;
    void logout();
    bool isLoggedIn() const;
    std::string getCurrentUser() const;

    // Persistence
    bool saveToFile(const std::string& filename) const;
    bool loadFromFile(const std::string& filename);

private:
    std::unordered_map<std::string, User> users;
    std::string currentUser;
    mutable std::mutex authMutex;
    std::string hashPassword(const std::string& password) const;
};

} // namespace mtfs::common

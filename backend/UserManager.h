#pragma once
#include <string>
#include <unordered_map>
#include <mutex>
#include <memory>
#include <functional>
#include <random>
#include "User.h"
#include "Storage.h"
#include "third_party/picosha2.h"

class UserManager {
private:
    std::unordered_map<std::string, User> users_;
    mutable std::mutex mtx_;

    static std::string generateSalt() {
        static const char hexChars[] = "0123456789abcdef";
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(0, 15);
        std::string salt;
        salt.reserve(16);
        for (int i = 0; i < 16; ++i) {
            salt += hexChars[dis(gen)];
        }
        return salt;
    }

public:
    UserManager() {
        for (const auto& u : Storage::loadAllUsers()) {
            users_[u.getUsername()] = u;
        }
    }

    bool userExists(const std::string& username) const {
        std::lock_guard<std::mutex> lock(mtx_);
        return users_.find(username) != users_.end();
    }

    bool registerUser(const std::string& username, const std::string& password, double weightKg) {
        std::lock_guard<std::mutex> lock(mtx_);
        if (users_.find(username) != users_.end()) {
            return false;
        }
        std::string salt = generateSalt();
        std::string hash = picosha2::hash256_hex_string(salt + password);
        User user(username, salt, hash, weightKg, 500);
        if (!Storage::saveUser(user)) return false;
        users_[username] = user;
        return true;
    }

    bool verify(const std::string& username, const std::string& password) const {
        std::lock_guard<std::mutex> lock(mtx_);
        auto it = users_.find(username);
        if (it == users_.end()) return false;
        std::string hash = picosha2::hash256_hex_string(it->second.getSalt() + password);
        return hash == it->second.getPasswordHash();
    }

    std::unique_ptr<User> get(const std::string& username) const {
        std::lock_guard<std::mutex> lock(mtx_);
        auto it = users_.find(username);
        if (it == users_.end()) return nullptr;
        return std::make_unique<User>(it->second);
    }

    bool save(const User& user) {
        std::lock_guard<std::mutex> lock(mtx_);
        users_[user.getUsername()] = user;
        return Storage::saveUser(user);
    }

    bool updateUser(const std::string& username, std::function<void(User&)> modifier) {
        std::lock_guard<std::mutex> lock(mtx_);
        auto it = users_.find(username);
        if (it == users_.end()) return false;
        modifier(it->second);
        return Storage::saveUser(it->second);
    }
};

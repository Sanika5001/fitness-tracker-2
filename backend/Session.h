#pragma once
#include <string>
#include <unordered_map>
#include <mutex>
#include <random>

class Session {
private:
    std::unordered_map<std::string, std::string> tokenToUser_;
    mutable std::mutex mtx_;

    static std::string generateToken() {
        static const char alphanum[] =
            "0123456789"
            "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
            "abcdefghijklmnopqrstuvwxyz";
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(0, sizeof(alphanum) - 2);
        std::string token;
        token.reserve(32);
        for (int i = 0; i < 32; ++i) {
            token += alphanum[dis(gen)];
        }
        return token;
    }

public:
    std::string createSession(const std::string& username) {
        std::lock_guard<std::mutex> lock(mtx_);
        std::string token = generateToken();
        tokenToUser_[token] = username;
        return token;
    }

    std::string getUsername(const std::string& token) const {
        std::lock_guard<std::mutex> lock(mtx_);
        auto it = tokenToUser_.find(token);
        if (it != tokenToUser_.end()) {
            return it->second;
        }
        return "";
    }

    void removeSession(const std::string& token) {
        std::lock_guard<std::mutex> lock(mtx_);
        tokenToUser_.erase(token);
    }
};

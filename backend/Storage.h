#pragma once
#include <string>
#include <vector>
#include <memory>
#include <fstream>
#include <filesystem>
#include "User.h"
#include "third_party/json.hpp"

class Storage {
public:
    static bool saveUser(const User& user) {
        try {
            std::filesystem::create_directories("data/users");
            std::string path = "data/users/" + user.getUsername() + ".json";
            std::ofstream file(path);
            if (!file.is_open()) return false;
            file << user.toJson().dump(2);
            return true;
        } catch (...) {
            return false;
        }
    }

    static std::unique_ptr<User> loadUser(const std::string& username) {
        try {
            std::string path = "data/users/" + username + ".json";
            std::ifstream file(path);
            if (!file.is_open()) return nullptr;
            nlohmann::json j;
            file >> j;
            return std::make_unique<User>(User::fromJson(j));
        } catch (...) {
            return nullptr;
        }
    }

    static std::vector<User> loadAllUsers() {
        std::vector<User> list;
        try {
            std::filesystem::create_directories("data/users");
            for (const auto& entry : std::filesystem::directory_iterator("data/users")) {
                if (entry.is_regular_file() && entry.path().extension() == ".json") {
                    std::ifstream file(entry.path());
                    if (file.is_open()) {
                        nlohmann::json j;
                        file >> j;
                        list.push_back(User::fromJson(j));
                    }
                }
            }
        } catch (...) {}
        return list;
    }
};

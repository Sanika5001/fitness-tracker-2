#pragma once
#include <string>
#include <vector>
#include <memory>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>
#include "Exercise.h"
#include "WorkoutPlan.h"
#include "third_party/json.hpp"

inline std::string getDateOffset(int offsetDays) {
    auto now = std::chrono::system_clock::now() + std::chrono::hours(24 * offsetDays);
    std::time_t tt = std::chrono::system_clock::to_time_t(now);
    std::tm tm{};
#if defined(_WIN32)
    localtime_s(&tm, &tt);
#else
    localtime_r(&tt, &tm);
#endif
    char buf[32];
    std::strftime(buf, sizeof(buf), "%Y-%m-%d", &tm);
    return std::string(buf);
}

class User {
private:
    std::string username_;
    std::string salt_;
    std::string passwordHash_;
    double weightKg_{70.0};
    int dailyGoal_{500};
    std::vector<std::shared_ptr<Exercise>> log_;
    std::vector<WorkoutPlan> plans_;

public:
    User() = default;

    User(std::string username, std::string salt, std::string passwordHash, double weightKg, int dailyGoal = 500)
        : username_(std::move(username)), salt_(std::move(salt)), passwordHash_(std::move(passwordHash)),
          weightKg_(weightKg), dailyGoal_(dailyGoal) {}

    const std::string& getUsername() const { return username_; }
    const std::string& getSalt() const { return salt_; }
    const std::string& getPasswordHash() const { return passwordHash_; }
    double getWeightKg() const { return weightKg_; }
    void setWeightKg(double w) { weightKg_ = w; }
    int getDailyGoal() const { return dailyGoal_; }
    void setDailyGoal(int g) { dailyGoal_ = g; }

    const std::vector<std::shared_ptr<Exercise>>& getLog() const { return log_; }
    const std::vector<WorkoutPlan>& getPlans() const { return plans_; }

    void logExercise(std::shared_ptr<Exercise> ex) {
        if (ex) log_.push_back(std::move(ex));
    }

    void addPlan(const WorkoutPlan& plan) {
        for (auto& p : plans_) {
            if (p.getPlanName() == plan.getPlanName()) {
                p = plan;
                return;
            }
        }
        plans_.push_back(plan);
    }

    bool addExerciseToPlan(const std::string& planName, std::shared_ptr<Exercise> ex) {
        for (auto& p : plans_) {
            if (p.getPlanName() == planName) {
                p.addExercise(std::move(ex));
                return true;
            }
        }
        return false;
    }

    double caloriesOn(const std::string& date) const {
        double total = 0.0;
        for (const auto& ex : log_) {
            if (ex->getDate() == date) {
                total += ex->getCalories();
            }
        }
        return std::round(total);
    }

    int minutesOn(const std::string& date) const {
        int total = 0;
        for (const auto& ex : log_) {
            if (ex->getDate() == date) {
                total += ex->getDurationMin();
            }
        }
        return total;
    }

    int streak() const {
        std::string today = getDateOffset(0);
        int count = 0;
        int offset = 0;

        if (caloriesOn(today) > 0) {
            count = 1;
            offset = -1;
        } else {
            std::string yesterday = getDateOffset(-1);
            if (caloriesOn(yesterday) > 0) {
                count = 1;
                offset = -2;
            } else {
                return 0;
            }
        }

        while (true) {
            std::string d = getDateOffset(offset);
            if (caloriesOn(d) > 0) {
                count++;
                offset--;
            } else {
                break;
            }
        }
        return count;
    }

    nlohmann::json weekSummary() const {
        auto now = std::chrono::system_clock::now();
        std::time_t tt = std::chrono::system_clock::to_time_t(now);
        std::tm tm{};
#if defined(_WIN32)
        localtime_s(&tm, &tt);
#else
        localtime_r(&tt, &tm);
#endif
        char buf[32];
        std::strftime(buf, sizeof(buf), "%Y-%m-%d", &tm);
        std::string todayStr = buf;

        int daysSinceMonday = (tm.tm_wday == 0) ? 6 : (tm.tm_wday - 1);
        const char* dayNames[] = {"Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun"};

        nlohmann::json weekArr = nlohmann::json::array();
        for (int i = 0; i < 7; ++i) {
            int diff = i - daysSinceMonday;
            auto dayTime = now + std::chrono::hours(24 * diff);
            std::time_t dtt = std::chrono::system_clock::to_time_t(dayTime);
            std::tm dtm{};
#if defined(_WIN32)
            localtime_s(&dtm, &dtt);
#else
            localtime_r(&dtm, &dtt);
#endif
            std::strftime(buf, sizeof(buf), "%Y-%m-%d", &dtm);
            std::string dateStr = buf;
            bool future = (dateStr > todayStr);
            double cal = future ? 0.0 : caloriesOn(dateStr);
            weekArr.push_back({
                {"day", dayNames[i]},
                {"calories", std::round(cal)},
                {"future", future}
            });
        }

        nlohmann::json todayExArr = nlohmann::json::array();
        for (const auto& ex : log_) {
            if (ex->getDate() == todayStr) {
                todayExArr.push_back(ex->toJson());
            }
        }

        int totalMin = 0;
        double totalCal = 0.0;
        for (const auto& ex : log_) {
            totalMin += ex->getDurationMin();
            totalCal += ex->getCalories();
        }

        return {
            {"todayCalories", caloriesOn(todayStr)},
            {"todayMinutes", minutesOn(todayStr)},
            {"goal", dailyGoal_},
            {"streak", streak()},
            {"week", weekArr},
            {"today", todayExArr},
            {"totals", {
                {"workouts", log_.size()},
                {"minutes", totalMin},
                {"calories", std::round(totalCal)}
            }}
        };
    }

    nlohmann::json toJson() const {
        nlohmann::json logArr = nlohmann::json::array();
        for (const auto& ex : log_) {
            logArr.push_back(ex->toJson());
        }

        nlohmann::json plansArr = nlohmann::json::array();
        for (const auto& p : plans_) {
            plansArr.push_back(p.toJson());
        }

        return {
            {"username", username_},
            {"salt", salt_},
            {"passwordHash", passwordHash_},
            {"weightKg", weightKg_},
            {"dailyGoal", dailyGoal_},
            {"log", logArr},
            {"plans", plansArr}
        };
    }

    static User fromJson(const nlohmann::json& j) {
        User user(
            j.value("username", ""),
            j.value("salt", ""),
            j.value("passwordHash", ""),
            j.value("weightKg", 70.0),
            j.value("dailyGoal", 500)
        );

        if (j.contains("log") && j["log"].is_array()) {
            for (const auto& item : j["log"]) {
                auto ex = Exercise::fromJson(item);
                if (ex) user.logExercise(ex);
            }
        }

        if (j.contains("plans") && j["plans"].is_array()) {
            for (const auto& item : j["plans"]) {
                user.addPlan(WorkoutPlan::fromJson(item));
            }
        }

        return user;
    }
};

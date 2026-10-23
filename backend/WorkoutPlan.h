#pragma once
#include <string>
#include <vector>
#include <memory>
#include "Exercise.h"
#include "third_party/json.hpp"

class WorkoutPlan {
private:
    std::string planName_;
    std::vector<std::shared_ptr<Exercise>> exercises_;

public:
    WorkoutPlan() = default;
    explicit WorkoutPlan(std::string name) : planName_(std::move(name)) {}

    const std::string& getPlanName() const { return planName_; }
    const std::vector<std::shared_ptr<Exercise>>& getExercises() const { return exercises_; }

    void addExercise(std::shared_ptr<Exercise> ex) {
        if (ex) exercises_.push_back(std::move(ex));
    }

    int totalDuration() const {
        int total = 0;
        for (const auto& ex : exercises_) {
            total += ex->getDurationMin();
        }
        return total;
    }

    double totalCalories() const {
        double total = 0.0;
        for (const auto& ex : exercises_) {
            total += ex->getCalories();
        }
        return std::round(total);
    }

    nlohmann::json toJson() const {
        nlohmann::json list = nlohmann::json::array();
        for (const auto& ex : exercises_) {
            list.push_back(ex->toJson());
        }
        return {
            {"planName", planName_},
            {"exercises", list},
            {"totalDuration", totalDuration()},
            {"totalCalories", totalCalories()}
        };
    }

    static WorkoutPlan fromJson(const nlohmann::json& j) {
        WorkoutPlan plan(j.value("planName", ""));
        if (j.contains("exercises") && j["exercises"].is_array()) {
            for (const auto& item : j["exercises"]) {
                auto ex = Exercise::fromJson(item);
                if (ex) plan.addExercise(ex);
            }
        }
        return plan;
    }
};

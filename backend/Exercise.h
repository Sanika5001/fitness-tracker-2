#pragma once
#include <string>
#include <memory>
#include <iostream>
#include <cmath>
#include "third_party/json.hpp"

class Exercise {
protected:
    std::string name_;
    int durationMin_;
    double calories_;
    std::string date_;

public:
    Exercise(std::string name, int durationMin, double calories, std::string date)
        : name_(std::move(name)), durationMin_(durationMin), calories_(calories), date_(std::move(date)) {}
    virtual ~Exercise() = default;

    const std::string& getName() const { return name_; }
    int getDurationMin() const { return durationMin_; }
    double getCalories() const { return calories_; }
    const std::string& getDate() const { return date_; }
    void setDate(const std::string& d) { date_ = d; }
    void setCalories(double c) { calories_ = c; }

    virtual double calculateCalories(double weightKg) = 0;
    virtual std::string type() const = 0;
    virtual nlohmann::json toJson() const = 0;

    static std::shared_ptr<Exercise> fromJson(const nlohmann::json& j);

    friend std::ostream& operator<<(std::ostream& os, const Exercise& ex) {
        os << ex.name_ << " (" << ex.type() << ", " << ex.durationMin_ << " min, " << std::round(ex.calories_) << " kcal)";
        return os;
    }
};

class Cardio : public Exercise {
private:
    double met_;

public:
    Cardio(std::string name, int durationMin, double met, double calories = 0.0, std::string date = "")
        : Exercise(std::move(name), durationMin, calories, std::move(date)), met_(met) {}

    double getMet() const { return met_; }

    double calculateCalories(double weightKg) override {
        calories_ = std::round(met_ * weightKg * durationMin_ / 60.0);
        return calories_;
    }

    std::string type() const override { return "cardio"; }

    nlohmann::json toJson() const override {
        return {
            {"type", "cardio"},
            {"name", name_},
            {"durationMin", durationMin_},
            {"calories", std::round(calories_)},
            {"date", date_},
            {"met", met_}
        };
    }
};

class Strength : public Exercise {
private:
    int sets_;
    int reps_;

public:
    Strength(std::string name, int durationMin, int sets, int reps, double calories = 0.0, std::string date = "")
        : Exercise(std::move(name), durationMin, calories, std::move(date)), sets_(sets), reps_(reps) {}

    int getSets() const { return sets_; }
    int getReps() const { return reps_; }

    double calculateCalories(double /*weightKg*/) override {
        calories_ = sets_ * reps_ * 0.5;
        return calories_;
    }

    std::string type() const override { return "strength"; }

    nlohmann::json toJson() const override {
        return {
            {"type", "strength"},
            {"name", name_},
            {"durationMin", durationMin_},
            {"calories", std::round(calories_)},
            {"date", date_},
            {"sets", sets_},
            {"reps", reps_}
        };
    }
};

inline std::shared_ptr<Exercise> Exercise::fromJson(const nlohmann::json& j) {
    if (!j.is_object()) return nullptr;
    std::string t = j.value("type", "");
    std::string name = j.value("name", "");
    int duration = j.value("durationMin", 0);
    double calories = j.value("calories", 0.0);
    std::string date = j.value("date", "");
    if (t == "cardio") {
        double met = j.value("met", 0.0);
        return std::make_shared<Cardio>(name, duration, met, calories, date);
    } else if (t == "strength") {
        int sets = j.value("sets", 0);
        int reps = j.value("reps", 0);
        return std::make_shared<Strength>(name, duration, sets, reps, calories, date);
    }
    return nullptr;
}

#pragma once
#include <memory>
#include "UserManager.h"
#include "Exercise.h"
#include "WorkoutPlan.h"
#include "User.h"

class Seed {
public:
    static void init(UserManager& userManager) {
        if (userManager.userExists("demo")) return;

        userManager.registerUser("demo", "demo123", 68.0);
        userManager.updateUser("demo", [](User& demo) {
            demo.setDailyGoal(500);

            std::string d0 = getDateOffset(0);
            demo.logExercise(std::make_shared<Cardio>("Running", 30, 8.24, 280.0, d0));
            demo.logExercise(std::make_shared<Strength>("Push-ups", 10, 4, 30, 60.0, d0));
            demo.logExercise(std::make_shared<Cardio>("Cycling", 45, 6.27, 320.0, d0));

            std::string d1 = getDateOffset(-1);
            demo.logExercise(std::make_shared<Strength>("Plank", 10, 3, 30, 45.0, d1));

            std::string d2 = getDateOffset(-2);
            demo.logExercise(std::make_shared<Strength>("Squats", 15, 4, 25, 50.0, d2));

            std::string d3 = getDateOffset(-3);
            demo.logExercise(std::make_shared<Cardio>("Skipping", 25, 7.20, 204.0, d3));

            std::string d4 = getDateOffset(-4);
            demo.logExercise(std::make_shared<Cardio>("Running", 25, 10.48, 297.0, d4));

            std::string d5 = getDateOffset(-5);
            demo.logExercise(std::make_shared<Cardio>("Cycling", 20, 5.29, 120.0, d5));

            std::string d6 = getDateOffset(-6);
            demo.logExercise(std::make_shared<Strength>("Push-ups", 15, 3, 20, 30.0, d6));
            demo.logExercise(std::make_shared<Strength>("Squats", 15, 4, 30, 60.0, d6));

            WorkoutPlan p1("Morning cardio");
            p1.addExercise(std::make_shared<Cardio>("Running", 20, 8.0, 181.0));
            p1.addExercise(std::make_shared<Cardio>("Cycling", 20, 6.0, 136.0));
            p1.addExercise(std::make_shared<Cardio>("Skipping", 10, 8.0, 91.0));
            demo.addPlan(p1);

            WorkoutPlan p2("Upper body");
            p2.addExercise(std::make_shared<Strength>("Push-ups", 15, 4, 25, 50.0));
            p2.addExercise(std::make_shared<Strength>("Plank", 10, 4, 30, 60.0));
            p2.addExercise(std::make_shared<Strength>("Pull-ups", 15, 3, 12, 18.0));
            demo.addPlan(p2);

            WorkoutPlan p3("Leg day");
            p3.addExercise(std::make_shared<Strength>("Squats", 20, 5, 20, 50.0));
            p3.addExercise(std::make_shared<Strength>("Lunges", 15, 4, 15, 30.0));
            p3.addExercise(std::make_shared<Cardio>("Skipping", 15, 8.0, 136.0));
            demo.addPlan(p3);
        });
    }
};

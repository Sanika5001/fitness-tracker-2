#include <iostream>
#include <string>
#include <sstream>
#include <cctype>
#include <memory>
#include <cstdlib>
#include <vector>
#include <mutex>
#include <algorithm>
#include <filesystem>

#include "Clock.h"
#include "third_party/httplib.h"
#include "third_party/json.hpp"
#include "UserManager.h"
#include "Session.h"
#include "Seed.h"
#include "User.h"
#include "Exercise.h"
#include "WorkoutPlan.h"

static bool isValidUsername(const std::string& u) {
    if (u.size() < 3 || u.size() > 20) return false;
    for (char c : u) {
        if (!std::isalnum(static_cast<unsigned char>(c)) && c != '_') {
            return false;
        }
    }
    return true;
}

static std::string urlDecode(const std::string& in) {
    std::string out;
    out.reserve(in.size());
    for (std::size_t i = 0; i < in.size(); ++i) {
        if (in[i] == '%' && i + 2 < in.size()) {
            int value = 0;
            std::istringstream is(in.substr(i + 1, 2));
            if (is >> std::hex >> value) {
                out += static_cast<char>(value);
                i += 2;
            } else {
                out += in[i];
            }
        } else if (in[i] == '+') {
            out += ' ';
        } else {
            out += in[i];
        }
    }
    return out;
}

static std::string authUser(const httplib::Request& req, const Session& sessions, httplib::Response& res) {
    auto auth = req.get_header_value("Authorization");
    if (auth.rfind("Bearer ", 0) == 0) {
        std::string token = auth.substr(7);
        std::string user = sessions.getUsername(token);
        if (!user.empty()) return user;
    }
    res.status = 401;
    res.set_content(nlohmann::json({{"error", "Unauthorized"}}).dump(), "application/json");
    return "";
}

static std::shared_ptr<Exercise> parseExercise(const nlohmann::json& j, double userWeight, const std::string& defaultDate) {
    std::string type = j.value("type", "");
    std::string name = j.value("name", "");
    int duration = j.value("durationMin", 0);
    std::string date = j.value("date", "");
    if (date.empty()) date = defaultDate;

    if (name.empty() || duration <= 0) return nullptr;

    if (type == "cardio") {
        double met = j.value("met", 0.0);
        if (met <= 0.0) return nullptr;
        auto ex = std::make_shared<Cardio>(name, duration, met, 0.0, date);
        ex->calculateCalories(userWeight);
        return ex;
    } else if (type == "strength") {
        int sets = j.value("sets", 0);
        int reps = j.value("reps", 0);
        if (sets <= 0 || reps <= 0) return nullptr;
        auto ex = std::make_shared<Strength>(name, duration, sets, reps, 0.0, date);
        ex->calculateCalories(userWeight);
        return ex;
    }
    return nullptr;
}

int main() {
    UserManager userManager;
    Session sessions;
    Seed::init(userManager);

    httplib::Server svr;

    svr.Post("/api/register", [&](const httplib::Request& req, httplib::Response& res) {
        try {
            auto body = nlohmann::json::parse(req.body);
            std::string username = body.value("username", "");
            std::string password = body.value("password", "");
            double weight = body.value("weight", 0.0);

            if (!isValidUsername(username) || password.size() < 6 || weight < 20.0 || weight > 300.0) {
                res.status = 400;
                res.set_content(nlohmann::json({{"error", "Invalid username, password (min 6 chars), or weight (20-300)"}}).dump(), "application/json");
                return;
            }

            if (!userManager.registerUser(username, password, weight)) {
                res.status = 409;
                res.set_content(nlohmann::json({{"error", "Username already exists"}}).dump(), "application/json");
                return;
            }

            res.status = 201;
            res.set_content(nlohmann::json({{"ok", true}}).dump(), "application/json");
        } catch (...) {
            res.status = 400;
            res.set_content(nlohmann::json({{"error", "Malformed request body"}}).dump(), "application/json");
        }
    });

    svr.Post("/api/login", [&](const httplib::Request& req, httplib::Response& res) {
        try {
            auto body = nlohmann::json::parse(req.body);
            std::string username = body.value("username", "");
            std::string password = body.value("password", "");

            if (!userManager.verify(username, password)) {
                res.status = 401;
                res.set_content(nlohmann::json({{"error", "Invalid username or password"}}).dump(), "application/json");
                return;
            }

            std::string token = sessions.createSession(username);
            res.status = 200;
            res.set_content(nlohmann::json({{"token", token}}).dump(), "application/json");
        } catch (...) {
            res.status = 400;
            res.set_content(nlohmann::json({{"error", "Malformed request body"}}).dump(), "application/json");
        }
    });

    svr.Post("/api/logout", [&](const httplib::Request& req, httplib::Response& res) {
        auto auth = req.get_header_value("Authorization");
        if (auth.rfind("Bearer ", 0) == 0) {
            sessions.removeSession(auth.substr(7));
        }
        res.status = 200;
        res.set_content(nlohmann::json({{"ok", true}}).dump(), "application/json");
    });

    svr.Get("/api/me", [&](const httplib::Request& req, httplib::Response& res) {
        std::string uname = authUser(req, sessions, res);
        if (uname.empty()) return;
        auto user = userManager.get(uname);
        if (!user) {
            res.status = 404;
            res.set_content(nlohmann::json({{"error", "User not found"}}).dump(), "application/json");
            return;
        }
        res.status = 200;
        res.set_content(nlohmann::json({
            {"username", user->getUsername()},
            {"weight", user->getWeightKg()},
            {"dailyGoal", user->getDailyGoal()}
        }).dump(), "application/json");
    });

    svr.Put("/api/me", [&](const httplib::Request& req, httplib::Response& res) {
        std::string uname = authUser(req, sessions, res);
        if (uname.empty()) return;
        try {
            auto body = nlohmann::json::parse(req.body);
            double weight = body.value("weight", -1.0);
            int dailyGoal = body.value("dailyGoal", -1);

            if (weight < 20.0 || weight > 300.0 || dailyGoal < 100 || dailyGoal > 10000) {
                res.status = 400;
                res.set_content(nlohmann::json({{"error", "Invalid weight (20-300) or goal (100-10000)"}}).dump(), "application/json");
                return;
            }

            userManager.updateUser(uname, [weight, dailyGoal](User& u) {
                u.setWeightKg(weight);
                u.setDailyGoal(dailyGoal);
            });

            auto updated = userManager.get(uname);
            res.status = 200;
            res.set_content(nlohmann::json({
                {"username", updated->getUsername()},
                {"weight", updated->getWeightKg()},
                {"dailyGoal", updated->getDailyGoal()}
            }).dump(), "application/json");
        } catch (...) {
            res.status = 400;
            res.set_content(nlohmann::json({{"error", "Malformed request body"}}).dump(), "application/json");
        }
    });

    svr.Get("/api/summary", [&](const httplib::Request& req, httplib::Response& res) {
        std::string uname = authUser(req, sessions, res);
        if (uname.empty()) return;
        auto user = userManager.get(uname);
        if (!user) {
            res.status = 404;
            res.set_content(nlohmann::json({{"error", "User not found"}}).dump(), "application/json");
            return;
        }
        res.status = 200;
        res.set_content(user->weekSummary().dump(), "application/json");
    });

    svr.Get("/api/exercises", [&](const httplib::Request& req, httplib::Response& res) {
        std::string uname = authUser(req, sessions, res);
        if (uname.empty()) return;
        auto user = userManager.get(uname);
        if (!user) {
            res.status = 404;
            res.set_content(nlohmann::json({{"error", "User not found"}}).dump(), "application/json");
            return;
        }

        nlohmann::json list = nlohmann::json::array();
        const auto& log = user->getLog();
        for (auto it = log.rbegin(); it != log.rend(); ++it) {
            list.push_back((*it)->toJson());
        }
        res.status = 200;
        res.set_content(list.dump(), "application/json");
    });

    svr.Post("/api/exercises", [&](const httplib::Request& req, httplib::Response& res) {
        std::string uname = authUser(req, sessions, res);
        if (uname.empty()) return;
        auto user = userManager.get(uname);
        if (!user) {
            res.status = 404;
            res.set_content(nlohmann::json({{"error", "User not found"}}).dump(), "application/json");
            return;
        }

        try {
            auto body = nlohmann::json::parse(req.body);
            auto ex = parseExercise(body, user->getWeightKg(), getDateOffset(0));
            if (!ex) {
                res.status = 400;
                res.set_content(nlohmann::json({{"error", "Invalid exercise parameters"}}).dump(), "application/json");
                return;
            }

            userManager.updateUser(uname, [&ex](User& u) {
                u.logExercise(ex);
            });

            res.status = 201;
            res.set_content(ex->toJson().dump(), "application/json");
        } catch (...) {
            res.status = 400;
            res.set_content(nlohmann::json({{"error", "Malformed request body"}}).dump(), "application/json");
        }
    });

    svr.Get("/api/plans", [&](const httplib::Request& req, httplib::Response& res) {
        std::string uname = authUser(req, sessions, res);
        if (uname.empty()) return;
        auto user = userManager.get(uname);
        if (!user) {
            res.status = 404;
            res.set_content(nlohmann::json({{"error", "User not found"}}).dump(), "application/json");
            return;
        }

        nlohmann::json list = nlohmann::json::array();
        for (const auto& p : user->getPlans()) {
            list.push_back(p.toJson());
        }
        res.status = 200;
        res.set_content(list.dump(), "application/json");
    });

    svr.Post("/api/plans", [&](const httplib::Request& req, httplib::Response& res) {
        std::string uname = authUser(req, sessions, res);
        if (uname.empty()) return;
        try {
            auto body = nlohmann::json::parse(req.body);
            std::string planName = body.value("planName", "");
            if (planName.empty()) {
                res.status = 400;
                res.set_content(nlohmann::json({{"error", "Plan name is required"}}).dump(), "application/json");
                return;
            }

            userManager.updateUser(uname, [&planName](User& u) {
                u.addPlan(WorkoutPlan(planName));
            });

            res.status = 201;
            res.set_content(nlohmann::json({{"ok", true}, {"planName", planName}}).dump(), "application/json");
        } catch (...) {
            res.status = 400;
            res.set_content(nlohmann::json({{"error", "Malformed request body"}}).dump(), "application/json");
        }
    });

    svr.Post(R"(/api/plans/([^/]+)/exercises)", [&](const httplib::Request& req, httplib::Response& res) {
        std::string uname = authUser(req, sessions, res);
        if (uname.empty()) return;
        auto user = userManager.get(uname);
        if (!user) {
            res.status = 404;
            res.set_content(nlohmann::json({{"error", "User not found"}}).dump(), "application/json");
            return;
        }

        std::string planName = urlDecode(req.matches[1].str());
        try {
            auto body = nlohmann::json::parse(req.body);
            auto ex = parseExercise(body, user->getWeightKg(), getDateOffset(0));
            if (!ex) {
                res.status = 400;
                res.set_content(nlohmann::json({{"error", "Invalid exercise parameters"}}).dump(), "application/json");
                return;
            }

            bool added = false;
            userManager.updateUser(uname, [&planName, &ex, &added](User& u) {
                added = u.addExerciseToPlan(planName, ex);
            });

            if (!added) {
                res.status = 404;
                res.set_content(nlohmann::json({{"error", "Plan not found"}}).dump(), "application/json");
                return;
            }

            res.status = 201;
            res.set_content(ex->toJson().dump(), "application/json");
        } catch (...) {
            res.status = 400;
            res.set_content(nlohmann::json({{"error", "Malformed request body"}}).dump(), "application/json");
        }
    });

    svr.Get("/", [](const httplib::Request&, httplib::Response& res) {
        res.set_redirect("/login.html");
    });

    svr.set_mount_point("/", "./frontend");
    svr.set_default_headers({{"Cache-Control", "no-cache"}});

    const char* portEnv = std::getenv("PORT");
    int port = portEnv ? std::stoi(portEnv) : 10000;
    std::cout << "FitTrack server listening on 0.0.0.0:" << port << "\n";
    svr.listen("0.0.0.0", port);

    return 0;
}

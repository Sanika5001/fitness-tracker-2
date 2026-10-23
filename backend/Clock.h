#pragma once
#include <ctime>
#include <string>
#include <chrono>

inline std::tm localTm(std::time_t t) {
    std::tm tm{};
#ifdef _WIN32
    localtime_s(&tm, &t);
#else
    localtime_r(&t, &tm);
#endif
    return tm;
}

inline std::string getDateOffset(int offsetDays) {
    auto now = std::chrono::system_clock::now() + std::chrono::hours(24 * offsetDays);
    std::time_t tt = std::chrono::system_clock::to_time_t(now);
    std::tm tm = localTm(tt);
    char buf[32];
    std::strftime(buf, sizeof(buf), "%Y-%m-%d", &tm);
    return std::string(buf);
}

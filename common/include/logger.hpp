#pragma once
#include <chrono>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <sstream>
#include <string>

namespace accel {

class Logger {
public:
    enum class Level { DEBUG, INFO, WARN, ERROR };

    static Logger& instance() {
        static Logger logger;
        return logger;
    }

    void set_level(Level level) { min_level_ = level; }
    void set_name(const std::string& name) { name_ = name; }

    void log(Level level, const std::string& msg) {
        if (level < min_level_) return;
        std::lock_guard<std::mutex> lock(mutex_);
        std::cerr << timestamp() << " [" << level_str(level) << "] ["
                  << name_ << "] " << msg << "\n";
    }

    void debug(const std::string& msg) { log(Level::DEBUG, msg); }
    void info(const std::string& msg)  { log(Level::INFO,  msg); }
    void warn(const std::string& msg)  { log(Level::WARN,  msg); }
    void error(const std::string& msg) { log(Level::ERROR, msg); }

private:
    Logger() = default;

    std::mutex  mutex_;
    Level       min_level_ = Level::DEBUG;
    std::string name_      = "app";

    static std::string timestamp() {
        auto now = std::chrono::system_clock::now();
        auto t   = std::chrono::system_clock::to_time_t(now);
        auto ms  = std::chrono::duration_cast<std::chrono::milliseconds>(
                       now.time_since_epoch()) % 1000;
        std::ostringstream ss;
        ss << std::put_time(std::localtime(&t), "%Y-%m-%d %H:%M:%S")
           << "." << std::setfill('0') << std::setw(3) << ms.count();
        return ss.str();
    }

    static const char* level_str(Level l) {
        switch (l) {
            case Level::DEBUG: return "DEBUG";
            case Level::INFO:  return "INFO ";
            case Level::WARN:  return "WARN ";
            case Level::ERROR: return "ERROR";
        }
        return "?????";
    }
};

#define LOG_DEBUG(msg) accel::Logger::instance().debug(msg)
#define LOG_INFO(msg)  accel::Logger::instance().info(msg)
#define LOG_WARN(msg)  accel::Logger::instance().warn(msg)
#define LOG_ERROR(msg) accel::Logger::instance().error(msg)

} // namespace accel

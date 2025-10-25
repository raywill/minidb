#include "log/logger.h"
#include <iostream>
#include <iomanip>
#include <chrono>
#include <ctime>

namespace minidb {

std::string LogLevelToString(LogLevel level) {
    switch (level) {
        case LogLevel::DEBUG: return "DEBUG";
        case LogLevel::INFO: return "INFO";
        case LogLevel::WARN: return "WARN";
        case LogLevel::ERROR: return "ERROR";
        case LogLevel::FATAL: return "FATAL";
        default: return "UNKNOWN";
    }
}

LogLevel StringToLogLevel(const std::string& str) {
    if (str == "DEBUG") return LogLevel::DEBUG;
    if (str == "INFO") return LogLevel::INFO;
    if (str == "WARN") return LogLevel::WARN;
    if (str == "ERROR") return LogLevel::ERROR;
    if (str == "FATAL") return LogLevel::FATAL;
    return LogLevel::INFO; // 默认级别
}

LogRecord::LogRecord(LogLevel l, const std::string& m, const std::string& c, const std::string& msg)
    : level(l), module(m), context(c), message(msg) {
    
    // 生成时间戳
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()) % 1000;
    
    std::ostringstream oss;
    oss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
    oss << '.' << std::setfill('0') << std::setw(3) << ms.count();
    timestamp = oss.str();
    
    // 获取线程ID
    thread_id = std::this_thread::get_id();
}

std::string LogRecord::format() const {
    std::ostringstream oss;
    oss << "[" << timestamp << "] "
        << "[" << LogLevelToString(level) << "] "
        << "[TID=" << thread_id << "] "
        << "[" << module << "] ";
    
    if (!context.empty()) {
        oss << "[" << context << "] ";
    }
    
    oss << message;
    return oss.str();
}

// FileSink 实现
FileSink::FileSink(const std::string& filename) {
    file_.open(filename, std::ios::app);
    if (!file_.is_open()) {
        throw std::runtime_error("Failed to open log file: " + filename);
    }
}

FileSink::~FileSink() {
    if (file_.is_open()) {
        file_.close();
    }
}

void FileSink::write(const LogRecord& record) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (file_.is_open()) {
        file_ << record.format() << std::endl;
    }
}

void FileSink::flush() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (file_.is_open()) {
        file_.flush();
    }
}

// ConsoleSink 实现
void ConsoleSink::write(const LogRecord& record) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // 根据日志级别选择输出流
    if (record.level >= LogLevel::ERROR) {
        std::cerr << record.format() << std::endl;
    } else {
        std::cout << record.format() << std::endl;
    }
}

void ConsoleSink::flush() {
    std::lock_guard<std::mutex> lock(mutex_);
    std::cout.flush();
    std::cerr.flush();
}

// Logger 实现
Logger* Logger::instance_ = nullptr;
std::once_flag Logger::init_flag_;

Logger::Logger() : level_(LogLevel::INFO) {
    // 默认添加控制台输出
    add_sink(std::make_shared<ConsoleSink>());
}

Logger::~Logger() = default;

void Logger::set_level(LogLevel level) {
    std::lock_guard<std::mutex> lock(mutex_);
    level_ = level;
}

LogLevel Logger::get_level() const {
    return level_;
}

void Logger::add_sink(std::shared_ptr<LogSink> sink) {
    std::lock_guard<std::mutex> lock(mutex_);
    sinks_.push_back(sink);
}

void Logger::clear_sinks() {
    std::lock_guard<std::mutex> lock(mutex_);
    sinks_.clear();
}

void Logger::log(LogLevel level, const std::string& module, const std::string& context, const std::string& message) {
    // 检查日志级别
    if (level < level_) {
        return;
    }
    
    LogRecord record(level, module, context, message);
    
    std::lock_guard<std::mutex> lock(mutex_);
    for (auto& sink : sinks_) {
        sink->write(record);
    }
    
    // 如果是FATAL级别，立即刷新所有输出
    if (level == LogLevel::FATAL) {
        for (auto& sink : sinks_) {
            sink->flush();
        }
    }
}

void Logger::debug(const std::string& module, const std::string& context, const std::string& message) {
    log(LogLevel::DEBUG, module, context, message);
}

void Logger::info(const std::string& module, const std::string& context, const std::string& message) {
    log(LogLevel::INFO, module, context, message);
}

void Logger::warn(const std::string& module, const std::string& context, const std::string& message) {
    log(LogLevel::WARN, module, context, message);
}

void Logger::error(const std::string& module, const std::string& context, const std::string& message) {
    log(LogLevel::ERROR, module, context, message);
}

void Logger::fatal(const std::string& module, const std::string& context, const std::string& message) {
    log(LogLevel::FATAL, module, context, message);
}

Logger* Logger::instance() {
    std::call_once(init_flag_, []() {
        instance_ = new Logger();
    });
    return instance_;
}

} // namespace minidb

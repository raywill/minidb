#pragma once

#include <string>
#include <fstream>
#include <memory>
#include <mutex>
#include <thread>
#include <sstream>

namespace minidb {

// 日志级别枚举
enum class LogLevel {
    DEBUG = 0,
    INFO = 1,
    WARN = 2,
    ERROR = 3,
    FATAL = 4
};

// 将日志级别转换为字符串
std::string LogLevelToString(LogLevel level);

// 从字符串解析日志级别
LogLevel StringToLogLevel(const std::string& str);

// 日志记录结构
struct LogRecord {
    LogLevel level;
    std::string timestamp;
    std::thread::id thread_id;
    std::string module;
    std::string context;
    std::string message;
    
    LogRecord(LogLevel l, const std::string& m, const std::string& c, const std::string& msg);
    
    // 格式化为字符串
    std::string format() const;
};

// 日志输出接口
class LogSink {
public:
    virtual ~LogSink() = default;
    virtual void write(const LogRecord& record) = 0;
    virtual void flush() = 0;
};

// 文件日志输出
class FileSink : public LogSink {
public:
    explicit FileSink(const std::string& filename);
    ~FileSink() override;
    
    void write(const LogRecord& record) override;
    void flush() override;
    
private:
    std::ofstream file_;
    std::mutex mutex_;
};

// 控制台日志输出
class ConsoleSink : public LogSink {
public:
    ConsoleSink() = default;
    ~ConsoleSink() override = default;
    
    void write(const LogRecord& record) override;
    void flush() override;
    
private:
    std::mutex mutex_;
};

// 日志器类
class Logger {
public:
    Logger();
    ~Logger();
    
    // 设置日志级别
    void set_level(LogLevel level);
    LogLevel get_level() const;
    
    // 添加日志输出
    void add_sink(std::shared_ptr<LogSink> sink);
    void clear_sinks();
    
    // 记录日志
    void log(LogLevel level, const std::string& module, const std::string& context, const std::string& message);
    
    void debug(const std::string& module, const std::string& context, const std::string& message);
    void info(const std::string& module, const std::string& context, const std::string& message);
    void warn(const std::string& module, const std::string& context, const std::string& message);
    void error(const std::string& module, const std::string& context, const std::string& message);
    void fatal(const std::string& module, const std::string& context, const std::string& message);
    
    // 获取全局日志器实例
    static Logger* instance();
    
private:
    LogLevel level_;
    std::vector<std::shared_ptr<LogSink>> sinks_;
    std::mutex mutex_;
    
    static Logger* instance_;
    static std::once_flag init_flag_;
};

// 日志宏定义
#define LOG_DEBUG(module, context, message) \
    Logger::instance()->debug(module, context, message)

#define LOG_INFO(module, context, message) \
    Logger::instance()->info(module, context, message)

#define LOG_WARN(module, context, message) \
    Logger::instance()->warn(module, context, message)

#define LOG_ERROR(module, context, message) \
    Logger::instance()->error(module, context, message)

#define LOG_FATAL(module, context, message) \
    Logger::instance()->fatal(module, context, message)

// 便利宏，使用流式语法
#define LOG_STREAM(level, module, context) \
    LogStream(level, module, context)

class LogStream {
public:
    LogStream(LogLevel level, const std::string& module, const std::string& context)
        : level_(level), module_(module), context_(context) {}
    
    ~LogStream() {
        Logger::instance()->log(level_, module_, context_, stream_.str());
    }
    
    template<typename T>
    LogStream& operator<<(const T& value) {
        stream_ << value;
        return *this;
    }
    
private:
    LogLevel level_;
    std::string module_;
    std::string context_;
    std::ostringstream stream_;
};

} // namespace minidb

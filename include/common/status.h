#pragma once

#include <string>
#include <memory>

namespace minidb {

// 状态码枚举
enum class StatusCode {
    OK,
    INVALID_ARGUMENT,
    NOT_FOUND,
    ALREADY_EXISTS,
    IO_ERROR,
    MEMORY_ERROR,
    PARSE_ERROR,
    EXECUTION_ERROR,
    NETWORK_ERROR,
    INTERNAL_ERROR
};

// 状态类
class Status {
public:
    // 构造函数
    Status() : code_(StatusCode::OK) {}
    Status(StatusCode code, const std::string& message) 
        : code_(code), message_(message) {}
    
    // 静态工厂方法
    static Status OK() { return Status(); }
    static Status InvalidArgument(const std::string& msg) {
        return Status(StatusCode::INVALID_ARGUMENT, msg);
    }
    static Status NotFound(const std::string& msg) {
        return Status(StatusCode::NOT_FOUND, msg);
    }
    static Status AlreadyExists(const std::string& msg) {
        return Status(StatusCode::ALREADY_EXISTS, msg);
    }
    static Status IOError(const std::string& msg) {
        return Status(StatusCode::IO_ERROR, msg);
    }
    static Status MemoryError(const std::string& msg) {
        return Status(StatusCode::MEMORY_ERROR, msg);
    }
    static Status ParseError(const std::string& msg) {
        return Status(StatusCode::PARSE_ERROR, msg);
    }
    static Status ExecutionError(const std::string& msg) {
        return Status(StatusCode::EXECUTION_ERROR, msg);
    }
    static Status NetworkError(const std::string& msg) {
        return Status(StatusCode::NETWORK_ERROR, msg);
    }
    static Status InternalError(const std::string& msg) {
        return Status(StatusCode::INTERNAL_ERROR, msg);
    }
    
    // 状态检查
    bool ok() const { return code_ == StatusCode::OK; }
    bool is_invalid_argument() const { return code_ == StatusCode::INVALID_ARGUMENT; }
    bool is_not_found() const { return code_ == StatusCode::NOT_FOUND; }
    bool is_already_exists() const { return code_ == StatusCode::ALREADY_EXISTS; }
    bool is_io_error() const { return code_ == StatusCode::IO_ERROR; }
    bool is_memory_error() const { return code_ == StatusCode::MEMORY_ERROR; }
    bool is_parse_error() const { return code_ == StatusCode::PARSE_ERROR; }
    bool is_execution_error() const { return code_ == StatusCode::EXECUTION_ERROR; }
    bool is_network_error() const { return code_ == StatusCode::NETWORK_ERROR; }
    bool is_internal_error() const { return code_ == StatusCode::INTERNAL_ERROR; }
    
    // 获取状态码和消息
    StatusCode code() const { return code_; }
    const std::string& message() const { return message_; }
    
    // 转换为字符串
    std::string ToString() const;
    
private:
    StatusCode code_;
    std::string message_;
};

// 异常类
class DatabaseException : public std::exception {
public:
    explicit DatabaseException(const Status& status) : status_(status) {}
    explicit DatabaseException(const std::string& message) 
        : status_(StatusCode::INTERNAL_ERROR, message) {}
    
    const char* what() const noexcept override {
        return status_.message().c_str();
    }
    
    const Status& status() const { return status_; }
    
private:
    Status status_;
};

// 宏定义用于错误检查
#define RETURN_IF_ERROR(status) \
    do { \
        const Status& _status = (status); \
        if (!_status.ok()) { \
            return _status; \
        } \
    } while (0)

#define THROW_IF_ERROR(status) \
    do { \
        const Status& _status = (status); \
        if (!_status.ok()) { \
            throw DatabaseException(_status); \
        } \
    } while (0)

} // namespace minidb

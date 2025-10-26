#include "common/value.h"
#include <sstream>
#include <cctype>
#include <cmath>

namespace minidb {

// ============= 构造函数 =============

Value::Value() : type_(DataType::INT), is_null_(true), int_val_(0) {}

Value Value::make_int(int64_t val) {
    Value v;
    v.type_ = DataType::INT;
    v.is_null_ = false;
    v.int_val_ = val;
    return v;
}

Value Value::make_double(double val) {
    Value v;
    v.type_ = DataType::DECIMAL;
    v.is_null_ = false;
    v.double_val_ = val;
    return v;
}

Value Value::make_string(const std::string& val) {
    Value v;
    v.type_ = DataType::STRING;
    v.is_null_ = false;
    v.string_val_ = val;
    return v;
}

Value Value::make_bool(bool val) {
    Value v;
    v.type_ = DataType::BOOL;
    v.is_null_ = false;
    v.bool_val_ = val;
    return v;
}

Value Value::make_null() {
    Value v;
    v.is_null_ = true;
    return v;
}

// ============= 类型转换 =============

int64_t Value::as_int() const {
    if (is_null_) return 0;

    switch (type_) {
        case DataType::INT:
            return int_val_;
        case DataType::DECIMAL:
            return static_cast<int64_t>(double_val_);
        case DataType::STRING:
            return parse_int(string_val_);
        case DataType::BOOL:
            return bool_val_ ? 1 : 0;
        default:
            return 0;
    }
}

double Value::as_double() const {
    if (is_null_) return 0.0;

    switch (type_) {
        case DataType::INT:
            return static_cast<double>(int_val_);
        case DataType::DECIMAL:
            return double_val_;
        case DataType::STRING:
            return parse_double(string_val_);
        case DataType::BOOL:
            return bool_val_ ? 1.0 : 0.0;
        default:
            return 0.0;
    }
}

std::string Value::as_string() const {
    if (is_null_) return "";

    switch (type_) {
        case DataType::INT: {
            std::ostringstream oss;
            oss << int_val_;
            return oss.str();
        }
        case DataType::DECIMAL: {
            std::ostringstream oss;
            oss << double_val_;
            return oss.str();
        }
        case DataType::STRING:
            return string_val_;
        case DataType::BOOL:
            return bool_val_ ? "true" : "false";
        default:
            return "";
    }
}

bool Value::as_bool() const {
    if (is_null_) return false;

    switch (type_) {
        case DataType::INT:
            return int_val_ != 0;
        case DataType::DECIMAL:
            return double_val_ != 0.0;
        case DataType::STRING:
            return !string_val_.empty();
        case DataType::BOOL:
            return bool_val_;
        default:
            return false;
    }
}

// ============= 辅助方法：string 转数值 =============

int64_t Value::parse_int(const std::string& str) {
    if (str.empty()) return 0;

    // 跳过前导空格
    size_t i = 0;
    while (i < str.length() && std::isspace(str[i])) {
        i++;
    }
    if (i >= str.length()) return 0;

    // 处理符号
    bool negative = false;
    if (str[i] == '-') {
        negative = true;
        i++;
    } else if (str[i] == '+') {
        i++;
    }

    // 解析数字，遇到非数字停止
    int64_t result = 0;
    while (i < str.length() && std::isdigit(str[i])) {
        result = result * 10 + (str[i] - '0');
        i++;
    }

    return negative ? -result : result;
}

double Value::parse_double(const std::string& str) {
    if (str.empty()) return 0.0;

    // 跳过前导空格
    size_t i = 0;
    while (i < str.length() && std::isspace(str[i])) {
        i++;
    }
    if (i >= str.length()) return 0.0;

    // 处理符号
    bool negative = false;
    if (str[i] == '-') {
        negative = true;
        i++;
    } else if (str[i] == '+') {
        i++;
    }

    // 解析整数部分
    double result = 0.0;
    while (i < str.length() && std::isdigit(str[i])) {
        result = result * 10.0 + (str[i] - '0');
        i++;
    }

    // 解析小数部分
    if (i < str.length() && str[i] == '.') {
        i++;
        double fraction = 0.0;
        double divisor = 10.0;
        while (i < str.length() && std::isdigit(str[i])) {
            fraction += (str[i] - '0') / divisor;
            divisor *= 10.0;
            i++;
        }
        result += fraction;
    }

    return negative ? -result : result;
}

// ============= 比较运算符 =============

bool Value::operator==(const Value& other) const {
    // NULL 比较
    if (is_null_ && other.is_null_) return true;
    if (is_null_ || other.is_null_) return false;

    // 类型相同，直接比较
    if (type_ == other.type_) {
        switch (type_) {
            case DataType::INT:
                return int_val_ == other.int_val_;
            case DataType::DECIMAL:
                return std::abs(double_val_ - other.double_val_) < 1e-9;
            case DataType::STRING:
                return string_val_ == other.string_val_;
            case DataType::BOOL:
                return bool_val_ == other.bool_val_;
            default:
                return false;
        }
    }

    // 类型不同，数值类型之间可以比较
    if ((type_ == DataType::INT || type_ == DataType::DECIMAL) &&
        (other.type_ == DataType::INT || other.type_ == DataType::DECIMAL)) {
        return std::abs(as_double() - other.as_double()) < 1e-9;
    }

    return false;
}

bool Value::operator!=(const Value& other) const {
    return !(*this == other);
}

bool Value::operator<(const Value& other) const {
    // NULL 比较
    if (is_null_ || other.is_null_) return false;

    // 数值类型比较
    if ((type_ == DataType::INT || type_ == DataType::DECIMAL) &&
        (other.type_ == DataType::INT || other.type_ == DataType::DECIMAL)) {
        return as_double() < other.as_double();
    }

    // 字符串比较
    if (type_ == DataType::STRING && other.type_ == DataType::STRING) {
        return string_val_ < other.string_val_;
    }

    // 类型不兼容
    return false;
}

bool Value::operator<=(const Value& other) const {
    return *this < other || *this == other;
}

bool Value::operator>(const Value& other) const {
    return !(*this <= other);
}

bool Value::operator>=(const Value& other) const {
    return !(*this < other);
}

} // namespace minidb

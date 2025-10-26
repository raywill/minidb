#pragma once

#include "common/types.h"
#include <string>
#include <cstdint>

namespace minidb {

// 类型化的值容器，用于高效的表达式计算
// 使用 union 存储数值类型，避免装箱开销
class Value {
public:
    // 构造函数
    Value();

    // 静态工厂方法
    static Value make_int(int64_t val);
    static Value make_double(double val);
    static Value make_string(const std::string& val);
    static Value make_bool(bool val);
    static Value make_null();

    // 类型检查
    DataType get_type() const { return type_; }
    bool is_null() const { return is_null_; }
    bool is_int() const { return type_ == DataType::INT && !is_null_; }
    bool is_double() const { return type_ == DataType::DECIMAL && !is_null_; }
    bool is_string() const { return type_ == DataType::STRING && !is_null_; }
    bool is_bool() const { return type_ == DataType::BOOL && !is_null_; }

    // 类型转换（带隐式转换规则）
    int64_t as_int() const;
    double as_double() const;
    std::string as_string() const;
    bool as_bool() const;

    // 直接访问（不做转换，调用前需检查类型）
    int64_t get_int() const { return int_val_; }
    double get_double() const { return double_val_; }
    const std::string& get_string() const { return string_val_; }
    bool get_bool() const { return bool_val_; }

    // 比较运算符
    bool operator==(const Value& other) const;
    bool operator!=(const Value& other) const;
    bool operator<(const Value& other) const;
    bool operator<=(const Value& other) const;
    bool operator>(const Value& other) const;
    bool operator>=(const Value& other) const;

private:
    DataType type_;
    bool is_null_;

    // 使用 union 存储数值类型
    union {
        int64_t int_val_;
        double double_val_;
        bool bool_val_;
    };

    // string 单独存储（不能放 union）
    std::string string_val_;

    // 辅助方法：string 转数值（遇到非数字停止）
    static int64_t parse_int(const std::string& str);
    static double parse_double(const std::string& str);
};

} // namespace minidb

#pragma once

#include <string>
#include <vector>
#include <cstdint>

namespace minidb {

// 数据类型枚举
enum class DataType {
    INT,
    STRING,
    BOOL,
    DECIMAL
};

// JOIN类型枚举
enum class JoinType {
    INNER,
    LEFT_OUTER,
    RIGHT_OUTER,
    FULL_OUTER
};

// 将数据类型转换为字符串
std::string DataTypeToString(DataType type);

// 从字符串解析数据类型
DataType StringToDataType(const std::string& str);

// 获取数据类型的大小（字节）
size_t GetDataTypeSize(DataType type);

// 列向量数据结构
struct ColumnVector {
    std::string name;
    DataType type;
    std::vector<uint8_t> data;  // 连续内存块
    size_t size;                // 行数
    
    ColumnVector() : type(DataType::INT), size(0) {}
    ColumnVector(const std::string& n, DataType t) : name(n), type(t), size(0) {}
    
    // 清空数据
    void clear();
    
    // 预留空间
    void reserve(size_t capacity);
    
    // 添加数据
    void append_int(int32_t value);
    void append_string(const std::string& value);
    void append_bool(bool value);
    void append_decimal(double value);
    
    // 获取数据
    int32_t get_int(size_t index) const;
    std::string get_string(size_t index) const;
    bool get_bool(size_t index) const;
    double get_decimal(size_t index) const;
};

// 行数据结构
struct Row {
    std::vector<std::string> values;
    
    Row() = default;
    explicit Row(size_t size) : values(size) {}
};

// 表结构定义
struct TableSchema {
    std::string table_name;
    std::vector<std::string> column_names;
    std::vector<DataType> column_types;
    
    TableSchema() = default;
    TableSchema(const std::string& name) : table_name(name) {}
    
    void add_column(const std::string& name, DataType type);
    size_t get_column_count() const;
    int get_column_index(const std::string& name) const;
    DataType get_column_type(const std::string& name) const;
};

// 常量定义
const size_t DEFAULT_BATCH_SIZE = 1024;
const size_t MAX_STRING_LENGTH = 4096;

} // namespace minidb

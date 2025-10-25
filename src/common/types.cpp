#include "common/types.h"
#include <stdexcept>
#include <cstring>

namespace minidb {

std::string DataTypeToString(DataType type) {
    switch (type) {
        case DataType::INT: return "INT";
        case DataType::STRING: return "STRING";
        case DataType::BOOL: return "BOOL";
        case DataType::DECIMAL: return "DECIMAL";
        default: return "UNKNOWN";
    }
}

DataType StringToDataType(const std::string& str) {
    if (str == "INT") return DataType::INT;
    if (str == "STRING") return DataType::STRING;
    if (str == "BOOL") return DataType::BOOL;
    if (str == "DECIMAL") return DataType::DECIMAL;
    throw std::invalid_argument("Unknown data type: " + str);
}

size_t GetDataTypeSize(DataType type) {
    switch (type) {
        case DataType::INT: return sizeof(int32_t);
        case DataType::STRING: return sizeof(uint32_t); // 存储字符串长度
        case DataType::BOOL: return sizeof(bool);
        case DataType::DECIMAL: return sizeof(double);
        default: return 0;
    }
}

void ColumnVector::clear() {
    data.clear();
    size = 0;
}

void ColumnVector::reserve(size_t capacity) {
    size_t bytes_per_row = GetDataTypeSize(type);
    if (type == DataType::STRING) {
        // 字符串类型预留更多空间
        data.reserve(capacity * 64); // 平均64字节每个字符串
    } else {
        data.reserve(capacity * bytes_per_row);
    }
}

void ColumnVector::append_int(int32_t value) {
    if (type != DataType::INT) {
        throw std::invalid_argument("Type mismatch: expected INT");
    }
    const uint8_t* bytes = reinterpret_cast<const uint8_t*>(&value);
    data.insert(data.end(), bytes, bytes + sizeof(int32_t));
    size++;
}

void ColumnVector::append_string(const std::string& value) {
    if (type != DataType::STRING) {
        throw std::invalid_argument("Type mismatch: expected STRING");
    }
    if (value.length() > MAX_STRING_LENGTH) {
        throw std::invalid_argument("String too long");
    }
    
    // 存储字符串长度
    uint32_t length = static_cast<uint32_t>(value.length());
    const uint8_t* len_bytes = reinterpret_cast<const uint8_t*>(&length);
    data.insert(data.end(), len_bytes, len_bytes + sizeof(uint32_t));
    
    // 存储字符串内容
    const uint8_t* str_bytes = reinterpret_cast<const uint8_t*>(value.c_str());
    data.insert(data.end(), str_bytes, str_bytes + length);
    size++;
}

void ColumnVector::append_bool(bool value) {
    if (type != DataType::BOOL) {
        throw std::invalid_argument("Type mismatch: expected BOOL");
    }
    data.push_back(value ? 1 : 0);
    size++;
}

void ColumnVector::append_decimal(double value) {
    if (type != DataType::DECIMAL) {
        throw std::invalid_argument("Type mismatch: expected DECIMAL");
    }
    const uint8_t* bytes = reinterpret_cast<const uint8_t*>(&value);
    data.insert(data.end(), bytes, bytes + sizeof(double));
    size++;
}

int32_t ColumnVector::get_int(size_t index) const {
    if (type != DataType::INT || index >= size) {
        throw std::out_of_range("Invalid index or type mismatch");
    }
    const uint8_t* bytes = &data[index * sizeof(int32_t)];
    return *reinterpret_cast<const int32_t*>(bytes);
}

std::string ColumnVector::get_string(size_t index) const {
    if (type != DataType::STRING || index >= size) {
        throw std::out_of_range("Invalid index or type mismatch");
    }
    
    size_t offset = 0;
    for (size_t i = 0; i < index; i++) {
        uint32_t length = *reinterpret_cast<const uint32_t*>(&data[offset]);
        offset += sizeof(uint32_t) + length;
    }
    
    uint32_t length = *reinterpret_cast<const uint32_t*>(&data[offset]);
    offset += sizeof(uint32_t);
    
    return std::string(reinterpret_cast<const char*>(&data[offset]), length);
}

bool ColumnVector::get_bool(size_t index) const {
    if (type != DataType::BOOL || index >= size) {
        throw std::out_of_range("Invalid index or type mismatch");
    }
    return data[index] != 0;
}

double ColumnVector::get_decimal(size_t index) const {
    if (type != DataType::DECIMAL || index >= size) {
        throw std::out_of_range("Invalid index or type mismatch");
    }
    const uint8_t* bytes = &data[index * sizeof(double)];
    return *reinterpret_cast<const double*>(bytes);
}

void TableSchema::add_column(const std::string& name, DataType type) {
    column_names.push_back(name);
    column_types.push_back(type);
}

size_t TableSchema::get_column_count() const {
    return column_names.size();
}

int TableSchema::get_column_index(const std::string& name) const {
    for (size_t i = 0; i < column_names.size(); i++) {
        if (column_names[i] == name) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

DataType TableSchema::get_column_type(const std::string& name) const {
    int index = get_column_index(name);
    if (index < 0) {
        throw std::invalid_argument("Column not found: " + name);
    }
    return column_types[index];
}

} // namespace minidb

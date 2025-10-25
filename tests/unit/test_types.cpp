#include "common/types.h"
#include "common/status.h"
#include <iostream>
#include <cassert>

using namespace minidb;

void test_data_types() {
    std::cout << "Testing data types..." << std::endl;
    
    // 测试数据类型转换
    assert(DataTypeToString(DataType::INT) == "INT");
    assert(DataTypeToString(DataType::STRING) == "STRING");
    assert(DataTypeToString(DataType::BOOL) == "BOOL");
    assert(DataTypeToString(DataType::DECIMAL) == "DECIMAL");
    
    assert(StringToDataType("INT") == DataType::INT);
    assert(StringToDataType("STRING") == DataType::STRING);
    assert(StringToDataType("BOOL") == DataType::BOOL);
    assert(StringToDataType("DECIMAL") == DataType::DECIMAL);
    
    std::cout << "Data types test passed!" << std::endl;
}

void test_column_vector() {
    std::cout << "Testing column vector..." << std::endl;
    
    // 测试INT列
    ColumnVector int_col("id", DataType::INT);
    int_col.append_int(1);
    int_col.append_int(2);
    int_col.append_int(3);
    
    assert(int_col.size == 3);
    assert(int_col.get_int(0) == 1);
    assert(int_col.get_int(1) == 2);
    assert(int_col.get_int(2) == 3);
    
    // 测试STRING列
    ColumnVector str_col("name", DataType::STRING);
    str_col.append_string("Alice");
    str_col.append_string("Bob");
    
    assert(str_col.size == 2);
    assert(str_col.get_string(0) == "Alice");
    assert(str_col.get_string(1) == "Bob");
    
    // 测试BOOL列
    ColumnVector bool_col("active", DataType::BOOL);
    bool_col.append_bool(true);
    bool_col.append_bool(false);
    
    assert(bool_col.size == 2);
    assert(bool_col.get_bool(0) == true);
    assert(bool_col.get_bool(1) == false);
    
    // 测试DECIMAL列
    ColumnVector decimal_col("score", DataType::DECIMAL);
    decimal_col.append_decimal(95.5);
    decimal_col.append_decimal(87.2);
    
    assert(decimal_col.size == 2);
    assert(decimal_col.get_decimal(0) == 95.5);
    assert(decimal_col.get_decimal(1) == 87.2);
    
    std::cout << "Column vector test passed!" << std::endl;
}

void test_table_schema() {
    std::cout << "Testing table schema..." << std::endl;
    
    TableSchema schema("student");
    schema.add_column("id", DataType::INT);
    schema.add_column("name", DataType::STRING);
    schema.add_column("age", DataType::INT);
    schema.add_column("active", DataType::BOOL);
    
    assert(schema.get_column_count() == 4);
    assert(schema.get_column_index("id") == 0);
    assert(schema.get_column_index("name") == 1);
    assert(schema.get_column_index("age") == 2);
    assert(schema.get_column_index("active") == 3);
    assert(schema.get_column_index("nonexistent") == -1);
    
    assert(schema.get_column_type("id") == DataType::INT);
    assert(schema.get_column_type("name") == DataType::STRING);
    assert(schema.get_column_type("age") == DataType::INT);
    assert(schema.get_column_type("active") == DataType::BOOL);
    
    std::cout << "Table schema test passed!" << std::endl;
}

void test_status() {
    std::cout << "Testing status..." << std::endl;
    
    Status ok = Status::OK();
    assert(ok.ok());
    
    Status error = Status::InvalidArgument("Test error");
    assert(!error.ok());
    assert(error.is_invalid_argument());
    assert(error.message() == "Test error");
    
    std::string error_str = error.ToString();
    assert(error_str.find("INVALID_ARGUMENT") != std::string::npos);
    assert(error_str.find("Test error") != std::string::npos);
    
    std::cout << "Status test passed!" << std::endl;
}

int main() {
    try {
        test_data_types();
        test_column_vector();
        test_table_schema();
        test_status();
        
        std::cout << "All tests passed!" << std::endl;
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Test failed with exception: " << e.what() << std::endl;
        return 1;
    }
}

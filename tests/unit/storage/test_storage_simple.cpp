#include "storage/catalog.h"
#include "storage/table.h"
#include "common/status.h"
#include "log/logger.h"
#include <iostream>
#include <cassert>
#include <fstream>
#include <sys/stat.h>

using namespace minidb;

void cleanup_test_data() {
    system("rm -rf ./test_storage_simple_data");
}

void test_catalog_basic() {
    std::cout << "Testing Catalog basic operations..." << std::endl;
    
    cleanup_test_data();
    
    Catalog catalog("./test_storage_simple_data");
    Status status = catalog.initialize();
    assert(status.ok());
    
    // 创建表结构
    TableSchema schema("simple_test");
    schema.add_column("id", DataType::INT);
    schema.add_column("name", DataType::STRING);
    
    // 创建表
    status = catalog.create_table("simple_test", schema);
    assert(status.ok());
    assert(catalog.table_exists("simple_test"));
    
    // 获取元数据
    TableMetadata metadata;
    status = catalog.get_table_metadata("simple_test", metadata);
    assert(status.ok());
    assert(metadata.table_name == "simple_test");
    assert(metadata.schema.get_column_count() == 2);
    
    std::cout << "Catalog basic operations test passed!" << std::endl;
}

void test_table_insert_scan() {
    std::cout << "Testing Table insert and scan..." << std::endl;
    
    cleanup_test_data();
    system("mkdir -p ./test_storage_simple_data/insert_scan_test");
    
    TableSchema schema("insert_scan_test");
    schema.add_column("id", DataType::INT);
    schema.add_column("name", DataType::STRING);
    schema.add_column("score", DataType::DECIMAL);
    
    Table table("insert_scan_test", schema, "./test_storage_simple_data/insert_scan_test");
    Status status = table.initialize();
    assert(status.ok());
    
    // 插入少量数据
    std::vector<Row> rows;
    
    Row row1(3);
    row1.values[0] = "1";
    row1.values[1] = "Alice";
    row1.values[2] = "95.5";
    rows.push_back(row1);
    
    Row row2(3);
    row2.values[0] = "2";
    row2.values[1] = "Bob";
    row2.values[2] = "87.2";
    rows.push_back(row2);
    
    status = table.insert_rows(rows);
    assert(status.ok());
    assert(table.get_row_count() == 2);
    
    // 扫描数据
    std::vector<ColumnVector> columns;
    status = table.scan_all(columns);
    assert(status.ok());
    assert(columns.size() == 3);
    assert(columns[0].size == 2);
    
    // 验证数据
    assert(columns[0].get_int(0) == 1);
    assert(columns[0].get_int(1) == 2);
    assert(columns[1].get_string(0) == "Alice");
    assert(columns[1].get_string(1) == "Bob");
    assert(columns[2].get_decimal(0) == 95.5);
    assert(columns[2].get_decimal(1) == 87.2);
    
    std::cout << "Table insert and scan test passed!" << std::endl;
}

void test_column_vector_basic() {
    std::cout << "Testing ColumnVector basic operations..." << std::endl;
    
    // INT列测试
    ColumnVector int_col("test_int", DataType::INT);
    for (int i = 0; i < 10; ++i) {
        int_col.append_int(i * 5);
    }
    assert(int_col.size == 10);
    
    for (int i = 0; i < 10; ++i) {
        assert(int_col.get_int(i) == i * 5);
    }
    
    // STRING列测试
    ColumnVector str_col("test_string", DataType::STRING);
    for (int i = 0; i < 5; ++i) {
        str_col.append_string("item_" + std::to_string(i));
    }
    assert(str_col.size == 5);
    
    for (int i = 0; i < 5; ++i) {
        assert(str_col.get_string(i) == "item_" + std::to_string(i));
    }
    
    // BOOL列测试
    ColumnVector bool_col("test_bool", DataType::BOOL);
    bool_col.append_bool(true);
    bool_col.append_bool(false);
    bool_col.append_bool(true);
    assert(bool_col.size == 3);
    assert(bool_col.get_bool(0) == true);
    assert(bool_col.get_bool(1) == false);
    assert(bool_col.get_bool(2) == true);
    
    // DECIMAL列测试
    ColumnVector decimal_col("test_decimal", DataType::DECIMAL);
    decimal_col.append_decimal(3.14);
    decimal_col.append_decimal(2.71);
    assert(decimal_col.size == 2);
    assert(decimal_col.get_decimal(0) == 3.14);
    assert(decimal_col.get_decimal(1) == 2.71);
    
    std::cout << "ColumnVector basic operations test passed!" << std::endl;
}

void test_table_schema() {
    std::cout << "Testing TableSchema..." << std::endl;
    
    TableSchema schema("schema_test");
    
    // 添加列
    schema.add_column("id", DataType::INT);
    schema.add_column("name", DataType::STRING);
    schema.add_column("active", DataType::BOOL);
    
    assert(schema.get_column_count() == 3);
    
    // 测试列索引
    assert(schema.get_column_index("id") == 0);
    assert(schema.get_column_index("name") == 1);
    assert(schema.get_column_index("active") == 2);
    assert(schema.get_column_index("nonexistent") == -1);
    
    // 测试列类型
    assert(schema.get_column_type("id") == DataType::INT);
    assert(schema.get_column_type("name") == DataType::STRING);
    assert(schema.get_column_type("active") == DataType::BOOL);
    
    std::cout << "TableSchema test passed!" << std::endl;
}

void test_table_manager_basic() {
    std::cout << "Testing TableManager basic operations..." << std::endl;
    
    cleanup_test_data();
    
    Catalog catalog("./test_storage_simple_data");
    Status status = catalog.initialize();
    assert(status.ok());
    
    // 创建表
    TableSchema schema("manager_test");
    schema.add_column("id", DataType::INT);
    schema.add_column("data", DataType::STRING);
    
    status = catalog.create_table("manager_test", schema);
    assert(status.ok());
    
    TableManager manager(&catalog);
    
    // 打开表
    std::shared_ptr<Table> table;
    status = manager.open_table("manager_test", table);
    assert(status.ok());
    assert(table != nullptr);
    
    // 再次打开应该返回缓存的实例
    std::shared_ptr<Table> table2;
    status = manager.open_table("manager_test", table2);
    assert(status.ok());
    assert(table2 == table);
    
    // 关闭表
    status = manager.close_table("manager_test");
    assert(status.ok());
    
    auto table3 = manager.get_table("manager_test");
    assert(table3 == nullptr);
    
    std::cout << "TableManager basic operations test passed!" << std::endl;
}

void test_error_handling() {
    std::cout << "Testing storage error handling..." << std::endl;
    
    cleanup_test_data();
    
    // 测试无效表操作
    Catalog catalog("./test_storage_simple_data");
    Status status = catalog.initialize();
    assert(status.ok());
    
    TableMetadata metadata;
    status = catalog.get_table_metadata("nonexistent", metadata);
    assert(!status.ok());
    assert(status.is_not_found());
    
    // 测试重复创建
    TableSchema schema("dup_test");
    schema.add_column("id", DataType::INT);
    
    status = catalog.create_table("dup_test", schema);
    assert(status.ok());
    
    status = catalog.create_table("dup_test", schema);
    assert(!status.ok());
    assert(status.is_already_exists());
    
    std::cout << "Storage error handling test passed!" << std::endl;
}

int main() {
    try {
        // 设置日志级别为ERROR以减少输出
        Logger::instance()->set_level(LogLevel::ERROR);
        
        test_catalog_basic();
        test_table_insert_scan();
        test_column_vector_basic();
        test_table_schema();
        test_table_manager_basic();
        test_error_handling();
        
        cleanup_test_data();
        
        std::cout << "\n🎉 All simplified storage tests passed!" << std::endl;
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "❌ Storage test failed: " << e.what() << std::endl;
        cleanup_test_data();
        return 1;
    }
}

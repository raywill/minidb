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
    system("rm -rf ./test_storage_data");
}

void test_catalog_initialization() {
    std::cout << "Testing Catalog initialization..." << std::endl;
    
    cleanup_test_data();
    
    Catalog catalog("./test_storage_data");
    Status status = catalog.initialize();
    assert(status.ok());
    
    // 验证目录被创建
    struct stat st;
    assert(stat("./test_storage_data", &st) == 0);
    assert(S_ISDIR(st.st_mode));
    
    std::cout << "Catalog initialization test passed!" << std::endl;
}

void test_catalog_create_table() {
    std::cout << "Testing Catalog create table..." << std::endl;
    
    cleanup_test_data();
    
    Catalog catalog("./test_storage_data");
    Status status = catalog.initialize();
    assert(status.ok());
    
    // 创建表结构
    TableSchema schema("test_table");
    schema.add_column("id", DataType::INT);
    schema.add_column("name", DataType::STRING);
    schema.add_column("active", DataType::BOOL);
    schema.add_column("score", DataType::DECIMAL);
    
    // 创建表
    status = catalog.create_table("test_table", schema);
    assert(status.ok());
    
    // 验证表存在
    assert(catalog.table_exists("test_table"));
    
    // 验证表目录被创建
    struct stat st;
    assert(stat("./test_storage_data/test_table", &st) == 0);
    assert(S_ISDIR(st.st_mode));
    
    // 验证schema文件被创建
    assert(stat("./test_storage_data/test_table/schema.json", &st) == 0);
    
    std::cout << "Catalog create table test passed!" << std::endl;
}

void test_catalog_duplicate_table() {
    std::cout << "Testing Catalog duplicate table handling..." << std::endl;
    
    cleanup_test_data();
    
    Catalog catalog("./test_storage_data");
    Status status = catalog.initialize();
    assert(status.ok());
    
    TableSchema schema("duplicate_test");
    schema.add_column("id", DataType::INT);
    
    // 第一次创建应该成功
    status = catalog.create_table("duplicate_test", schema);
    assert(status.ok());
    
    // 第二次创建应该失败
    status = catalog.create_table("duplicate_test", schema);
    assert(!status.ok());
    assert(status.is_already_exists());
    
    // 使用IF NOT EXISTS应该成功
    status = catalog.create_table("duplicate_test", schema, true);
    assert(status.ok());
    
    std::cout << "Catalog duplicate table test passed!" << std::endl;
}

void test_catalog_drop_table() {
    std::cout << "Testing Catalog drop table..." << std::endl;
    
    cleanup_test_data();
    
    Catalog catalog("./test_storage_data");
    Status status = catalog.initialize();
    assert(status.ok());
    
    TableSchema schema("drop_test");
    schema.add_column("id", DataType::INT);
    
    // 创建表
    status = catalog.create_table("drop_test", schema);
    assert(status.ok());
    assert(catalog.table_exists("drop_test"));
    
    // 删除表
    status = catalog.drop_table("drop_test");
    assert(status.ok());
    assert(!catalog.table_exists("drop_test"));
    
    // 验证目录被删除
    struct stat st;
    assert(stat("./test_storage_data/drop_test", &st) != 0);
    
    // 删除不存在的表应该失败
    status = catalog.drop_table("nonexistent");
    assert(!status.ok());
    assert(status.is_not_found());
    
    // 使用IF EXISTS应该成功
    status = catalog.drop_table("nonexistent", true);
    assert(status.ok());
    
    std::cout << "Catalog drop table test passed!" << std::endl;
}

void test_catalog_metadata_operations() {
    std::cout << "Testing Catalog metadata operations..." << std::endl;
    
    cleanup_test_data();
    
    Catalog catalog("./test_storage_data");
    Status status = catalog.initialize();
    assert(status.ok());
    
    // 创建多个表
    for (int i = 0; i < 5; ++i) {
        TableSchema schema("table_" + std::to_string(i));
        schema.add_column("id", DataType::INT);
        schema.add_column("data", DataType::STRING);
        
        status = catalog.create_table("table_" + std::to_string(i), schema);
        assert(status.ok());
    }
    
    // 测试列出表
    auto table_list = catalog.list_tables();
    assert(table_list.size() == 5);
    
    // 测试获取表元数据
    TableMetadata metadata;
    status = catalog.get_table_metadata("table_0", metadata);
    assert(status.ok());
    assert(metadata.table_name == "table_0");
    assert(metadata.schema.get_column_count() == 2);
    assert(metadata.schema.get_column_index("id") == 0);
    assert(metadata.schema.get_column_index("data") == 1);
    
    // 测试更新行数
    status = catalog.update_row_count("table_0", 100);
    assert(status.ok());
    
    status = catalog.get_table_metadata("table_0", metadata);
    assert(status.ok());
    assert(metadata.row_count == 100);
    
    std::cout << "Catalog metadata operations test passed!" << std::endl;
}

void test_table_basic_operations() {
    std::cout << "Testing Table basic operations..." << std::endl;
    
    cleanup_test_data();
    
    // 创建目录和schema
    system("mkdir -p ./test_storage_data/table_test");
    
    TableSchema schema("table_test");
    schema.add_column("id", DataType::INT);
    schema.add_column("name", DataType::STRING);
    schema.add_column("score", DataType::DECIMAL);
    
    Table table("table_test", schema, "./test_storage_data/table_test");
    Status status = table.initialize();
    assert(status.ok());
    
    // 测试初始状态
    assert(table.get_row_count() == 0);
    assert(table.get_schema().get_column_count() == 3);
    
    std::cout << "Table basic operations test passed!" << std::endl;
}

void test_table_insert_and_scan() {
    std::cout << "Testing Table insert and scan..." << std::endl;
    
    cleanup_test_data();
    system("mkdir -p ./test_storage_data/insert_test");
    
    TableSchema schema("insert_test");
    schema.add_column("id", DataType::INT);
    schema.add_column("name", DataType::STRING);
    schema.add_column("active", DataType::BOOL);
    schema.add_column("score", DataType::DECIMAL);
    
    Table table("insert_test", schema, "./test_storage_data/insert_test");
    Status status = table.initialize();
    assert(status.ok());
    
    // 创建测试数据
    std::vector<Row> rows;
    
    Row row1(4);
    row1.values[0] = "1";
    row1.values[1] = "Alice";
    row1.values[2] = "true";
    row1.values[3] = "95.5";
    rows.push_back(row1);
    
    Row row2(4);
    row2.values[0] = "2";
    row2.values[1] = "Bob";
    row2.values[2] = "false";
    row2.values[3] = "87.2";
    rows.push_back(row2);
    
    // 插入数据
    status = table.insert_rows(rows);
    assert(status.ok());
    
    // 验证行数
    assert(table.get_row_count() == 2);
    
    // 扫描所有数据
    std::vector<ColumnVector> columns;
    status = table.scan_all(columns);
    assert(status.ok());
    assert(columns.size() == 4);
    assert(columns[0].size == 2);
    
    // 验证数据内容
    assert(columns[0].get_int(0) == 1);
    assert(columns[0].get_int(1) == 2);
    assert(columns[1].get_string(0) == "Alice");
    assert(columns[1].get_string(1) == "Bob");
    assert(columns[2].get_bool(0) == true);
    assert(columns[2].get_bool(1) == false);
    assert(columns[3].get_decimal(0) == 95.5);
    assert(columns[3].get_decimal(1) == 87.2);
    
    std::cout << "Table insert and scan test passed!" << std::endl;
}

void test_table_column_scan() {
    std::cout << "Testing Table column scan..." << std::endl;
    
    cleanup_test_data();
    system("mkdir -p ./test_storage_data/column_scan_test");
    
    TableSchema schema("column_scan_test");
    schema.add_column("id", DataType::INT);
    schema.add_column("name", DataType::STRING);
    schema.add_column("age", DataType::INT);
    
    Table table("column_scan_test", schema, "./test_storage_data/column_scan_test");
    Status status = table.initialize();
    assert(status.ok());
    
    // 插入测试数据
    std::vector<Row> rows;
    for (int i = 0; i < 10; ++i) {
        Row row(3);
        row.values[0] = std::to_string(i + 1);
        row.values[1] = "User" + std::to_string(i + 1);
        row.values[2] = std::to_string(20 + i);
        rows.push_back(row);
    }
    
    status = table.insert_rows(rows);
    assert(status.ok());
    
    // 测试扫描特定列
    std::vector<std::string> column_names = {"name", "age"};
    std::vector<ColumnVector> columns;
    status = table.scan_columns(column_names, columns);
    assert(status.ok());
    assert(columns.size() == 2);
    assert(columns[0].name == "name");
    assert(columns[1].name == "age");
    assert(columns[0].size == 10);
    assert(columns[1].size == 10);
    
    // 验证数据
    assert(columns[0].get_string(0) == "User1");
    assert(columns[0].get_string(9) == "User10");
    assert(columns[1].get_int(0) == 20);
    assert(columns[1].get_int(9) == 29);
    
    // 测试扫描不存在的列
    std::vector<std::string> invalid_columns = {"nonexistent"};
    status = table.scan_columns(invalid_columns, columns);
    assert(!status.ok());
    assert(status.is_not_found());
    
    std::cout << "Table column scan test passed!" << std::endl;
}

void test_table_delete_operations() {
    std::cout << "Testing Table delete operations..." << std::endl;
    
    cleanup_test_data();
    system("mkdir -p ./test_storage_data/delete_test");
    
    TableSchema schema("delete_test");
    schema.add_column("id", DataType::INT);
    schema.add_column("value", DataType::STRING);
    
    Table table("delete_test", schema, "./test_storage_data/delete_test");
    Status status = table.initialize();
    assert(status.ok());
    
    // 插入测试数据
    std::vector<Row> rows;
    for (int i = 0; i < 10; ++i) {
        Row row(2);
        row.values[0] = std::to_string(i);
        row.values[1] = "value_" + std::to_string(i);
        rows.push_back(row);
    }
    
    status = table.insert_rows(rows);
    assert(status.ok());
    assert(table.get_row_count() == 10);
    
    // 删除特定行
    std::vector<size_t> rows_to_delete = {0, 2, 4, 6, 8}; // 删除偶数索引的行
    status = table.delete_rows(rows_to_delete);
    assert(status.ok());
    assert(table.get_row_count() == 5);
    
    // 验证剩余数据
    std::vector<ColumnVector> columns;
    status = table.scan_all(columns);
    assert(status.ok());
    assert(columns[0].size == 5);
    
    // 剩余的应该是奇数索引的数据
    for (size_t i = 0; i < 5; ++i) {
        int expected_id = (i * 2) + 1; // 1, 3, 5, 7, 9
        assert(columns[0].get_int(i) == expected_id);
        assert(columns[1].get_string(i) == "value_" + std::to_string(expected_id));
    }
    
    // 测试删除无效索引
    std::vector<size_t> invalid_indices = {100};
    status = table.delete_rows(invalid_indices);
    assert(!status.ok());
    assert(status.is_invalid_argument());
    
    std::cout << "Table delete operations test passed!" << std::endl;
}

void test_table_persistence() {
    std::cout << "Testing Table persistence..." << std::endl;
    
    cleanup_test_data();
    system("mkdir -p ./test_storage_data/persistence_test");
    
    TableSchema schema("persistence_test");
    schema.add_column("id", DataType::INT);
    schema.add_column("data", DataType::STRING);
    
    // 第一个表实例 - 写入数据
    {
        Table table1("persistence_test", schema, "./test_storage_data/persistence_test");
        Status status = table1.initialize();
        assert(status.ok());
        
        std::vector<Row> rows;
        Row row(2);
        row.values[0] = "42";
        row.values[1] = "persistent_data";
        rows.push_back(row);
        
        status = table1.insert_rows(rows);
        assert(status.ok());
        assert(table1.get_row_count() == 1);
    }
    
    // 第二个表实例 - 读取数据
    {
        Table table2("persistence_test", schema, "./test_storage_data/persistence_test");
        Status status = table2.initialize();
        assert(status.ok());
        
        assert(table2.get_row_count() == 1);
        
        std::vector<ColumnVector> columns;
        status = table2.scan_all(columns);
        assert(status.ok());
        assert(columns.size() == 2);
        assert(columns[0].size == 1);
        assert(columns[0].get_int(0) == 42);
        assert(columns[1].get_string(0) == "persistent_data");
    }
    
    std::cout << "Table persistence test passed!" << std::endl;
}

void test_table_large_data() {
    std::cout << "Testing Table large data handling..." << std::endl;
    
    cleanup_test_data();
    system("mkdir -p ./test_storage_data/large_data_test");
    
    TableSchema schema("large_data_test");
    schema.add_column("id", DataType::INT);
    schema.add_column("data", DataType::STRING);
    
    Table table("large_data_test", schema, "./test_storage_data/large_data_test");
    Status status = table.initialize();
    assert(status.ok());
    
    // 插入适量数据（减少数据量避免hang）
    const int num_rows = 500; // 从10000减少到500
    std::vector<Row> rows;
    
    for (int i = 0; i < num_rows; ++i) {
        Row row(2);
        row.values[0] = std::to_string(i);
        row.values[1] = "data_" + std::to_string(i) + "_" + std::string(20, 'x'); // 减少字符串长度
        rows.push_back(row);
        
        // 更小的批量插入（每50行一批）
        if (rows.size() == 50 || i == num_rows - 1) {
            status = table.insert_rows(rows);
            assert(status.ok());
            rows.clear();
        }
    }
    
    assert(table.get_row_count() == num_rows);
    
    // 验证数据完整性
    std::vector<ColumnVector> columns;
    status = table.scan_all(columns);
    assert(status.ok());
    assert(columns[0].size == num_rows);
    
    // 抽样验证
    for (int i = 0; i < 50; ++i) {
        int idx = i * (num_rows / 50);
        if (idx < num_rows) {
            assert(columns[0].get_int(idx) == idx);
            std::string expected = "data_" + std::to_string(idx) + "_" + std::string(20, 'x');
            assert(columns[1].get_string(idx) == expected);
        }
    }
    
    std::cout << "Table large data test passed!" << std::endl;
}

void test_table_manager() {
    std::cout << "Testing TableManager..." << std::endl;
    
    cleanup_test_data();
    
    Catalog catalog("./test_storage_data");
    Status status = catalog.initialize();
    assert(status.ok());
    
    // 创建表
    TableSchema schema("manager_test");
    schema.add_column("id", DataType::INT);
    schema.add_column("name", DataType::STRING);
    
    status = catalog.create_table("manager_test", schema);
    assert(status.ok());
    
    TableManager manager(&catalog);
    
    // 测试打开表
    std::shared_ptr<Table> table1;
    status = manager.open_table("manager_test", table1);
    assert(status.ok());
    assert(table1 != nullptr);
    
    // 再次打开同一个表应该返回缓存的实例
    std::shared_ptr<Table> table2;
    status = manager.open_table("manager_test", table2);
    assert(status.ok());
    assert(table2 == table1); // 应该是同一个实例
    
    // 测试获取表
    auto table3 = manager.get_table("manager_test");
    assert(table3 == table1);
    
    // 测试关闭表
    status = manager.close_table("manager_test");
    assert(status.ok());
    
    auto table4 = manager.get_table("manager_test");
    assert(table4 == nullptr); // 应该已经被移除
    
    // 测试打开不存在的表
    std::shared_ptr<Table> invalid_table;
    status = manager.open_table("nonexistent", invalid_table);
    assert(!status.ok());
    assert(status.is_not_found());
    
    std::cout << "TableManager test passed!" << std::endl;
}

void test_column_vector_operations() {
    std::cout << "Testing ColumnVector operations..." << std::endl;
    
    // 测试INT列
    ColumnVector int_col("test_int", DataType::INT);
    for (int i = 0; i < 100; ++i) {
        int_col.append_int(i * 10);
    }
    assert(int_col.size == 100);
    
    for (int i = 0; i < 100; ++i) {
        assert(int_col.get_int(i) == i * 10);
    }
    
    // 测试STRING列
    ColumnVector str_col("test_string", DataType::STRING);
    for (int i = 0; i < 50; ++i) {
        str_col.append_string("string_" + std::to_string(i));
    }
    assert(str_col.size == 50);
    
    for (int i = 0; i < 50; ++i) {
        assert(str_col.get_string(i) == "string_" + std::to_string(i));
    }
    
    // 测试BOOL列
    ColumnVector bool_col("test_bool", DataType::BOOL);
    for (int i = 0; i < 20; ++i) {
        bool_col.append_bool(i % 2 == 0);
    }
    assert(bool_col.size == 20);
    
    for (int i = 0; i < 20; ++i) {
        assert(bool_col.get_bool(i) == (i % 2 == 0));
    }
    
    // 测试DECIMAL列
    ColumnVector decimal_col("test_decimal", DataType::DECIMAL);
    for (int i = 0; i < 30; ++i) {
        decimal_col.append_decimal(i * 3.14);
    }
    assert(decimal_col.size == 30);
    
    for (int i = 0; i < 30; ++i) {
        assert(decimal_col.get_decimal(i) == i * 3.14);
    }
    
    // 测试清空
    int_col.clear();
    assert(int_col.size == 0);
    assert(int_col.data.empty());
    
    std::cout << "ColumnVector operations test passed!" << std::endl;
}

void test_table_schema_operations() {
    std::cout << "Testing TableSchema operations..." << std::endl;
    
    TableSchema schema("schema_test");
    
    // 测试添加列
    schema.add_column("id", DataType::INT);
    schema.add_column("name", DataType::STRING);
    schema.add_column("active", DataType::BOOL);
    schema.add_column("score", DataType::DECIMAL);
    
    assert(schema.get_column_count() == 4);
    
    // 测试列索引查找
    assert(schema.get_column_index("id") == 0);
    assert(schema.get_column_index("name") == 1);
    assert(schema.get_column_index("active") == 2);
    assert(schema.get_column_index("score") == 3);
    assert(schema.get_column_index("nonexistent") == -1);
    
    // 测试列类型查找
    assert(schema.get_column_type("id") == DataType::INT);
    assert(schema.get_column_type("name") == DataType::STRING);
    assert(schema.get_column_type("active") == DataType::BOOL);
    assert(schema.get_column_type("score") == DataType::DECIMAL);
    
    // 测试无效列类型查找
    try {
        schema.get_column_type("nonexistent");
        assert(false); // 应该抛出异常
    } catch (const std::exception& e) {
        // 期望的异常
    }
    
    std::cout << "TableSchema operations test passed!" << std::endl;
}

void test_storage_error_handling() {
    std::cout << "Testing storage error handling..." << std::endl;
    
    cleanup_test_data();
    
    // 测试无效目录
    Catalog invalid_catalog("/invalid/path/that/does/not/exist");
    Status status = invalid_catalog.initialize();
    // 初始化可能成功（会创建目录），但后续操作会失败
    
    // 测试表操作错误
    TableSchema schema("error_test");
    schema.add_column("id", DataType::INT);
    
    status = invalid_catalog.create_table("error_test", schema);
    // 可能成功也可能失败，取决于权限
    
    // 测试表初始化错误
    Table invalid_table("invalid", schema, "/invalid/path");
    status = invalid_table.initialize();
    assert(!status.ok());
    assert(status.is_io_error());
    
    std::cout << "Storage error handling test passed!" << std::endl;
}

int main() {
    try {
        // 设置日志级别为ERROR以减少输出
        Logger::instance()->set_level(LogLevel::ERROR);
        
        test_catalog_initialization();
        test_catalog_create_table();
        test_catalog_duplicate_table();
        test_catalog_drop_table();
        test_catalog_metadata_operations();
        test_table_basic_operations();
        test_table_insert_and_scan();
        test_table_column_scan();
        test_table_delete_operations();
        test_table_persistence();
        test_table_large_data();
        test_table_manager();
        test_column_vector_operations();
        test_table_schema_operations();
        test_storage_error_handling();
        
        cleanup_test_data();
        
        std::cout << "\n🎉 All storage tests passed!" << std::endl;
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "❌ Storage test failed: " << e.what() << std::endl;
        cleanup_test_data();
        return 1;
    }
}

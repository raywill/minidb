#include "exec/executor/executor.h"
#include "storage/catalog.h"
#include "storage/table.h"
#include "sql/parser/parser.h"
#include "log/logger.h"
#include <iostream>
#include <cassert>

using namespace minidb;

// 创建测试目录
std::unique_ptr<Catalog> setup_test_catalog() {
    system("rm -rf ./test_where_simple_data");
    system("mkdir -p ./test_where_simple_data");
    
    auto catalog = std::unique_ptr<Catalog>(new Catalog("./test_where_simple_data"));
    Status status = catalog->initialize();
    assert(status.ok());
    
    return catalog;
}

void test_where_basic_filtering() {
    std::cout << "Testing WHERE basic filtering..." << std::endl;
    
    auto catalog = setup_test_catalog();
    TableManager table_manager(catalog.get());
    Executor executor(catalog.get(), &table_manager);
    
    // 创建表
    {
        Parser parser("CREATE TABLE simple_filter(id INT, num INT);");
        std::unique_ptr<Statement> stmt;
        Status status = parser.parse(stmt);
        assert(status.ok());
        QueryResult result = executor.execute_statement(stmt.get());
        assert(result.success);
    }
    
    // 插入数据
    {
        Parser parser("INSERT INTO simple_filter VALUES (1, 10), (2, 20), (3, 30);");
        std::unique_ptr<Statement> stmt;
        Status status = parser.parse(stmt);
        assert(status.ok());
        QueryResult result = executor.execute_statement(stmt.get());
        assert(result.success);
    }
    
    // 测试无WHERE子句
    {
        Parser parser("SELECT * FROM simple_filter;");
        std::unique_ptr<Statement> stmt;
        Status status = parser.parse(stmt);
        assert(status.ok());
        QueryResult result = executor.execute_statement(stmt.get());
        assert(result.success);
        assert(result.result_text.find("1 | 10") != std::string::npos);
        assert(result.result_text.find("2 | 20") != std::string::npos);
        assert(result.result_text.find("3 | 30") != std::string::npos);
    }
    
    // 测试WHERE id = 2
    {
        Parser parser("SELECT * FROM simple_filter WHERE id = 2;");
        std::unique_ptr<Statement> stmt;
        Status status = parser.parse(stmt);
        assert(status.ok());
        QueryResult result = executor.execute_statement(stmt.get());
        assert(result.success);
        assert(result.result_text.find("2 | 20") != std::string::npos);
        assert(result.result_text.find("1 | 10") == std::string::npos);
        assert(result.result_text.find("3 | 30") == std::string::npos);
    }
    
    // 测试WHERE num > 15
    {
        Parser parser("SELECT * FROM simple_filter WHERE num > 15;");
        std::unique_ptr<Statement> stmt;
        Status status = parser.parse(stmt);
        assert(status.ok());
        QueryResult result = executor.execute_statement(stmt.get());
        assert(result.success);
        assert(result.result_text.find("2 | 20") != std::string::npos);
        assert(result.result_text.find("3 | 30") != std::string::npos);
        assert(result.result_text.find("1 | 10") == std::string::npos);
    }
    
    // 测试WHERE无匹配
    {
        Parser parser("SELECT * FROM simple_filter WHERE id = 999;");
        std::unique_ptr<Statement> stmt;
        Status status = parser.parse(stmt);
        assert(status.ok());
        QueryResult result = executor.execute_statement(stmt.get());
        assert(result.success);
        // 应该只有表头
        assert(result.result_text.find("ID | NUM") != std::string::npos);
        assert(result.result_text.find("999") == std::string::npos);
    }
    
    std::cout << "WHERE basic filtering test passed!" << std::endl;
}

void test_where_different_operators() {
    std::cout << "Testing WHERE different operators..." << std::endl;
    
    auto catalog = setup_test_catalog();
    TableManager table_manager(catalog.get());
    Executor executor(catalog.get(), &table_manager);
    
    // 创建表
    {
        Parser parser("CREATE TABLE op_test(id INT, val INT);");
        std::unique_ptr<Statement> stmt;
        Status status = parser.parse(stmt);
        assert(status.ok());
        QueryResult result = executor.execute_statement(stmt.get());
        assert(result.success);
    }
    
    // 插入数据
    {
        Parser parser("INSERT INTO op_test VALUES (1, 50), (2, 100), (3, 150);");
        std::unique_ptr<Statement> stmt;
        Status status = parser.parse(stmt);
        assert(status.ok());
        QueryResult result = executor.execute_statement(stmt.get());
        assert(result.success);
    }
    
    // 测试 < 操作符
    {
        Parser parser("SELECT * FROM op_test WHERE val < 75;");
        std::unique_ptr<Statement> stmt;
        Status status = parser.parse(stmt);
        assert(status.ok());
        QueryResult result = executor.execute_statement(stmt.get());
        assert(result.success);
        assert(result.result_text.find("1 | 50") != std::string::npos);
        assert(result.result_text.find("2 | 100") == std::string::npos);
    }
    
    // 测试 >= 操作符
    {
        Parser parser("SELECT * FROM op_test WHERE val >= 100;");
        std::unique_ptr<Statement> stmt;
        Status status = parser.parse(stmt);
        assert(status.ok());
        QueryResult result = executor.execute_statement(stmt.get());
        assert(result.success);
        assert(result.result_text.find("2 | 100") != std::string::npos);
        assert(result.result_text.find("3 | 150") != std::string::npos);
        assert(result.result_text.find("1 | 50") == std::string::npos);
    }
    
    std::cout << "WHERE different operators test passed!" << std::endl;
}

void test_where_string_comparison() {
    std::cout << "Testing WHERE string comparison..." << std::endl;
    
    auto catalog = setup_test_catalog();
    TableManager table_manager(catalog.get());
    Executor executor(catalog.get(), &table_manager);
    
    // 创建表
    {
        Parser parser("CREATE TABLE str_test(id INT, name STRING);");
        std::unique_ptr<Statement> stmt;
        Status status = parser.parse(stmt);
        assert(status.ok());
        QueryResult result = executor.execute_statement(stmt.get());
        assert(result.success);
    }
    
    // 插入数据
    {
        Parser parser("INSERT INTO str_test VALUES (1, 'Apple'), (2, 'Banana'), (3, 'Cherry');");
        std::unique_ptr<Statement> stmt;
        Status status = parser.parse(stmt);
        assert(status.ok());
        QueryResult result = executor.execute_statement(stmt.get());
        assert(result.success);
    }
    
    // 测试字符串等值比较
    {
        Parser parser("SELECT * FROM str_test WHERE name = 'Banana';");
        std::unique_ptr<Statement> stmt;
        Status status = parser.parse(stmt);
        assert(status.ok());
        QueryResult result = executor.execute_statement(stmt.get());
        assert(result.success);
        assert(result.result_text.find("2 | Banana") != std::string::npos);
        assert(result.result_text.find("Apple") == std::string::npos);
        assert(result.result_text.find("Cherry") == std::string::npos);
    }
    
    std::cout << "WHERE string comparison test passed!" << std::endl;
}

int main() {
    try {
        // 设置日志级别
        Logger::instance()->set_level(LogLevel::ERROR);
        
        test_where_basic_filtering();
        test_where_different_operators();
        test_where_string_comparison();
        
        // 清理
        system("rm -rf ./test_where_simple_data");
        
        std::cout << "\n🎉 All WHERE clause simple tests passed!" << std::endl;
        std::cout << "✅ WHERE子句过滤功能已修复并正常工作！" << std::endl;
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "❌ WHERE clause test failed: " << e.what() << std::endl;
        return 1;
    }
}

#include "server/database_server.h"
#include "client/cli_client.h"
#include <iostream>
#include <thread>
#include <chrono>
#include <cassert>

using namespace minidb;

void test_database_operations() {
    std::cout << "Testing database operations..." << std::endl;
    
    // 启动服务器
    DatabaseServer server("./test_data", 9877);
    Status status = server.start();
    assert(status.ok());
    
    // 等待服务器启动
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    // 连接客户端
    CLIClient client;
    status = client.connect("localhost", 9877);
    assert(status.ok());
    
    std::string result;
    
    // 测试CREATE TABLE
    status = client.execute_sql("CREATE TABLE test_table(id INT, name STRING, score DECIMAL);", result);
    assert(status.ok());
    std::cout << "CREATE TABLE result: " << result << std::endl;
    
    // 测试INSERT
    status = client.execute_sql("INSERT INTO test_table VALUES (1, 'Alice', 95.5), (2, 'Bob', 87.2), (3, 'Charlie', 92.0);", result);
    assert(status.ok());
    std::cout << "INSERT result: " << result << std::endl;
    
    // 测试SELECT *
    status = client.execute_sql("SELECT * FROM test_table;", result);
    assert(status.ok());
    std::cout << "SELECT * result:\n" << result << std::endl;
    
    // 测试SELECT with WHERE
    status = client.execute_sql("SELECT name, score FROM test_table WHERE score > 90;", result);
    assert(status.ok());
    std::cout << "SELECT with WHERE result:\n" << result << std::endl;
    
    // 测试函数
    status = client.execute_sql("SELECT name, sin(score) FROM test_table WHERE id <= 2;", result);
    assert(status.ok());
    std::cout << "SELECT with function result:\n" << result << std::endl;
    
    // 测试DELETE
    status = client.execute_sql("DELETE FROM test_table WHERE score < 90;", result);
    assert(status.ok());
    std::cout << "DELETE result: " << result << std::endl;
    
    // 验证DELETE效果
    status = client.execute_sql("SELECT * FROM test_table;", result);
    assert(status.ok());
    std::cout << "SELECT after DELETE result:\n" << result << std::endl;
    
    // 测试DROP TABLE
    status = client.execute_sql("DROP TABLE test_table;", result);
    assert(status.ok());
    std::cout << "DROP TABLE result: " << result << std::endl;
    
    // 断开客户端
    client.disconnect();
    
    // 停止服务器
    server.stop();
    
    std::cout << "All database operations test passed!" << std::endl;
}

void test_error_handling() {
    std::cout << "Testing error handling..." << std::endl;
    
    // 启动服务器
    DatabaseServer server("./test_data", 9878);
    Status status = server.start();
    assert(status.ok());
    
    // 等待服务器启动
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    // 连接客户端
    CLIClient client;
    status = client.connect("localhost", 9878);
    assert(status.ok());
    
    std::string result;
    
    // 测试语法错误
    status = client.execute_sql("INVALID SQL SYNTAX;", result);
    assert(!status.ok() || result.find("ERROR") != std::string::npos);
    std::cout << "Syntax error result: " << result << std::endl;
    
    // 测试表不存在错误
    status = client.execute_sql("SELECT * FROM nonexistent_table;", result);
    assert(!status.ok() || result.find("ERROR") != std::string::npos);
    std::cout << "Table not found result: " << result << std::endl;
    
    // 测试重复创建表
    client.execute_sql("CREATE TABLE duplicate_test(id INT);", result);
    status = client.execute_sql("CREATE TABLE duplicate_test(id INT);", result);
    assert(!status.ok() || result.find("ERROR") != std::string::npos);
    std::cout << "Duplicate table result: " << result << std::endl;
    
    // 清理
    client.execute_sql("DROP TABLE IF EXISTS duplicate_test;", result);
    
    // 断开客户端
    client.disconnect();
    
    // 停止服务器
    server.stop();
    
    std::cout << "Error handling test passed!" << std::endl;
}

int main() {
    try {
        // 清理测试数据目录
        system("rm -rf ./test_data");
        
        test_database_operations();
        test_error_handling();
        
        // 清理测试数据目录
        system("rm -rf ./test_data");
        
        std::cout << "All integration tests passed!" << std::endl;
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Integration test failed with exception: " << e.what() << std::endl;
        return 1;
    }
}

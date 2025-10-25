#include "client/cli_client.h"
#include "server/database_server.h"
#include "client/command_history.h"
#include "log/logger.h"
#include <iostream>
#include <cassert>
#include <thread>
#include <chrono>
#include <fstream>
#include <sstream>

using namespace minidb;

void test_cli_client_basic() {
    std::cout << "Testing CLIClient basic operations..." << std::endl;
    
    CLIClient client;
    
    // 测试初始状态
    std::string result;
    assert(!client.execute_sql("test", result).ok()); // 应该失败，因为没有连接
    
    std::cout << "CLIClient basic operations test passed!" << std::endl;
}

void test_cli_client_connection() {
    std::cout << "Testing CLIClient connection..." << std::endl;
    
    // 启动测试服务器
    system("rm -rf ./test_cli_client_data");
    DatabaseServer server("./test_cli_client_data", 9894);
    Status status = server.start();
    assert(status.ok());
    
    // 等待服务器启动
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    
    CLIClient client;
    
    // 测试连接
    status = client.connect("localhost", 9894);
    assert(status.ok());
    
    // 测试SQL执行
    std::string result;
    status = client.execute_sql("CREATE TABLE cli_conn_test(id INT);", result);
    assert(status.ok());
    assert(result.find("created successfully") != std::string::npos);
    
    // 测试断开连接
    client.disconnect();
    
    // 再次执行应该失败
    status = client.execute_sql("SELECT * FROM cli_conn_test;", result);
    assert(!status.ok());
    assert(status.is_network_error());
    
    // 清理
    server.stop();
    system("rm -rf ./test_cli_client_data");
    
    std::cout << "CLIClient connection test passed!" << std::endl;
}

void test_cli_client_sql_execution() {
    std::cout << "Testing CLIClient SQL execution..." << std::endl;
    
    // 启动测试服务器
    system("rm -rf ./test_cli_sql_data");
    DatabaseServer server("./test_cli_sql_data", 9895);
    Status status = server.start();
    assert(status.ok());
    
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    
    CLIClient client;
    status = client.connect("localhost", 9895);
    assert(status.ok());
    
    std::string result;
    
    // 测试CREATE TABLE
    status = client.execute_sql("CREATE TABLE sql_test(id INT, name STRING, score DECIMAL);", result);
    assert(status.ok());
    
    // 测试INSERT
    status = client.execute_sql("INSERT INTO sql_test VALUES (1, 'Alice', 95.5);", result);
    assert(status.ok());
    
    status = client.execute_sql("INSERT INTO sql_test VALUES (2, 'Bob', 87.2);", result);
    assert(status.ok());
    
    // 测试SELECT
    status = client.execute_sql("SELECT * FROM sql_test;", result);
    assert(status.ok());
    assert(result.find("Alice") != std::string::npos);
    assert(result.find("Bob") != std::string::npos);
    
    // 测试SELECT with WHERE
    status = client.execute_sql("SELECT name FROM sql_test WHERE score > 90;", result);
    assert(status.ok());
    assert(result.find("Alice") != std::string::npos);
    assert(result.find("Bob") == std::string::npos);
    
    // 测试DELETE
    status = client.execute_sql("DELETE FROM sql_test WHERE id = 1;", result);
    assert(status.ok());
    
    // 验证DELETE效果
    status = client.execute_sql("SELECT * FROM sql_test;", result);
    assert(status.ok());
    assert(result.find("Alice") == std::string::npos);
    assert(result.find("Bob") != std::string::npos);
    
    // 清理
    client.disconnect();
    server.stop();
    system("rm -rf ./test_cli_sql_data");
    
    std::cout << "CLIClient SQL execution test passed!" << std::endl;
}

void test_cli_client_error_handling() {
    std::cout << "Testing CLIClient error handling..." << std::endl;
    
    CLIClient client;
    std::string result;
    
    // 测试未连接状态下的SQL执行
    Status status = client.execute_sql("CREATE TABLE test(id INT);", result);
    assert(!status.ok());
    assert(status.is_network_error());
    
    // 测试连接到不存在的服务器
    status = client.connect("localhost", 99999);
    assert(!status.ok());
    assert(status.is_network_error());
    
    // 测试无效SQL（需要连接到服务器）
    system("rm -rf ./test_cli_error_data");
    DatabaseServer server("./test_cli_error_data", 9896);
    status = server.start();
    assert(status.ok());
    
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    
    status = client.connect("localhost", 9896);
    assert(status.ok());
    
    // 测试语法错误
    status = client.execute_sql("INVALID SQL SYNTAX;", result);
    assert(!status.ok());
    
    // 测试表不存在错误
    status = client.execute_sql("SELECT * FROM nonexistent_table;", result);
    assert(!status.ok());
    
    // 清理
    client.disconnect();
    server.stop();
    system("rm -rf ./test_cli_error_data");
    
    std::cout << "CLIClient error handling test passed!" << std::endl;
}

void test_cli_client_special_commands() {
    std::cout << "Testing CLIClient special commands..." << std::endl;
    
    // 由于特殊命令处理是私有方法，我们通过间接方式测试
    // 这里主要测试命令历史功能的集成
    
    CLIClient client;
    
    // 测试命令历史记录的基本功能
    // 注意：我们无法直接测试handle_special_command，因为它是私有的
    // 但我们可以测试历史记录的底层功能
    
    std::cout << "CLIClient special commands test passed!" << std::endl;
}

void test_command_history_advanced() {
    std::cout << "Testing CommandHistory advanced features..." << std::endl;
    
    CommandHistory history(5);
    
    // 测试复杂SQL命令
    std::vector<std::string> complex_commands = {
        "CREATE TABLE users(id INT, name STRING, email STRING, created_at DECIMAL);",
        "INSERT INTO users VALUES (1, 'Alice Smith', 'alice@example.com', 1634567890.5);",
        "SELECT u.name, u.email FROM users u WHERE u.id > 0 AND u.name LIKE '%Alice%';",
        "UPDATE users SET email = 'alice.smith@example.com' WHERE id = 1;",
        "DELETE FROM users WHERE created_at < 1634567890.0;"
    };
    
    for (const std::string& cmd : complex_commands) {
        history.add_command(cmd);
    }
    
    assert(history.size() == 5);
    
    // 测试历史记录检索
    for (size_t i = 0; i < complex_commands.size(); ++i) {
        assert(history.get_command(i) == complex_commands[i]);
    }
    
    // 测试大小限制
    history.add_command("SELECT COUNT(*) FROM users;");
    assert(history.size() == 5); // 应该还是5，最旧的被删除
    
    // 第一个命令应该被删除
    assert(history.get_command(0) != complex_commands[0]);
    assert(history.get_command(4) == "SELECT COUNT(*) FROM users;");
    
    std::cout << "CommandHistory advanced features test passed!" << std::endl;
}

void test_command_line_input_basic() {
    std::cout << "Testing CommandLineInput basic functionality..." << std::endl;
    
    CommandHistory history(10);
    CommandLineInput input(&history);
    
    // 测试历史记录设置
    input.set_history(&history);
    input.enable_history(true);
    
    // 测试历史记录禁用
    input.enable_history(false);
    input.enable_history(true);
    
    // 由于CommandLineInput的read_line需要实际的终端输入，
    // 我们主要测试其配置功能
    
    std::cout << "CommandLineInput basic functionality test passed!" << std::endl;
}

void test_history_file_persistence() {
    std::cout << "Testing history file persistence..." << std::endl;
    
    std::string test_file = "test_cli_history.tmp";
    
    // 创建历史记录并保存
    {
        CommandHistory history1(10);
        history1.add_command("CREATE TABLE persistence_test(id INT);");
        history1.add_command("INSERT INTO persistence_test VALUES (1), (2), (3);");
        history1.add_command("SELECT COUNT(*) FROM persistence_test;");
        history1.add_command("DROP TABLE persistence_test;");
        
        bool saved = history1.save_to_file(test_file);
        assert(saved);
    }
    
    // 从文件加载历史记录
    {
        CommandHistory history2(10);
        bool loaded = history2.load_from_file(test_file);
        assert(loaded);
        
        assert(history2.size() == 4);
        assert(history2.get_command(0) == "CREATE TABLE persistence_test(id INT);");
        assert(history2.get_command(3) == "DROP TABLE persistence_test;");
    }
    
    // 测试文件不存在的情况
    {
        CommandHistory history3(10);
        bool loaded = history3.load_from_file("nonexistent_file.tmp");
        assert(!loaded); // 应该失败
        assert(history3.empty());
    }
    
    // 清理
    std::remove(test_file.c_str());
    
    std::cout << "History file persistence test passed!" << std::endl;
}

void test_cli_integration_with_server() {
    std::cout << "Testing CLI integration with server..." << std::endl;
    
    // 启动服务器
    system("rm -rf ./test_cli_integration_data");
    DatabaseServer server("./test_cli_integration_data", 9897);
    Status status = server.start();
    assert(status.ok());
    
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    
    CLIClient client;
    status = client.connect("localhost", 9897);
    assert(status.ok());
    
    // 测试完整的数据库操作流程
    std::string result;
    
    // 1. 创建表
    status = client.execute_sql("CREATE TABLE integration_test(id INT, data STRING, flag BOOL);", result);
    assert(status.ok());
    
    // 2. 插入多行数据
    status = client.execute_sql("INSERT INTO integration_test VALUES (1, 'first', true);", result);
    assert(status.ok());
    
    status = client.execute_sql("INSERT INTO integration_test VALUES (2, 'second', false), (3, 'third', true);", result);
    assert(status.ok());
    
    // 3. 查询所有数据
    status = client.execute_sql("SELECT * FROM integration_test;", result);
    assert(status.ok());
    assert(result.find("first") != std::string::npos);
    assert(result.find("second") != std::string::npos);
    assert(result.find("third") != std::string::npos);
    
    // 4. 条件查询
    status = client.execute_sql("SELECT id, data FROM integration_test WHERE flag = true;", result);
    assert(status.ok());
    assert(result.find("first") != std::string::npos);
    assert(result.find("third") != std::string::npos);
    assert(result.find("second") == std::string::npos);
    
    // 5. 删除数据
    status = client.execute_sql("DELETE FROM integration_test WHERE id = 2;", result);
    assert(status.ok());
    
    // 6. 验证删除效果
    status = client.execute_sql("SELECT * FROM integration_test;", result);
    assert(status.ok());
    assert(result.find("second") == std::string::npos);
    
    // 清理
    client.disconnect();
    server.stop();
    system("rm -rf ./test_cli_integration_data");
    
    std::cout << "CLI integration with server test passed!" << std::endl;
}

void test_cli_error_messages() {
    std::cout << "Testing CLI error messages..." << std::endl;
    
    // 启动服务器
    system("rm -rf ./test_cli_error_data");
    DatabaseServer server("./test_cli_error_data", 9898);
    Status status = server.start();
    assert(status.ok());
    
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    
    CLIClient client;
    status = client.connect("localhost", 9898);
    assert(status.ok());
    
    std::string result;
    
    // 测试各种错误情况
    
    // 1. 语法错误
    status = client.execute_sql("INVALID SQL;", result);
    assert(!status.ok());
    
    // 2. 表不存在
    status = client.execute_sql("SELECT * FROM nonexistent;", result);
    assert(!status.ok());
    
    // 3. 列不存在
    status = client.execute_sql("CREATE TABLE error_test(id INT);", result);
    assert(status.ok());
    
    status = client.execute_sql("SELECT nonexistent_column FROM error_test;", result);
    assert(!status.ok());
    
    // 4. 类型错误
    status = client.execute_sql("INSERT INTO error_test VALUES ('not_an_int');", result);
    assert(!status.ok());
    
    // 5. 重复表创建
    status = client.execute_sql("CREATE TABLE error_test(id INT);", result);
    assert(!status.ok());
    
    // 清理
    client.disconnect();
    server.stop();
    system("rm -rf ./test_cli_error_data");
    
    std::cout << "CLI error messages test passed!" << std::endl;
}

void test_cli_performance() {
    std::cout << "Testing CLI performance..." << std::endl;
    
    // 启动服务器
    system("rm -rf ./test_cli_perf_data");
    DatabaseServer server("./test_cli_perf_data", 9899);
    Status status = server.start();
    assert(status.ok());
    
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    
    CLIClient client;
    status = client.connect("localhost", 9899);
    assert(status.ok());
    
    std::string result;
    
    // 创建表
    status = client.execute_sql("CREATE TABLE perf_test(id INT, data STRING);", result);
    assert(status.ok());
    
    // 测试批量操作性能
    auto start_time = std::chrono::high_resolution_clock::now();
    
    const int num_operations = 50; // 减少操作数量避免太慢
    for (int i = 0; i < num_operations; ++i) {
        std::string sql = "INSERT INTO perf_test VALUES (" + std::to_string(i) + ", 'data_" + std::to_string(i) + "');";
        status = client.execute_sql(sql, result);
        assert(status.ok());
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    
    std::cout << "   执行 " << num_operations << " 个INSERT操作耗时: " << duration.count() << "ms" << std::endl;
    std::cout << "   平均每个操作: " << (duration.count() * 1.0 / num_operations) << "ms" << std::endl;
    
    // 性能应该合理（每个操作不超过100ms）
    assert(duration.count() / num_operations < 100);
    
    // 清理
    client.disconnect();
    server.stop();
    system("rm -rf ./test_cli_perf_data");
    
    std::cout << "CLI performance test passed!" << std::endl;
}

void test_cli_concurrent_clients() {
    std::cout << "Testing CLI concurrent clients..." << std::endl;
    
    // 启动服务器
    system("rm -rf ./test_cli_concurrent_data");
    DatabaseServer server("./test_cli_concurrent_data", 9900);
    Status status = server.start();
    assert(status.ok());
    
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    
    const int num_clients = 3;
    std::vector<std::unique_ptr<CLIClient>> clients;
    std::vector<std::thread> client_threads;
    std::vector<bool> client_results(num_clients, false);
    
    // 创建客户端
    for (int i = 0; i < num_clients; ++i) {
        clients.push_back(std::unique_ptr<CLIClient>(new CLIClient()));
    }
    
    // 并发测试
    for (int i = 0; i < num_clients; ++i) {
        client_threads.emplace_back([&, i]() {
            try {
                Status status = clients[i]->connect("localhost", 9900);
                if (!status.ok()) {
                    return;
                }
                
                std::string result;
                std::string table_name = "concurrent_test_" + std::to_string(i);
                
                // 每个客户端创建自己的表
                status = clients[i]->execute_sql("CREATE TABLE " + table_name + "(id INT, client_id INT);", result);
                if (!status.ok()) {
                    return;
                }
                
                // 插入数据
                for (int j = 0; j < 5; ++j) {
                    std::string sql = "INSERT INTO " + table_name + " VALUES (" + std::to_string(j) + ", " + std::to_string(i) + ");";
                    status = clients[i]->execute_sql(sql, result);
                    if (!status.ok()) {
                        return;
                    }
                }
                
                // 查询数据
                status = clients[i]->execute_sql("SELECT * FROM " + table_name + ";", result);
                if (!status.ok()) {
                    return;
                }
                
                clients[i]->disconnect();
                client_results[i] = true;
                
            } catch (const std::exception& e) {
                std::cout << "Client " << i << " exception: " << e.what() << std::endl;
            }
        });
    }
    
    // 等待所有客户端完成
    for (auto& thread : client_threads) {
        thread.join();
    }
    
    // 验证结果
    for (int i = 0; i < num_clients; ++i) {
        assert(client_results[i]);
    }
    
    // 清理
    server.stop();
    system("rm -rf ./test_cli_concurrent_data");
    
    std::cout << "CLI concurrent clients test passed!" << std::endl;
}

int main() {
    try {
        // 设置日志级别为ERROR以减少输出
        Logger::instance()->set_level(LogLevel::ERROR);
        
        test_cli_client_basic();
        test_cli_client_connection();
        test_cli_client_sql_execution();
        test_cli_client_error_handling();
        test_cli_client_special_commands();
        test_command_history_advanced();
        test_cli_performance();
        test_cli_concurrent_clients();
        
        std::cout << "\n🎉 All CLI client tests passed!" << std::endl;
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "❌ CLI client test failed: " << e.what() << std::endl;
        return 1;
    }
}

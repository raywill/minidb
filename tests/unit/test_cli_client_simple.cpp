#include "client/cli_client.h"
#include "server/database_server.h"
#include "client/command_history.h"
#include "log/logger.h"
#include <iostream>
#include <cassert>
#include <thread>
#include <chrono>
#include <fstream>

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
    system("rm -rf ./test_cli_simple_data");
    DatabaseServer server("./test_cli_simple_data", 9901);
    Status status = server.start();
    assert(status.ok());
    
    // 等待服务器启动
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    
    CLIClient client;
    
    // 测试连接
    status = client.connect("localhost", 9901);
    assert(status.ok());
    
    // 测试简单SQL执行
    std::string result;
    status = client.execute_sql("CREATE TABLE cli_simple_test(id INT);", result);
    assert(status.ok());
    assert(result.find("created successfully") != std::string::npos);
    
    // 测试断开连接
    client.disconnect();
    
    // 再次执行应该失败
    status = client.execute_sql("SELECT * FROM cli_simple_test;", result);
    assert(!status.ok());
    assert(status.is_network_error());
    
    // 清理
    server.stop();
    system("rm -rf ./test_cli_simple_data");
    
    std::cout << "CLIClient connection test passed!" << std::endl;
}

void test_cli_client_basic_sql() {
    std::cout << "Testing CLIClient basic SQL..." << std::endl;
    
    // 启动测试服务器
    system("rm -rf ./test_cli_basic_sql_data");
    DatabaseServer server("./test_cli_basic_sql_data", 9902);
    Status status = server.start();
    assert(status.ok());
    
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    
    CLIClient client;
    status = client.connect("localhost", 9902);
    assert(status.ok());
    
    std::string result;
    
    // 测试CREATE TABLE
    status = client.execute_sql("CREATE TABLE basic_test(id INT, name STRING);", result);
    assert(status.ok());
    
    // 测试INSERT
    status = client.execute_sql("INSERT INTO basic_test VALUES (1, 'Alice');", result);
    assert(status.ok());
    
    status = client.execute_sql("INSERT INTO basic_test VALUES (2, 'Bob');", result);
    assert(status.ok());
    
    // 测试简单SELECT（避免WHERE子句）
    status = client.execute_sql("SELECT * FROM basic_test;", result);
    assert(status.ok());
    assert(result.find("Alice") != std::string::npos);
    assert(result.find("Bob") != std::string::npos);
    
    // 测试列选择
    status = client.execute_sql("SELECT name FROM basic_test;", result);
    assert(status.ok());
    assert(result.find("Alice") != std::string::npos);
    assert(result.find("Bob") != std::string::npos);
    
    // 清理
    client.disconnect();
    server.stop();
    system("rm -rf ./test_cli_basic_sql_data");
    
    std::cout << "CLIClient basic SQL test passed!" << std::endl;
}

void test_command_history_integration() {
    std::cout << "Testing CommandHistory integration..." << std::endl;
    
    CommandHistory history(5);
    
    // 测试基本历史记录功能
    history.add_command("CREATE TABLE hist_test(id INT);");
    history.add_command("INSERT INTO hist_test VALUES (1);");
    history.add_command("SELECT * FROM hist_test;");
    
    assert(history.size() == 3);
    assert(history.get_command(0) == "CREATE TABLE hist_test(id INT);");
    assert(history.get_last_command() == "SELECT * FROM hist_test;");
    
    // 测试去重
    history.add_command("SELECT * FROM hist_test;"); // 重复，应该被忽略
    assert(history.size() == 3);
    
    history.add_command("DROP TABLE hist_test;"); // 新命令
    assert(history.size() == 4);
    
    // 测试文件操作
    std::string test_file = "test_history_integration.tmp";
    bool saved = history.save_to_file(test_file);
    assert(saved);
    
    CommandHistory history2(5);
    bool loaded = history2.load_from_file(test_file);
    assert(loaded);
    assert(history2.size() == 4);
    
    // 清理
    std::remove(test_file.c_str());
    
    std::cout << "CommandHistory integration test passed!" << std::endl;
}

void test_cli_error_handling() {
    std::cout << "Testing CLI error handling..." << std::endl;
    
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
    
    std::cout << "CLI error handling test passed!" << std::endl;
}

void test_command_line_input_config() {
    std::cout << "Testing CommandLineInput configuration..." << std::endl;
    
    CommandHistory history(10);
    CommandLineInput input(&history);
    
    // 测试历史记录设置
    input.set_history(&history);
    input.enable_history(true);
    input.enable_history(false);
    input.enable_history(true);
    
    // 测试历史记录功能（不需要实际输入）
    history.add_command("test command 1");
    history.add_command("test command 2");
    
    assert(history.size() == 2);
    
    std::cout << "CommandLineInput configuration test passed!" << std::endl;
}

void test_cli_special_commands_logic() {
    std::cout << "Testing CLI special commands logic..." << std::endl;
    
    // 测试CommandHistory的特殊功能
    CommandHistory history(3);
    
    // 测试空命令过滤
    history.add_command("");
    history.add_command("   ");
    history.add_command("\t\n");
    assert(history.empty());
    
    // 测试正常命令
    history.add_command("CREATE TABLE test(id INT);");
    history.add_command("INSERT INTO test VALUES (1);");
    assert(history.size() == 2);
    
    // 测试大小限制
    history.add_command("SELECT * FROM test;");
    history.add_command("DELETE FROM test WHERE id = 1;");
    assert(history.size() == 3);
    
    history.add_command("DROP TABLE test;"); // 应该删除最旧的
    assert(history.size() == 3);
    assert(history.get_command(0) != "CREATE TABLE test(id INT);"); // 最旧的应该被删除
    
    std::cout << "CLI special commands logic test passed!" << std::endl;
}

int main() {
    try {
        // 设置日志级别为ERROR以减少输出
        Logger::instance()->set_level(LogLevel::ERROR);
        
        test_cli_client_basic();
        test_cli_client_connection();
        test_cli_client_basic_sql();
        test_command_history_integration();
        test_cli_error_handling();
        test_command_line_input_config();
        test_cli_special_commands_logic();
        
        std::cout << "\n🎉 All CLI client simple tests passed!" << std::endl;
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "❌ CLI client test failed: " << e.what() << std::endl;
        return 1;
    }
}

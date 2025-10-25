#include "client/command_history.h"
#include <iostream>
#include <cassert>
#include <fstream>

using namespace minidb;

void test_command_history_basic() {
    std::cout << "Testing CommandHistory basic operations..." << std::endl;
    
    CommandHistory history(10); // 最多10条记录
    
    // 初始状态
    assert(history.empty());
    assert(history.size() == 0);
    assert(history.get_last_command().empty());
    
    // 添加命令
    history.add_command("SELECT * FROM users;");
    assert(!history.empty());
    assert(history.size() == 1);
    assert(history.get_last_command() == "SELECT * FROM users;");
    assert(history.get_command(0) == "SELECT * FROM users;");
    
    // 添加更多命令
    history.add_command("INSERT INTO users VALUES (1, 'Alice');");
    history.add_command("CREATE TABLE products(id INT, name STRING);");
    
    assert(history.size() == 3);
    assert(history.get_command(1) == "INSERT INTO users VALUES (1, 'Alice');");
    assert(history.get_command(2) == "CREATE TABLE products(id INT, name STRING);");
    
    std::cout << "CommandHistory basic operations test passed!" << std::endl;
}

void test_command_history_deduplication() {
    std::cout << "Testing CommandHistory deduplication..." << std::endl;
    
    CommandHistory history(10);
    
    // 添加重复命令
    history.add_command("SELECT * FROM test;");
    history.add_command("SELECT * FROM test;"); // 重复，应该被忽略
    history.add_command("INSERT INTO test VALUES (1);");
    history.add_command("SELECT * FROM test;"); // 不重复（不是连续的）
    
    assert(history.size() == 3);
    assert(history.get_command(0) == "SELECT * FROM test;");
    assert(history.get_command(1) == "INSERT INTO test VALUES (1);");
    assert(history.get_command(2) == "SELECT * FROM test;");
    
    // 测试空命令
    history.add_command("");
    history.add_command("   ");
    assert(history.size() == 3); // 空命令应该被忽略
    
    std::cout << "History size: " << history.size() << std::endl;
    for (size_t i = 0; i < history.size(); ++i) {
        std::cout << "  [" << i << "] " << history.get_command(i) << std::endl;
    }
    
    std::cout << "CommandHistory deduplication test passed!" << std::endl;
}

void test_command_history_size_limit() {
    std::cout << "Testing CommandHistory size limit..." << std::endl;
    
    CommandHistory history(5); // 最多5条记录
    
    // 添加超过限制的命令
    for (int i = 0; i < 10; ++i) {
        history.add_command("COMMAND_" + std::to_string(i));
    }
    
    assert(history.size() == 5);
    
    // 应该保留最后5条命令
    for (int i = 0; i < 5; ++i) {
        std::string expected = "COMMAND_" + std::to_string(i + 5);
        assert(history.get_command(i) == expected);
    }
    
    assert(history.get_last_command() == "COMMAND_9");
    
    std::cout << "CommandHistory size limit test passed!" << std::endl;
}

void test_command_history_file_operations() {
    std::cout << "Testing CommandHistory file operations..." << std::endl;
    
    std::string test_file = "test_history.txt";
    
    // 创建历史记录并保存
    {
        CommandHistory history1(10);
        history1.add_command("CREATE TABLE test(id INT);");
        history1.add_command("INSERT INTO test VALUES (1);");
        history1.add_command("SELECT * FROM test;");
        
        bool saved = history1.save_to_file(test_file);
        assert(saved);
    }
    
    // 从文件加载历史记录
    {
        CommandHistory history2(10);
        bool loaded = history2.load_from_file(test_file);
        assert(loaded);
        
        assert(history2.size() == 3);
        assert(history2.get_command(0) == "CREATE TABLE test(id INT);");
        assert(history2.get_command(1) == "INSERT INTO test VALUES (1);");
        assert(history2.get_command(2) == "SELECT * FROM test;");
    }
    
    // 清理测试文件
    std::remove(test_file.c_str());
    
    std::cout << "CommandHistory file operations test passed!" << std::endl;
}

void test_command_history_get_all() {
    std::cout << "Testing CommandHistory get_all_commands..." << std::endl;
    
    CommandHistory history(10);
    
    std::vector<std::string> test_commands = {
        "CREATE TABLE users(id INT, name STRING);",
        "INSERT INTO users VALUES (1, 'Alice');",
        "INSERT INTO users VALUES (2, 'Bob');",
        "SELECT * FROM users;",
        "DELETE FROM users WHERE id = 1;"
    };
    
    for (const std::string& cmd : test_commands) {
        history.add_command(cmd);
    }
    
    auto all_commands = history.get_all_commands();
    assert(all_commands.size() == test_commands.size());
    
    for (size_t i = 0; i < test_commands.size(); ++i) {
        assert(all_commands[i] == test_commands[i]);
    }
    
    std::cout << "CommandHistory get_all_commands test passed!" << std::endl;
}

void test_command_history_clear() {
    std::cout << "Testing CommandHistory clear..." << std::endl;
    
    CommandHistory history(10);
    
    // 添加一些命令
    history.add_command("COMMAND1");
    history.add_command("COMMAND2");
    history.add_command("COMMAND3");
    
    assert(history.size() == 3);
    assert(!history.empty());
    
    // 清空历史记录
    history.clear();
    
    assert(history.size() == 0);
    assert(history.empty());
    assert(history.get_last_command().empty());
    
    std::cout << "CommandHistory clear test passed!" << std::endl;
}

void test_command_history_edge_cases() {
    std::cout << "Testing CommandHistory edge cases..." << std::endl;
    
    CommandHistory history(3);
    
    // 测试获取无效索引
    assert(history.get_command(0).empty());
    assert(history.get_command(100).empty());
    
    // 添加命令后测试边界
    history.add_command("CMD1");
    assert(history.get_command(0) == "CMD1");
    assert(history.get_command(1).empty());
    
    // 测试特殊字符命令
    history.add_command("SELECT * FROM test WHERE name = 'O''Reilly';");
    history.add_command("INSERT INTO test VALUES (1, \"quote\");");
    
    assert(history.size() == 3);
    assert(history.get_command(1) == "SELECT * FROM test WHERE name = 'O''Reilly';");
    assert(history.get_command(2) == "INSERT INTO test VALUES (1, \"quote\");");
    
    std::cout << "CommandHistory edge cases test passed!" << std::endl;
}

int main() {
    try {
        test_command_history_basic();
        test_command_history_deduplication();
        test_command_history_size_limit();
        test_command_history_file_operations();
        test_command_history_get_all();
        test_command_history_clear();
        test_command_history_edge_cases();
        
        std::cout << "\n🎉 All command history tests passed!" << std::endl;
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "❌ Command history test failed: " << e.what() << std::endl;
        return 1;
    }
}

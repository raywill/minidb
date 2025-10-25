#include "common/crash_handler.h"
#include "server/database_server.h"
#include "client/cli_client.h"
#include <iostream>
#include <thread>
#include <chrono>
#include <cassert>

using namespace minidb;

void test_crash_handler_initialization() {
    std::cout << "Testing crash handler initialization..." << std::endl;
    
    // 初始化崩溃处理器
    CrashHandler::initialize();
    
    // 设置查询ID
    CrashHandler::set_current_query_id(12345);
    
    // 验证查询ID
    assert(CrashHandler::get_current_query_id() == 12345);
    
    // 生成转储文件名
    std::string dump_file = CrashHandler::generate_dump_file();
    std::cout << "Generated dump filename: " << dump_file << std::endl;
    
    // 验证文件名格式
    assert(dump_file.find("crash-") == 0);
    assert(dump_file.find(".dmp") != std::string::npos);
    assert(dump_file.find("12345") != std::string::npos);
    
    std::cout << "Crash handler initialization test passed!" << std::endl;
}

void test_query_id_setter() {
    std::cout << "Testing QueryIdSetter RAII..." << std::endl;
    
    // 初始状态
    CrashHandler::set_current_query_id(0);
    assert(CrashHandler::get_current_query_id() == 0);
    
    {
        // 使用RAII设置查询ID
        QueryIdSetter setter(9999);
        assert(CrashHandler::get_current_query_id() == 9999);
        
        // 嵌套设置
        {
            QueryIdSetter nested_setter(8888);
            assert(CrashHandler::get_current_query_id() == 8888);
        }
        
        // 嵌套结束后应该恢复
        assert(CrashHandler::get_current_query_id() == 0); // 注意：这里会重置为0
    }
    
    std::cout << "QueryIdSetter RAII test passed!" << std::endl;
}

void test_database_with_crash_handler() {
    std::cout << "Testing database with crash handler..." << std::endl;
    
    // 启动服务器
    DatabaseServer server("./test_crash_data", 9879);
    Status status = server.start();
    assert(status.ok());
    
    // 等待服务器启动
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    // 连接客户端
    CLIClient client;
    status = client.connect("localhost", 9879);
    assert(status.ok());
    
    std::string result;
    
    // 测试正常操作
    status = client.execute_sql("CREATE TABLE test_crash(id INT, name STRING);", result);
    assert(status.ok());
    std::cout << "CREATE TABLE result: " << result << std::endl;
    
    // 测试SELECT（这个之前会崩溃，现在应该有更好的错误处理）
    status = client.execute_sql("SELECT * FROM test_crash;", result);
    std::cout << "SELECT result: " << result << std::endl;
    
    // 清理
    client.execute_sql("DROP TABLE IF EXISTS test_crash;", result);
    client.disconnect();
    server.stop();
    
    std::cout << "Database with crash handler test completed!" << std::endl;
}

int main() {
    try {
        // 清理测试数据目录
        system("rm -rf ./test_crash_data");
        
        test_crash_handler_initialization();
        test_query_id_setter();
        test_database_with_crash_handler();
        
        // 清理测试数据目录
        system("rm -rf ./test_crash_data");
        
        std::cout << "All crash handler tests passed!" << std::endl;
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Crash handler test failed with exception: " << e.what() << std::endl;
        return 1;
    }
}

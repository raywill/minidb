#include "common/crash_handler.h"
#include "log/logger.h"
#include <iostream>
#include <cassert>
#include <fstream>
#include <thread>
#include <vector>
#include <signal.h>
#include <unistd.h>

using namespace minidb;

void test_crash_handler_initialization() {
    std::cout << "Testing CrashHandler initialization..." << std::endl;
    
    // 多次初始化应该是安全的
    CrashHandler::initialize();
    CrashHandler::initialize();
    CrashHandler::initialize();
    
    std::cout << "CrashHandler initialization test passed!" << std::endl;
}

void test_query_id_management() {
    std::cout << "Testing query ID management..." << std::endl;
    
    CrashHandler::initialize();
    
    // 测试基本设置和获取
    assert(CrashHandler::get_current_query_id() == 0);
    
    CrashHandler::set_current_query_id(12345);
    assert(CrashHandler::get_current_query_id() == 12345);
    
    CrashHandler::set_current_query_id(67890);
    assert(CrashHandler::get_current_query_id() == 67890);
    
    CrashHandler::set_current_query_id(0);
    assert(CrashHandler::get_current_query_id() == 0);
    
    std::cout << "Query ID management test passed!" << std::endl;
}

void test_query_id_setter_raii() {
    std::cout << "Testing QueryIdSetter RAII..." << std::endl;
    
    CrashHandler::initialize();
    
    // 初始状态
    CrashHandler::set_current_query_id(0);
    assert(CrashHandler::get_current_query_id() == 0);
    
    {
        QueryIdSetter setter(11111);
        assert(CrashHandler::get_current_query_id() == 11111);
        
        {
            QueryIdSetter nested_setter(22222);
            assert(CrashHandler::get_current_query_id() == 22222);
        }
        
        // 嵌套结束后应该重置为0（因为析构函数设置为0）
        assert(CrashHandler::get_current_query_id() == 0);
    }
    
    assert(CrashHandler::get_current_query_id() == 0);
    
    std::cout << "QueryIdSetter RAII test passed!" << std::endl;
}

void test_dump_file_generation() {
    std::cout << "Testing dump file generation..." << std::endl;
    
    CrashHandler::initialize();
    
    // 设置不同的查询ID并生成转储文件名
    std::vector<size_t> query_ids = {1, 100, 999, 12345, 0};
    
    for (size_t query_id : query_ids) {
        CrashHandler::set_current_query_id(query_id);
        
        std::string dump_file = CrashHandler::generate_dump_file();
        
        // 验证文件名格式
        assert(dump_file.find("crash-") == 0);
        assert(dump_file.find(".dmp") != std::string::npos);
        assert(dump_file.find(std::to_string(query_id)) != std::string::npos);
        
        // 验证包含进程ID
        pid_t pid = getpid();
        assert(dump_file.find(std::to_string(pid)) != std::string::npos);
        
        std::cout << "Query ID " << query_id << " -> " << dump_file << std::endl;
    }
    
    std::cout << "Dump file generation test passed!" << std::endl;
}

void test_thread_local_query_id() {
    std::cout << "Testing thread-local query ID..." << std::endl;
    
    CrashHandler::initialize();
    
    const int num_threads = 4;
    std::vector<std::thread> threads;
    std::vector<bool> thread_results(num_threads, false);
    
    // 每个线程设置不同的查询ID
    for (int t = 0; t < num_threads; ++t) {
        threads.emplace_back([&, t]() {
            try {
                size_t thread_query_id = 1000 + t;
                
                CrashHandler::set_current_query_id(thread_query_id);
                
                // 验证当前线程的查询ID
                assert(CrashHandler::get_current_query_id() == thread_query_id);
                
                // 生成转储文件名
                std::string dump_file = CrashHandler::generate_dump_file();
                assert(dump_file.find(std::to_string(thread_query_id)) != std::string::npos);
                
                // 等待一段时间，确保其他线程也在运行
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                
                // 再次验证查询ID没有被其他线程影响
                assert(CrashHandler::get_current_query_id() == thread_query_id);
                
                thread_results[t] = true;
                
            } catch (const std::exception& e) {
                std::cout << "Thread " << t << " exception: " << e.what() << std::endl;
            }
        });
    }
    
    // 等待所有线程完成
    for (auto& thread : threads) {
        thread.join();
    }
    
    // 验证所有线程都成功
    for (int t = 0; t < num_threads; ++t) {
        assert(thread_results[t]);
    }
    
    std::cout << "Thread-local query ID test passed!" << std::endl;
}

void test_crash_handler_with_query_execution() {
    std::cout << "Testing CrashHandler with query execution..." << std::endl;
    
    CrashHandler::initialize();
    
    // 模拟查询执行过程
    {
        SET_QUERY_ID(55555);
        assert(CrashHandler::get_current_query_id() == 55555);
        
        // 生成转储文件名
        std::string dump_file = CrashHandler::generate_dump_file();
        assert(dump_file.find("55555") != std::string::npos);
        
        {
            SET_QUERY_ID(66666);
            assert(CrashHandler::get_current_query_id() == 66666);
        }
        
        // 嵌套结束后应该重置
        assert(CrashHandler::get_current_query_id() == 0);
    }
    
    assert(CrashHandler::get_current_query_id() == 0);
    
    std::cout << "CrashHandler with query execution test passed!" << std::endl;
}

void test_crash_dump_file_format() {
    std::cout << "Testing crash dump file format..." << std::endl;
    
    CrashHandler::initialize();
    
    // 设置查询ID
    CrashHandler::set_current_query_id(77777);
    
    // 生成转储文件名
    std::string dump_filename = CrashHandler::generate_dump_file();
    
    // 验证文件名组件
    assert(dump_filename.find("crash-") == 0);
    assert(dump_filename.find(".dmp") == dump_filename.length() - 4);
    
    // 提取组件
    std::string prefix = "crash-";
    std::string suffix = ".dmp";
    std::string middle = dump_filename.substr(prefix.length(), 
                                            dump_filename.length() - prefix.length() - suffix.length());
    
    // 中间部分应该包含进程ID、线程ID和查询ID
    assert(middle.find("-") != std::string::npos); // 至少有两个分隔符
    assert(middle.find("77777") != std::string::npos);
    
    std::cout << "Crash dump file format test passed!" << std::endl;
}

void test_crash_handler_edge_cases() {
    std::cout << "Testing CrashHandler edge cases..." << std::endl;
    
    CrashHandler::initialize();
    
    // 测试极大的查询ID
    size_t large_query_id = SIZE_MAX;
    CrashHandler::set_current_query_id(large_query_id);
    assert(CrashHandler::get_current_query_id() == large_query_id);
    
    std::string dump_file = CrashHandler::generate_dump_file();
    assert(dump_file.find(std::to_string(large_query_id)) != std::string::npos);
    
    // 测试快速连续的查询ID设置
    for (size_t i = 0; i < 10000; ++i) {
        CrashHandler::set_current_query_id(i);
        assert(CrashHandler::get_current_query_id() == i);
    }
    
    std::cout << "CrashHandler edge cases test passed!" << std::endl;
}

void test_crash_handler_performance() {
    std::cout << "Testing CrashHandler performance..." << std::endl;
    
    CrashHandler::initialize();
    
    const int num_operations = 100000;
    
    auto start_time = std::chrono::high_resolution_clock::now();
    
    // 大量查询ID设置操作
    for (int i = 0; i < num_operations; ++i) {
        CrashHandler::set_current_query_id(i);
        size_t retrieved_id = CrashHandler::get_current_query_id();
        assert(retrieved_id == static_cast<size_t>(i));
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
    
    std::cout << "Performed " << num_operations << " query ID operations in " 
              << duration.count() << " microseconds" << std::endl;
    std::cout << "Average: " << (duration.count() * 1.0 / num_operations) 
              << " microseconds per operation" << std::endl;
    
    // 性能应该很好（每个操作应该在几微秒内）
    assert(duration.count() / num_operations < 10); // 平均每个操作少于10微秒
    
    std::cout << "CrashHandler performance test passed!" << std::endl;
}

int main() {
    try {
        // 设置日志级别
        Logger::instance()->set_level(LogLevel::ERROR);
        
        test_crash_handler_initialization();
        test_query_id_management();
        test_query_id_setter_raii();
        test_dump_file_generation();
        test_thread_local_query_id();
        test_crash_handler_with_query_execution();
        test_crash_dump_file_format();
        test_crash_handler_edge_cases();
        test_crash_handler_performance();
        
        std::cout << "\n🎉 All crash handler tests passed!" << std::endl;
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "❌ Crash handler test failed: " << e.what() << std::endl;
        return 1;
    }
}

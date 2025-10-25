#include "log/logger.h"
#include <iostream>
#include <cassert>
#include <fstream>
#include <thread>
#include <vector>
#include <chrono>
#include <sstream>

using namespace minidb;

void test_log_levels() {
    std::cout << "Testing log levels..." << std::endl;
    
    // 测试日志级别转换
    assert(LogLevelToString(LogLevel::DEBUG) == "DEBUG");
    assert(LogLevelToString(LogLevel::INFO) == "INFO");
    assert(LogLevelToString(LogLevel::WARN) == "WARN");
    assert(LogLevelToString(LogLevel::ERROR) == "ERROR");
    assert(LogLevelToString(LogLevel::FATAL) == "FATAL");
    
    assert(StringToLogLevel("DEBUG") == LogLevel::DEBUG);
    assert(StringToLogLevel("INFO") == LogLevel::INFO);
    assert(StringToLogLevel("WARN") == LogLevel::WARN);
    assert(StringToLogLevel("ERROR") == LogLevel::ERROR);
    assert(StringToLogLevel("FATAL") == LogLevel::FATAL);
    
    // 测试无效级别
    assert(StringToLogLevel("INVALID") == LogLevel::INFO); // 默认级别
    
    std::cout << "Log levels test passed!" << std::endl;
}

void test_log_record() {
    std::cout << "Testing LogRecord..." << std::endl;
    
    LogRecord record(LogLevel::INFO, "TestModule", "TestContext", "Test message");
    
    assert(record.level == LogLevel::INFO);
    assert(record.module == "TestModule");
    assert(record.context == "TestContext");
    assert(record.message == "Test message");
    assert(!record.timestamp.empty());
    
    // 测试格式化
    std::string formatted = record.format();
    assert(formatted.find("[INFO]") != std::string::npos);
    assert(formatted.find("[TestModule]") != std::string::npos);
    assert(formatted.find("[TestContext]") != std::string::npos);
    assert(formatted.find("Test message") != std::string::npos);
    assert(formatted.find("[TID=") != std::string::npos);
    
    std::cout << "LogRecord test passed!" << std::endl;
}

void test_console_sink() {
    std::cout << "Testing ConsoleSink..." << std::endl;
    
    ConsoleSink sink;
    
    // 测试不同级别的日志输出
    LogRecord info_record(LogLevel::INFO, "Console", "Test", "Info message");
    LogRecord error_record(LogLevel::ERROR, "Console", "Test", "Error message");
    
    // 这些调用会输出到控制台，我们主要测试不会崩溃
    sink.write(info_record);
    sink.write(error_record);
    sink.flush();
    
    std::cout << "ConsoleSink test passed!" << std::endl;
}

void test_file_sink() {
    std::cout << "Testing FileSink..." << std::endl;
    
    std::string test_file = "test_log.txt";
    
    {
        FileSink sink(test_file);
        
        LogRecord record1(LogLevel::INFO, "File", "Test1", "First message");
        LogRecord record2(LogLevel::ERROR, "File", "Test2", "Second message");
        
        sink.write(record1);
        sink.write(record2);
        sink.flush();
    }
    
    // 验证文件内容
    std::ifstream file(test_file);
    assert(file.is_open());
    
    std::string line;
    bool found_first = false, found_second = false;
    
    while (std::getline(file, line)) {
        if (line.find("First message") != std::string::npos) {
            found_first = true;
            assert(line.find("[INFO]") != std::string::npos);
            assert(line.find("[File]") != std::string::npos);
        }
        if (line.find("Second message") != std::string::npos) {
            found_second = true;
            assert(line.find("[ERROR]") != std::string::npos);
            assert(line.find("[File]") != std::string::npos);
        }
    }
    
    assert(found_first);
    assert(found_second);
    
    file.close();
    
    // 清理测试文件
    std::remove(test_file.c_str());
    
    std::cout << "FileSink test passed!" << std::endl;
}

void test_logger_basic() {
    std::cout << "Testing Logger basic functionality..." << std::endl;
    
    Logger* logger = Logger::instance();
    assert(logger != nullptr);
    
    // 测试级别设置
    logger->set_level(LogLevel::WARN);
    assert(logger->get_level() == LogLevel::WARN);
    
    logger->set_level(LogLevel::DEBUG);
    assert(logger->get_level() == LogLevel::DEBUG);
    
    // 清除现有的sink
    logger->clear_sinks();
    
    // 添加测试用的文件sink
    std::string test_file = "logger_test.txt";
    logger->add_sink(std::shared_ptr<LogSink>(new FileSink(test_file)));
    
    // 测试不同级别的日志
    logger->debug("TestModule", "Context1", "Debug message");
    logger->info("TestModule", "Context2", "Info message");
    logger->warn("TestModule", "Context3", "Warning message");
    logger->error("TestModule", "Context4", "Error message");
    logger->fatal("TestModule", "Context5", "Fatal message");
    
    // 验证文件内容
    std::ifstream file(test_file);
    assert(file.is_open());
    
    std::string content((std::istreambuf_iterator<char>(file)),
                       std::istreambuf_iterator<char>());
    file.close();
    
    assert(content.find("Debug message") != std::string::npos);
    assert(content.find("Info message") != std::string::npos);
    assert(content.find("Warning message") != std::string::npos);
    assert(content.find("Error message") != std::string::npos);
    assert(content.find("Fatal message") != std::string::npos);
    
    // 清理
    std::remove(test_file.c_str());
    
    std::cout << "Logger basic test passed!" << std::endl;
}

void test_logger_level_filtering() {
    std::cout << "Testing Logger level filtering..." << std::endl;
    
    Logger* logger = Logger::instance();
    logger->clear_sinks();
    
    std::string test_file = "level_filter_test.txt";
    logger->add_sink(std::shared_ptr<LogSink>(new FileSink(test_file)));
    
    // 设置级别为WARN，DEBUG和INFO应该被过滤
    logger->set_level(LogLevel::WARN);
    
    logger->debug("Filter", "Test", "This should not appear");
    logger->info("Filter", "Test", "This should not appear");
    logger->warn("Filter", "Test", "This should appear");
    logger->error("Filter", "Test", "This should appear");
    
    // 验证过滤效果
    std::ifstream file(test_file);
    std::string content((std::istreambuf_iterator<char>(file)),
                       std::istreambuf_iterator<char>());
    file.close();
    
    assert(content.find("This should not appear") == std::string::npos);
    assert(content.find("This should appear") != std::string::npos);
    
    // 清理
    std::remove(test_file.c_str());
    
    std::cout << "Logger level filtering test passed!" << std::endl;
}

void test_logger_multiple_sinks() {
    std::cout << "Testing Logger multiple sinks..." << std::endl;
    
    Logger* logger = Logger::instance();
    logger->clear_sinks();
    logger->set_level(LogLevel::DEBUG);
    
    std::string file1 = "multi_sink1.txt";
    std::string file2 = "multi_sink2.txt";
    
    // 添加多个sink
    logger->add_sink(std::shared_ptr<LogSink>(new FileSink(file1)));
    logger->add_sink(std::shared_ptr<LogSink>(new FileSink(file2)));
    
    logger->info("MultiSink", "Test", "Message to multiple sinks");
    
    // 验证两个文件都有内容
    std::ifstream f1(file1);
    std::string content1((std::istreambuf_iterator<char>(f1)),
                        std::istreambuf_iterator<char>());
    f1.close();
    
    std::ifstream f2(file2);
    std::string content2((std::istreambuf_iterator<char>(f2)),
                        std::istreambuf_iterator<char>());
    f2.close();
    
    assert(content1.find("Message to multiple sinks") != std::string::npos);
    assert(content2.find("Message to multiple sinks") != std::string::npos);
    
    // 清理
    std::remove(file1.c_str());
    std::remove(file2.c_str());
    
    std::cout << "Logger multiple sinks test passed!" << std::endl;
}

void test_logger_macros() {
    std::cout << "Testing Logger macros..." << std::endl;
    
    Logger* logger = Logger::instance();
    logger->clear_sinks();
    logger->set_level(LogLevel::DEBUG);
    
    std::string test_file = "macro_test.txt";
    logger->add_sink(std::shared_ptr<LogSink>(new FileSink(test_file)));
    
    // 测试宏
    LOG_DEBUG("MacroTest", "Context", "Debug via macro");
    LOG_INFO("MacroTest", "Context", "Info via macro");
    LOG_WARN("MacroTest", "Context", "Warning via macro");
    LOG_ERROR("MacroTest", "Context", "Error via macro");
    LOG_FATAL("MacroTest", "Context", "Fatal via macro");
    
    // 验证内容
    std::ifstream file(test_file);
    std::string content((std::istreambuf_iterator<char>(file)),
                       std::istreambuf_iterator<char>());
    file.close();
    
    assert(content.find("Debug via macro") != std::string::npos);
    assert(content.find("Info via macro") != std::string::npos);
    assert(content.find("Warning via macro") != std::string::npos);
    assert(content.find("Error via macro") != std::string::npos);
    assert(content.find("Fatal via macro") != std::string::npos);
    
    // 清理
    std::remove(test_file.c_str());
    
    std::cout << "Logger macros test passed!" << std::endl;
}

void test_logger_thread_safety() {
    std::cout << "Testing Logger thread safety..." << std::endl;
    
    Logger* logger = Logger::instance();
    logger->clear_sinks();
    logger->set_level(LogLevel::DEBUG);
    
    std::string test_file = "thread_safety_test.txt";
    logger->add_sink(std::shared_ptr<LogSink>(new FileSink(test_file)));
    
    const int num_threads = 4;
    const int messages_per_thread = 100;
    std::vector<std::thread> threads;
    
    // 启动多个线程并发写日志
    for (int t = 0; t < num_threads; ++t) {
        threads.emplace_back([&, t]() {
            for (int i = 0; i < messages_per_thread; ++i) {
                std::string context = "Thread" + std::to_string(t);
                std::string message = "Message " + std::to_string(i) + " from thread " + std::to_string(t);
                
                switch (i % 4) {
                    case 0: LOG_DEBUG("ThreadTest", context, message); break;
                    case 1: LOG_INFO("ThreadTest", context, message); break;
                    case 2: LOG_WARN("ThreadTest", context, message); break;
                    case 3: LOG_ERROR("ThreadTest", context, message); break;
                }
                
                // 小延迟增加并发竞争
                std::this_thread::sleep_for(std::chrono::microseconds(1));
            }
        });
    }
    
    // 等待所有线程完成
    for (auto& thread : threads) {
        thread.join();
    }
    
    // 验证日志文件
    std::ifstream file(test_file);
    assert(file.is_open());
    
    int line_count = 0;
    std::string line;
    while (std::getline(file, line)) {
        line_count++;
    }
    file.close();
    
    // 应该有所有线程的所有消息
    assert(line_count == num_threads * messages_per_thread);
    
    // 清理
    std::remove(test_file.c_str());
    
    std::cout << "Logger thread safety test passed!" << std::endl;
}

void test_log_stream() {
    std::cout << "Testing LogStream..." << std::endl;
    
    Logger* logger = Logger::instance();
    logger->clear_sinks();
    logger->set_level(LogLevel::DEBUG);
    
    std::string test_file = "stream_test.txt";
    logger->add_sink(std::shared_ptr<LogSink>(new FileSink(test_file)));
    
    // 测试流式日志
    {
        LogStream stream(LogLevel::INFO, "StreamTest", "Context");
        stream << "Stream message with number: " << 42 << " and string: " << "test";
    } // 析构时自动写入日志
    
    // 验证内容
    std::ifstream file(test_file);
    std::string content((std::istreambuf_iterator<char>(file)),
                       std::istreambuf_iterator<char>());
    file.close();
    
    assert(content.find("Stream message with number: 42 and string: test") != std::string::npos);
    assert(content.find("[INFO]") != std::string::npos);
    assert(content.find("[StreamTest]") != std::string::npos);
    
    // 清理
    std::remove(test_file.c_str());
    
    std::cout << "LogStream test passed!" << std::endl;
}

void test_logger_performance() {
    std::cout << "Testing Logger performance..." << std::endl;
    
    Logger* logger = Logger::instance();
    logger->clear_sinks();
    logger->set_level(LogLevel::INFO);
    
    std::string test_file = "performance_test.txt";
    logger->add_sink(std::shared_ptr<LogSink>(new FileSink(test_file)));
    
    const int num_messages = 10000;
    
    auto start_time = std::chrono::high_resolution_clock::now();
    
    // 大量日志写入
    for (int i = 0; i < num_messages; ++i) {
        LOG_INFO("PerfTest", "Batch", "Performance test message " + std::to_string(i));
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    
    std::cout << "Logged " << num_messages << " messages in " << duration.count() << "ms" << std::endl;
    std::cout << "Average: " << (duration.count() * 1000.0 / num_messages) << " microseconds per message" << std::endl;
    
    // 验证所有消息都被写入
    std::ifstream file(test_file);
    int line_count = 0;
    std::string line;
    while (std::getline(file, line)) {
        line_count++;
    }
    file.close();
    
    assert(line_count == num_messages);
    
    // 清理
    std::remove(test_file.c_str());
    
    std::cout << "Logger performance test passed!" << std::endl;
}

void test_logger_error_handling() {
    std::cout << "Testing Logger error handling..." << std::endl;
    
    Logger* logger = Logger::instance();
    logger->clear_sinks();
    
    try {
        // 尝试创建无效路径的文件sink
        std::string invalid_path = "/invalid/path/that/does/not/exist/test.log";
        logger->add_sink(std::shared_ptr<LogSink>(new FileSink(invalid_path)));
        std::cout << "FileSink with invalid path should have thrown exception" << std::endl;
        assert(false); // 不应该到达这里
    } catch (const std::exception& e) {
        std::cout << "Caught expected exception: " << e.what() << std::endl;
    }
    
    // 测试空消息
    logger->add_sink(std::shared_ptr<LogSink>(new ConsoleSink()));
    logger->info("ErrorTest", "", ""); // 空上下文和消息
    logger->info("", "Context", "Message"); // 空模块
    
    std::cout << "Logger error handling test passed!" << std::endl;
}

void test_logger_edge_cases() {
    std::cout << "Testing Logger edge cases..." << std::endl;
    
    Logger* logger = Logger::instance();
    logger->clear_sinks();
    logger->set_level(LogLevel::DEBUG);
    
    std::string test_file = "edge_cases_test.txt";
    logger->add_sink(std::shared_ptr<LogSink>(new FileSink(test_file)));
    
    // 测试特殊字符
    logger->info("EdgeTest", "Special", "Message with special chars: \n\t\r\"'\\");
    
    // 测试长消息
    std::string long_message(10000, 'A');
    logger->info("EdgeTest", "Long", long_message);
    
    // 测试Unicode字符（如果支持）
    logger->info("EdgeTest", "Unicode", "Unicode test: 你好世界 🎉");
    
    // 验证文件内容
    std::ifstream file(test_file);
    std::string content((std::istreambuf_iterator<char>(file)),
                       std::istreambuf_iterator<char>());
    file.close();
    
    assert(content.find("Special") != std::string::npos);
    assert(content.find("Long") != std::string::npos);
    assert(content.find("Unicode") != std::string::npos);
    
    // 清理
    std::remove(test_file.c_str());
    
    std::cout << "Logger edge cases test passed!" << std::endl;
}

void test_logger_singleton() {
    std::cout << "Testing Logger singleton..." << std::endl;
    
    Logger* logger1 = Logger::instance();
    Logger* logger2 = Logger::instance();
    
    // 应该是同一个实例
    assert(logger1 == logger2);
    
    // 在不同线程中获取实例
    Logger* thread_logger = nullptr;
    std::thread t([&]() {
        thread_logger = Logger::instance();
    });
    t.join();
    
    assert(thread_logger == logger1);
    
    std::cout << "Logger singleton test passed!" << std::endl;
}

int main() {
    try {
        test_log_levels();
        test_log_record();
        test_console_sink();
        test_file_sink();
        test_logger_basic();
        test_logger_level_filtering();
        test_logger_multiple_sinks();
        test_logger_macros();
        test_logger_thread_safety();
        test_log_stream();
        test_logger_performance();
        test_logger_error_handling();
        test_logger_edge_cases();
        test_logger_singleton();
        
        std::cout << "\n🎉 All logger tests passed!" << std::endl;
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "❌ Logger test failed: " << e.what() << std::endl;
        return 1;
    }
}

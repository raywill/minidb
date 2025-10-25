#pragma once

#include <string>
#include <signal.h>

namespace minidb {

// 崩溃处理器
class CrashHandler {
public:
    // 初始化崩溃处理器
    static void initialize();
    
    // 设置当前查询ID（线程局部）
    static void set_current_query_id(size_t query_id);
    
    // 获取当前查询ID
    static size_t get_current_query_id();
    
    // 生成堆栈转储文件
    static std::string generate_dump_file();
    
private:
    // 信号处理函数
    static void signal_handler(int sig, siginfo_t* info, void* context);
    
    // 获取堆栈跟踪
    static std::string get_stack_trace();
    
    // 获取进程和线程信息
    static std::string get_process_info();
    
    // 写入转储文件
    static void write_dump_file(const std::string& filename, const std::string& content);
    
    static bool initialized_;
};

// RAII查询ID设置器
class QueryIdSetter {
public:
    explicit QueryIdSetter(size_t query_id) {
        CrashHandler::set_current_query_id(query_id);
    }
    
    ~QueryIdSetter() {
        CrashHandler::set_current_query_id(0);
    }
};

// 宏定义用于自动设置查询ID
#define SET_QUERY_ID(id) QueryIdSetter _query_id_setter(id)

} // namespace minidb

#include "common/crash_handler.h"
#include "log/logger.h"
#include <execinfo.h>
#include <unistd.h>
#include <sys/types.h>
#include <pthread.h>
#include <fstream>
#include <sstream>
#include <iostream>
#include <ctime>
#include <cstring>
#include <cstdlib>

namespace minidb {

bool CrashHandler::initialized_ = false;
thread_local size_t current_query_id = 0;

void CrashHandler::initialize() {
    if (initialized_) {
        return;
    }
    
    // 设置信号处理器
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_sigaction = signal_handler;
    sa.sa_flags = SA_SIGINFO | SA_RESTART;
    sigemptyset(&sa.sa_mask);
    
    // 捕获各种崩溃信号
    sigaction(SIGSEGV, &sa, nullptr);  // 段错误
    sigaction(SIGBUS, &sa, nullptr);   // 总线错误
    sigaction(SIGFPE, &sa, nullptr);   // 浮点异常
    sigaction(SIGILL, &sa, nullptr);   // 非法指令
    sigaction(SIGABRT, &sa, nullptr);  // 中止信号
    
    initialized_ = true;
    
    LOG_INFO("CrashHandler", "Initialize", "Crash handler initialized successfully");
}

void CrashHandler::set_current_query_id(size_t query_id) {
    current_query_id = query_id;
}

size_t CrashHandler::get_current_query_id() {
    return current_query_id;
}

std::string CrashHandler::generate_dump_file() {
    pid_t pid = getpid();
    pthread_t tid = pthread_self();
    size_t query_id = get_current_query_id();
    
    std::ostringstream filename;
    filename << "crash-" << pid << "-" << tid << "-" << query_id << ".dmp";
    
    return filename.str();
}

void CrashHandler::signal_handler(int sig, siginfo_t* info, void* context) {
    // 获取信号名称
    const char* signal_name = "UNKNOWN";
    switch (sig) {
        case SIGSEGV: signal_name = "SIGSEGV (Segmentation fault)"; break;
        case SIGBUS: signal_name = "SIGBUS (Bus error)"; break;
        case SIGFPE: signal_name = "SIGFPE (Floating point exception)"; break;
        case SIGILL: signal_name = "SIGILL (Illegal instruction)"; break;
        case SIGABRT: signal_name = "SIGABRT (Abort)"; break;
    }
    
    // 生成转储文件名
    std::string dump_filename = generate_dump_file();
    
    // 收集崩溃信息
    std::ostringstream crash_info;
    crash_info << "=== CRASH DUMP ===" << std::endl;
    crash_info << "Signal: " << signal_name << " (" << sig << ")" << std::endl;
    crash_info << "Signal info:" << std::endl;
    crash_info << "  si_code: " << info->si_code << std::endl;
    crash_info << "  si_addr: " << info->si_addr << std::endl;
    crash_info << std::endl;
    
    // 添加进程信息
    crash_info << get_process_info() << std::endl;
    
    // 添加堆栈跟踪
    crash_info << "Stack trace:" << std::endl;
    crash_info << get_stack_trace() << std::endl;
    
    // 写入转储文件
    write_dump_file(dump_filename, crash_info.str());
    
    // 记录到日志
    LOG_FATAL("CrashHandler", "Signal", 
              "Process crashed with " + std::string(signal_name) + 
              ", dump file: " + dump_filename);
    
    // 输出到stderr
    std::cerr << "FATAL: Process crashed with " << signal_name << std::endl;
    std::cerr << "Dump file generated: " << dump_filename << std::endl;
    std::cerr << "Query ID: " << get_current_query_id() << std::endl;
    
    // 刷新日志
    Logger::instance()->add_sink(std::shared_ptr<LogSink>(new ConsoleSink()));
    
    // 恢复默认信号处理并重新发送信号
    signal(sig, SIG_DFL);
    raise(sig);
}

std::string CrashHandler::get_stack_trace() {
    const int max_frames = 64;
    void* frames[max_frames];
    
    int frame_count = backtrace(frames, max_frames);
    char** symbols = backtrace_symbols(frames, frame_count);
    
    std::ostringstream trace;
    if (symbols) {
        for (int i = 0; i < frame_count; ++i) {
            trace << "  [" << i << "] " << symbols[i] << std::endl;
        }
        free(symbols);
    } else {
        trace << "  Unable to get stack trace symbols" << std::endl;
    }
    
    return trace.str();
}

std::string CrashHandler::get_process_info() {
    std::ostringstream info;
    
    // 进程信息
    pid_t pid = getpid();
    pthread_t tid = pthread_self();
    size_t query_id = get_current_query_id();
    
    info << "Process info:" << std::endl;
    info << "  PID: " << pid << std::endl;
    info << "  Thread ID: " << tid << std::endl;
    info << "  Query ID: " << query_id << std::endl;
    
    // 时间信息
    time_t now = time(nullptr);
    char time_str[64];
    strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", localtime(&now));
    info << "  Crash time: " << time_str << std::endl;
    
    return info.str();
}

void CrashHandler::write_dump_file(const std::string& filename, const std::string& content) {
    try {
        std::ofstream file(filename);
        if (file.is_open()) {
            file << content;
            file.close();
        }
    } catch (...) {
        // 忽略写文件时的异常，避免在崩溃处理中再次崩溃
    }
}

} // namespace minidb

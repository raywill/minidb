#include "server/database_server.h"
#include "log/logger.h"
#include <iostream>
#include <signal.h>
#include <unistd.h>

using namespace minidb;

// 全局服务器实例
DatabaseServer* g_server = nullptr;

// 信号处理函数
void signal_handler(int signal) {
    if (g_server) {
        std::cout << "\nShutting down server..." << std::endl;
        g_server->stop();
        exit(0);
    }
}

int main(int argc, char* argv[]) {
    // 解析命令行参数
    std::string data_dir = "./data";
    int port = 9876;
    
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "--data-dir" && i + 1 < argc) {
            data_dir = argv[++i];
        } else if (arg == "--port" && i + 1 < argc) {
            port = std::stoi(argv[++i]);
        } else if (arg == "--help" || arg == "-h") {
            std::cout << "Usage: " << argv[0] << " [options]" << std::endl;
            std::cout << "Options:" << std::endl;
            std::cout << "  --data-dir DIR    Data directory (default: ./data)" << std::endl;
            std::cout << "  --port PORT       Server port (default: 9876)" << std::endl;
            std::cout << "  --help, -h        Show this help message" << std::endl;
            return 0;
        }
    }
    
    try {
        // 设置信号处理
        signal(SIGINT, signal_handler);
        signal(SIGTERM, signal_handler);
        
        // 创建并启动服务器
        DatabaseServer server(data_dir, port);
        g_server = &server;
        
        Status status = server.start();
        if (!status.ok()) {
            std::cerr << "Failed to start server: " << status.ToString() << std::endl;
            return 1;
        }
        
        std::cout << "MiniDB server started on port " << port << std::endl;
        std::cout << "Data directory: " << data_dir << std::endl;
        std::cout << "Press Ctrl+C to stop the server" << std::endl;
        
        // 保持服务器运行
        while (server.is_running()) {
            sleep(1);
        }
        
    } catch (const std::exception& e) {
        std::cerr << "Server error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}

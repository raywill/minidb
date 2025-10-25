#include "client/cli_client.h"
#include <iostream>

using namespace minidb;

int main(int argc, char* argv[]) {
    // 解析命令行参数
    std::string host = "localhost";
    int port = 9876;
    
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "--host" && i + 1 < argc) {
            host = argv[++i];
        } else if (arg == "--port" && i + 1 < argc) {
            port = std::stoi(argv[++i]);
        } else if (arg == "--help" || arg == "-h") {
            std::cout << "Usage: " << argv[0] << " [options]" << std::endl;
            std::cout << "Options:" << std::endl;
            std::cout << "  --host HOST       Server host (default: localhost)" << std::endl;
            std::cout << "  --port PORT       Server port (default: 9876)" << std::endl;
            std::cout << "  --help, -h        Show this help message" << std::endl;
            return 0;
        }
    }
    
    try {
        CLIClient client;
        
        // 连接到服务器
        std::cout << "Connecting to " << host << ":" << port << "..." << std::endl;
        Status status = client.connect(host, port);
        if (!status.ok()) {
            std::cerr << "Failed to connect to server: " << status.ToString() << std::endl;
            std::cerr << "You can still use the client in offline mode for testing commands." << std::endl;
            std::cerr << "Note: SQL commands will fail, but 'help', 'history', etc. will work." << std::endl;
        } else {
            std::cout << "Connected successfully!" << std::endl;
        }
        
        // 运行交互式界面（即使连接失败也可以运行）
        client.run_interactive();
        
    } catch (const std::exception& e) {
        std::cerr << "Client error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}

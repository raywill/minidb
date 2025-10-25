#include "client/cli_client.h"
#include <iostream>
#include <sstream>
#include <algorithm>
#include <unistd.h>

namespace minidb {

CLIClient::CLIClient() : connected_(false) {
    initialize_history();
}

CLIClient::~CLIClient() {
    save_history();
    disconnect();
}

Status CLIClient::connect(const std::string& host, int port) {
    Status status = client_.connect(host, port);
    if (status.ok()) {
        connected_ = true;
    }
    return status;
}

void CLIClient::disconnect() {
    if (connected_) {
        client_.disconnect();
        connected_ = false;
    }
}

void CLIClient::run_interactive() {
    show_welcome();
    
    // 显示历史记录功能提示（仅在交互式终端中）
    if (isatty(STDIN_FILENO)) {
        std::cout << "📝 History enabled: Use ↑/↓ arrows to navigate command history" << std::endl;
        std::cout << "💾 History file: ~/.minidb_history" << std::endl;
        std::cout << std::endl;
    }
    
    while (true) {
        std::string input = read_input();
        
        // 处理特殊命令
        if (handle_special_command(input)) {
            continue;
        }
        
        // 跳过空输入
        if (input.empty()) {
            continue;
        }
        
        // 执行SQL
        std::string result;
        Status status = execute_sql(input, result);
        
        if (status.ok()) {
            display_result(result);
        } else {
            std::cerr << "Error: " << status.ToString() << std::endl;
        }
    }
}

Status CLIClient::execute_sql(const std::string& sql, std::string& result) {
    if (!connected_) {
        return Status::NetworkError("Not connected to server");
    }
    
    return client_.send_request(sql, result);
}

void CLIClient::show_welcome() {
    std::cout << "Welcome to MiniDB!" << std::endl;
    std::cout << "Type 'help' for help, 'quit' or 'exit' to quit." << std::endl;
    std::cout << std::endl;
}

void CLIClient::show_help() {
    std::cout << "MiniDB Commands:" << std::endl;
    std::cout << "  help                    - Show this help message" << std::endl;
    std::cout << "  history                 - Show command history" << std::endl;
    std::cout << "  quit, exit              - Exit the client" << std::endl;
    std::cout << "  clear                   - Clear the screen" << std::endl;
    std::cout << std::endl;
    std::cout << "Navigation:" << std::endl;
    std::cout << "  ↑ (Up Arrow)           - Previous command in history" << std::endl;
    std::cout << "  ↓ (Down Arrow)         - Next command in history" << std::endl;
    std::cout << "  ← → (Left/Right Arrow) - Move cursor" << std::endl;
    std::cout << "  Ctrl+C                 - Exit client" << std::endl;
    std::cout << "  Ctrl+D                 - Exit client (if input is empty)" << std::endl;
    std::cout << std::endl;
    std::cout << "SQL Commands:" << std::endl;
    std::cout << "  CREATE TABLE name(col1 TYPE, col2 TYPE, ...);" << std::endl;
    std::cout << "  DROP TABLE name;" << std::endl;
    std::cout << "  INSERT INTO name VALUES (val1, val2, ...);" << std::endl;
    std::cout << "  SELECT col1, col2 FROM name WHERE condition;" << std::endl;
    std::cout << "  DELETE FROM name WHERE condition;" << std::endl;
    std::cout << std::endl;
    std::cout << "Supported data types: INT, STRING, BOOL, DECIMAL" << std::endl;
    std::cout << "Supported functions: SIN(x), COS(x), SUBSTR(str, start, len)" << std::endl;
    std::cout << std::endl;
}

bool CLIClient::handle_special_command(const std::string& input) {
    std::string cmd = input;
    std::transform(cmd.begin(), cmd.end(), cmd.begin(), ::tolower);
    
    if (cmd == "help") {
        show_help();
        return true;
    }
    
    if (cmd == "history") {
        show_history();
        return true;
    }
    
    if (cmd == "quit" || cmd == "exit") {
        save_history();
        std::cout << "Goodbye!" << std::endl;
        exit(0);
    }
    
    if (cmd == "clear") {
        system("clear");
        return true;
    }
    
    return false;
}

void CLIClient::display_result(const std::string& result) {
    if (result.empty()) {
        std::cout << "OK" << std::endl;
        return;
    }
    
    // 检查是否是错误消息
    if (result.substr(0, 6) == "ERROR:") {
        std::cerr << result << std::endl;
        return;
    }
    
    // 显示结果
    std::cout << result << std::endl;
}

std::string CLIClient::read_input() {
    std::string line;
    
    if (isatty(STDIN_FILENO) && input_reader_) {
        // 交互式终端，使用历史记录功能
        try {
            line = input_reader_->read_line("minidb> ");
        } catch (const std::exception& e) {
            // 如果历史记录输入失败，回退到标准输入
            std::cout << "minidb> ";
            std::getline(std::cin, line);
        }
    } else {
        // 非交互式或回退模式
        std::cout << "minidb> ";
        std::getline(std::cin, line);
        if (std::cin.eof()) {
            exit(0);
        }
    }
    
    // 移除前后空白字符
    size_t start = line.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) {
        return "";
    }
    
    size_t end = line.find_last_not_of(" \t\r\n");
    return line.substr(start, end - start + 1);
}

void CLIClient::initialize_history() {
    try {
        history_ = std::unique_ptr<CommandHistory>(new CommandHistory(500));
        input_reader_ = std::unique_ptr<CommandLineInput>(new CommandLineInput(history_.get()));
        
        // 尝试从文件加载历史记录
        std::string home_dir = getenv("HOME") ? getenv("HOME") : ".";
        std::string history_file = home_dir + "/.minidb_history";
        history_->load_from_file(history_file);
    } catch (const std::exception& e) {
        // 如果历史记录初始化失败，继续运行但不使用历史记录
        std::cerr << "Warning: Failed to initialize command history: " << e.what() << std::endl;
        history_.reset();
        input_reader_.reset();
    }
}

void CLIClient::save_history() {
    if (history_) {
        try {
            std::string home_dir = getenv("HOME") ? getenv("HOME") : ".";
            std::string history_file = home_dir + "/.minidb_history";
            history_->save_to_file(history_file);
        } catch (const std::exception& e) {
            std::cerr << "Warning: Failed to save command history: " << e.what() << std::endl;
        }
    }
}

void CLIClient::show_history() {
    if (!history_ || history_->empty()) {
        std::cout << "No command history available." << std::endl;
        return;
    }
    
    std::cout << "Command History:" << std::endl;
    auto commands = history_->get_all_commands();
    
    // 显示最近的20条命令
    size_t start = commands.size() > 20 ? commands.size() - 20 : 0;
    
    for (size_t i = start; i < commands.size(); ++i) {
        std::cout << "  " << (i + 1) << ": " << commands[i] << std::endl;
    }
    
    if (commands.size() > 20) {
        std::cout << "  ... (showing last 20 of " << commands.size() << " commands)" << std::endl;
    }
    
    std::cout << std::endl;
}

} // namespace minidb

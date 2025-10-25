#pragma once

#include "net/tcp_client.h"
#include "client/command_history.h"
#include "common/status.h"
#include <string>
#include <memory>

namespace minidb {

// 命令行客户端
class CLIClient {
public:
    CLIClient();
    ~CLIClient();
    
    // 连接到服务器
    Status connect(const std::string& host, int port);
    
    // 断开连接
    void disconnect();
    
    // 运行交互式命令行界面
    void run_interactive();
    
    // 执行单个SQL命令
    Status execute_sql(const std::string& sql, std::string& result);
    
private:
    TCPClient client_;
    bool connected_;
    std::unique_ptr<CommandHistory> history_;
    std::unique_ptr<CommandLineInput> input_reader_;
    
    // 显示欢迎信息
    void show_welcome();
    
    // 显示帮助信息
    void show_help();
    
    // 处理特殊命令
    bool handle_special_command(const std::string& input);
    
    // 格式化并显示结果
    void display_result(const std::string& result);
    
    // 读取用户输入（支持历史记录）
    std::string read_input();
    
    // 初始化历史记录
    void initialize_history();
    
    // 保存历史记录
    void save_history();
    
    // 显示历史记录
    void show_history();
};

} // namespace minidb

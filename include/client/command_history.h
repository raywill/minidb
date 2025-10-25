#pragma once

#include <string>
#include <vector>
#include <memory>

namespace minidb {

// 命令历史管理器
class CommandHistory {
public:
    explicit CommandHistory(size_t max_history = 1000);
    ~CommandHistory();
    
    // 添加命令到历史记录
    void add_command(const std::string& command);
    
    // 获取历史记录数量
    size_t size() const;
    
    // 检查是否为空
    bool empty() const;
    
    // 获取指定索引的命令
    std::string get_command(size_t index) const;
    
    // 获取最近的命令
    std::string get_last_command() const;
    
    // 清空历史记录
    void clear();
    
    // 保存历史记录到文件
    bool save_to_file(const std::string& filename) const;
    
    // 从文件加载历史记录
    bool load_from_file(const std::string& filename);
    
    // 获取所有历史记录
    std::vector<std::string> get_all_commands() const;
    
private:
    std::vector<std::string> history_;
    size_t max_history_;
    
    // 清理重复和空命令
    void cleanup_history();
};

// 命令行输入管理器（支持历史记录和箭头键）
class CommandLineInput {
public:
    explicit CommandLineInput(CommandHistory* history = nullptr);
    ~CommandLineInput();
    
    // 读取一行输入（支持历史记录和编辑）
    std::string read_line(const std::string& prompt = "");
    
    // 设置历史记录管理器
    void set_history(CommandHistory* history);
    
    // 启用/禁用历史记录功能
    void enable_history(bool enable);
    
private:
    CommandHistory* history_;
    bool history_enabled_;
    int current_history_index_;
    std::string current_input_;
    
    // 终端控制
    void setup_terminal();
    void restore_terminal();
    bool is_terminal_setup_;
    
    // 键盘输入处理
    int read_key();
    void handle_arrow_key(int key, std::string& input, int& cursor_pos);
    void handle_special_key(int key, std::string& input, int& cursor_pos);
    
    // 显示控制
    void clear_line();
    void move_cursor_to_start();
    void display_line(const std::string& prompt, const std::string& input, int cursor_pos);
    
    // 历史记录导航
    void navigate_history_up(std::string& input);
    void navigate_history_down(std::string& input);
    void reset_history_navigation();
    
    // 终端状态保存/恢复
    struct TerminalState;
    std::unique_ptr<TerminalState> saved_terminal_state_;
};

// 键码定义
enum class KeyCode {
    ENTER = 10,        // Unix/Linux下的换行符
    CARRIAGE_RETURN = 13,  // Windows下的回车符
    ESCAPE = 27,
    BACKSPACE = 127,
    CTRL_C = 3,
    CTRL_D = 4,
    ARROW_UP = 1001,
    ARROW_DOWN = 1002,
    ARROW_LEFT = 1003,
    ARROW_RIGHT = 1004,
    HOME = 1005,
    END = 1006,
    DELETE = 1007,
    UNKNOWN = -1
};

} // namespace minidb

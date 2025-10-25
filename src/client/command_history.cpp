#include "client/command_history.h"
#include <fstream>
#include <algorithm>
#include <termios.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <iostream>

namespace minidb {

// CommandHistory 实现
CommandHistory::CommandHistory(size_t max_history) : max_history_(max_history) {
}

CommandHistory::~CommandHistory() = default;

void CommandHistory::add_command(const std::string& command) {
    // 移除前后空白字符
    std::string trimmed = command;
    size_t start = trimmed.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) {
        return; // 空命令
    }
    size_t end = trimmed.find_last_not_of(" \t\r\n");
    trimmed = trimmed.substr(start, end - start + 1);
    
    // 跳过空命令
    if (trimmed.empty()) {
        return;
    }
    
    // 跳过与最后一个命令相同的命令
    if (!history_.empty() && history_.back() == trimmed) {
        return;
    }
    
    history_.push_back(trimmed);
    
    // 限制历史记录大小
    if (history_.size() > max_history_) {
        history_.erase(history_.begin());
    }
}

size_t CommandHistory::size() const {
    return history_.size();
}

bool CommandHistory::empty() const {
    return history_.empty();
}

std::string CommandHistory::get_command(size_t index) const {
    if (index >= history_.size()) {
        return "";
    }
    return history_[index];
}

std::string CommandHistory::get_last_command() const {
    if (history_.empty()) {
        return "";
    }
    return history_.back();
}

void CommandHistory::clear() {
    history_.clear();
}

bool CommandHistory::save_to_file(const std::string& filename) const {
    std::ofstream file(filename);
    if (!file.is_open()) {
        return false;
    }
    
    for (const std::string& command : history_) {
        file << command << std::endl;
    }
    
    return file.good();
}

bool CommandHistory::load_from_file(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        return false;
    }
    
    std::string line;
    while (std::getline(file, line)) {
        if (!line.empty()) {
            add_command(line);
        }
    }
    
    return true;
}

std::vector<std::string> CommandHistory::get_all_commands() const {
    return history_;
}

void CommandHistory::cleanup_history() {
    // 移除空命令
    history_.erase(
        std::remove_if(history_.begin(), history_.end(),
                      [](const std::string& cmd) { return cmd.empty(); }),
        history_.end());
    
    // 移除连续重复的命令
    auto last = std::unique(history_.begin(), history_.end());
    history_.erase(last, history_.end());
}

// CommandLineInput 实现

// 终端状态结构
struct CommandLineInput::TerminalState {
    struct termios original_termios;
    bool valid;
    
    TerminalState() : valid(false) {}
};

CommandLineInput::CommandLineInput(CommandHistory* history)
    : history_(history), history_enabled_(true), current_history_index_(-1),
      is_terminal_setup_(false), saved_terminal_state_(new TerminalState()) {
}

CommandLineInput::~CommandLineInput() {
    restore_terminal();
}

std::string CommandLineInput::read_line(const std::string& prompt) {
    if (!isatty(STDIN_FILENO)) {
        // 如果不是终端，使用标准输入
        std::string line;
        std::cout << prompt;
        std::getline(std::cin, line);
        return line;
    }
    
    setup_terminal();
    reset_history_navigation();
    
    std::string input;
    int cursor_pos = 0;
    
    // 显示提示符
    std::cout << prompt;
    std::cout.flush();
    
    while (true) {
        int key = read_key();
        
        switch (key) {
            case static_cast<int>(KeyCode::ENTER):
            case static_cast<int>(KeyCode::CARRIAGE_RETURN):
                std::cout << std::endl;
                restore_terminal();
                
                // 添加到历史记录
                if (history_enabled_ && history_ && !input.empty()) {
                    history_->add_command(input);
                }
                
                return input;
                
            case static_cast<int>(KeyCode::CTRL_C):
                std::cout << "^C" << std::endl;
                restore_terminal();
                exit(0);
                
            case static_cast<int>(KeyCode::CTRL_D):
                if (input.empty()) {
                    std::cout << std::endl;
                    restore_terminal();
                    exit(0);
                }
                break;
                
            case static_cast<int>(KeyCode::BACKSPACE):
                if (cursor_pos > 0) {
                    input.erase(cursor_pos - 1, 1);
                    cursor_pos--;
                    display_line(prompt, input, cursor_pos);
                }
                break;
                
            case static_cast<int>(KeyCode::ARROW_UP):
                if (history_enabled_ && history_) {
                    navigate_history_up(input);
                    cursor_pos = input.length();
                    display_line(prompt, input, cursor_pos);
                }
                break;
                
            case static_cast<int>(KeyCode::ARROW_DOWN):
                if (history_enabled_ && history_) {
                    navigate_history_down(input);
                    cursor_pos = input.length();
                    display_line(prompt, input, cursor_pos);
                }
                break;
                
            case static_cast<int>(KeyCode::ARROW_LEFT):
                if (cursor_pos > 0) {
                    cursor_pos--;
                    std::cout << "\b";
                    std::cout.flush();
                }
                break;
                
            case static_cast<int>(KeyCode::ARROW_RIGHT):
                if (cursor_pos < static_cast<int>(input.length())) {
                    cursor_pos++;
                    std::cout << input[cursor_pos - 1];
                    std::cout.flush();
                }
                break;
                
            default:
                if (key >= 32 && key <= 126) { // 可打印字符
                    input.insert(cursor_pos, 1, static_cast<char>(key));
                    cursor_pos++;
                    display_line(prompt, input, cursor_pos);
                }
                break;
        }
    }
}

void CommandLineInput::set_history(CommandHistory* history) {
    history_ = history;
}

void CommandLineInput::enable_history(bool enable) {
    history_enabled_ = enable;
}

void CommandLineInput::setup_terminal() {
    if (is_terminal_setup_) {
        return;
    }
    
    // 保存当前终端设置
    if (tcgetattr(STDIN_FILENO, &saved_terminal_state_->original_termios) == 0) {
        saved_terminal_state_->valid = true;
        
        // 设置原始模式
        struct termios raw = saved_terminal_state_->original_termios;
        raw.c_lflag &= ~(ECHO | ICANON);
        raw.c_cc[VMIN] = 1;
        raw.c_cc[VTIME] = 0;
        
        if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == 0) {
            is_terminal_setup_ = true;
        }
    }
}

void CommandLineInput::restore_terminal() {
    if (is_terminal_setup_ && saved_terminal_state_->valid) {
        tcsetattr(STDIN_FILENO, TCSAFLUSH, &saved_terminal_state_->original_termios);
        is_terminal_setup_ = false;
    }
}

int CommandLineInput::read_key() {
    char c;
    if (read(STDIN_FILENO, &c, 1) != 1) {
        return static_cast<int>(KeyCode::UNKNOWN);
    }
    
    if (c == 27) { // ESC序列
        char seq[3];
        
        if (read(STDIN_FILENO, &seq[0], 1) != 1) {
            return static_cast<int>(KeyCode::ESCAPE);
        }
        
        if (read(STDIN_FILENO, &seq[1], 1) != 1) {
            return static_cast<int>(KeyCode::ESCAPE);
        }
        
        if (seq[0] == '[') {
            switch (seq[1]) {
                case 'A': return static_cast<int>(KeyCode::ARROW_UP);
                case 'B': return static_cast<int>(KeyCode::ARROW_DOWN);
                case 'C': return static_cast<int>(KeyCode::ARROW_RIGHT);
                case 'D': return static_cast<int>(KeyCode::ARROW_LEFT);
                case 'H': return static_cast<int>(KeyCode::HOME);
                case 'F': return static_cast<int>(KeyCode::END);
                case '3':
                    if (read(STDIN_FILENO, &seq[2], 1) == 1 && seq[2] == '~') {
                        return static_cast<int>(KeyCode::DELETE);
                    }
                    break;
            }
        }
        
        return static_cast<int>(KeyCode::ESCAPE);
    }
    
    return static_cast<int>(c);
}

void CommandLineInput::clear_line() {
    std::cout << "\r\033[K";
    std::cout.flush();
}

void CommandLineInput::move_cursor_to_start() {
    std::cout << "\r";
    std::cout.flush();
}

void CommandLineInput::display_line(const std::string& prompt, const std::string& input, int cursor_pos) {
    clear_line();
    std::cout << prompt << input;
    
    // 移动光标到正确位置
    int total_length = prompt.length() + input.length();
    int target_pos = prompt.length() + cursor_pos;
    
    if (target_pos < total_length) {
        std::cout << "\r";
        for (int i = 0; i < target_pos; ++i) {
            std::cout << (i < static_cast<int>(prompt.length()) ? prompt[i] : input[i - prompt.length()]);
        }
    }
    
    std::cout.flush();
}

void CommandLineInput::navigate_history_up(std::string& input) {
    if (!history_ || history_->empty()) {
        return;
    }
    
    if (current_history_index_ == -1) {
        // 第一次按上箭头，保存当前输入
        current_input_ = input;
        current_history_index_ = static_cast<int>(history_->size()) - 1;
    } else if (current_history_index_ > 0) {
        current_history_index_--;
    }
    
    if (current_history_index_ >= 0 && current_history_index_ < static_cast<int>(history_->size())) {
        input = history_->get_command(current_history_index_);
    }
}

void CommandLineInput::navigate_history_down(std::string& input) {
    if (!history_ || current_history_index_ == -1) {
        return;
    }
    
    current_history_index_++;
    
    if (current_history_index_ >= static_cast<int>(history_->size())) {
        // 超出历史记录，恢复原始输入
        input = current_input_;
        current_history_index_ = -1;
    } else {
        input = history_->get_command(current_history_index_);
    }
}

void CommandLineInput::reset_history_navigation() {
    current_history_index_ = -1;
    current_input_.clear();
}

} // namespace minidb

#pragma once

#include <string>
#include <vector>
#include <unordered_set>

namespace minidb {

// Token类型枚举
enum class TokenType {
    // 字面量
    INTEGER,
    DECIMAL,
    STRING,
    BOOLEAN,
    
    // 标识符
    IDENTIFIER,
    
    // 关键字
    CREATE,
    DROP,
    TABLE,
    INSERT,
    INTO,
    VALUES,
    SELECT,
    FROM,
    WHERE,
    DELETE,
    IF,
    NOT,
    EXISTS,
    AND,
    OR,

    // JOIN 关键字
    JOIN,
    INNER,
    LEFT,
    RIGHT,
    FULL,
    OUTER,
    ON,
    AS,
    
    // 数据类型
    INT,
    STRING_TYPE,
    BOOL,
    DECIMAL_TYPE,
    
    // 函数
    SIN,
    COS,
    SUBSTR,
    
    // 操作符
    PLUS,           // +
    MINUS,          // -
    MULTIPLY,       // *
    DIVIDE,         // /
    EQUAL,          // =
    NOT_EQUAL,      // !=
    LESS_THAN,      // <
    LESS_EQUAL,     // <=
    GREATER_THAN,   // >
    GREATER_EQUAL,  // >=
    
    // 分隔符
    LEFT_PAREN,     // (
    RIGHT_PAREN,    // )
    COMMA,          // ,
    SEMICOLON,      // ;
    DOT,            // .
    
    // 特殊
    WHITESPACE,
    NEWLINE,
    END_OF_FILE,
    UNKNOWN
};

// Token结构
struct Token {
    TokenType type;
    std::string value;
    size_t line;
    size_t column;
    
    Token(TokenType t, const std::string& v, size_t l, size_t c)
        : type(t), value(v), line(l), column(c) {}
    
    std::string to_string() const;
};

// 词法分析器
class Tokenizer {
public:
    explicit Tokenizer(const std::string& input);
    
    // 获取下一个token
    Token next_token();
    
    // 查看下一个token但不消费
    Token peek_token();
    
    // 检查是否到达输入末尾
    bool is_end() const;
    
    // 获取当前位置
    size_t get_line() const { return line_; }
    size_t get_column() const { return column_; }
    
    // 获取所有tokens（用于调试）
    std::vector<Token> tokenize_all();
    
private:
    std::string input_;
    size_t position_;
    size_t line_;
    size_t column_;
    
    // 关键字映射
    static std::unordered_set<std::string> keywords_;
    static std::unordered_set<std::string> data_types_;
    static std::unordered_set<std::string> functions_;
    
    // 辅助方法
    char current_char() const;
    char peek_char(size_t offset = 1) const;
    void advance();
    void skip_whitespace();
    void skip_comment();
    
    Token read_string();
    Token read_number();
    Token read_identifier();
    Token read_operator();
    
    TokenType get_keyword_type(const std::string& word) const;
    bool is_alpha(char c) const;
    bool is_digit(char c) const;
    bool is_alnum(char c) const;
};

// 工具函数
std::string TokenTypeToString(TokenType type);

} // namespace minidb

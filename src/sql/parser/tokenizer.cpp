#include "sql/parser/tokenizer.h"
#include <cctype>
#include <stdexcept>
#include <sstream>

namespace minidb {

// 静态成员初始化
std::unordered_set<std::string> Tokenizer::keywords_ = {
    "CREATE", "DROP", "TABLE", "INSERT", "INTO", "VALUES", 
    "SELECT", "FROM", "WHERE", "DELETE", "IF", "NOT", "EXISTS", 
    "AND", "OR", "TRUE", "FALSE"
};

std::unordered_set<std::string> Tokenizer::data_types_ = {
    "INT", "STRING", "BOOL", "DECIMAL"
};

std::unordered_set<std::string> Tokenizer::functions_ = {
    "SIN", "COS", "SUBSTR"
};

std::string Token::to_string() const {
    return "Token(" + TokenTypeToString(type) + ", '" + value + "', " + 
           std::to_string(line) + ":" + std::to_string(column) + ")";
}

Tokenizer::Tokenizer(const std::string& input) 
    : input_(input), position_(0), line_(1), column_(1) {
}

Token Tokenizer::next_token() {
    skip_whitespace();
    
    if (position_ >= input_.length()) {
        return Token(TokenType::END_OF_FILE, "", line_, column_);
    }
    
    char ch = current_char();
    
    // 字符串字面量
    if (ch == '\'' || ch == '"') {
        return read_string();
    }
    
    // 数字
    if (is_digit(ch)) {
        return read_number();
    }
    
    // 标识符或关键字
    if (is_alpha(ch) || ch == '_') {
        return read_identifier();
    }
    
    // 操作符和分隔符
    return read_operator();
}

Token Tokenizer::peek_token() {
    size_t saved_pos = position_;
    size_t saved_line = line_;
    size_t saved_col = column_;
    
    Token token = next_token();
    
    position_ = saved_pos;
    line_ = saved_line;
    column_ = saved_col;
    
    return token;
}

bool Tokenizer::is_end() const {
    return position_ >= input_.length();
}

std::vector<Token> Tokenizer::tokenize_all() {
    std::vector<Token> tokens;
    
    while (!is_end()) {
        Token token = next_token();
        if (token.type == TokenType::END_OF_FILE) {
            break;
        }
        tokens.push_back(token);
    }
    
    return tokens;
}

char Tokenizer::current_char() const {
    if (position_ >= input_.length()) {
        return '\0';
    }
    return input_[position_];
}

char Tokenizer::peek_char(size_t offset) const {
    size_t pos = position_ + offset;
    if (pos >= input_.length()) {
        return '\0';
    }
    return input_[pos];
}

void Tokenizer::advance() {
    if (position_ < input_.length()) {
        if (input_[position_] == '\n') {
            line_++;
            column_ = 1;
        } else {
            column_++;
        }
        position_++;
    }
}

void Tokenizer::skip_whitespace() {
    while (position_ < input_.length() && std::isspace(current_char())) {
        advance();
    }
}

void Tokenizer::skip_comment() {
    // 跳过单行注释 --
    if (current_char() == '-' && peek_char() == '-') {
        while (position_ < input_.length() && current_char() != '\n') {
            advance();
        }
    }
}

Token Tokenizer::read_string() {
    size_t start_line = line_;
    size_t start_col = column_;
    char quote_char = current_char();
    advance(); // 跳过开始引号
    
    std::string value;
    while (position_ < input_.length() && current_char() != quote_char) {
        if (current_char() == '\\') {
            advance();
            if (position_ < input_.length()) {
                char escaped = current_char();
                switch (escaped) {
                    case 'n': value += '\n'; break;
                    case 't': value += '\t'; break;
                    case 'r': value += '\r'; break;
                    case '\\': value += '\\'; break;
                    case '\'': value += '\''; break;
                    case '"': value += '"'; break;
                    default: value += escaped; break;
                }
                advance();
            }
        } else {
            value += current_char();
            advance();
        }
    }
    
    if (position_ >= input_.length()) {
        throw std::runtime_error("Unterminated string literal");
    }
    
    advance(); // 跳过结束引号
    return Token(TokenType::STRING, value, start_line, start_col);
}

Token Tokenizer::read_number() {
    size_t start_line = line_;
    size_t start_col = column_;
    std::string value;
    bool is_decimal = false;
    
    while (position_ < input_.length() && (is_digit(current_char()) || current_char() == '.')) {
        if (current_char() == '.') {
            if (is_decimal) {
                break; // 第二个小数点，停止
            }
            is_decimal = true;
        }
        value += current_char();
        advance();
    }
    
    TokenType type = is_decimal ? TokenType::DECIMAL : TokenType::INTEGER;
    return Token(type, value, start_line, start_col);
}

Token Tokenizer::read_identifier() {
    size_t start_line = line_;
    size_t start_col = column_;
    std::string value;
    
    while (position_ < input_.length() && (is_alnum(current_char()) || current_char() == '_')) {
        value += std::toupper(current_char()); // 转换为大写
        advance();
    }
    
    TokenType type = get_keyword_type(value);
    return Token(type, value, start_line, start_col);
}

Token Tokenizer::read_operator() {
    size_t start_line = line_;
    size_t start_col = column_;
    char ch = current_char();
    
    switch (ch) {
        case '+':
            advance();
            return Token(TokenType::PLUS, "+", start_line, start_col);
        case '-':
            advance();
            return Token(TokenType::MINUS, "-", start_line, start_col);
        case '*':
            advance();
            return Token(TokenType::MULTIPLY, "*", start_line, start_col);
        case '/':
            advance();
            return Token(TokenType::DIVIDE, "/", start_line, start_col);
        case '=':
            advance();
            return Token(TokenType::EQUAL, "=", start_line, start_col);
        case '!':
            if (peek_char() == '=') {
                advance();
                advance();
                return Token(TokenType::NOT_EQUAL, "!=", start_line, start_col);
            }
            break;
        case '<':
            advance();
            if (current_char() == '=') {
                advance();
                return Token(TokenType::LESS_EQUAL, "<=", start_line, start_col);
            }
            return Token(TokenType::LESS_THAN, "<", start_line, start_col);
        case '>':
            advance();
            if (current_char() == '=') {
                advance();
                return Token(TokenType::GREATER_EQUAL, ">=", start_line, start_col);
            }
            return Token(TokenType::GREATER_THAN, ">", start_line, start_col);
        case '(':
            advance();
            return Token(TokenType::LEFT_PAREN, "(", start_line, start_col);
        case ')':
            advance();
            return Token(TokenType::RIGHT_PAREN, ")", start_line, start_col);
        case ',':
            advance();
            return Token(TokenType::COMMA, ",", start_line, start_col);
        case ';':
            advance();
            return Token(TokenType::SEMICOLON, ";", start_line, start_col);
        case '.':
            advance();
            return Token(TokenType::DOT, ".", start_line, start_col);
        default:
            advance();
            return Token(TokenType::UNKNOWN, std::string(1, ch), start_line, start_col);
    }
    
    advance();
    return Token(TokenType::UNKNOWN, std::string(1, ch), start_line, start_col);
}

TokenType Tokenizer::get_keyword_type(const std::string& word) const {
    if (keywords_.count(word)) {
        if (word == "CREATE") return TokenType::CREATE;
        if (word == "DROP") return TokenType::DROP;
        if (word == "TABLE") return TokenType::TABLE;
        if (word == "INSERT") return TokenType::INSERT;
        if (word == "INTO") return TokenType::INTO;
        if (word == "VALUES") return TokenType::VALUES;
        if (word == "SELECT") return TokenType::SELECT;
        if (word == "FROM") return TokenType::FROM;
        if (word == "WHERE") return TokenType::WHERE;
        if (word == "DELETE") return TokenType::DELETE;
        if (word == "IF") return TokenType::IF;
        if (word == "NOT") return TokenType::NOT;
        if (word == "EXISTS") return TokenType::EXISTS;
        if (word == "AND") return TokenType::AND;
        if (word == "OR") return TokenType::OR;
        if (word == "TRUE" || word == "FALSE") return TokenType::BOOLEAN;
    }
    
    if (data_types_.count(word)) {
        if (word == "INT") return TokenType::INT;
        if (word == "STRING") return TokenType::STRING_TYPE;
        if (word == "BOOL") return TokenType::BOOL;
        if (word == "DECIMAL") return TokenType::DECIMAL_TYPE;
    }
    
    if (functions_.count(word)) {
        if (word == "SIN") return TokenType::SIN;
        if (word == "COS") return TokenType::COS;
        if (word == "SUBSTR") return TokenType::SUBSTR;
    }
    
    return TokenType::IDENTIFIER;
}

bool Tokenizer::is_alpha(char c) const {
    return std::isalpha(c);
}

bool Tokenizer::is_digit(char c) const {
    return std::isdigit(c);
}

bool Tokenizer::is_alnum(char c) const {
    return std::isalnum(c);
}

std::string TokenTypeToString(TokenType type) {
    switch (type) {
        case TokenType::INTEGER: return "INTEGER";
        case TokenType::DECIMAL: return "DECIMAL";
        case TokenType::STRING: return "STRING";
        case TokenType::BOOLEAN: return "BOOLEAN";
        case TokenType::IDENTIFIER: return "IDENTIFIER";
        case TokenType::CREATE: return "CREATE";
        case TokenType::DROP: return "DROP";
        case TokenType::TABLE: return "TABLE";
        case TokenType::INSERT: return "INSERT";
        case TokenType::INTO: return "INTO";
        case TokenType::VALUES: return "VALUES";
        case TokenType::SELECT: return "SELECT";
        case TokenType::FROM: return "FROM";
        case TokenType::WHERE: return "WHERE";
        case TokenType::DELETE: return "DELETE";
        case TokenType::IF: return "IF";
        case TokenType::NOT: return "NOT";
        case TokenType::EXISTS: return "EXISTS";
        case TokenType::AND: return "AND";
        case TokenType::OR: return "OR";
        case TokenType::INT: return "INT";
        case TokenType::STRING_TYPE: return "STRING_TYPE";
        case TokenType::BOOL: return "BOOL";
        case TokenType::DECIMAL_TYPE: return "DECIMAL_TYPE";
        case TokenType::SIN: return "SIN";
        case TokenType::COS: return "COS";
        case TokenType::SUBSTR: return "SUBSTR";
        case TokenType::PLUS: return "PLUS";
        case TokenType::MINUS: return "MINUS";
        case TokenType::MULTIPLY: return "MULTIPLY";
        case TokenType::DIVIDE: return "DIVIDE";
        case TokenType::EQUAL: return "EQUAL";
        case TokenType::NOT_EQUAL: return "NOT_EQUAL";
        case TokenType::LESS_THAN: return "LESS_THAN";
        case TokenType::LESS_EQUAL: return "LESS_EQUAL";
        case TokenType::GREATER_THAN: return "GREATER_THAN";
        case TokenType::GREATER_EQUAL: return "GREATER_EQUAL";
        case TokenType::LEFT_PAREN: return "LEFT_PAREN";
        case TokenType::RIGHT_PAREN: return "RIGHT_PAREN";
        case TokenType::COMMA: return "COMMA";
        case TokenType::SEMICOLON: return "SEMICOLON";
        case TokenType::DOT: return "DOT";
        case TokenType::WHITESPACE: return "WHITESPACE";
        case TokenType::NEWLINE: return "NEWLINE";
        case TokenType::END_OF_FILE: return "END_OF_FILE";
        case TokenType::UNKNOWN: return "UNKNOWN";
        default: return "UNKNOWN";
    }
}

} // namespace minidb

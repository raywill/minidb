#pragma once

#include "sql/parser/tokenizer.h"
#include "sql/ast/statements.h"
#include "common/status.h"
#include <memory>

namespace minidb {

// SQL语法分析器
class Parser {
public:
    explicit Parser(const std::string& sql);
    
    // 解析SQL语句
    Status parse(std::unique_ptr<Statement>& stmt);
    
    // 获取错误信息
    const std::string& get_error() const { return error_message_; }
    
private:
    Tokenizer tokenizer_;
    Token current_token_;
    std::string error_message_;
    
    // 辅助方法
    void advance();
    bool match(TokenType type);
    bool consume(TokenType type, const std::string& error_msg = "");
    void set_error(const std::string& message);
    
    // 解析方法
    std::unique_ptr<Statement> parse_statement();
    std::unique_ptr<CreateTableStatement> parse_create_table();
    std::unique_ptr<DropTableStatement> parse_drop_table();
    std::unique_ptr<InsertStatement> parse_insert();
    std::unique_ptr<SelectStatement> parse_select();
    std::unique_ptr<DeleteStatement> parse_delete();
    
    // 表达式解析
    std::unique_ptr<Expression> parse_expression();
    std::unique_ptr<Expression> parse_or_expression();
    std::unique_ptr<Expression> parse_and_expression();
    std::unique_ptr<Expression> parse_equality_expression();
    std::unique_ptr<Expression> parse_relational_expression();
    std::unique_ptr<Expression> parse_additive_expression();
    std::unique_ptr<Expression> parse_multiplicative_expression();
    std::unique_ptr<Expression> parse_unary_expression();
    std::unique_ptr<Expression> parse_primary_expression();
    std::unique_ptr<Expression> parse_function_call();
    
    // 其他解析方法
    std::unique_ptr<ColumnDefinition> parse_column_definition();
    std::unique_ptr<TableReference> parse_table_reference();
    std::vector<std::unique_ptr<Expression>> parse_expression_list();
    std::vector<std::string> parse_identifier_list();
    
    // 数据类型解析
    DataType parse_data_type();
    
    // 操作符转换
    BinaryOperatorType token_to_binary_operator(TokenType type);
    FunctionType token_to_function_type(TokenType type);
};

} // namespace minidb

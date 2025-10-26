#pragma once

#include "sql/parser/tokenizer.h"
#include "sql/ast/ast.h"
#include "common/status.h"
#include <memory>

namespace minidb {

// SQL语法分析器 - 只负责生成AST（纯语法树）
class SQLParser {
public:
    explicit SQLParser(const std::string& sql);

    // 解析SQL语句，返回AST
    Status parse(std::unique_ptr<StmtAST>& ast);

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

    // 解析语句
    std::unique_ptr<StmtAST> parse_statement();
    std::unique_ptr<CreateTableAST> parse_create_table();
    std::unique_ptr<DropTableAST> parse_drop_table();
    std::unique_ptr<InsertAST> parse_insert();
    std::unique_ptr<SelectAST> parse_select();
    std::unique_ptr<DeleteAST> parse_delete();

    // 解析表达式
    std::unique_ptr<ExprAST> parse_expression();
    std::unique_ptr<ExprAST> parse_or_expression();
    std::unique_ptr<ExprAST> parse_and_expression();
    std::unique_ptr<ExprAST> parse_equality_expression();
    std::unique_ptr<ExprAST> parse_relational_expression();
    std::unique_ptr<ExprAST> parse_additive_expression();
    std::unique_ptr<ExprAST> parse_multiplicative_expression();
    std::unique_ptr<ExprAST> parse_unary_expression();
    std::unique_ptr<ExprAST> parse_primary_expression();
    std::unique_ptr<ExprAST> parse_function_call();

    // 解析其他元素
    std::unique_ptr<ColumnDefAST> parse_column_definition();
    std::unique_ptr<TableRefAST> parse_table_reference();
    std::vector<std::unique_ptr<ExprAST>> parse_expression_list();
    std::vector<std::string> parse_identifier_list();

    // JOIN parsing
    JoinType parse_join_type();
    std::unique_ptr<JoinClauseAST> parse_join_clause();

    // 数据类型解析
    DataType parse_data_type();

    // 操作符转换
    BinaryOp token_to_binary_op(TokenType type);
    FuncType token_to_func_type(TokenType type);
};

} // namespace minidb

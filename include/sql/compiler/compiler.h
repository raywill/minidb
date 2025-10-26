#pragma once

#include "sql/ast/ast.h"
#include "sql/compiler/statement.h"
#include "storage/catalog.h"
#include "common/status.h"
#include <memory>

namespace minidb {

// SQL编译器 - 将AST转换为Statement（逻辑查询）
// 在这个过程中会解析表结构、列索引等语义信息
class Compiler {
public:
    explicit Compiler(Catalog* catalog);

    // 编译AST为Statement
    Status compile(StmtAST* ast, std::unique_ptr<Statement>& stmt);

private:
    Catalog* catalog_;
    Status last_error_;  // 保存最后的编译错误

    // 编译各种语句
    std::unique_ptr<CreateTableStatement> compile_create_table(CreateTableAST* ast);
    std::unique_ptr<DropTableStatement> compile_drop_table(DropTableAST* ast);
    std::unique_ptr<InsertStatement> compile_insert(InsertAST* ast);
    std::unique_ptr<SelectStatement> compile_select(SelectAST* ast);
    std::unique_ptr<DeleteStatement> compile_delete(DeleteAST* ast);

    // JOIN编译
    std::unique_ptr<SelectStatement> compile_select_with_join(
        SelectAST* ast,
        const std::string& from_table_name,
        const std::string& from_table_alias,
        const TableSchema& from_schema);

    // 编译表达式
    std::unique_ptr<Expression> compile_expression(ExprAST* ast, const TableSchema& schema);
    std::unique_ptr<LiteralExpression> compile_literal(LiteralAST* ast);
    std::unique_ptr<ColumnRefExpression> compile_column_ref(ColumnRefAST* ast, const TableSchema& schema);
    std::unique_ptr<BinaryExpression> compile_binary_op(BinaryOpAST* ast, const TableSchema& schema);
    std::unique_ptr<FunctionExpression> compile_function_call(FunctionCallAST* ast, const TableSchema& schema);

    // JOIN表达式编译（支持多表上下文）
    std::unique_ptr<Expression> compile_expression_multi_table(
        ExprAST* ast,
        const std::vector<TableSchema>& schemas,
        const std::vector<std::string>& aliases);
    std::unique_ptr<ColumnRefExpression> compile_column_ref_multi_table(
        ColumnRefAST* ast,
        const std::vector<TableSchema>& schemas,
        const std::vector<std::string>& aliases);
    std::unique_ptr<BinaryExpression> compile_binary_op_multi_table(
        BinaryOpAST* ast,
        const std::vector<TableSchema>& schemas,
        const std::vector<std::string>& aliases);

    // 辅助方法
    BinaryOperatorType convert_binary_op(BinaryOp op);
    FunctionType convert_function_type(FuncType func);
    JoinType convert_join_type(JoinType ast_join_type);

    // 查找列索引
    Status find_column_index(const TableSchema& schema, const std::string& col_name, size_t& index);

    // 类型推导
    DataType infer_binary_result_type(DataType left_type, DataType right_type, BinaryOperatorType op);
    DataType infer_function_result_type(FunctionType func, const std::vector<DataType>& arg_types);
};

} // namespace minidb

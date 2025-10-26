#include "sql/compiler/compiler.h"
#include "common/utils.h"
#include <iostream>
#include <algorithm>
#include <cctype>

namespace minidb {

// Helper function to convert string to uppercase
static std::string to_upper(const std::string& str) {
    std::string result = str;
    std::transform(result.begin(), result.end(), result.begin(),
                   [](unsigned char c) { return std::toupper(c); });
    return result;
}

Compiler::Compiler(Catalog* catalog) : catalog_(catalog) {}

Status Compiler::compile(StmtAST* ast, std::unique_ptr<Statement>& stmt) {
    if (!ast) {
        return Status::InvalidArgument("NULL AST");
    }

    last_error_ = Status::OK();  // 重置错误状态

    switch (ast->get_type()) {
        case ASTType::CREATE_TABLE:
            stmt = compile_create_table(static_cast<CreateTableAST*>(ast));
            break;
        case ASTType::DROP_TABLE:
            stmt = compile_drop_table(static_cast<DropTableAST*>(ast));
            break;
        case ASTType::INSERT:
            stmt = compile_insert(static_cast<InsertAST*>(ast));
            break;
        case ASTType::SELECT:
            stmt = compile_select(static_cast<SelectAST*>(ast));
            break;
        case ASTType::DELETE:
            stmt = compile_delete(static_cast<DeleteAST*>(ast));
            break;
        default:
            return Status::InvalidArgument("Unsupported AST type");
    }

    if (!stmt) {
        // 如果有保存的错误，返回它；否则返回通用错误
        if (!last_error_.ok()) {
            return last_error_;
        }
        return Status::InvalidArgument("Failed to compile AST");
    }

    return Status::OK();
}

// ============= DDL语句编译 =============
std::unique_ptr<CreateTableStatement> Compiler::compile_create_table(CreateTableAST* ast) {
    std::vector<ColumnDefinition> columns;

    for (const auto& col_ast : ast->get_columns()) {
        columns.emplace_back(col_ast->get_column_name(), col_ast->get_data_type());
    }

    return make_unique<CreateTableStatement>(
        ast->get_table_name(),
        std::move(columns),
        ast->get_if_not_exists()
    );
}

std::unique_ptr<DropTableStatement> Compiler::compile_drop_table(DropTableAST* ast) {
    return make_unique<DropTableStatement>(
        ast->get_table_name(),
        ast->get_if_exists()
    );
}

// ============= DML语句编译 =============
std::unique_ptr<InsertStatement> Compiler::compile_insert(InsertAST* ast) {
    // 获取表模式
    TableMetadata metadata;
    Status status = catalog_->get_table_metadata(ast->get_table_name(), metadata);
    if (!status.ok()) {
        last_error_ = status;  // 保存错误状态
        return nullptr; // 表不存在
    }
    const TableSchema& schema = metadata.schema;

    // 解析列名和索引
    std::vector<std::string> column_names;
    std::vector<size_t> column_indices;

    if (ast->get_columns().empty()) {
        // 如果没有指定列，使用所有列
        column_names = schema.column_names;
        for (size_t i = 0; i < schema.get_column_count(); ++i) {
            column_indices.push_back(i);
        }
    } else {
        // 解析指定的列
        for (const auto& col_name : ast->get_columns()) {
            size_t index;
            if (!find_column_index(schema, col_name, index).ok()) {
                return nullptr; // 列不存在
            }
            column_names.push_back(col_name);
            column_indices.push_back(index);
        }
    }

    // 编译值表达式
    std::vector<std::vector<std::unique_ptr<Expression>>> values;
    for (const auto& value_row : ast->get_values()) {
        // 验证值的数量与列数匹配
        if (value_row.size() != column_names.size()) {
            last_error_ = Status::InvalidArgument(
                "Column count mismatch: expected " + std::to_string(column_names.size()) +
                " but got " + std::to_string(value_row.size()));
            return nullptr;
        }

        std::vector<std::unique_ptr<Expression>> compiled_row;
        for (const auto& expr_ast : value_row) {
            auto compiled_expr = compile_expression(expr_ast.get(), schema);
            if (!compiled_expr) return nullptr;
            compiled_row.push_back(std::move(compiled_expr));
        }
        values.push_back(std::move(compiled_row));
    }

    return make_unique<InsertStatement>(
        ast->get_table_name(),
        std::move(column_names),
        std::move(column_indices),
        std::move(values)
    );
}

std::unique_ptr<SelectStatement> Compiler::compile_select(SelectAST* ast) {
    // 获取表名
    if (!ast->get_from_table()) {
        last_error_ = Status::InvalidArgument("SELECT requires FROM clause");
        return nullptr; // 需要FROM子句
    }
    std::string table_name = ast->get_from_table()->get_table_name();
    std::string table_alias = ast->get_from_table()->has_alias() ?
                               ast->get_from_table()->get_alias() : "";

    // 获取表模式
    TableMetadata metadata;
    Status status = catalog_->get_table_metadata(table_name, metadata);
    if (!status.ok()) {
        last_error_ = status;  // 保存错误状态
        return nullptr; // 表不存在
    }
    const TableSchema& schema = metadata.schema;

    // Check if this is a JOIN query
    if (!ast->get_joins().empty()) {
        return compile_select_with_join(ast, table_name, table_alias, schema);
    }

    // 解析选择列（单表查询）
    std::vector<std::string> select_columns;
    std::vector<size_t> select_column_indices;

    for (const auto& expr_ast : ast->get_select_list()) {
        if (expr_ast->get_type() == ASTType::COLUMN_REF) {
            ColumnRefAST* col_ref = static_cast<ColumnRefAST*>(expr_ast.get());
            if (col_ref->get_column_name() == "*") {
                // SELECT * - 选择所有列
                select_columns = schema.column_names;
                select_column_indices.clear();
                for (size_t i = 0; i < schema.get_column_count(); ++i) {
                    select_column_indices.push_back(i);
                }
                break;
            } else {
                // 具体列
                size_t index;
                if (!find_column_index(schema, col_ref->get_column_name(), index).ok()) {
                    return nullptr;
                }
                // 使用schema中的原始列名（保持大小写一致性）
                select_columns.push_back(schema.column_names[index]);
                select_column_indices.push_back(index);
            }
        }
    }

    // 编译WHERE子句
    std::unique_ptr<Expression> where_clause;
    if (ast->get_where_clause()) {
        where_clause = compile_expression(ast->get_where_clause(), schema);
        if (!where_clause) return nullptr;
    }

    return make_unique<SelectStatement>(
        table_name,
        std::move(select_columns),
        std::move(select_column_indices),
        std::move(where_clause)
    );
}

std::unique_ptr<DeleteStatement> Compiler::compile_delete(DeleteAST* ast) {
    std::string table_name = ast->get_from_table()->get_table_name();

    // 获取表模式
    TableMetadata metadata;
    Status status = catalog_->get_table_metadata(table_name, metadata);
    if (!status.ok()) {
        last_error_ = status;  // 保存错误状态
        return nullptr; // 表不存在
    }
    const TableSchema& schema = metadata.schema;

    // 编译WHERE子句
    std::unique_ptr<Expression> where_clause;
    if (ast->get_where_clause()) {
        where_clause = compile_expression(ast->get_where_clause(), schema);
        if (!where_clause) return nullptr;
    }

    return make_unique<DeleteStatement>(table_name, std::move(where_clause));
}

// ============= 表达式编译 =============
std::unique_ptr<Expression> Compiler::compile_expression(ExprAST* ast, const TableSchema& schema) {
    if (!ast) return nullptr;

    switch (ast->get_type()) {
        case ASTType::LITERAL:
            return compile_literal(static_cast<LiteralAST*>(ast));
        case ASTType::COLUMN_REF:
            return compile_column_ref(static_cast<ColumnRefAST*>(ast), schema);
        case ASTType::BINARY_OP:
            return compile_binary_op(static_cast<BinaryOpAST*>(ast), schema);
        case ASTType::FUNCTION_CALL:
            return compile_function_call(static_cast<FunctionCallAST*>(ast), schema);
        default:
            return nullptr;
    }
}

std::unique_ptr<LiteralExpression> Compiler::compile_literal(LiteralAST* ast) {
    return make_unique<LiteralExpression>(ast->get_data_type(), ast->get_value());
}

std::unique_ptr<ColumnRefExpression> Compiler::compile_column_ref(ColumnRefAST* ast, const TableSchema& schema) {
    size_t index;
    Status status = find_column_index(schema, ast->get_column_name(), index);
    if (!status.ok()) {
        return nullptr;
    }

    std::string table_name = ast->get_table_name().empty() ? schema.table_name : ast->get_table_name();
    return make_unique<ColumnRefExpression>(table_name, ast->get_column_name(), index);
}

std::unique_ptr<BinaryExpression> Compiler::compile_binary_op(BinaryOpAST* ast, const TableSchema& schema) {
    auto left = compile_expression(ast->get_left(), schema);
    if (!left) return nullptr;

    auto right = compile_expression(ast->get_right(), schema);
    if (!right) return nullptr;

    BinaryOperatorType op = convert_binary_op(ast->get_op());
    return make_unique<BinaryExpression>(op, std::move(left), std::move(right));
}

std::unique_ptr<FunctionExpression> Compiler::compile_function_call(FunctionCallAST* ast, const TableSchema& schema) {
    std::vector<std::unique_ptr<Expression>> args;
    for (const auto& arg_ast : ast->get_args()) {
        auto arg = compile_expression(arg_ast.get(), schema);
        if (!arg) return nullptr;
        args.push_back(std::move(arg));
    }

    FunctionType func = convert_function_type(ast->get_func_type());
    return make_unique<FunctionExpression>(func, std::move(args));
}

// ============= 辅助方法 =============
BinaryOperatorType Compiler::convert_binary_op(BinaryOp op) {
    switch (op) {
        case BinaryOp::ADD: return BinaryOperatorType::ADD;
        case BinaryOp::SUBTRACT: return BinaryOperatorType::SUBTRACT;
        case BinaryOp::MULTIPLY: return BinaryOperatorType::MULTIPLY;
        case BinaryOp::DIVIDE: return BinaryOperatorType::DIVIDE;
        case BinaryOp::EQUAL: return BinaryOperatorType::EQUAL;
        case BinaryOp::NOT_EQUAL: return BinaryOperatorType::NOT_EQUAL;
        case BinaryOp::LESS_THAN: return BinaryOperatorType::LESS_THAN;
        case BinaryOp::LESS_EQUAL: return BinaryOperatorType::LESS_EQUAL;
        case BinaryOp::GREATER_THAN: return BinaryOperatorType::GREATER_THAN;
        case BinaryOp::GREATER_EQUAL: return BinaryOperatorType::GREATER_EQUAL;
        case BinaryOp::AND: return BinaryOperatorType::AND;
        case BinaryOp::OR: return BinaryOperatorType::OR;
        default: return BinaryOperatorType::EQUAL;
    }
}

FunctionType Compiler::convert_function_type(FuncType func) {
    switch (func) {
        case FuncType::SIN: return FunctionType::SIN;
        case FuncType::COS: return FunctionType::COS;
        case FuncType::SUBSTR: return FunctionType::SUBSTR;
        default: return FunctionType::SIN;
    }
}

Status Compiler::find_column_index(const TableSchema& schema, const std::string& col_name, size_t& index) {
    // 大小写不敏感的列名比较
    std::string col_name_upper = to_upper(col_name);
    for (size_t i = 0; i < schema.column_names.size(); ++i) {
        std::string schema_col_upper = to_upper(schema.column_names[i]);
        if (schema_col_upper == col_name_upper) {
            index = i;
            return Status::OK();
        }
    }
    return Status::NotFound("Column '" + col_name + "' not found in table '" + schema.table_name + "'");
}

// ============= JOIN编译 =============
JoinType Compiler::convert_join_type(JoinType ast_join_type) {
    // Since JoinType is now shared between AST and Statement, no conversion needed
    return ast_join_type;
}

std::unique_ptr<SelectStatement> Compiler::compile_select_with_join(
    SelectAST* ast,
    const std::string& from_table_name,
    const std::string& from_table_alias,
    const TableSchema& from_schema) {

    // 收集所有表的schema和alias
    std::vector<TableSchema> all_schemas;
    std::vector<std::string> all_aliases;

    // 添加FROM表
    all_schemas.push_back(from_schema);
    all_aliases.push_back(from_table_alias.empty() ? from_table_name : from_table_alias);

    // 编译JOIN子句
    std::vector<JoinInfo> join_infos;
    for (const auto& join_ast : ast->get_joins()) {
        // 获取JOIN表的schema
        std::string join_table_name = join_ast->get_right_table()->get_table_name();
        std::string join_table_alias = join_ast->get_right_table()->has_alias() ?
                                       join_ast->get_right_table()->get_alias() : "";

        TableMetadata join_metadata;
        Status status = catalog_->get_table_metadata(join_table_name, join_metadata);
        if (!status.ok()) {
            last_error_ = status;
            return nullptr;
        }
        const TableSchema& join_schema = join_metadata.schema;

        // 添加到schemas列表
        all_schemas.push_back(join_schema);
        all_aliases.push_back(join_table_alias.empty() ? join_table_name : join_table_alias);

        // 编译JOIN条件
        auto condition = compile_expression_multi_table(
            join_ast->get_condition(), all_schemas, all_aliases);
        if (!condition) return nullptr;

        // 创建JoinInfo
        JoinInfo join_info(
            join_table_name,
            join_table_alias,
            convert_join_type(join_ast->get_join_type()),
            std::move(condition)
        );

        // 填充列信息
        join_info.column_names = join_schema.column_names;
        join_info.column_types = join_schema.column_types;

        join_infos.push_back(std::move(join_info));
    }

    // 解析SELECT列（支持多表）
    std::vector<std::string> select_columns;
    std::vector<size_t> select_column_indices;

    for (const auto& expr_ast : ast->get_select_list()) {
        if (expr_ast->get_type() == ASTType::COLUMN_REF) {
            ColumnRefAST* col_ref = static_cast<ColumnRefAST*>(expr_ast.get());
            if (col_ref->get_column_name() == "*") {
                // SELECT * - 选择所有表的所有列
                for (const auto& schema : all_schemas) {
                    for (const auto& col_name : schema.column_names) {
                        select_columns.push_back(col_name);
                        // 列索引需要在执行时根据表的顺序计算
                        select_column_indices.push_back(0); // Placeholder
                    }
                }
                break;
            } else {
                // 具体列 - 需要在多表中解析
                auto col_expr = compile_column_ref_multi_table(col_ref, all_schemas, all_aliases);
                if (!col_expr) return nullptr;

                select_columns.push_back(col_expr->get_column_name());
                select_column_indices.push_back(col_expr->get_column_index());
            }
        }
    }

    // 编译WHERE子句（支持多表）
    std::unique_ptr<Expression> where_clause;
    if (ast->get_where_clause()) {
        where_clause = compile_expression_multi_table(
            ast->get_where_clause(), all_schemas, all_aliases);
        if (!where_clause) return nullptr;
    }

    return make_unique<SelectStatement>(
        from_table_name,
        from_table_alias,
        std::move(join_infos),
        std::move(select_columns),
        std::move(select_column_indices),
        std::move(where_clause)
    );
}

// ============= 多表表达式编译 =============
std::unique_ptr<Expression> Compiler::compile_expression_multi_table(
    ExprAST* ast,
    const std::vector<TableSchema>& schemas,
    const std::vector<std::string>& aliases) {

    if (!ast) return nullptr;

    switch (ast->get_type()) {
        case ASTType::LITERAL:
            return compile_literal(static_cast<LiteralAST*>(ast));
        case ASTType::COLUMN_REF:
            return compile_column_ref_multi_table(
                static_cast<ColumnRefAST*>(ast), schemas, aliases);
        case ASTType::BINARY_OP:
            return compile_binary_op_multi_table(
                static_cast<BinaryOpAST*>(ast), schemas, aliases);
        case ASTType::FUNCTION_CALL:
            // 函数调用暂不支持多表
            last_error_ = Status::InvalidArgument("Function calls in JOIN conditions are not yet supported");
            return nullptr;
        default:
            return nullptr;
    }
}

std::unique_ptr<ColumnRefExpression> Compiler::compile_column_ref_multi_table(
    ColumnRefAST* ast,
    const std::vector<TableSchema>& schemas,
    const std::vector<std::string>& aliases) {

    std::string table_qualifier = ast->get_table_name();
    std::string column_name = ast->get_column_name();

    // 如果列名有表限定符（如 t1.id）
    if (!table_qualifier.empty()) {
        // 查找对应的表
        std::string table_qualifier_upper = to_upper(table_qualifier);
        for (size_t i = 0; i < schemas.size(); ++i) {
            std::string alias_upper = to_upper(aliases[i]);
            std::string table_name_upper = to_upper(schemas[i].table_name);

            if (alias_upper == table_qualifier_upper || table_name_upper == table_qualifier_upper) {
                // 找到了表，查找列
                size_t col_index;
                Status status = find_column_index(schemas[i], column_name, col_index);
                if (!status.ok()) {
                    last_error_ = status;
                    return nullptr;
                }
                return make_unique<ColumnRefExpression>(
                    schemas[i].table_name, column_name, col_index);
            }
        }
        // 表限定符不存在
        last_error_ = Status::NotFound("Table or alias '" + table_qualifier + "' not found");
        return nullptr;
    }

    // 如果列名没有表限定符，需要在所有表中查找
    // 检查是否有歧义（同名列出现在多个表中）
    int found_count = 0;
    size_t found_table_idx = 0;
    size_t found_col_idx = 0;

    for (size_t i = 0; i < schemas.size(); ++i) {
        size_t col_idx;
        if (find_column_index(schemas[i], column_name, col_idx).ok()) {
            found_count++;
            found_table_idx = i;
            found_col_idx = col_idx;
        }
    }

    if (found_count == 0) {
        last_error_ = Status::NotFound("Column '" + column_name + "' not found in any table");
        return nullptr;
    }

    if (found_count > 1) {
        last_error_ = Status::InvalidArgument(
            "Column '" + column_name + "' is ambiguous (found in multiple tables)");
        return nullptr;
    }

    // 找到唯一的列
    return make_unique<ColumnRefExpression>(
        schemas[found_table_idx].table_name,
        column_name,
        found_col_idx);
}

std::unique_ptr<BinaryExpression> Compiler::compile_binary_op_multi_table(
    BinaryOpAST* ast,
    const std::vector<TableSchema>& schemas,
    const std::vector<std::string>& aliases) {

    auto left = compile_expression_multi_table(ast->get_left(), schemas, aliases);
    if (!left) return nullptr;

    auto right = compile_expression_multi_table(ast->get_right(), schemas, aliases);
    if (!right) return nullptr;

    BinaryOperatorType op = convert_binary_op(ast->get_op());
    return make_unique<BinaryExpression>(op, std::move(left), std::move(right));
}

} // namespace minidb

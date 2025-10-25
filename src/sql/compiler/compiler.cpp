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

    // 获取表模式
    TableMetadata metadata;
    Status status = catalog_->get_table_metadata(table_name, metadata);
    if (!status.ok()) {
        last_error_ = status;  // 保存错误状态
        return nullptr; // 表不存在
    }
    const TableSchema& schema = metadata.schema;

    // 解析选择列
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

} // namespace minidb

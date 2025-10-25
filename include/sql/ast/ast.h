#pragma once

#include "common/types.h"
#include <memory>
#include <vector>
#include <string>

namespace minidb {

// AST节点类型（纯语法树节点）
enum class ASTType {
    // 语句类型
    CREATE_TABLE,
    DROP_TABLE,
    INSERT,
    SELECT,
    DELETE,

    // 表达式类型
    LITERAL,
    COLUMN_REF,
    BINARY_OP,
    FUNCTION_CALL,

    // 子句类型
    TABLE_REF,
    COLUMN_DEF,
    WHERE_CLAUSE,
    ORDER_BY_CLAUSE
};

// 二元操作符
enum class BinaryOp {
    ADD, SUBTRACT, MULTIPLY, DIVIDE,
    EQUAL, NOT_EQUAL,
    LESS_THAN, LESS_EQUAL,
    GREATER_THAN, GREATER_EQUAL,
    AND, OR
};

// 函数类型
enum class FuncType {
    SIN, COS, SUBSTR
};

// ============= AST基类 =============
class ASTNode {
public:
    explicit ASTNode(ASTType type) : type_(type) {}
    virtual ~ASTNode() = default;

    ASTType get_type() const { return type_; }
    virtual std::string to_string() const = 0;

private:
    ASTType type_;
};

// ============= 表达式节点 =============
class ExprAST : public ASTNode {
public:
    explicit ExprAST(ASTType type) : ASTNode(type) {}
};

// 字面量
class LiteralAST : public ExprAST {
public:
    LiteralAST(DataType dtype, const std::string& val)
        : ExprAST(ASTType::LITERAL), data_type_(dtype), value_(val) {}

    DataType get_data_type() const { return data_type_; }
    const std::string& get_value() const { return value_; }
    std::string to_string() const override;

private:
    DataType data_type_;
    std::string value_;
};

// 列引用
class ColumnRefAST : public ExprAST {
public:
    explicit ColumnRefAST(const std::string& col_name)
        : ExprAST(ASTType::COLUMN_REF), column_name_(col_name) {}

    ColumnRefAST(const std::string& tbl_name, const std::string& col_name)
        : ExprAST(ASTType::COLUMN_REF), table_name_(tbl_name), column_name_(col_name) {}

    const std::string& get_table_name() const { return table_name_; }
    const std::string& get_column_name() const { return column_name_; }
    std::string to_string() const override;

private:
    std::string table_name_;
    std::string column_name_;
};

// 二元表达式
class BinaryOpAST : public ExprAST {
public:
    BinaryOpAST(BinaryOp op, std::unique_ptr<ExprAST> left, std::unique_ptr<ExprAST> right)
        : ExprAST(ASTType::BINARY_OP), op_(op), left_(std::move(left)), right_(std::move(right)) {}

    BinaryOp get_op() const { return op_; }
    ExprAST* get_left() const { return left_.get(); }
    ExprAST* get_right() const { return right_.get(); }
    std::string to_string() const override;

private:
    BinaryOp op_;
    std::unique_ptr<ExprAST> left_;
    std::unique_ptr<ExprAST> right_;
};

// 函数调用
class FunctionCallAST : public ExprAST {
public:
    FunctionCallAST(FuncType func, std::vector<std::unique_ptr<ExprAST>> args)
        : ExprAST(ASTType::FUNCTION_CALL), func_type_(func), args_(std::move(args)) {}

    FuncType get_func_type() const { return func_type_; }
    const std::vector<std::unique_ptr<ExprAST>>& get_args() const { return args_; }
    std::string to_string() const override;

private:
    FuncType func_type_;
    std::vector<std::unique_ptr<ExprAST>> args_;
};

// ============= 其他节点 =============
// 列定义
class ColumnDefAST : public ASTNode {
public:
    ColumnDefAST(const std::string& name, DataType type)
        : ASTNode(ASTType::COLUMN_DEF), column_name_(name), data_type_(type) {}

    const std::string& get_column_name() const { return column_name_; }
    DataType get_data_type() const { return data_type_; }
    std::string to_string() const override;

private:
    std::string column_name_;
    DataType data_type_;
};

// 表引用
class TableRefAST : public ASTNode {
public:
    explicit TableRefAST(const std::string& name)
        : ASTNode(ASTType::TABLE_REF), table_name_(name) {}

    const std::string& get_table_name() const { return table_name_; }
    std::string to_string() const override;

private:
    std::string table_name_;
};

// ============= 语句节点 =============
class StmtAST : public ASTNode {
public:
    explicit StmtAST(ASTType type) : ASTNode(type) {}
};

// CREATE TABLE
class CreateTableAST : public StmtAST {
public:
    CreateTableAST(const std::string& name,
                   std::vector<std::unique_ptr<ColumnDefAST>> cols,
                   bool if_not_exists = false)
        : StmtAST(ASTType::CREATE_TABLE),
          table_name_(name), columns_(std::move(cols)), if_not_exists_(if_not_exists) {}

    const std::string& get_table_name() const { return table_name_; }
    const std::vector<std::unique_ptr<ColumnDefAST>>& get_columns() const { return columns_; }
    bool get_if_not_exists() const { return if_not_exists_; }
    std::string to_string() const override;

private:
    std::string table_name_;
    std::vector<std::unique_ptr<ColumnDefAST>> columns_;
    bool if_not_exists_;
};

// DROP TABLE
class DropTableAST : public StmtAST {
public:
    DropTableAST(const std::string& name, bool if_exists = false)
        : StmtAST(ASTType::DROP_TABLE), table_name_(name), if_exists_(if_exists) {}

    const std::string& get_table_name() const { return table_name_; }
    bool get_if_exists() const { return if_exists_; }
    std::string to_string() const override;

private:
    std::string table_name_;
    bool if_exists_;
};

// INSERT
class InsertAST : public StmtAST {
public:
    InsertAST(const std::string& name,
              std::vector<std::string> cols,
              std::vector<std::vector<std::unique_ptr<ExprAST>>> vals)
        : StmtAST(ASTType::INSERT),
          table_name_(name), columns_(std::move(cols)), values_(std::move(vals)) {}

    const std::string& get_table_name() const { return table_name_; }
    const std::vector<std::string>& get_columns() const { return columns_; }
    const std::vector<std::vector<std::unique_ptr<ExprAST>>>& get_values() const { return values_; }
    std::string to_string() const override;

private:
    std::string table_name_;
    std::vector<std::string> columns_;
    std::vector<std::vector<std::unique_ptr<ExprAST>>> values_;
};

// SELECT
class SelectAST : public StmtAST {
public:
    SelectAST(std::vector<std::unique_ptr<ExprAST>> select_list,
              std::unique_ptr<TableRefAST> from,
              std::unique_ptr<ExprAST> where = nullptr)
        : StmtAST(ASTType::SELECT),
          select_list_(std::move(select_list)),
          from_table_(std::move(from)),
          where_clause_(std::move(where)) {}

    const std::vector<std::unique_ptr<ExprAST>>& get_select_list() const { return select_list_; }
    TableRefAST* get_from_table() const { return from_table_.get(); }
    ExprAST* get_where_clause() const { return where_clause_.get(); }
    std::string to_string() const override;

private:
    std::vector<std::unique_ptr<ExprAST>> select_list_;
    std::unique_ptr<TableRefAST> from_table_;
    std::unique_ptr<ExprAST> where_clause_;
};

// DELETE
class DeleteAST : public StmtAST {
public:
    DeleteAST(std::unique_ptr<TableRefAST> from, std::unique_ptr<ExprAST> where = nullptr)
        : StmtAST(ASTType::DELETE),
          from_table_(std::move(from)),
          where_clause_(std::move(where)) {}

    TableRefAST* get_from_table() const { return from_table_.get(); }
    ExprAST* get_where_clause() const { return where_clause_.get(); }
    std::string to_string() const override;

private:
    std::unique_ptr<TableRefAST> from_table_;
    std::unique_ptr<ExprAST> where_clause_;
};

} // namespace minidb

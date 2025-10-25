#pragma once

#include "common/types.h"
#include <memory>
#include <vector>
#include <string>

namespace minidb {

// 语句类型
enum class StatementType {
    CREATE_TABLE,
    DROP_TABLE,
    INSERT,
    SELECT,
    DELETE
};

// ============= 表达式（逻辑表达式） =============
class Expression {
public:
    virtual ~Expression() = default;
    virtual std::string to_string() const = 0;
    virtual std::unique_ptr<Expression> clone() const = 0;

    // 提供虚方法以支持RTTI和dynamic_cast
    virtual bool is_literal() const { return false; }
    virtual bool is_column_ref() const { return false; }
    virtual bool is_binary() const { return false; }
    virtual bool is_function() const { return false; }
};

// 字面量表达式
class LiteralExpression : public Expression {
public:
    LiteralExpression(DataType type, const std::string& value)
        : data_type_(type), value_(value) {}

    DataType get_data_type() const { return data_type_; }
    const std::string& get_value() const { return value_; }
    std::string to_string() const override;
    std::unique_ptr<Expression> clone() const override;

private:
    DataType data_type_;
    std::string value_;
};

// 列引用表达式
class ColumnRefExpression : public Expression {
public:
    ColumnRefExpression(const std::string& table, const std::string& column, size_t column_idx)
        : table_name_(table), column_name_(column), column_index_(column_idx) {}

    const std::string& get_table_name() const { return table_name_; }
    const std::string& get_column_name() const { return column_name_; }
    size_t get_column_index() const { return column_index_; }
    std::string to_string() const override;
    std::unique_ptr<Expression> clone() const override;

private:
    std::string table_name_;
    std::string column_name_;
    size_t column_index_;  // 编译后的列索引
};

// 二元操作符类型
enum class BinaryOperatorType {
    ADD, SUBTRACT, MULTIPLY, DIVIDE,
    EQUAL, NOT_EQUAL,
    LESS_THAN, LESS_EQUAL,
    GREATER_THAN, GREATER_EQUAL,
    AND, OR
};

// 二元表达式
class BinaryExpression : public Expression {
public:
    BinaryExpression(BinaryOperatorType op,
                    std::unique_ptr<Expression> left,
                    std::unique_ptr<Expression> right)
        : operator_(op), left_(std::move(left)), right_(std::move(right)) {}

    BinaryOperatorType get_operator() const { return operator_; }
    Expression* get_left() const { return left_.get(); }
    Expression* get_right() const { return right_.get(); }
    std::string to_string() const override;
    std::unique_ptr<Expression> clone() const override;

private:
    BinaryOperatorType operator_;
    std::unique_ptr<Expression> left_;
    std::unique_ptr<Expression> right_;
};

// 函数表达式
enum class FunctionType {
    SIN, COS, SUBSTR
};

class FunctionExpression : public Expression {
public:
    FunctionExpression(FunctionType func, std::vector<std::unique_ptr<Expression>> args)
        : function_type_(func), arguments_(std::move(args)) {}

    FunctionType get_function_type() const { return function_type_; }
    const std::vector<std::unique_ptr<Expression>>& get_arguments() const { return arguments_; }
    std::string to_string() const override;
    std::unique_ptr<Expression> clone() const override;

private:
    FunctionType function_type_;
    std::vector<std::unique_ptr<Expression>> arguments_;
};

// ============= Statement 基类 =============
class Statement {
public:
    explicit Statement(StatementType type) : type_(type) {}
    virtual ~Statement() = default;

    StatementType get_type() const { return type_; }
    virtual std::string to_string() const = 0;

private:
    StatementType type_;
};

// ============= CREATE TABLE Statement =============
struct ColumnDefinition {
    std::string name;
    DataType type;

    ColumnDefinition(const std::string& n, DataType t) : name(n), type(t) {}
};

class CreateTableStatement : public Statement {
public:
    CreateTableStatement(const std::string& table_name,
                        std::vector<ColumnDefinition> columns,
                        bool if_not_exists = false)
        : Statement(StatementType::CREATE_TABLE),
          table_name_(table_name),
          columns_(std::move(columns)),
          if_not_exists_(if_not_exists) {}

    const std::string& get_table_name() const { return table_name_; }
    const std::vector<ColumnDefinition>& get_columns() const { return columns_; }
    bool get_if_not_exists() const { return if_not_exists_; }
    std::string to_string() const override;

private:
    std::string table_name_;
    std::vector<ColumnDefinition> columns_;
    bool if_not_exists_;
};

// ============= DROP TABLE Statement =============
class DropTableStatement : public Statement {
public:
    DropTableStatement(const std::string& table_name, bool if_exists = false)
        : Statement(StatementType::DROP_TABLE),
          table_name_(table_name),
          if_exists_(if_exists) {}

    const std::string& get_table_name() const { return table_name_; }
    bool get_if_exists() const { return if_exists_; }
    std::string to_string() const override;

private:
    std::string table_name_;
    bool if_exists_;
};

// ============= INSERT Statement =============
class InsertStatement : public Statement {
public:
    InsertStatement(const std::string& table_name,
                   std::vector<std::string> column_names,
                   std::vector<size_t> column_indices,
                   std::vector<std::vector<std::unique_ptr<Expression>>> values)
        : Statement(StatementType::INSERT),
          table_name_(table_name),
          column_names_(std::move(column_names)),
          column_indices_(std::move(column_indices)),
          values_(std::move(values)) {}

    const std::string& get_table_name() const { return table_name_; }
    const std::vector<std::string>& get_column_names() const { return column_names_; }
    const std::vector<size_t>& get_column_indices() const { return column_indices_; }
    const std::vector<std::vector<std::unique_ptr<Expression>>>& get_values() const { return values_; }
    std::string to_string() const override;

private:
    std::string table_name_;
    std::vector<std::string> column_names_;
    std::vector<size_t> column_indices_;  // 编译后的列索引
    std::vector<std::vector<std::unique_ptr<Expression>>> values_;
};

// ============= SELECT Statement =============
class SelectStatement : public Statement {
public:
    SelectStatement(const std::string& table_name,
                   std::vector<std::string> select_columns,
                   std::vector<size_t> select_column_indices,
                   std::unique_ptr<Expression> where_clause = nullptr)
        : Statement(StatementType::SELECT),
          table_name_(table_name),
          select_columns_(std::move(select_columns)),
          select_column_indices_(std::move(select_column_indices)),
          where_clause_(std::move(where_clause)) {}

    const std::string& get_table_name() const { return table_name_; }
    const std::vector<std::string>& get_select_columns() const { return select_columns_; }
    const std::vector<size_t>& get_select_column_indices() const { return select_column_indices_; }
    Expression* get_where_clause() const { return where_clause_.get(); }
    std::string to_string() const override;

private:
    std::string table_name_;
    std::vector<std::string> select_columns_;
    std::vector<size_t> select_column_indices_;  // 编译后的列索引
    std::unique_ptr<Expression> where_clause_;
};

// ============= DELETE Statement =============
class DeleteStatement : public Statement {
public:
    DeleteStatement(const std::string& table_name,
                   std::unique_ptr<Expression> where_clause = nullptr)
        : Statement(StatementType::DELETE),
          table_name_(table_name),
          where_clause_(std::move(where_clause)) {}

    const std::string& get_table_name() const { return table_name_; }
    Expression* get_where_clause() const { return where_clause_.get(); }
    std::string to_string() const override;

private:
    std::string table_name_;
    std::unique_ptr<Expression> where_clause_;
};

} // namespace minidb

#pragma once

#include "common/types.h"
#include <memory>
#include <vector>
#include <string>

namespace minidb {

// AST节点类型枚举
enum class ASTNodeType {
    // 语句类型
    CREATE_TABLE_STMT,
    DROP_TABLE_STMT,
    INSERT_STMT,
    SELECT_STMT,
    DELETE_STMT,
    
    // 表达式类型
    LITERAL_EXPR,
    COLUMN_REF_EXPR,
    BINARY_EXPR,
    FUNCTION_EXPR,
    
    // 其他类型
    TABLE_REF,
    COLUMN_DEF,
    WHERE_CLAUSE,
    ORDER_BY_CLAUSE
};

// 二元操作符类型
enum class BinaryOperatorType {
    ADD,        // +
    SUBTRACT,   // -
    MULTIPLY,   // *
    DIVIDE,     // /
    EQUAL,      // =
    NOT_EQUAL,  // !=
    LESS_THAN,  // <
    LESS_EQUAL, // <=
    GREATER_THAN,   // >
    GREATER_EQUAL,  // >=
    AND,        // AND
    OR          // OR
};

// 函数类型
enum class FunctionType {
    SIN,
    COS,
    SUBSTR
};

// AST节点基类
class ASTNode {
public:
    explicit ASTNode(ASTNodeType type) : type_(type) {}
    virtual ~ASTNode() = default;
    
    ASTNodeType get_type() const { return type_; }
    
    // 访问者模式接口
    virtual void accept(class ASTVisitor* visitor) = 0;
    
    // 转换为字符串（用于调试）
    virtual std::string to_string() const = 0;
    
private:
    ASTNodeType type_;
};

// 语句基类
class Statement : public ASTNode {
public:
    explicit Statement(ASTNodeType type) : ASTNode(type) {}
};

// 表达式基类
class Expression : public ASTNode {
public:
    explicit Expression(ASTNodeType type) : ASTNode(type) {}
};

// 字面量表达式
class LiteralExpression : public Expression {
public:
    LiteralExpression(DataType type, const std::string& value)
        : Expression(ASTNodeType::LITERAL_EXPR), data_type_(type), value_(value) {}
    
    DataType get_data_type() const { return data_type_; }
    const std::string& get_value() const { return value_; }
    
    void accept(ASTVisitor* visitor) override;
    std::string to_string() const override;
    
private:
    DataType data_type_;
    std::string value_;
};

// 列引用表达式
class ColumnRefExpression : public Expression {
public:
    explicit ColumnRefExpression(const std::string& column_name)
        : Expression(ASTNodeType::COLUMN_REF_EXPR), column_name_(column_name) {}
    
    ColumnRefExpression(const std::string& table_name, const std::string& column_name)
        : Expression(ASTNodeType::COLUMN_REF_EXPR), 
          table_name_(table_name), column_name_(column_name) {}
    
    const std::string& get_table_name() const { return table_name_; }
    const std::string& get_column_name() const { return column_name_; }
    
    void accept(ASTVisitor* visitor) override;
    std::string to_string() const override;
    
private:
    std::string table_name_;
    std::string column_name_;
};

// 二元表达式
class BinaryExpression : public Expression {
public:
    BinaryExpression(BinaryOperatorType op, 
                    std::unique_ptr<Expression> left,
                    std::unique_ptr<Expression> right)
        : Expression(ASTNodeType::BINARY_EXPR), 
          operator_(op), left_(std::move(left)), right_(std::move(right)) {}
    
    BinaryOperatorType get_operator() const { return operator_; }
    Expression* get_left() const { return left_.get(); }
    Expression* get_right() const { return right_.get(); }
    
    void accept(ASTVisitor* visitor) override;
    std::string to_string() const override;
    
private:
    BinaryOperatorType operator_;
    std::unique_ptr<Expression> left_;
    std::unique_ptr<Expression> right_;
};

// 函数表达式
class FunctionExpression : public Expression {
public:
    FunctionExpression(FunctionType func_type, std::vector<std::unique_ptr<Expression>> args)
        : Expression(ASTNodeType::FUNCTION_EXPR), 
          function_type_(func_type), arguments_(std::move(args)) {}
    
    FunctionType get_function_type() const { return function_type_; }
    const std::vector<std::unique_ptr<Expression>>& get_arguments() const { return arguments_; }
    
    void accept(ASTVisitor* visitor) override;
    std::string to_string() const override;
    
private:
    FunctionType function_type_;
    std::vector<std::unique_ptr<Expression>> arguments_;
};

// 列定义
class ColumnDefinition : public ASTNode {
public:
    ColumnDefinition(const std::string& name, DataType type)
        : ASTNode(ASTNodeType::COLUMN_DEF), column_name_(name), data_type_(type) {}
    
    const std::string& get_column_name() const { return column_name_; }
    DataType get_data_type() const { return data_type_; }
    
    void accept(ASTVisitor* visitor) override;
    std::string to_string() const override;
    
private:
    std::string column_name_;
    DataType data_type_;
};

// 表引用
class TableReference : public ASTNode {
public:
    explicit TableReference(const std::string& table_name)
        : ASTNode(ASTNodeType::TABLE_REF), table_name_(table_name) {}
    
    const std::string& get_table_name() const { return table_name_; }
    
    void accept(ASTVisitor* visitor) override;
    std::string to_string() const override;
    
private:
    std::string table_name_;
};

} // namespace minidb

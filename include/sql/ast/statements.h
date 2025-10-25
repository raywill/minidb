#pragma once

#include "sql/ast/ast_node.h"
#include <vector>
#include <memory>

namespace minidb {

// CREATE TABLE 语句
class CreateTableStatement : public Statement {
public:
    CreateTableStatement(const std::string& table_name, 
                        std::vector<std::unique_ptr<ColumnDefinition>> columns,
                        bool if_not_exists = false)
        : Statement(ASTNodeType::CREATE_TABLE_STMT),
          table_name_(table_name),
          columns_(std::move(columns)),
          if_not_exists_(if_not_exists) {}
    
    const std::string& get_table_name() const { return table_name_; }
    const std::vector<std::unique_ptr<ColumnDefinition>>& get_columns() const { return columns_; }
    bool get_if_not_exists() const { return if_not_exists_; }
    
    void accept(ASTVisitor* visitor) override;
    std::string to_string() const override;
    
private:
    std::string table_name_;
    std::vector<std::unique_ptr<ColumnDefinition>> columns_;
    bool if_not_exists_;
};

// DROP TABLE 语句
class DropTableStatement : public Statement {
public:
    explicit DropTableStatement(const std::string& table_name, bool if_exists = false)
        : Statement(ASTNodeType::DROP_TABLE_STMT),
          table_name_(table_name),
          if_exists_(if_exists) {}
    
    const std::string& get_table_name() const { return table_name_; }
    bool get_if_exists() const { return if_exists_; }
    
    void accept(ASTVisitor* visitor) override;
    std::string to_string() const override;
    
private:
    std::string table_name_;
    bool if_exists_;
};

// INSERT 语句
class InsertStatement : public Statement {
public:
    InsertStatement(const std::string& table_name,
                   std::vector<std::string> columns,
                   std::vector<std::vector<std::unique_ptr<Expression>>> values)
        : Statement(ASTNodeType::INSERT_STMT),
          table_name_(table_name),
          columns_(std::move(columns)),
          values_(std::move(values)) {}
    
    const std::string& get_table_name() const { return table_name_; }
    const std::vector<std::string>& get_columns() const { return columns_; }
    const std::vector<std::vector<std::unique_ptr<Expression>>>& get_values() const { return values_; }
    
    void accept(ASTVisitor* visitor) override;
    std::string to_string() const override;
    
private:
    std::string table_name_;
    std::vector<std::string> columns_;
    std::vector<std::vector<std::unique_ptr<Expression>>> values_;
};

// SELECT 语句
class SelectStatement : public Statement {
public:
    SelectStatement(std::vector<std::unique_ptr<Expression>> select_list,
                   std::unique_ptr<TableReference> from_table,
                   std::unique_ptr<Expression> where_clause = nullptr)
        : Statement(ASTNodeType::SELECT_STMT),
          select_list_(std::move(select_list)),
          from_table_(std::move(from_table)),
          where_clause_(std::move(where_clause)) {}
    
    const std::vector<std::unique_ptr<Expression>>& get_select_list() const { return select_list_; }
    TableReference* get_from_table() const { return from_table_.get(); }
    Expression* get_where_clause() const { return where_clause_.get(); }
    
    void accept(ASTVisitor* visitor) override;
    std::string to_string() const override;
    
private:
    std::vector<std::unique_ptr<Expression>> select_list_;
    std::unique_ptr<TableReference> from_table_;
    std::unique_ptr<Expression> where_clause_;
};

// DELETE 语句
class DeleteStatement : public Statement {
public:
    DeleteStatement(std::unique_ptr<TableReference> from_table,
                   std::unique_ptr<Expression> where_clause = nullptr)
        : Statement(ASTNodeType::DELETE_STMT),
          from_table_(std::move(from_table)),
          where_clause_(std::move(where_clause)) {}
    
    TableReference* get_from_table() const { return from_table_.get(); }
    Expression* get_where_clause() const { return where_clause_.get(); }
    
    void accept(ASTVisitor* visitor) override;
    std::string to_string() const override;
    
private:
    std::unique_ptr<TableReference> from_table_;
    std::unique_ptr<Expression> where_clause_;
};

// AST访问者接口
class ASTVisitor {
public:
    virtual ~ASTVisitor() = default;
    
    // 语句访问方法
    virtual void visit(CreateTableStatement* stmt) = 0;
    virtual void visit(DropTableStatement* stmt) = 0;
    virtual void visit(InsertStatement* stmt) = 0;
    virtual void visit(SelectStatement* stmt) = 0;
    virtual void visit(DeleteStatement* stmt) = 0;
    
    // 表达式访问方法
    virtual void visit(LiteralExpression* expr) = 0;
    virtual void visit(ColumnRefExpression* expr) = 0;
    virtual void visit(BinaryExpression* expr) = 0;
    virtual void visit(FunctionExpression* expr) = 0;
    
    // 其他节点访问方法
    virtual void visit(ColumnDefinition* col_def) = 0;
    virtual void visit(TableReference* table_ref) = 0;
};

} // namespace minidb

#pragma once

#include "exec/operator.h"
#include "sql/compiler/statement.h"
#include "storage/table.h"
#include "common/status.h"
#include <memory>
#include <string>
#include <vector>

namespace minidb {

// 计划类型
enum class PlanType {
    // DDL Plans
    CREATE_TABLE,
    DROP_TABLE,

    // DML Plans
    INSERT,
    SELECT,
    DELETE
};

// ============= Plan 基类 =============
class Plan {
public:
    explicit Plan(PlanType type) : type_(type) {}
    virtual ~Plan() = default;

    PlanType get_type() const { return type_; }
    virtual std::string to_string() const = 0;

private:
    PlanType type_;
};

// ============= CREATE TABLE Plan =============
class CreateTablePlan : public Plan {
public:
    CreateTablePlan(const std::string& table_name,
                   std::vector<ColumnDefinition> columns,
                   bool if_not_exists)
        : Plan(PlanType::CREATE_TABLE),
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

// ============= DROP TABLE Plan =============
class DropTablePlan : public Plan {
public:
    DropTablePlan(const std::string& table_name, bool if_exists)
        : Plan(PlanType::DROP_TABLE),
          table_name_(table_name),
          if_exists_(if_exists) {}

    const std::string& get_table_name() const { return table_name_; }
    bool get_if_exists() const { return if_exists_; }
    std::string to_string() const override;

private:
    std::string table_name_;
    bool if_exists_;
};

// ============= INSERT Plan =============
class InsertPlan : public Plan {
public:
    InsertPlan(const std::string& table_name,
              std::shared_ptr<Table> table,
              std::vector<size_t> column_indices,
              std::vector<std::vector<std::unique_ptr<Expression>>> values)
        : Plan(PlanType::INSERT),
          table_name_(table_name),
          table_(table),
          column_indices_(std::move(column_indices)),
          values_(std::move(values)) {}

    const std::string& get_table_name() const { return table_name_; }
    std::shared_ptr<Table> get_table() const { return table_; }
    const std::vector<size_t>& get_column_indices() const { return column_indices_; }
    const std::vector<std::vector<std::unique_ptr<Expression>>>& get_values() const { return values_; }
    std::string to_string() const override;

private:
    std::string table_name_;
    std::shared_ptr<Table> table_;
    std::vector<size_t> column_indices_;
    std::vector<std::vector<std::unique_ptr<Expression>>> values_;
};

// ============= SELECT Plan =============
// SELECT计划使用Operator树表示物理执行计划
class SelectPlan : public Plan {
public:
    SelectPlan(const std::string& table_name,
              std::unique_ptr<Operator> root_operator)
        : Plan(PlanType::SELECT),
          table_name_(table_name),
          root_operator_(std::move(root_operator)) {}

    const std::string& get_table_name() const { return table_name_; }
    Operator* get_root_operator() const { return root_operator_.get(); }
    std::unique_ptr<Operator> take_root_operator() { return std::move(root_operator_); }
    std::string to_string() const override;

private:
    std::string table_name_;
    std::unique_ptr<Operator> root_operator_;
};

// ============= DELETE Plan =============
class DeletePlan : public Plan {
public:
    DeletePlan(const std::string& table_name,
              std::shared_ptr<Table> table,
              std::unique_ptr<Expression> where_clause)
        : Plan(PlanType::DELETE),
          table_name_(table_name),
          table_(table),
          where_clause_(std::move(where_clause)) {}

    const std::string& get_table_name() const { return table_name_; }
    std::shared_ptr<Table> get_table() const { return table_; }
    Expression* get_where_clause() const { return where_clause_.get(); }
    std::string to_string() const override;

private:
    std::string table_name_;
    std::shared_ptr<Table> table_;
    std::unique_ptr<Expression> where_clause_;
};

} // namespace minidb

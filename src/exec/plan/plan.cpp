#include "exec/plan/plan.h"

namespace minidb {

// ============= CreateTablePlan =============
std::string CreateTablePlan::to_string() const {
    std::string result = "CreateTablePlan(" + table_name_;
    if (if_not_exists_) result += ", IF_NOT_EXISTS";
    result += ", " + std::to_string(columns_.size()) + " columns)";
    return result;
}

// ============= DropTablePlan =============
std::string DropTablePlan::to_string() const {
    std::string result = "DropTablePlan(" + table_name_;
    if (if_exists_) result += ", IF_EXISTS";
    result += ")";
    return result;
}

// ============= InsertPlan =============
std::string InsertPlan::to_string() const {
    return "InsertPlan(table=" + table_name_ +
           ", columns=" + std::to_string(column_indices_.size()) +
           ", rows=" + std::to_string(values_.size()) + ")";
}

// ============= SelectPlan =============
std::string SelectPlan::to_string() const {
    std::string result = "SelectPlan(table=" + table_name_;
    if (root_operator_) {
        result += ", operator_tree=...";
    }
    result += ")";
    return result;
}

// ============= DeletePlan =============
std::string DeletePlan::to_string() const {
    std::string result = "DeletePlan(table=" + table_name_;
    if (where_clause_) {
        result += ", has_where=true";
    }
    result += ")";
    return result;
}

} // namespace minidb

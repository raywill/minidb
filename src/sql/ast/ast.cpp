#include "sql/ast/ast.h"

namespace minidb {

// ============= LiteralAST =============
std::string LiteralAST::to_string() const {
    return value_;
}

// ============= ColumnRefAST =============
std::string ColumnRefAST::to_string() const {
    if (!table_name_.empty()) {
        return table_name_ + "." + column_name_;
    }
    return column_name_;
}

// ============= BinaryOpAST =============
std::string BinaryOpAST::to_string() const {
    std::string op_str;
    switch (op_) {
        case BinaryOp::ADD: op_str = "+"; break;
        case BinaryOp::SUBTRACT: op_str = "-"; break;
        case BinaryOp::MULTIPLY: op_str = "*"; break;
        case BinaryOp::DIVIDE: op_str = "/"; break;
        case BinaryOp::EQUAL: op_str = "="; break;
        case BinaryOp::NOT_EQUAL: op_str = "!="; break;
        case BinaryOp::LESS_THAN: op_str = "<"; break;
        case BinaryOp::LESS_EQUAL: op_str = "<="; break;
        case BinaryOp::GREATER_THAN: op_str = ">"; break;
        case BinaryOp::GREATER_EQUAL: op_str = ">="; break;
        case BinaryOp::AND: op_str = "AND"; break;
        case BinaryOp::OR: op_str = "OR"; break;
    }
    return "(" + left_->to_string() + " " + op_str + " " + right_->to_string() + ")";
}

// ============= FunctionCallAST =============
std::string FunctionCallAST::to_string() const {
    std::string func_name;
    switch (func_type_) {
        case FuncType::SIN: func_name = "SIN"; break;
        case FuncType::COS: func_name = "COS"; break;
        case FuncType::SUBSTR: func_name = "SUBSTR"; break;
    }

    std::string result = func_name + "(";
    for (size_t i = 0; i < args_.size(); ++i) {
        if (i > 0) result += ", ";
        result += args_[i]->to_string();
    }
    result += ")";
    return result;
}

// ============= ColumnDefAST =============
std::string ColumnDefAST::to_string() const {
    std::string type_str;
    switch (data_type_) {
        case DataType::INT: type_str = "INT"; break;
        case DataType::STRING: type_str = "STRING"; break;
        case DataType::BOOL: type_str = "BOOL"; break;
        case DataType::DECIMAL: type_str = "DECIMAL"; break;
    }
    return column_name_ + " " + type_str;
}

// ============= TableRefAST =============
std::string TableRefAST::to_string() const {
    if (!alias_.empty()) {
        return table_name_ + " AS " + alias_;
    }
    return table_name_;
}

// ============= CreateTableAST =============
std::string CreateTableAST::to_string() const {
    std::string result = "CREATE TABLE ";
    if (if_not_exists_) result += "IF NOT EXISTS ";
    result += table_name_ + " (";
    for (size_t i = 0; i < columns_.size(); ++i) {
        if (i > 0) result += ", ";
        result += columns_[i]->to_string();
    }
    result += ")";
    return result;
}

// ============= DropTableAST =============
std::string DropTableAST::to_string() const {
    std::string result = "DROP TABLE ";
    if (if_exists_) result += "IF EXISTS ";
    result += table_name_;
    return result;
}

// ============= InsertAST =============
std::string InsertAST::to_string() const {
    std::string result = "INSERT INTO " + table_name_;
    if (!columns_.empty()) {
        result += " (";
        for (size_t i = 0; i < columns_.size(); ++i) {
            if (i > 0) result += ", ";
            result += columns_[i];
        }
        result += ")";
    }
    result += " VALUES ";
    for (size_t i = 0; i < values_.size(); ++i) {
        if (i > 0) result += ", ";
        result += "(";
        for (size_t j = 0; j < values_[i].size(); ++j) {
            if (j > 0) result += ", ";
            result += values_[i][j]->to_string();
        }
        result += ")";
    }
    return result;
}

// ============= JoinClauseAST =============
std::string JoinClauseAST::to_string() const {
    std::string result;

    // Join type
    switch (join_type_) {
        case JoinType::INNER:
            result = "INNER JOIN ";
            break;
        case JoinType::LEFT_OUTER:
            result = "LEFT OUTER JOIN ";
            break;
        case JoinType::RIGHT_OUTER:
            result = "RIGHT OUTER JOIN ";
            break;
        case JoinType::FULL_OUTER:
            result = "FULL OUTER JOIN ";
            break;
    }

    // Right table
    result += right_table_->to_string();

    // ON condition
    result += " ON " + condition_->to_string();

    return result;
}

// ============= SelectAST =============
std::string SelectAST::to_string() const {
    std::string result = "SELECT ";
    for (size_t i = 0; i < select_list_.size(); ++i) {
        if (i > 0) result += ", ";
        result += select_list_[i]->to_string();
    }
    if (from_table_) {
        result += " FROM " + from_table_->to_string();
    }
    // JOIN clauses
    for (const auto& join : join_clauses_) {
        result += " " + join->to_string();
    }
    if (where_clause_) {
        result += " WHERE " + where_clause_->to_string();
    }
    return result;
}

// ============= DeleteAST =============
std::string DeleteAST::to_string() const {
    std::string result = "DELETE FROM " + from_table_->to_string();
    if (where_clause_) {
        result += " WHERE " + where_clause_->to_string();
    }
    return result;
}

} // namespace minidb

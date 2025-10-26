#include "sql/compiler/statement.h"
#include "common/utils.h"

namespace minidb {

// ============= LiteralExpression =============
std::string LiteralExpression::to_string() const {
    return value_;
}

std::unique_ptr<Expression> LiteralExpression::clone() const {
    return make_unique<LiteralExpression>(data_type_, value_);
}

// ============= ColumnRefExpression =============
std::string ColumnRefExpression::to_string() const {
    if (!table_name_.empty()) {
        return table_name_ + "." + column_name_ + "[" + std::to_string(column_index_) + "]";
    }
    return column_name_ + "[" + std::to_string(column_index_) + "]";
}

std::unique_ptr<Expression> ColumnRefExpression::clone() const {
    return make_unique<ColumnRefExpression>(table_name_, column_name_, column_index_);
}

// ============= BinaryExpression =============
std::string BinaryExpression::to_string() const {
    std::string op_str;
    switch (operator_) {
        case BinaryOperatorType::ADD: op_str = "+"; break;
        case BinaryOperatorType::SUBTRACT: op_str = "-"; break;
        case BinaryOperatorType::MULTIPLY: op_str = "*"; break;
        case BinaryOperatorType::DIVIDE: op_str = "/"; break;
        case BinaryOperatorType::EQUAL: op_str = "="; break;
        case BinaryOperatorType::NOT_EQUAL: op_str = "!="; break;
        case BinaryOperatorType::LESS_THAN: op_str = "<"; break;
        case BinaryOperatorType::LESS_EQUAL: op_str = "<="; break;
        case BinaryOperatorType::GREATER_THAN: op_str = ">"; break;
        case BinaryOperatorType::GREATER_EQUAL: op_str = ">="; break;
        case BinaryOperatorType::AND: op_str = "AND"; break;
        case BinaryOperatorType::OR: op_str = "OR"; break;
    }
    return "(" + left_->to_string() + " " + op_str + " " + right_->to_string() + ")";
}

std::unique_ptr<Expression> BinaryExpression::clone() const {
    return make_unique<BinaryExpression>(operator_, left_->clone(), right_->clone());
}

// ============= FunctionExpression =============
std::string FunctionExpression::to_string() const {
    std::string func_name;
    switch (function_type_) {
        case FunctionType::SIN: func_name = "SIN"; break;
        case FunctionType::COS: func_name = "COS"; break;
        case FunctionType::SUBSTR: func_name = "SUBSTR"; break;
    }

    std::string result = func_name + "(";
    for (size_t i = 0; i < arguments_.size(); ++i) {
        if (i > 0) result += ", ";
        result += arguments_[i]->to_string();
    }
    result += ")";
    return result;
}

std::unique_ptr<Expression> FunctionExpression::clone() const {
    std::vector<std::unique_ptr<Expression>> cloned_args;
    for (const auto& arg : arguments_) {
        cloned_args.push_back(arg->clone());
    }
    return make_unique<FunctionExpression>(function_type_, std::move(cloned_args));
}

// ============= CreateTableStatement =============
std::string CreateTableStatement::to_string() const {
    std::string result = "CreateTable(";
    result += table_name_;
    if (if_not_exists_) result += ", IF_NOT_EXISTS";
    result += ", columns=[";
    for (size_t i = 0; i < columns_.size(); ++i) {
        if (i > 0) result += ", ";
        result += columns_[i].name + ":";
        switch (columns_[i].type) {
            case DataType::INT: result += "INT"; break;
            case DataType::STRING: result += "STRING"; break;
            case DataType::BOOL: result += "BOOL"; break;
            case DataType::DECIMAL: result += "DECIMAL"; break;
        }
    }
    result += "])";
    return result;
}

// ============= DropTableStatement =============
std::string DropTableStatement::to_string() const {
    std::string result = "DropTable(" + table_name_;
    if (if_exists_) result += ", IF_EXISTS";
    result += ")";
    return result;
}

// ============= InsertStatement =============
std::string InsertStatement::to_string() const {
    std::string result = "Insert(table=" + table_name_ + ", columns=[";
    for (size_t i = 0; i < column_names_.size(); ++i) {
        if (i > 0) result += ", ";
        result += column_names_[i] + "[" + std::to_string(column_indices_[i]) + "]";
    }
    result += "], values=" + std::to_string(values_.size()) + " rows)";
    return result;
}

// ============= SelectStatement =============
std::string SelectStatement::to_string() const {
    std::string result = "Select(table=" + table_name_;
    if (!table_alias_.empty()) {
        result += " AS " + table_alias_;
    }

    // JOIN clauses
    if (!joins_.empty()) {
        result += ", joins=[";
        for (size_t i = 0; i < joins_.size(); ++i) {
            if (i > 0) result += ", ";
            result += joins_[i].table_name;
            if (!joins_[i].table_alias.empty()) {
                result += " AS " + joins_[i].table_alias;
            }
            result += " ON " + joins_[i].condition->to_string();
        }
        result += "]";
    }

    result += ", columns=[";
    for (size_t i = 0; i < select_columns_.size(); ++i) {
        if (i > 0) result += ", ";
        result += select_columns_[i] + "[" + std::to_string(select_column_indices_[i]) + "]";
    }
    result += "]";
    if (where_clause_) {
        result += ", where=" + where_clause_->to_string();
    }
    result += ")";
    return result;
}

// ============= DeleteStatement =============
std::string DeleteStatement::to_string() const {
    std::string result = "Delete(table=" + table_name_;
    if (where_clause_) {
        result += ", where=" + where_clause_->to_string();
    }
    result += ")";
    return result;
}

} // namespace minidb

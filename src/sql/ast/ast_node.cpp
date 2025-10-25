#include "sql/ast/ast_node.h"
#include "sql/ast/statements.h"
#include <sstream>

namespace minidb {

// LiteralExpression 实现
void LiteralExpression::accept(ASTVisitor* visitor) {
    visitor->visit(this);
}

std::string LiteralExpression::to_string() const {
    return "Literal(" + DataTypeToString(data_type_) + ", " + value_ + ")";
}

// ColumnRefExpression 实现
void ColumnRefExpression::accept(ASTVisitor* visitor) {
    visitor->visit(this);
}

std::string ColumnRefExpression::to_string() const {
    if (table_name_.empty()) {
        return "ColumnRef(" + column_name_ + ")";
    } else {
        return "ColumnRef(" + table_name_ + "." + column_name_ + ")";
    }
}

// BinaryExpression 实现
void BinaryExpression::accept(ASTVisitor* visitor) {
    visitor->visit(this);
}

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
        default: op_str = "UNKNOWN"; break;
    }
    
    return "BinaryExpr(" + left_->to_string() + " " + op_str + " " + right_->to_string() + ")";
}

// FunctionExpression 实现
void FunctionExpression::accept(ASTVisitor* visitor) {
    visitor->visit(this);
}

std::string FunctionExpression::to_string() const {
    std::string func_name;
    switch (function_type_) {
        case FunctionType::SIN: func_name = "SIN"; break;
        case FunctionType::COS: func_name = "COS"; break;
        case FunctionType::SUBSTR: func_name = "SUBSTR"; break;
        default: func_name = "UNKNOWN"; break;
    }
    
    std::ostringstream oss;
    oss << "Function(" << func_name << "(";
    for (size_t i = 0; i < arguments_.size(); ++i) {
        if (i > 0) oss << ", ";
        oss << arguments_[i]->to_string();
    }
    oss << "))";
    return oss.str();
}

// ColumnDefinition 实现
void ColumnDefinition::accept(ASTVisitor* visitor) {
    visitor->visit(this);
}

std::string ColumnDefinition::to_string() const {
    return "ColumnDef(" + column_name_ + " " + DataTypeToString(data_type_) + ")";
}

// TableReference 实现
void TableReference::accept(ASTVisitor* visitor) {
    visitor->visit(this);
}

std::string TableReference::to_string() const {
    return "TableRef(" + table_name_ + ")";
}

} // namespace minidb

#include "sql/ast/statements.h"
#include <sstream>

namespace minidb {

// CreateTableStatement 实现
void CreateTableStatement::accept(ASTVisitor* visitor) {
    visitor->visit(this);
}

std::string CreateTableStatement::to_string() const {
    std::ostringstream oss;
    oss << "CreateTable(";
    if (if_not_exists_) {
        oss << "IF NOT EXISTS ";
    }
    oss << table_name_ << " (";
    
    for (size_t i = 0; i < columns_.size(); ++i) {
        if (i > 0) oss << ", ";
        oss << columns_[i]->to_string();
    }
    oss << "))";
    return oss.str();
}

// DropTableStatement 实现
void DropTableStatement::accept(ASTVisitor* visitor) {
    visitor->visit(this);
}

std::string DropTableStatement::to_string() const {
    std::ostringstream oss;
    oss << "DropTable(";
    if (if_exists_) {
        oss << "IF EXISTS ";
    }
    oss << table_name_ << ")";
    return oss.str();
}

// InsertStatement 实现
void InsertStatement::accept(ASTVisitor* visitor) {
    visitor->visit(this);
}

std::string InsertStatement::to_string() const {
    std::ostringstream oss;
    oss << "Insert(" << table_name_;
    
    if (!columns_.empty()) {
        oss << " (";
        for (size_t i = 0; i < columns_.size(); ++i) {
            if (i > 0) oss << ", ";
            oss << columns_[i];
        }
        oss << ")";
    }
    
    oss << " VALUES ";
    for (size_t i = 0; i < values_.size(); ++i) {
        if (i > 0) oss << ", ";
        oss << "(";
        for (size_t j = 0; j < values_[i].size(); ++j) {
            if (j > 0) oss << ", ";
            oss << values_[i][j]->to_string();
        }
        oss << ")";
    }
    oss << ")";
    return oss.str();
}

// SelectStatement 实现
void SelectStatement::accept(ASTVisitor* visitor) {
    visitor->visit(this);
}

std::string SelectStatement::to_string() const {
    std::ostringstream oss;
    oss << "Select(";
    
    // SELECT 列表
    for (size_t i = 0; i < select_list_.size(); ++i) {
        if (i > 0) oss << ", ";
        oss << select_list_[i]->to_string();
    }
    
    // FROM 子句
    if (from_table_) {
        oss << " FROM " << from_table_->to_string();
    }
    
    // WHERE 子句
    if (where_clause_) {
        oss << " WHERE " << where_clause_->to_string();
    }
    
    oss << ")";
    return oss.str();
}

// DeleteStatement 实现
void DeleteStatement::accept(ASTVisitor* visitor) {
    visitor->visit(this);
}

std::string DeleteStatement::to_string() const {
    std::ostringstream oss;
    oss << "Delete(FROM " << from_table_->to_string();
    
    if (where_clause_) {
        oss << " WHERE " << where_clause_->to_string();
    }
    
    oss << ")";
    return oss.str();
}

} // namespace minidb

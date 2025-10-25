#pragma once

#include "exec/operator.h"
#include "sql/ast/statements.h"
#include "storage/catalog.h"
#include "storage/table.h"
#include "common/status.h"
#include "mem/arena.h"
#include <memory>
#include <atomic>

namespace minidb {

// 查询执行结果
struct QueryResult {
    bool success;
    std::string result_text;
    std::string error_message;
    size_t rows_affected;
    
    QueryResult() : success(false), rows_affected(0) {}
    
    static QueryResult success_result(const std::string& text, size_t affected = 0) {
        QueryResult result;
        result.success = true;
        result.result_text = text;
        result.rows_affected = affected;
        return result;
    }
    
    static QueryResult error_result(const std::string& error) {
        QueryResult result;
        result.success = false;
        result.error_message = error;
        return result;
    }
};

// SQL执行引擎
class Executor {
public:
    Executor(Catalog* catalog, TableManager* table_manager);
    ~Executor();
    
    // 执行SQL语句
    QueryResult execute_statement(Statement* stmt);
    
    // 执行具体的语句类型
    QueryResult execute_create_table(CreateTableStatement* stmt);
    QueryResult execute_drop_table(DropTableStatement* stmt);
    QueryResult execute_insert(InsertStatement* stmt);
    QueryResult execute_select(SelectStatement* stmt);
    QueryResult execute_delete(DeleteStatement* stmt);
    
private:
    Catalog* catalog_;
    TableManager* table_manager_;
    std::atomic<size_t> next_query_id_;
    
    // 构建查询执行计划
    Status build_select_plan(SelectStatement* stmt, std::unique_ptr<Operator>& root_op);
    
    // 执行查询计划
    Status execute_plan(std::unique_ptr<Operator> root_op, std::string& result_text);
    
    // 辅助方法
    Status convert_rows_from_values(const std::vector<std::vector<std::unique_ptr<Expression>>>& values,
                                   const TableSchema& schema,
                                   std::vector<Row>& rows);
    
    std::string evaluate_literal_expression(Expression* expr);
    
    size_t get_next_query_id() {
        return next_query_id_.fetch_add(1);
    }
};

} // namespace minidb

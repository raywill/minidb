#pragma once

#include "exec/plan/plan.h"
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

// SQL执行引擎 - 只负责执行Plan（物理执行计划）
class QueryExecutor {
public:
    QueryExecutor(Catalog* catalog, TableManager* table_manager);
    ~QueryExecutor();

    // 执行Plan
    QueryResult execute_plan(Plan* plan);

private:
    Catalog* catalog_;
    TableManager* table_manager_;
    std::atomic<size_t> next_query_id_;

    // 执行各种Plan
    QueryResult execute_create_table(CreateTablePlan* plan);
    QueryResult execute_drop_table(DropTablePlan* plan);
    QueryResult execute_insert(InsertPlan* plan);
    QueryResult execute_select(SelectPlan* plan);
    QueryResult execute_delete(DeletePlan* plan);

    // 执行Operator树
    Status execute_operator_tree(std::unique_ptr<Operator> root_op, std::string& result_text);

    // 辅助方法
    std::string evaluate_literal_expression(Expression* expr);

    size_t get_next_query_id() {
        return next_query_id_.fetch_add(1);
    }
};

} // namespace minidb

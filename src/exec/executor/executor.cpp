#include "exec/executor/executor.h"
#include "exec/operators/scan_operator.h"
#include "exec/operators/filter_operator.h"
#include "exec/operators/projection_operator.h"
#include "exec/operators/final_result_operator.h"
#include "exec/operators/typed_expression_evaluator.h"
#include "log/logger.h"
#include "common/utils.h"
#include "common/crash_handler.h"

namespace minidb {

Executor::Executor(Catalog* catalog, TableManager* table_manager)
    : catalog_(catalog), table_manager_(table_manager), next_query_id_(1) {
}

Executor::~Executor() = default;

QueryResult Executor::execute_statement(Statement* stmt) {
    try {
        size_t query_id = get_next_query_id();
        
        // 设置当前查询ID用于崩溃处理
        SET_QUERY_ID(query_id);
        
        LOG_INFO("Executor", "Query#" + std::to_string(query_id), 
                 "Executing statement: " + stmt->to_string());
        
        QueryResult result;
        
        switch (stmt->get_type()) {
            case ASTNodeType::CREATE_TABLE_STMT:
                result = execute_create_table(static_cast<CreateTableStatement*>(stmt));
                break;
            case ASTNodeType::DROP_TABLE_STMT:
                result = execute_drop_table(static_cast<DropTableStatement*>(stmt));
                break;
            case ASTNodeType::INSERT_STMT:
                result = execute_insert(static_cast<InsertStatement*>(stmt));
                break;
            case ASTNodeType::SELECT_STMT:
                result = execute_select(static_cast<SelectStatement*>(stmt));
                break;
            case ASTNodeType::DELETE_STMT:
                result = execute_delete(static_cast<DeleteStatement*>(stmt));
                break;
            default:
                result = QueryResult::error_result("Unsupported statement type");
                break;
        }
        
        if (result.success) {
            LOG_INFO("Executor", "Query#" + std::to_string(query_id), 
                     "Statement executed successfully");
        } else {
            LOG_ERROR("Executor", "Query#" + std::to_string(query_id), 
                      "Statement execution failed: " + result.error_message);
        }
        
        return result;
        
    } catch (const DatabaseException& e) {
        return QueryResult::error_result("ERROR: " + std::string(e.what()));
    } catch (const std::exception& e) {
        return QueryResult::error_result("ERROR: " + std::string(e.what()));
    }
}

QueryResult Executor::execute_create_table(CreateTableStatement* stmt) {
    // 构建表结构
    TableSchema schema(stmt->get_table_name());
    
    for (const auto& col_def : stmt->get_columns()) {
        schema.add_column(col_def->get_column_name(), col_def->get_data_type());
    }
    
    // 创建表
    Status status = catalog_->create_table(stmt->get_table_name(), schema, stmt->get_if_not_exists());
    if (!status.ok()) {
        return QueryResult::error_result(status.ToString());
    }
    
    return QueryResult::success_result("Table created successfully");
}

QueryResult Executor::execute_drop_table(DropTableStatement* stmt) {
    Status status = catalog_->drop_table(stmt->get_table_name(), stmt->get_if_exists());
    if (!status.ok()) {
        return QueryResult::error_result(status.ToString());
    }
    
    // 从表管理器中关闭表
    table_manager_->close_table(stmt->get_table_name());
    
    return QueryResult::success_result("Table dropped successfully");
}

QueryResult Executor::execute_insert(InsertStatement* stmt) {
    // 获取表
    std::shared_ptr<Table> table;
    Status status = table_manager_->open_table(stmt->get_table_name(), table);
    if (!status.ok()) {
        return QueryResult::error_result(status.ToString());
    }
    
    // 转换值为行数据
    std::vector<Row> rows;
    status = convert_rows_from_values(stmt->get_values(), table->get_schema(), rows);
    if (!status.ok()) {
        return QueryResult::error_result(status.ToString());
    }
    
    // 插入数据
    status = table->insert_rows(rows);
    if (!status.ok()) {
        return QueryResult::error_result(status.ToString());
    }
    
    // 更新行数统计
    size_t new_row_count = table->get_row_count();
    catalog_->update_row_count(stmt->get_table_name(), new_row_count);
    
    return QueryResult::success_result("Rows inserted successfully", rows.size());
}

QueryResult Executor::execute_select(SelectStatement* stmt) {
    // 构建执行计划
    std::unique_ptr<Operator> root_op;
    Status status = build_select_plan(stmt, root_op);
    if (!status.ok()) {
        return QueryResult::error_result(status.ToString());
    }
    
    // 执行计划
    std::string result_text;
    status = execute_plan(std::move(root_op), result_text);
    if (!status.ok()) {
        return QueryResult::error_result(status.ToString());
    }
    
    return QueryResult::success_result(result_text);
}

QueryResult Executor::execute_delete(DeleteStatement* stmt) {
    // 获取表
    std::shared_ptr<Table> table;
    Status status = table_manager_->open_table(stmt->get_from_table()->get_table_name(), table);
    if (!status.ok()) {
        return QueryResult::error_result(status.ToString());
    }
    
    // 简单实现：扫描所有行，找到匹配的行进行删除
    // 在实际实现中，应该使用更高效的方法
    
    std::vector<ColumnVector> all_columns;
    status = table->scan_all(all_columns);
    if (!status.ok()) {
        return QueryResult::error_result(status.ToString());
    }
    
    if (all_columns.empty()) {
        return QueryResult::success_result("No rows to delete", 0);
    }
    
    size_t total_rows = all_columns[0].size;
    std::vector<size_t> rows_to_delete;
    
    // 如果有WHERE子句，评估条件
    if (stmt->get_where_clause()) {
        // 创建数据块用于条件评估
        DataChunk chunk;
        chunk.row_count = total_rows;
        for (const auto& col : all_columns) {
            chunk.add_column(col);
        }
        
        // 使用类型化表达式求值器
        TypedExpressionEvaluator evaluator(stmt->get_where_clause());
        std::vector<Value> results;
        status = evaluator.evaluate(chunk, results);
        if (!status.ok()) {
            return QueryResult::error_result(status.ToString());
        }

        // 收集要删除的行索引（只删除WHERE条件为true的行）
        for (size_t i = 0; i < results.size(); ++i) {
            if (results[i].as_bool()) {
                rows_to_delete.push_back(i);
            }
        }
    } else {
        // 删除所有行
        for (size_t i = 0; i < total_rows; ++i) {
            rows_to_delete.push_back(i);
        }
    }
    
    // 执行删除
    if (!rows_to_delete.empty()) {
        status = table->delete_rows(rows_to_delete);
        if (!status.ok()) {
            return QueryResult::error_result(status.ToString());
        }
        
        // 更新行数统计
        size_t new_row_count = table->get_row_count();
        catalog_->update_row_count(stmt->get_from_table()->get_table_name(), new_row_count);
    }
    
    return QueryResult::success_result("Rows deleted successfully", rows_to_delete.size());
}

Status Executor::build_select_plan(SelectStatement* stmt, std::unique_ptr<Operator>& root_op) {
    LOG_DEBUG("Executor", "BuildSelectPlan", "Building SELECT plan");
    
    // 检查FROM子句
    if (!stmt->get_from_table()) {
        LOG_ERROR("Executor", "BuildSelectPlan", "SELECT statement has no FROM table");
        return Status::InvalidArgument("SELECT statement requires FROM clause");
    }
    
    std::string table_name = stmt->get_from_table()->get_table_name();
    LOG_DEBUG("Executor", "BuildSelectPlan", "Target table: " + table_name);
    
    // 获取表
    std::shared_ptr<Table> table;
    Status status = table_manager_->open_table(table_name, table);
    if (!status.ok()) {
        LOG_ERROR("Executor", "BuildSelectPlan", "Failed to open table: " + status.ToString());
        return status;
    }
    
    LOG_DEBUG("Executor", "BuildSelectPlan", "Table opened successfully");
    
    // 确定要扫描的列
    std::vector<std::string> scan_columns;
    bool select_all = false;
    
    for (const auto& expr : stmt->get_select_list()) {
        if (expr->get_type() == ASTNodeType::COLUMN_REF_EXPR) {
            ColumnRefExpression* col_ref = static_cast<ColumnRefExpression*>(expr.get());
            if (col_ref->get_column_name() == "*") {
                select_all = true;
                break;
            }
            scan_columns.push_back(col_ref->get_column_name());
        }
    }
    
    // 如果是SELECT *或没有明确的列引用，扫描所有列
    if (select_all || scan_columns.empty()) {
        const TableSchema& schema = table->get_schema();
        scan_columns = schema.column_names;
        LOG_DEBUG("Executor", "BuildSelectPlan", "Scanning all columns: " + std::to_string(scan_columns.size()));
    } else {
        LOG_DEBUG("Executor", "BuildSelectPlan", "Scanning specific columns: " + std::to_string(scan_columns.size()));
    }
    
    // 创建扫描算子
    auto scan_op = make_unique<ScanOperator>(stmt->get_from_table()->get_table_name(), scan_columns, table);
    std::unique_ptr<Operator> current_op = std::move(scan_op);
    
    // 添加过滤算子
    if (stmt->get_where_clause()) {
        LOG_DEBUG("Executor", "BuildSelectPlan", "Adding filter operator for WHERE clause");
        
        // 创建过滤算子，直接使用表达式指针（不转移所有权）
        // 注意：这要求SelectStatement在执行期间保持有效
        auto filter_op = std::unique_ptr<FilterOperator>(new FilterOperator(stmt->get_where_clause()));
        filter_op->set_child(std::move(current_op));
        current_op = std::move(filter_op);
        
        LOG_DEBUG("Executor", "BuildSelectPlan", "Filter operator added successfully");
    }
    
    // 添加投影算子
    std::vector<std::string> projection_columns;
    for (const auto& expr : stmt->get_select_list()) {
        if (expr->get_type() == ASTNodeType::COLUMN_REF_EXPR) {
            ColumnRefExpression* col_ref = static_cast<ColumnRefExpression*>(expr.get());
            projection_columns.push_back(col_ref->get_column_name());
        }
    }
    
    if (!projection_columns.empty()) {
        auto proj_op = make_unique<ProjectionOperator>(projection_columns);
        proj_op->set_child(std::move(current_op));
        current_op = std::move(proj_op);
    }
    
    // 添加最终结果算子
    auto final_op = make_unique<FinalResultOperator>();
    final_op->set_child(std::move(current_op));
    
    root_op = std::move(final_op);
    return Status::OK();
}

Status Executor::execute_plan(std::unique_ptr<Operator> root_op, std::string& result_text) {
    size_t query_id = get_next_query_id();
    
    // 创建执行上下文
    ScopedArena arena;
    ExecutionContext context(arena.get(), query_id);
    
    // 初始化算子
    Status status = root_op->initialize(&context);
    if (!status.ok()) {
        return status;
    }
    
    // 执行算子
    DataChunk chunk;
    status = root_op->get_next(&context, chunk);
    if (!status.ok()) {
        return status;
    }
    
    // 获取结果文本
    if (root_op->get_type() == OperatorType::FINAL_RESULT) {
        FinalResultOperator* final_op = static_cast<FinalResultOperator*>(root_op.get());
        result_text = final_op->get_result_text();
    }
    
    return Status::OK();
}

Status Executor::convert_rows_from_values(const std::vector<std::vector<std::unique_ptr<Expression>>>& values,
                                         const TableSchema& schema,
                                         std::vector<Row>& rows) {
    rows.clear();
    
    for (const auto& value_list : values) {
        if (value_list.size() != schema.get_column_count()) {
            return Status::InvalidArgument("Value count does not match column count");
        }
        
        Row row(schema.get_column_count());
        
        for (size_t i = 0; i < value_list.size(); ++i) {
            row.values[i] = evaluate_literal_expression(value_list[i].get());
        }
        
        rows.push_back(row);
    }
    
    return Status::OK();
}

std::string Executor::evaluate_literal_expression(Expression* expr) {
    if (expr->get_type() == ASTNodeType::LITERAL_EXPR) {
        LiteralExpression* literal = static_cast<LiteralExpression*>(expr);
        return literal->get_value();
    }
    
    return ""; // 简单处理，实际应该支持更复杂的表达式
}

} // namespace minidb

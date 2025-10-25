#include "exec/executor/new_executor.h"
#include "exec/operators/scan_operator.h"
#include "exec/operators/filter_operator.h"
#include "exec/operators/projection_operator.h"
#include "exec/operators/final_result_operator.h"
#include "log/logger.h"
#include "common/utils.h"
#include "common/crash_handler.h"

namespace minidb {

QueryExecutor::QueryExecutor(Catalog* catalog, TableManager* table_manager)
    : catalog_(catalog), table_manager_(table_manager), next_query_id_(1) {}

QueryExecutor::~QueryExecutor() = default;

QueryResult QueryExecutor::execute_plan(Plan* plan) {
    try {
        size_t query_id = get_next_query_id();

        SET_QUERY_ID(query_id);

        LOG_INFO("QueryExecutor", "Query#" + std::to_string(query_id),
                 "Executing plan: " + plan->to_string());

        QueryResult result;

        switch (plan->get_type()) {
            case PlanType::CREATE_TABLE:
                result = execute_create_table(static_cast<CreateTablePlan*>(plan));
                break;
            case PlanType::DROP_TABLE:
                result = execute_drop_table(static_cast<DropTablePlan*>(plan));
                break;
            case PlanType::INSERT:
                result = execute_insert(static_cast<InsertPlan*>(plan));
                break;
            case PlanType::SELECT:
                result = execute_select(static_cast<SelectPlan*>(plan));
                break;
            case PlanType::DELETE:
                result = execute_delete(static_cast<DeletePlan*>(plan));
                break;
            default:
                result = QueryResult::error_result("Unsupported plan type");
                break;
        }

        if (result.success) {
            LOG_INFO("QueryExecutor", "Query#" + std::to_string(query_id),
                     "Plan executed successfully");
        } else {
            LOG_ERROR("QueryExecutor", "Query#" + std::to_string(query_id),
                      "Plan execution failed: " + result.error_message);
        }

        return result;

    } catch (const DatabaseException& e) {
        return QueryResult::error_result("ERROR: " + std::string(e.what()));
    } catch (const std::exception& e) {
        return QueryResult::error_result("ERROR: " + std::string(e.what()));
    }
}

// ============= DDL Plan执行 =============
QueryResult QueryExecutor::execute_create_table(CreateTablePlan* plan) {
    TableSchema schema(plan->get_table_name());

    for (const auto& col_def : plan->get_columns()) {
        schema.add_column(col_def.name, col_def.type);
    }

    Status status = catalog_->create_table(plan->get_table_name(), schema, plan->get_if_not_exists());
    if (!status.ok()) {
        return QueryResult::error_result(status.ToString());
    }

    return QueryResult::success_result("Table created successfully");
}

QueryResult QueryExecutor::execute_drop_table(DropTablePlan* plan) {
    Status status = catalog_->drop_table(plan->get_table_name(), plan->get_if_exists());
    if (!status.ok()) {
        return QueryResult::error_result(status.ToString());
    }

    table_manager_->close_table(plan->get_table_name());

    return QueryResult::success_result("Table dropped successfully");
}

// ============= DML Plan执行 =============
QueryResult QueryExecutor::execute_insert(InsertPlan* plan) {
    std::shared_ptr<Table> table = plan->get_table();
    if (!table) {
        return QueryResult::error_result("Table not found");
    }

    const TableSchema& schema = table->get_schema();
    const auto& column_indices = plan->get_column_indices();
    const auto& values = plan->get_values();

    // 转换值为行数据
    std::vector<Row> rows;
    rows.reserve(values.size());

    for (const auto& value_list : values) {
        // 验证值数量与列索引数量匹配
        if (value_list.size() != column_indices.size()) {
            return QueryResult::error_result(
                Status::InvalidArgument("Value count does not match column count").ToString());
        }

        // 创建完整的行（所有列）
        Row row(schema.get_column_count());

        // 填充指定的列
        for (size_t i = 0; i < value_list.size(); ++i) {
            size_t col_idx = column_indices[i];
            row.values[col_idx] = evaluate_literal_expression(value_list[i].get());
        }

        // 对于未指定的列，填充类型适当的默认值
        for (size_t i = 0; i < schema.get_column_count(); ++i) {
            if (row.values[i].empty()) {
                // 根据列类型提供默认值
                DataType col_type = schema.column_types[i];
                switch (col_type) {
                    case DataType::INT:
                        row.values[i] = "0";
                        break;
                    case DataType::DECIMAL:
                        row.values[i] = "0.0";
                        break;
                    case DataType::BOOL:
                        row.values[i] = "0";
                        break;
                    case DataType::STRING:
                        row.values[i] = "";  // 空字符串对 STRING 类型有效
                        break;
                }
            }
        }

        rows.push_back(row);
    }

    // 插入数据
    Status status = table->insert_rows(rows);
    if (!status.ok()) {
        return QueryResult::error_result(status.ToString());
    }

    // 更新行数统计
    size_t new_row_count = table->get_row_count();
    catalog_->update_row_count(plan->get_table_name(), new_row_count);

    return QueryResult::success_result("Rows inserted successfully", rows.size());
}

QueryResult QueryExecutor::execute_select(SelectPlan* plan) {
    // 执行Operator树
    std::string result_text;
    Status status = execute_operator_tree(plan->take_root_operator(), result_text);
    if (!status.ok()) {
        return QueryResult::error_result(status.ToString());
    }

    return QueryResult::success_result(result_text);
}

QueryResult QueryExecutor::execute_delete(DeletePlan* plan) {
    std::shared_ptr<Table> table = plan->get_table();
    if (!table) {
        return QueryResult::error_result("Table not found");
    }

    // 扫描所有行
    std::vector<ColumnVector> all_columns;
    Status status = table->scan_all(all_columns);
    if (!status.ok()) {
        return QueryResult::error_result(status.ToString());
    }

    if (all_columns.empty()) {
        return QueryResult::success_result("No rows to delete", 0);
    }

    size_t total_rows = all_columns[0].size;
    std::vector<size_t> rows_to_delete;

    // 如果有WHERE子句，评估条件
    if (plan->get_where_clause()) {
        DataChunk chunk;
        chunk.row_count = total_rows;
        for (const auto& col : all_columns) {
            chunk.add_column(col);
        }

        class ExpressionEvaluator evaluator(plan->get_where_clause());
        std::vector<bool> selection;
        status = evaluator.evaluate(chunk, selection);
        if (!status.ok()) {
            return QueryResult::error_result(status.ToString());
        }

        for (size_t i = 0; i < selection.size(); ++i) {
            if (selection[i]) {
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

        size_t new_row_count = table->get_row_count();
        catalog_->update_row_count(plan->get_table_name(), new_row_count);
    }

    return QueryResult::success_result("Rows deleted successfully", rows_to_delete.size());
}

// ============= 辅助方法 =============
Status QueryExecutor::execute_operator_tree(std::unique_ptr<Operator> root_op, std::string& result_text) {
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

std::string QueryExecutor::evaluate_literal_expression(Expression* expr) {
    // 简单实现：只支持字面量表达式
    // 未来可以扩展为完整的表达式求值器
    if (!expr) {
        return "";
    }

    // 尝试转换为LiteralExpression
    LiteralExpression* lit_expr = dynamic_cast<LiteralExpression*>(expr);
    if (lit_expr) {
        return lit_expr->get_value();
    }

    return "";
}

} // namespace minidb

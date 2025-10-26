#include "exec/plan/planner.h"
#include "exec/operators/scan_operator.h"
#include "exec/operators/filter_operator.h"
#include "exec/operators/projection_operator.h"
#include "exec/operators/final_result_operator.h"
#include "exec/operators/nested_loop_join_operator.h"
#include "common/utils.h"

namespace minidb {

Planner::Planner(Catalog* catalog, TableManager* table_manager)
    : catalog_(catalog), table_manager_(table_manager) {}

Status Planner::create_plan(Statement* stmt, std::unique_ptr<Plan>& plan) {
    if (!stmt) {
        return Status::InvalidArgument("NULL statement");
    }

    switch (stmt->get_type()) {
        case StatementType::CREATE_TABLE:
            plan = plan_create_table(static_cast<CreateTableStatement*>(stmt));
            break;
        case StatementType::DROP_TABLE:
            plan = plan_drop_table(static_cast<DropTableStatement*>(stmt));
            break;
        case StatementType::INSERT:
            plan = plan_insert(static_cast<InsertStatement*>(stmt));
            break;
        case StatementType::SELECT:
            plan = plan_select(static_cast<SelectStatement*>(stmt));
            break;
        case StatementType::DELETE:
            plan = plan_delete(static_cast<DeleteStatement*>(stmt));
            break;
        default:
            return Status::InvalidArgument("Unsupported statement type");
    }

    if (!plan) {
        return Status::InvalidArgument("Failed to create plan");
    }

    return Status::OK();
}

// ============= DDL Plans =============
std::unique_ptr<CreateTablePlan> Planner::plan_create_table(CreateTableStatement* stmt) {
    return make_unique<CreateTablePlan>(
        stmt->get_table_name(),
        stmt->get_columns(),
        stmt->get_if_not_exists()
    );
}

std::unique_ptr<DropTablePlan> Planner::plan_drop_table(DropTableStatement* stmt) {
    return make_unique<DropTablePlan>(
        stmt->get_table_name(),
        stmt->get_if_exists()
    );
}

// ============= DML Plans =============
std::unique_ptr<InsertPlan> Planner::plan_insert(InsertStatement* stmt) {
    // 打开表
    std::shared_ptr<Table> table;
    Status status = table_manager_->open_table(stmt->get_table_name(), table);
    if (!status.ok()) {
        return nullptr;
    }

    // 深拷贝值表达式
    std::vector<std::vector<std::unique_ptr<Expression>>> values;
    for (const auto& row : stmt->get_values()) {
        std::vector<std::unique_ptr<Expression>> cloned_row;
        for (const auto& expr : row) {
            cloned_row.push_back(expr->clone());
        }
        values.push_back(std::move(cloned_row));
    }

    return make_unique<InsertPlan>(
        stmt->get_table_name(),
        table,
        stmt->get_column_indices(),
        std::move(values)
    );
}

std::unique_ptr<SelectPlan> Planner::plan_select(SelectStatement* stmt) {
    // 打开主表
    std::shared_ptr<Table> table;
    Status status = table_manager_->open_table(stmt->get_table_name(), table);
    if (!status.ok()) {
        return nullptr;
    }

    // 构建Operator树
    std::unique_ptr<Operator> root_op;
    status = build_select_operator_tree(stmt, table, root_op);
    if (!status.ok()) {
        return nullptr;
    }

    return make_unique<SelectPlan>(stmt->get_table_name(), std::move(root_op));
}

std::unique_ptr<DeletePlan> Planner::plan_delete(DeleteStatement* stmt) {
    // 打开表
    std::shared_ptr<Table> table;
    Status status = table_manager_->open_table(stmt->get_table_name(), table);
    if (!status.ok()) {
        return nullptr;
    }

    // 复制WHERE子句（需要深拷贝）
    std::unique_ptr<Expression> where_clause;
    // 注意：这里简化处理，实际应该深拷贝Expression

    return make_unique<DeletePlan>(stmt->get_table_name(), table, std::move(where_clause));
}

// ============= SELECT Operator树构建 =============
Status Planner::build_select_operator_tree(SelectStatement* stmt,
                                           std::shared_ptr<Table> table,
                                           std::unique_ptr<Operator>& root_op) {
    std::unique_ptr<Operator> current_op;

    // 检查是否有JOIN
    if (stmt->has_joins()) {
        // 构建JOIN的Operator树
        const TableSchema& left_schema = table->get_schema();

        // 1. 创建左表的Scan算子（扫描所有列）
        auto left_scan = make_unique<ScanOperator>(
            stmt->get_table_name(),
            left_schema.column_names,
            table);

        // 2. 处理JOIN子句（支持多个JOIN）
        const auto& joins = stmt->get_joins();
        if (joins.empty()) {
            return Status::InvalidArgument("JOIN statement has no join clauses");
        }

        // 将左表作为当前的operator
        current_op = std::move(left_scan);

        // 依次处理每个JOIN，构建嵌套的JOIN operator树
        for (const auto& join_info : joins) {
            // 打开JOIN表
            std::shared_ptr<Table> join_table;
            Status status = table_manager_->open_table(join_info.table_name, join_table);
            if (!status.ok()) {
                return status;
            }

            const TableSchema& join_schema = join_table->get_schema();

            // 创建JOIN表的Scan算子（扫描所有列）
            auto join_scan = make_unique<ScanOperator>(
                join_info.table_name,
                join_schema.column_names,
                join_table);

            // 克隆JOIN条件表达式
            std::unique_ptr<Expression> join_condition;
            if (join_info.condition) {
                join_condition = join_info.condition->clone();
            }

            // 创建NestedLoopJoin算子
            // current_op作为左子树，join_scan作为右子树
            auto join_op = make_unique<NestedLoopJoinOperator>(
                std::move(current_op),
                std::move(join_scan),
                std::move(join_condition),
                join_info.join_type);

            // 将新的JOIN算子作为当前算子，用于下一次JOIN
            current_op = std::move(join_op);
        }

    } else {
        // 没有JOIN，创建普通的Scan算子
        auto scan_op = make_unique<ScanOperator>(
            stmt->get_table_name(),
            stmt->get_select_columns(),
            table);
        current_op = std::move(scan_op);
    }

    // 6. 如果有WHERE子句，添加Filter算子
    if (stmt->get_where_clause()) {
        // 克隆WHERE表达式
        auto where_expr = stmt->get_where_clause()->clone();
        auto filter_op = make_unique<FilterOperator>(std::move(where_expr));
        filter_op->set_child(std::move(current_op));
        current_op = std::move(filter_op);
    }

    // 7. 添加Projection算子（如果需要投影特定列）
    if (!stmt->get_select_columns().empty() && stmt->has_joins()) {
        // JOIN查询需要投影到指定列
        auto proj_op = make_unique<ProjectionOperator>(stmt->get_select_columns());
        proj_op->set_child(std::move(current_op));
        current_op = std::move(proj_op);
    }

    // 8. 添加最终结果算子
    auto final_op = make_unique<FinalResultOperator>();
    final_op->set_child(std::move(current_op));

    root_op = std::move(final_op);
    return Status::OK();
}

} // namespace minidb

#pragma once

#include "exec/plan/plan.h"
#include "sql/compiler/statement.h"
#include "storage/catalog.h"
#include "storage/table.h"
#include "common/status.h"
#include <memory>

namespace minidb {

// 查询计划器 - 将Statement（逻辑查询）转换为Plan（物理执行计划）
class Planner {
public:
    Planner(Catalog* catalog, TableManager* table_manager);

    // 生成执行计划
    Status create_plan(Statement* stmt, std::unique_ptr<Plan>& plan);

private:
    Catalog* catalog_;
    TableManager* table_manager_;

    // 为各种语句生成计划
    std::unique_ptr<CreateTablePlan> plan_create_table(CreateTableStatement* stmt);
    std::unique_ptr<DropTablePlan> plan_drop_table(DropTableStatement* stmt);
    std::unique_ptr<InsertPlan> plan_insert(InsertStatement* stmt);
    std::unique_ptr<SelectPlan> plan_select(SelectStatement* stmt);
    std::unique_ptr<DeletePlan> plan_delete(DeleteStatement* stmt);

    // 构建SELECT的Operator树
    Status build_select_operator_tree(SelectStatement* stmt,
                                      std::shared_ptr<Table> table,
                                      std::unique_ptr<Operator>& root_op);
};

} // namespace minidb

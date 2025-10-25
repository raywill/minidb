#pragma once

#include "sql/compiler/statement.h"
#include "common/status.h"
#include <memory>

namespace minidb {

// SQL优化器 - 优化Statement（逻辑查询）
// 当前实现为简单的pass-through，未来可以添加：
// - 谓词下推（Predicate Pushdown）
// - 常量折叠（Constant Folding）
// - 列裁剪（Column Pruning）
// - 子查询优化等
class Optimizer {
public:
    Optimizer();

    // 优化Statement
    // 当前实现：简单地返回原Statement
    // 未来：可以返回优化后的新Statement
    Status optimize(Statement* stmt, std::unique_ptr<Statement>& optimized_stmt);

private:
    // 未来可添加各种优化规则
    // Status apply_predicate_pushdown(Statement* stmt);
    // Status apply_constant_folding(Statement* stmt);
    // etc.
};

} // namespace minidb

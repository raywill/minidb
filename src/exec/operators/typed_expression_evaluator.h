#pragma once

#include "sql/compiler/statement.h"
#include "exec/operator.h"
#include "common/value.h"
#include "common/status.h"
#include <vector>

namespace minidb {

// 类型化的表达式求值器
// 使用 Value 类型进行高效计算，避免字符串转换开销
class TypedExpressionEvaluator {
public:
    explicit TypedExpressionEvaluator(Expression* expression)
        : expression_(expression) {}

    // 对整个 chunk 求值，返回类型化的结果向量
    Status evaluate(const DataChunk& chunk, std::vector<Value>& results);

    // 对单行求值
    Status evaluate_row(const DataChunk& chunk, size_t row_index, Value& result);

private:
    Expression* expression_;

    // 按表达式类型分发
    Value evaluate_literal(LiteralExpression* expr);
    Value evaluate_column_ref(ColumnRefExpression* expr, const DataChunk& chunk, size_t row_idx);
    Value evaluate_binary_op(BinaryExpression* expr, const DataChunk& chunk, size_t row_idx);
    Value evaluate_function(FunctionExpression* expr, const DataChunk& chunk, size_t row_idx);

    // 二元运算实现
    Value compute_arithmetic(const Value& left, const Value& right, BinaryOperatorType op);
    Value compute_comparison(const Value& left, const Value& right, BinaryOperatorType op);
    Value compute_logical(const Value& left, const Value& right, BinaryOperatorType op);

    // 辅助方法
    int find_column_index(const DataChunk& chunk, const std::string& table_name, const std::string& column_name);
};

} // namespace minidb

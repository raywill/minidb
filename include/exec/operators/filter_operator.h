#pragma once

#include "exec/operator.h"
#include "sql/compiler/statement.h"
#include <memory>

namespace minidb {

// 前向声明
class TypedExpressionEvaluator;

// 过滤算子 - 根据谓词过滤数据
class FilterOperator : public PushOperator {
public:
    explicit FilterOperator(std::unique_ptr<Expression> predicate);
    explicit FilterOperator(Expression* predicate); // 不拥有所有权的版本
    ~FilterOperator() override;

    Status initialize(ExecutionContext* context) override;
    Status get_next(ExecutionContext* context, DataChunk& chunk) override;
    Status reset() override;

    std::vector<std::string> get_output_columns() const override;
    std::vector<DataType> get_output_types() const override;

private:
    std::unique_ptr<Expression> predicate_;
    Expression* predicate_ptr_; // 不拥有所有权的指针
    bool owns_predicate_; // 是否拥有表达式的所有权

    // 类型化表达式求值器
    TypedExpressionEvaluator* evaluator_;

    Status evaluate_predicate(const DataChunk& input_chunk, std::vector<bool>& selection);
    Status apply_selection(const DataChunk& input_chunk, const std::vector<bool>& selection, DataChunk& output_chunk);
};

} // namespace minidb

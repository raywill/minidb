#pragma once

#include "exec/operator.h"
#include "sql/ast/ast_node.h"
#include <memory>

namespace minidb {

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
    
    // 表达式求值器
    class ExpressionEvaluator* evaluator_;
    
    Status evaluate_predicate(const DataChunk& input_chunk, std::vector<bool>& selection);
    Status apply_selection(const DataChunk& input_chunk, const std::vector<bool>& selection, DataChunk& output_chunk);
};

// 表达式求值器
class ExpressionEvaluator {
public:
    explicit ExpressionEvaluator(Expression* expr) : expression_(expr) {}
    
    // 对数据块中的所有行求值表达式
    Status evaluate(const DataChunk& chunk, std::vector<bool>& results);
    
private:
    Expression* expression_;
    
    // 求值单个表达式
    Status evaluate_expression(Expression* expr, const DataChunk& chunk, size_t row_index, std::string& result);
    Status evaluate_literal(LiteralExpression* expr, std::string& result);
    Status evaluate_column_ref(ColumnRefExpression* expr, const DataChunk& chunk, size_t row_index, std::string& result);
    Status evaluate_binary_expr(BinaryExpression* expr, const DataChunk& chunk, size_t row_index, std::string& result);
    Status evaluate_function_expr(FunctionExpression* expr, const DataChunk& chunk, size_t row_index, std::string& result);
    
    // 辅助方法
    int find_column_index(const DataChunk& chunk, const std::string& column_name);
    bool compare_values(const std::string& left, const std::string& right, BinaryOperatorType op, DataType type);
    std::string apply_arithmetic(const std::string& left, const std::string& right, BinaryOperatorType op, DataType type);
};

} // namespace minidb

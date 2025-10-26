#pragma once

#include "exec/operator.h"
#include "sql/compiler/statement.h"
#include <memory>

namespace minidb {

// Nested Loop Join算子
// 实现简单的嵌套循环连接算法
class NestedLoopJoinOperator : public Operator {
public:
    NestedLoopJoinOperator(
        std::unique_ptr<Operator> left_child,
        std::unique_ptr<Operator> right_child,
        std::unique_ptr<Expression> join_condition,
        JoinType join_type);

    ~NestedLoopJoinOperator() override = default;

    // Operator接口实现
    Status initialize(ExecutionContext* context) override;
    Status get_next(ExecutionContext* context, DataChunk& chunk) override;
    Status reset() override;

    std::vector<std::string> get_output_columns() const override;
    std::vector<DataType> get_output_types() const override;

private:
    std::unique_ptr<Operator> left_child_;      // 左表算子
    std::unique_ptr<Operator> right_child_;     // 右表算子
    std::unique_ptr<Expression> join_condition_; // JOIN条件
    JoinType join_type_;                        // JOIN类型

    // 缓存的左表数据块
    DataChunk left_chunk_;
    size_t left_row_index_;     // 当前左表行索引
    bool left_finished_;        // 左表是否读取完毕

    // 缓存的右表数据块
    DataChunk right_chunk_;
    size_t right_row_index_;    // 当前右表行索引
    bool right_finished_;       // 右表是否读取完毕

    // 输出列信息
    std::vector<std::string> output_columns_;
    std::vector<DataType> output_types_;

    // 辅助方法
    bool evaluate_join_condition(const DataChunk& left, size_t left_idx,
                                 const DataChunk& right, size_t right_idx);
    void merge_rows(const DataChunk& left, size_t left_idx,
                   const DataChunk& right, size_t right_idx,
                   DataChunk& output);
    Status fetch_left_chunk(ExecutionContext* context);
    Status fetch_right_chunk(ExecutionContext* context);
};

} // namespace minidb

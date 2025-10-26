#include "exec/operators/nested_loop_join_operator.h"
#include "exec/operators/typed_expression_evaluator.h"
#include "log/logger.h"

namespace minidb {

NestedLoopJoinOperator::NestedLoopJoinOperator(
    std::unique_ptr<Operator> left_child,
    std::unique_ptr<Operator> right_child,
    std::unique_ptr<Expression> join_condition,
    JoinType join_type)
    : Operator(OperatorType::NESTED_LOOP_JOIN),
      left_child_(std::move(left_child)),
      right_child_(std::move(right_child)),
      join_condition_(std::move(join_condition)),
      join_type_(join_type),
      left_row_index_(0),
      left_finished_(false),
      right_row_index_(0),
      right_finished_(false) {
}

Status NestedLoopJoinOperator::initialize(ExecutionContext* context) {
    LOG_INFO("NestedLoopJoinOperator", "Query#" + std::to_string(context->query_id),
             "Initializing nested loop join operator");

    if (!left_child_ || !right_child_) {
        return Status::InvalidArgument("Join operator requires two child operators");
    }

    // 初始化左右子算子
    Status status = left_child_->initialize(context);
    if (!status.ok()) {
        return status;
    }

    status = right_child_->initialize(context);
    if (!status.ok()) {
        return status;
    }

    // 构建输出列信息（左表列 + 右表列）
    std::vector<std::string> left_columns = left_child_->get_output_columns();
    std::vector<std::string> right_columns = right_child_->get_output_columns();

    output_columns_ = left_columns;
    output_types_ = left_child_->get_output_types();

    std::vector<DataType> right_types = right_child_->get_output_types();

    output_columns_.insert(output_columns_.end(), right_columns.begin(), right_columns.end());
    output_types_.insert(output_types_.end(), right_types.begin(), right_types.end());

    // 记录左表列数，用于区分左右表列
    left_column_count_ = left_columns.size();

    // 重置状态
    left_row_index_ = 0;
    left_finished_ = false;
    right_row_index_ = 0;
    right_finished_ = false;
    left_chunk_.clear();
    right_chunk_.clear();

    set_state(OperatorState::READY);
    LOG_INFO("NestedLoopJoinOperator", "Query#" + std::to_string(context->query_id),
             "Join operator initialized with " + std::to_string(output_columns_.size()) + " output columns");

    return Status::OK();
}

Status NestedLoopJoinOperator::get_next(ExecutionContext* context, DataChunk& chunk) {
    chunk.clear();

    if (get_state() == OperatorState::FINISHED) {
        return Status::OK();
    }

    set_state(OperatorState::RUNNING);

    // Nested loop join 实现
    // 外层循环遍历左表，内层循环遍历右表
    while (true) {
        // 如果左表当前chunk处理完，获取新的左表chunk
        if (left_chunk_.empty() || left_row_index_ >= left_chunk_.row_count) {
            Status status = fetch_left_chunk(context);
            if (!status.ok()) {
                set_state(OperatorState::ERROR);
                return status;
            }

            if (left_finished_) {
                set_state(OperatorState::FINISHED);
                return Status::OK();
            }

            left_row_index_ = 0;
            // 重新开始右表
            right_chunk_.clear();
            right_row_index_ = 0;
            right_finished_ = false;
            Status reset_status = right_child_->reset();
            if (!reset_status.ok()) {
                set_state(OperatorState::ERROR);
                return reset_status;
            }
        }

        // 如果右表当前chunk处理完，获取新的右表chunk
        if (right_chunk_.empty() || right_row_index_ >= right_chunk_.row_count) {
            Status status = fetch_right_chunk(context);
            if (!status.ok()) {
                set_state(OperatorState::ERROR);
                return status;
            }

            if (right_finished_) {
                // 右表遍历完，移动到下一个左表行
                left_row_index_++;
                right_chunk_.clear();
                right_row_index_ = 0;
                right_finished_ = false;
                Status reset_status = right_child_->reset();
                if (!reset_status.ok()) {
                    set_state(OperatorState::ERROR);
                    return reset_status;
                }
                continue;
            }

            right_row_index_ = 0;
        }

        // 评估JOIN条件
        if (evaluate_join_condition(left_chunk_, left_row_index_,
                                    right_chunk_, right_row_index_)) {
            // 条件满足，合并行并返回
            merge_rows(left_chunk_, left_row_index_,
                      right_chunk_, right_row_index_,
                      chunk);

            right_row_index_++;
            return Status::OK();
        }

        // 条件不满足，继续下一个右表行
        right_row_index_++;
    }

    return Status::OK();
}

Status NestedLoopJoinOperator::reset() {
    left_row_index_ = 0;
    left_finished_ = false;
    right_row_index_ = 0;
    right_finished_ = false;
    left_chunk_.clear();
    right_chunk_.clear();

    if (left_child_) {
        Status status = left_child_->reset();
        if (!status.ok()) {
            return status;
        }
    }

    if (right_child_) {
        Status status = right_child_->reset();
        if (!status.ok()) {
            return status;
        }
    }

    set_state(OperatorState::READY);
    return Status::OK();
}

std::vector<std::string> NestedLoopJoinOperator::get_output_columns() const {
    return output_columns_;
}

std::vector<DataType> NestedLoopJoinOperator::get_output_types() const {
    return output_types_;
}

// ============= 私有辅助方法 =============

Status NestedLoopJoinOperator::fetch_left_chunk(ExecutionContext* context) {
    left_chunk_.clear();
    Status status = left_child_->get_next(context, left_chunk_);
    if (!status.ok()) {
        return status;
    }

    if (left_chunk_.empty()) {
        left_finished_ = true;
    }

    return Status::OK();
}

Status NestedLoopJoinOperator::fetch_right_chunk(ExecutionContext* context) {
    right_chunk_.clear();
    Status status = right_child_->get_next(context, right_chunk_);
    if (!status.ok()) {
        return status;
    }

    if (right_chunk_.empty()) {
        right_finished_ = true;
    }

    return Status::OK();
}

bool NestedLoopJoinOperator::evaluate_join_condition(
    const DataChunk& left, size_t left_idx,
    const DataChunk& right, size_t right_idx) {

    if (!join_condition_) {
        // 没有JOIN条件，返回true（笛卡尔积）
        return true;
    }

    // 创建合并的DataChunk用于求值
    DataChunk merged_chunk;
    merged_chunk.row_count = 1;

    // 添加左表的列（列名已经是完整限定名：表名.列名）
    for (const auto& left_col : left.columns) {
        ColumnVector col(left_col.name, left_col.type);
        switch (left_col.type) {
            case DataType::INT:
                col.append_int(left_col.get_int(left_idx));
                break;
            case DataType::STRING:
                col.append_string(left_col.get_string(left_idx));
                break;
            case DataType::BOOL:
                col.append_bool(left_col.get_bool(left_idx));
                break;
            case DataType::DECIMAL:
                col.append_decimal(left_col.get_decimal(left_idx));
                break;
        }
        merged_chunk.add_column(col);
    }

    // 添加右表的列（列名已经是完整限定名：表名.列名）
    for (const auto& right_col : right.columns) {
        ColumnVector col(right_col.name, right_col.type);
        switch (right_col.type) {
            case DataType::INT:
                col.append_int(right_col.get_int(right_idx));
                break;
            case DataType::STRING:
                col.append_string(right_col.get_string(right_idx));
                break;
            case DataType::BOOL:
                col.append_bool(right_col.get_bool(right_idx));
                break;
            case DataType::DECIMAL:
                col.append_decimal(right_col.get_decimal(right_idx));
                break;
        }
        merged_chunk.add_column(col);
    }

    // 使用TypedExpressionEvaluator求值
    TypedExpressionEvaluator evaluator(join_condition_.get());
    Value result;
    Status status = evaluator.evaluate_row(merged_chunk, 0, result);

    if (!status.ok()) {
        LOG_ERROR("NestedLoopJoinOperator", "EvaluateJoinCondition",
                 "JOIN condition evaluation failed: " + status.message());
        return false;
    }

    // 将Value转换为bool
    return result.as_bool();
}

void NestedLoopJoinOperator::merge_rows(
    const DataChunk& left, size_t left_idx,
    const DataChunk& right, size_t right_idx,
    DataChunk& output) {

    output.clear();
    output.row_count = 1;

    // 复制左表的列
    for (const auto& left_col : left.columns) {
        ColumnVector col(left_col.name, left_col.type);
        switch (left_col.type) {
            case DataType::INT:
                col.append_int(left_col.get_int(left_idx));
                break;
            case DataType::STRING:
                col.append_string(left_col.get_string(left_idx));
                break;
            case DataType::BOOL:
                col.append_bool(left_col.get_bool(left_idx));
                break;
            case DataType::DECIMAL:
                col.append_decimal(left_col.get_decimal(left_idx));
                break;
        }
        output.add_column(col);
    }

    // 复制右表的列
    for (const auto& right_col : right.columns) {
        ColumnVector col(right_col.name, right_col.type);
        switch (right_col.type) {
            case DataType::INT:
                col.append_int(right_col.get_int(right_idx));
                break;
            case DataType::STRING:
                col.append_string(right_col.get_string(right_idx));
                break;
            case DataType::BOOL:
                col.append_bool(right_col.get_bool(right_idx));
                break;
            case DataType::DECIMAL:
                col.append_decimal(right_col.get_decimal(right_idx));
                break;
        }
        output.add_column(col);
    }
}

} // namespace minidb

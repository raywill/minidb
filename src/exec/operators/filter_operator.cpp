#include "exec/operators/filter_operator.h"
#include "exec/operators/typed_expression_evaluator.h"
#include "log/logger.h"
#include "common/utils.h"
#include <cmath>
#include <algorithm>

namespace minidb {

FilterOperator::FilterOperator(std::unique_ptr<Expression> predicate)
    : PushOperator(OperatorType::FILTER), predicate_(std::move(predicate)),
      predicate_ptr_(predicate_.get()), owns_predicate_(true) {
    evaluator_ = new TypedExpressionEvaluator(predicate_ptr_);
}

FilterOperator::FilterOperator(Expression* predicate)
    : PushOperator(OperatorType::FILTER), predicate_ptr_(predicate), owns_predicate_(false) {
    evaluator_ = new TypedExpressionEvaluator(predicate_ptr_);
}

FilterOperator::~FilterOperator() {
    delete evaluator_;
}

Status FilterOperator::initialize(ExecutionContext* context) {
    LOG_INFO("FilterOperator", "Query#" + std::to_string(context->query_id), 
             "Initializing filter operator");
    
    if (!child_) {
        return Status::InvalidArgument("Filter operator requires a child operator");
    }
    
    Status status = child_->initialize(context);
    if (!status.ok()) {
        return status;
    }
    
    set_state(OperatorState::READY);
    return Status::OK();
}

Status FilterOperator::get_next(ExecutionContext* context, DataChunk& chunk) {
    chunk.clear();
    
    if (get_state() == OperatorState::FINISHED) {
        return Status::OK();
    }
    
    set_state(OperatorState::RUNNING);
    
    while (true) {
        DataChunk input_chunk;
        Status status = child_->get_next(context, input_chunk);
        if (!status.ok()) {
            set_state(OperatorState::ERROR);
            return status;
        }
        
        if (input_chunk.empty()) {
            set_state(OperatorState::FINISHED);
            return Status::OK();
        }
        
        // 评估谓词
        std::vector<bool> selection;
        status = evaluate_predicate(input_chunk, selection);
        if (!status.ok()) {
            set_state(OperatorState::ERROR);
            return status;
        }
        
        // 应用过滤
        status = apply_selection(input_chunk, selection, chunk);
        if (!status.ok()) {
            set_state(OperatorState::ERROR);
            return status;
        }
        
        // 如果过滤后有数据，返回
        if (!chunk.empty()) {
            LOG_DEBUG("FilterOperator", "Query#" + std::to_string(context->query_id), 
                     "Filtered " + std::to_string(input_chunk.row_count) + " rows to " + 
                     std::to_string(chunk.row_count) + " rows");
            return Status::OK();
        }
        
        // 如果过滤后没有数据，继续获取下一批
    }
}

Status FilterOperator::reset() {
    if (child_) {
        Status status = child_->reset();
        if (!status.ok()) {
            return status;
        }
    }
    
    set_state(OperatorState::READY);
    return Status::OK();
}

std::vector<std::string> FilterOperator::get_output_columns() const {
    if (child_) {
        return child_->get_output_columns();
    }
    return {};
}

std::vector<DataType> FilterOperator::get_output_types() const {
    if (child_) {
        return child_->get_output_types();
    }
    return {};
}

Status FilterOperator::evaluate_predicate(const DataChunk& input_chunk, std::vector<bool>& selection) {
    // 使用类型化求值器获取 Value 结果
    std::vector<Value> results;
    Status status = evaluator_->evaluate(input_chunk, results);
    if (!status.ok()) {
        return status;
    }

    // 将 Value 结果转换为布尔选择向量
    selection.clear();
    selection.reserve(results.size());
    for (const Value& val : results) {
        selection.push_back(val.as_bool());
    }

    return Status::OK();
}

Status FilterOperator::apply_selection(const DataChunk& input_chunk, const std::vector<bool>& selection, DataChunk& output_chunk) {
    output_chunk.clear();
    
    // 计算选中的行数
    size_t selected_count = 0;
    for (bool selected : selection) {
        if (selected) {
            selected_count++;
        }
    }
    
    if (selected_count == 0) {
        return Status::OK();
    }
    
    output_chunk.row_count = selected_count;
    
    // 为每一列创建过滤后的数据
    for (size_t col_idx = 0; col_idx < input_chunk.columns.size(); ++col_idx) {
        const ColumnVector& input_col = input_chunk.columns[col_idx];
        ColumnVector output_col(input_col.name, input_col.type);
        output_col.reserve(selected_count);
        
        // 复制选中的行
        for (size_t row_idx = 0; row_idx < selection.size(); ++row_idx) {
            if (selection[row_idx]) {
                switch (input_col.type) {
                    case DataType::INT:
                        output_col.append_int(input_col.get_int(row_idx));
                        break;
                    case DataType::STRING:
                        output_col.append_string(input_col.get_string(row_idx));
                        break;
                    case DataType::BOOL:
                        output_col.append_bool(input_col.get_bool(row_idx));
                        break;
                    case DataType::DECIMAL:
                        output_col.append_decimal(input_col.get_decimal(row_idx));
                        break;
                }
            }
        }
        
        output_chunk.add_column(output_col);
    }
    
    return Status::OK();
}

} // namespace minidb

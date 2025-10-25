#include "exec/operators/projection_operator.h"
#include "log/logger.h"

namespace minidb {

ProjectionOperator::ProjectionOperator(const std::vector<std::string>& columns)
    : PushOperator(OperatorType::PROJECTION), projection_columns_(columns) {
}

Status ProjectionOperator::initialize(ExecutionContext* context) {
    LOG_INFO("ProjectionOperator", "Query#" + std::to_string(context->query_id), 
             "Initializing projection operator");
    
    if (!child_) {
        return Status::InvalidArgument("Projection operator requires a child operator");
    }
    
    Status status = child_->initialize(context);
    if (!status.ok()) {
        return status;
    }
    
    // 构建投影映射
    std::vector<std::string> input_columns = child_->get_output_columns();
    std::vector<DataType> input_types = child_->get_output_types();
    
    status = build_projection_mapping(input_columns, input_types);
    if (!status.ok()) {
        return status;
    }
    
    set_state(OperatorState::READY);
    
    LOG_INFO("ProjectionOperator", "Query#" + std::to_string(context->query_id), 
             "Projection operator initialized with " + std::to_string(projection_columns_.size()) + " columns");
    return Status::OK();
}

Status ProjectionOperator::get_next(ExecutionContext* context, DataChunk& chunk) {
    chunk.clear();
    
    if (get_state() == OperatorState::FINISHED) {
        return Status::OK();
    }
    
    set_state(OperatorState::RUNNING);
    
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
    
    // 应用投影
    status = apply_projection(input_chunk, chunk);
    if (!status.ok()) {
        set_state(OperatorState::ERROR);
        return status;
    }
    
    LOG_DEBUG("ProjectionOperator", "Query#" + std::to_string(context->query_id), 
              "Projected " + std::to_string(input_chunk.columns.size()) + " columns to " + 
              std::to_string(chunk.columns.size()) + " columns");
    
    return Status::OK();
}

Status ProjectionOperator::reset() {
    if (child_) {
        Status status = child_->reset();
        if (!status.ok()) {
            return status;
        }
    }
    
    set_state(OperatorState::READY);
    return Status::OK();
}

std::vector<std::string> ProjectionOperator::get_output_columns() const {
    return projection_columns_;
}

std::vector<DataType> ProjectionOperator::get_output_types() const {
    return output_types_;
}

Status ProjectionOperator::build_projection_mapping(const std::vector<std::string>& input_columns,
                                                   const std::vector<DataType>& input_types) {
    column_indices_.clear();
    output_types_.clear();
    
    for (const std::string& proj_col : projection_columns_) {
        // 特殊处理 * (选择所有列)
        if (proj_col == "*") {
            // 添加所有输入列
            for (size_t i = 0; i < input_columns.size(); ++i) {
                column_indices_.push_back(static_cast<int>(i));
                output_types_.push_back(input_types[i]);
            }
            // 更新输出列名为实际列名
            projection_columns_.clear();
            projection_columns_ = input_columns;
            break;
        } else {
            // 查找列在输入中的索引
            int col_index = -1;
            for (size_t i = 0; i < input_columns.size(); ++i) {
                if (input_columns[i] == proj_col) {
                    col_index = static_cast<int>(i);
                    break;
                }
            }
            
            if (col_index < 0) {
                return Status::NotFound("Column not found in input: " + proj_col);
            }
            
            column_indices_.push_back(col_index);
            output_types_.push_back(input_types[col_index]);
        }
    }
    
    return Status::OK();
}

Status ProjectionOperator::apply_projection(const DataChunk& input_chunk, DataChunk& output_chunk) {
    output_chunk.clear();
    output_chunk.row_count = input_chunk.row_count;
    
    for (size_t i = 0; i < projection_columns_.size(); ++i) {
        int input_col_index = column_indices_[i];
        const ColumnVector& input_col = input_chunk.columns[input_col_index];
        
        // 创建输出列（直接复制）
        ColumnVector output_col = input_col;
        output_col.name = projection_columns_[i];
        
        output_chunk.add_column(output_col);
    }
    
    return Status::OK();
}

} // namespace minidb

#include "exec/operators/final_result_operator.h"
#include "log/logger.h"

namespace minidb {

FinalResultOperator::FinalResultOperator()
    : PushOperator(OperatorType::FINAL_RESULT), header_written_(false) {
}

Status FinalResultOperator::initialize(ExecutionContext* context) {
    LOG_INFO("FinalResultOperator", "Query#" + std::to_string(context->query_id), 
             "Initializing final result operator");
    
    if (!child_) {
        return Status::InvalidArgument("Final result operator requires a child operator");
    }
    
    Status status = child_->initialize(context);
    if (!status.ok()) {
        return status;
    }
    
    result_stream_.str("");
    result_stream_.clear();
    header_written_ = false;
    
    set_state(OperatorState::READY);
    return Status::OK();
}

Status FinalResultOperator::get_next(ExecutionContext* context, DataChunk& chunk) {
    chunk.clear();
    
    if (get_state() == OperatorState::FINISHED) {
        return Status::OK();
    }
    
    set_state(OperatorState::RUNNING);
    
    // 写入表头
    if (!header_written_) {
        std::vector<std::string> columns = child_->get_output_columns();
        Status status = write_header(columns);
        if (!status.ok()) {
            set_state(OperatorState::ERROR);
            return status;
        }
        header_written_ = true;
    }
    
    // 处理所有数据
    while (true) {
        DataChunk input_chunk;
        Status status = child_->get_next(context, input_chunk);
        if (!status.ok()) {
            set_state(OperatorState::ERROR);
            return status;
        }
        
        if (input_chunk.empty()) {
            break;
        }
        
        status = write_data_chunk(input_chunk);
        if (!status.ok()) {
            set_state(OperatorState::ERROR);
            return status;
        }
    }
    
    set_state(OperatorState::FINISHED);
    
    LOG_INFO("FinalResultOperator", "Query#" + std::to_string(context->query_id), 
             "Final result generated successfully");
    
    return Status::OK();
}

Status FinalResultOperator::reset() {
    if (child_) {
        Status status = child_->reset();
        if (!status.ok()) {
            return status;
        }
    }
    
    result_stream_.str("");
    result_stream_.clear();
    header_written_ = false;
    set_state(OperatorState::READY);
    return Status::OK();
}

std::vector<std::string> FinalResultOperator::get_output_columns() const {
    if (child_) {
        return child_->get_output_columns();
    }
    return {};
}

std::vector<DataType> FinalResultOperator::get_output_types() const {
    if (child_) {
        return child_->get_output_types();
    }
    return {};
}

std::string FinalResultOperator::get_result_text() const {
    return result_stream_.str();
}

Status FinalResultOperator::write_header(const std::vector<std::string>& columns) {
    if (columns.empty()) {
        return Status::OK();
    }
    
    for (size_t i = 0; i < columns.size(); ++i) {
        if (i > 0) {
            result_stream_ << " | ";
        }
        result_stream_ << columns[i];
    }
    result_stream_ << "\n";
    
    return Status::OK();
}

Status FinalResultOperator::write_data_chunk(const DataChunk& chunk) {
    for (size_t row = 0; row < chunk.row_count; ++row) {
        for (size_t col = 0; col < chunk.columns.size(); ++col) {
            if (col > 0) {
                result_stream_ << " | ";
            }
            
            std::string value = format_value(chunk.columns[col], row);
            result_stream_ << value;
        }
        result_stream_ << "\n";
    }
    
    return Status::OK();
}

std::string FinalResultOperator::format_value(const ColumnVector& column, size_t row_index) {
    switch (column.type) {
        case DataType::INT:
            return std::to_string(column.get_int(row_index));
        case DataType::STRING:
            return column.get_string(row_index);
        case DataType::BOOL:
            return column.get_bool(row_index) ? "true" : "false";
        case DataType::DECIMAL: {
            double value = column.get_decimal(row_index);
            // 格式化小数，保留2位小数
            char buffer[32];
            snprintf(buffer, sizeof(buffer), "%.2f", value);
            return std::string(buffer);
        }
        default:
            return "NULL";
    }
}

} // namespace minidb

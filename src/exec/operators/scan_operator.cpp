#include "exec/operators/scan_operator.h"
#include "log/logger.h"
#include <algorithm>

namespace minidb {

ScanOperator::ScanOperator(const std::string& table_name, 
                          const std::vector<std::string>& columns,
                          std::shared_ptr<Table> table)
    : Operator(OperatorType::SCAN), 
      table_name_(table_name), 
      columns_(columns), 
      table_(table),
      current_offset_(0),
      batch_size_(DEFAULT_BATCH_SIZE),
      data_loaded_(false) {
}

Status ScanOperator::initialize(ExecutionContext* context) {
    LOG_INFO("ScanOperator", "Query#" + std::to_string(context->query_id),
             "Initializing scan on table: " + table_name_);

    if (!table_) {
        return Status::InvalidArgument("Table is null");
    }

    // 验证列是否存在
    const TableSchema& schema = table_->get_schema();
    for (const std::string& col_name : columns_) {
        if (schema.get_column_index(col_name) < 0) {
            return Status::NotFound("Column not found: " + col_name);
        }
    }

    // 构建完整限定的输出列名（表名.列名）
    output_columns_.clear();
    output_columns_.reserve(columns_.size());
    for (const std::string& col_name : columns_) {
        output_columns_.push_back(table_name_ + "." + col_name);
    }

    current_offset_ = 0;
    data_loaded_ = false;
    set_state(OperatorState::READY);

    LOG_INFO("ScanOperator", "Query#" + std::to_string(context->query_id),
             "Scan operator initialized successfully");
    return Status::OK();
}

Status ScanOperator::get_next(ExecutionContext* context, DataChunk& chunk) {
    chunk.clear();
    
    if (get_state() == OperatorState::FINISHED) {
        return Status::OK(); // 没有更多数据
    }
    
    set_state(OperatorState::RUNNING);
    
    // 第一次调用时加载数据
    if (!data_loaded_) {
        Status status = load_table_data();
        if (!status.ok()) {
            set_state(OperatorState::ERROR);
            return status;
        }
        data_loaded_ = true;
    }
    
    // 检查是否还有数据
    if (table_data_.empty() || current_offset_ >= table_data_[0].size) {
        set_state(OperatorState::FINISHED);
        return Status::OK();
    }
    
    // 计算这一批的行数
    size_t remaining_rows = table_data_[0].size - current_offset_;
    size_t chunk_size = std::min(batch_size_, remaining_rows);
    
    // 创建数据块
    Status status = create_chunk_from_offset(current_offset_, chunk_size, chunk);
    if (!status.ok()) {
        set_state(OperatorState::ERROR);
        return status;
    }
    
    current_offset_ += chunk_size;
    
    LOG_DEBUG("ScanOperator", "Query#" + std::to_string(context->query_id), 
              "Read " + std::to_string(chunk_size) + " rows from table: " + table_name_);
    
    return Status::OK();
}

Status ScanOperator::reset() {
    current_offset_ = 0;
    data_loaded_ = false;
    set_state(OperatorState::READY);
    return Status::OK();
}

std::vector<std::string> ScanOperator::get_output_columns() const {
    return output_columns_;  // 返回完整限定名（表名.列名）
}

std::vector<DataType> ScanOperator::get_output_types() const {
    std::vector<DataType> types;
    const TableSchema& schema = table_->get_schema();
    
    for (const std::string& col_name : columns_) {
        types.push_back(schema.get_column_type(col_name));
    }
    
    return types;
}

Status ScanOperator::load_table_data() {
    if (columns_.empty()) {
        // 扫描所有列
        return table_->scan_all(table_data_);
    } else {
        // 扫描指定列
        return table_->scan_columns(columns_, table_data_);
    }
}

Status ScanOperator::create_chunk_from_offset(size_t offset, size_t count, DataChunk& chunk) {
    chunk.clear();
    chunk.row_count = count;

    for (size_t col_idx = 0; col_idx < table_data_.size(); ++col_idx) {
        const ColumnVector& source_col = table_data_[col_idx];

        // 使用完整限定名：表名.列名
        std::string qualified_name = table_name_ + "." + source_col.name;
        ColumnVector chunk_col(qualified_name, source_col.type);
        chunk_col.size = count;
        
        // 复制数据
        if (source_col.type == DataType::STRING) {
            // 字符串类型需要特殊处理
            for (size_t i = 0; i < count; ++i) {
                std::string value = source_col.get_string(offset + i);
                chunk_col.append_string(value);
            }
        } else {
            // 固定大小类型
            size_t type_size = GetDataTypeSize(source_col.type);
            size_t start_byte = offset * type_size;
            size_t copy_bytes = count * type_size;
            
            chunk_col.data.resize(copy_bytes);
            std::copy(source_col.data.begin() + start_byte,
                     source_col.data.begin() + start_byte + copy_bytes,
                     chunk_col.data.begin());
        }
        
        chunk.add_column(chunk_col);
    }
    
    return Status::OK();
}

} // namespace minidb

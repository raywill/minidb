#pragma once

#include "exec/operator.h"
#include "storage/table.h"
#include <string>
#include <vector>

namespace minidb {

// 扫描算子 - 从表中读取数据
class ScanOperator : public Operator {
public:
    ScanOperator(const std::string& table_name, 
                const std::vector<std::string>& columns,
                std::shared_ptr<Table> table);
    
    ~ScanOperator() override = default;
    
    Status initialize(ExecutionContext* context) override;
    Status get_next(ExecutionContext* context, DataChunk& chunk) override;
    Status reset() override;
    
    std::vector<std::string> get_output_columns() const override;
    std::vector<DataType> get_output_types() const override;
    
private:
    std::string table_name_;
    std::vector<std::string> columns_;  // 原始列名（不带表前缀）
    std::vector<std::string> output_columns_;  // 完整限定名（表名.列名）
    std::shared_ptr<Table> table_;

    // 扫描状态
    std::vector<ColumnVector> table_data_;
    size_t current_offset_;
    size_t batch_size_;
    bool data_loaded_;

    Status load_table_data();
    Status create_chunk_from_offset(size_t offset, size_t count, DataChunk& chunk);
};

} // namespace minidb

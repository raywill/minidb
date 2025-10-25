#pragma once

#include "exec/operator.h"
#include <vector>
#include <string>

namespace minidb {

// 投影算子 - 选择指定的列
class ProjectionOperator : public PushOperator {
public:
    explicit ProjectionOperator(const std::vector<std::string>& columns);
    ~ProjectionOperator() override = default;
    
    Status initialize(ExecutionContext* context) override;
    Status get_next(ExecutionContext* context, DataChunk& chunk) override;
    Status reset() override;
    
    std::vector<std::string> get_output_columns() const override;
    std::vector<DataType> get_output_types() const override;
    
private:
    std::vector<std::string> projection_columns_;
    std::vector<int> column_indices_;  // 在输入数据中的列索引
    std::vector<DataType> output_types_;
    
    Status build_projection_mapping(const std::vector<std::string>& input_columns,
                                   const std::vector<DataType>& input_types);
    Status apply_projection(const DataChunk& input_chunk, DataChunk& output_chunk);
};

} // namespace minidb

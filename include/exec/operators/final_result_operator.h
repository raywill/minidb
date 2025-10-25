#pragma once

#include "exec/operator.h"
#include <sstream>

namespace minidb {

// 最终结果算子 - 将数据格式化为文本输出
class FinalResultOperator : public PushOperator {
public:
    FinalResultOperator();
    ~FinalResultOperator() override = default;
    
    Status initialize(ExecutionContext* context) override;
    Status get_next(ExecutionContext* context, DataChunk& chunk) override;
    Status reset() override;
    
    std::vector<std::string> get_output_columns() const override;
    std::vector<DataType> get_output_types() const override;
    
    // 获取格式化的结果文本
    std::string get_result_text() const;
    
private:
    std::ostringstream result_stream_;
    bool header_written_;
    
    Status write_header(const std::vector<std::string>& columns);
    Status write_data_chunk(const DataChunk& chunk);
    std::string format_value(const ColumnVector& column, size_t row_index);
};

} // namespace minidb

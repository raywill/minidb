#pragma once

#include "common/types.h"
#include "common/status.h"
#include "mem/arena.h"
#include <vector>
#include <memory>

namespace minidb {

// 算子类型枚举
enum class OperatorType {
    SCAN,
    FILTER,
    PROJECTION,
    FINAL_RESULT
};

// 算子状态枚举
enum class OperatorState {
    READY,      // 准备就绪
    RUNNING,    // 正在执行
    FINISHED,   // 执行完成
    ERROR       // 执行错误
};

// 执行上下文
struct ExecutionContext {
    Arena* arena;
    size_t query_id;
    
    ExecutionContext(Arena* a, size_t qid) : arena(a), query_id(qid) {}
};

// 数据块 - 包含一批列向量
struct DataChunk {
    std::vector<ColumnVector> columns;
    size_t row_count;
    
    DataChunk() : row_count(0) {}
    
    void clear() {
        columns.clear();
        row_count = 0;
    }
    
    bool empty() const {
        return row_count == 0;
    }
    
    void add_column(const ColumnVector& column) {
        columns.push_back(column);
        if (columns.size() == 1) {
            row_count = column.size;
        }
    }
};

// 算子基类
class Operator {
public:
    explicit Operator(OperatorType type) : type_(type), state_(OperatorState::READY) {}
    virtual ~Operator() = default;
    
    // 获取算子类型
    OperatorType get_type() const { return type_; }
    
    // 获取算子状态
    OperatorState get_state() const { return state_; }
    
    // 初始化算子
    virtual Status initialize(ExecutionContext* context) = 0;
    
    // 获取下一个数据块
    virtual Status get_next(ExecutionContext* context, DataChunk& chunk) = 0;
    
    // 重置算子状态
    virtual Status reset() = 0;
    
    // 获取输出列信息
    virtual std::vector<std::string> get_output_columns() const = 0;
    virtual std::vector<DataType> get_output_types() const = 0;
    
protected:
    OperatorType type_;
    OperatorState state_;
    
    void set_state(OperatorState state) { state_ = state; }
};

// Push模式算子基类
class PushOperator : public Operator {
public:
    explicit PushOperator(OperatorType type) : Operator(type), child_(nullptr) {}
    
    // 设置子算子
    void set_child(std::unique_ptr<Operator> child) {
        child_ = std::move(child);
    }
    
    Operator* get_child() const { return child_.get(); }
    
protected:
    std::unique_ptr<Operator> child_;
};

// 算子工厂
class OperatorFactory {
public:
    static std::unique_ptr<Operator> create_scan_operator(const std::string& table_name,
                                                         const std::vector<std::string>& columns);
    
    static std::unique_ptr<Operator> create_filter_operator(std::unique_ptr<class Expression> predicate);
    
    static std::unique_ptr<Operator> create_projection_operator(const std::vector<std::string>& columns);
    
    static std::unique_ptr<Operator> create_final_result_operator();
};

} // namespace minidb

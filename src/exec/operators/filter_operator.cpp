#include "exec/operators/filter_operator.h"
#include "log/logger.h"
#include "common/utils.h"
#include <cmath>
#include <algorithm>

namespace minidb {

FilterOperator::FilterOperator(std::unique_ptr<Expression> predicate)
    : PushOperator(OperatorType::FILTER), predicate_(std::move(predicate)), 
      predicate_ptr_(predicate_.get()), owns_predicate_(true) {
    evaluator_ = new ExpressionEvaluator(predicate_ptr_);
}

FilterOperator::FilterOperator(Expression* predicate)
    : PushOperator(OperatorType::FILTER), predicate_ptr_(predicate), owns_predicate_(false) {
    evaluator_ = new ExpressionEvaluator(predicate_ptr_);
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
    return evaluator_->evaluate(input_chunk, selection);
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

// ExpressionEvaluator 实现
Status ExpressionEvaluator::evaluate(const DataChunk& chunk, std::vector<bool>& results) {
    results.clear();
    results.resize(chunk.row_count);
    
    for (size_t i = 0; i < chunk.row_count; ++i) {
        std::string result_value;
        Status status = evaluate_expression(expression_, chunk, i, result_value);
        if (!status.ok()) {
            return status;
        }
        
        // 将结果转换为布尔值
        results[i] = (result_value == "true" || result_value == "TRUE" || result_value == "1");
    }
    
    return Status::OK();
}

Status ExpressionEvaluator::evaluate_expression(Expression* expr, const DataChunk& chunk, size_t row_index, std::string& result) {
    // 使用dynamic_cast判断Expression类型
    if (auto* literal = dynamic_cast<LiteralExpression*>(expr)) {
        return evaluate_literal(literal, result);
    } else if (auto* col_ref = dynamic_cast<ColumnRefExpression*>(expr)) {
        return evaluate_column_ref(col_ref, chunk, row_index, result);
    } else if (auto* binary = dynamic_cast<BinaryExpression*>(expr)) {
        return evaluate_binary_expr(binary, chunk, row_index, result);
    } else if (auto* func = dynamic_cast<FunctionExpression*>(expr)) {
        return evaluate_function_expr(func, chunk, row_index, result);
    } else {
        return Status::ExecutionError("Unsupported expression type");
    }
}

Status ExpressionEvaluator::evaluate_literal(LiteralExpression* expr, std::string& result) {
    result = expr->get_value();
    return Status::OK();
}

Status ExpressionEvaluator::evaluate_column_ref(ColumnRefExpression* expr, const DataChunk& chunk, size_t row_index, std::string& result) {
    int col_index = find_column_index(chunk, expr->get_column_name());
    if (col_index < 0) {
        // Debug: print available columns
        std::string avail_cols = "";
        for (size_t i = 0; i < chunk.columns.size(); ++i) {
            avail_cols += chunk.columns[i].name;
            if (i < chunk.columns.size() - 1) avail_cols += ", ";
        }
        return Status::NotFound("Column not found: '" + expr->get_column_name() +
                               "'. Available: [" + avail_cols + "]");
    }

    const ColumnVector& column = chunk.columns[col_index];

    switch (column.type) {
        case DataType::INT:
            result = std::to_string(column.get_int(row_index));
            break;
        case DataType::STRING:
            result = column.get_string(row_index);
            break;
        case DataType::BOOL:
            result = column.get_bool(row_index) ? "true" : "false";
            break;
        case DataType::DECIMAL:
            result = std::to_string(column.get_decimal(row_index));
            break;
    }

    return Status::OK();
}

Status ExpressionEvaluator::evaluate_binary_expr(BinaryExpression* expr, const DataChunk& chunk, size_t row_index, std::string& result) {
    std::string left_value, right_value;

    Status status = evaluate_expression(expr->get_left(), chunk, row_index, left_value);
    if (!status.ok()) {
        return status;
    }

    status = evaluate_expression(expr->get_right(), chunk, row_index, right_value);
    if (!status.ok()) {
        return status;
    }

    BinaryOperatorType op = expr->get_operator();

    // 比较操作符
    if (op >= BinaryOperatorType::EQUAL && op <= BinaryOperatorType::GREATER_EQUAL) {
        // 尝试数值比较（如果两边都是数字）
        bool is_numeric = true;
        try {
            std::stod(left_value);
            std::stod(right_value);
        } catch (...) {
            is_numeric = false;
        }

        bool comparison_result;
        if (is_numeric) {
            // 数值比较
            comparison_result = compare_values(left_value, right_value, op, DataType::INT);
        } else {
            // 字符串比较
            comparison_result = compare_values(left_value, right_value, op, DataType::STRING);
        }

        result = comparison_result ? "true" : "false";
        return Status::OK();
    }
    
    // 逻辑操作符
    if (op == BinaryOperatorType::AND || op == BinaryOperatorType::OR) {
        bool left_bool = (left_value == "true" || left_value == "TRUE" || left_value == "1");
        bool right_bool = (right_value == "true" || right_value == "TRUE" || right_value == "1");
        
        bool logical_result;
        if (op == BinaryOperatorType::AND) {
            logical_result = left_bool && right_bool;
        } else {
            logical_result = left_bool || right_bool;
        }
        
        result = logical_result ? "true" : "false";
        return Status::OK();
    }
    
    // 算术操作符
    result = apply_arithmetic(left_value, right_value, op, DataType::DECIMAL);
    return Status::OK();
}

Status ExpressionEvaluator::evaluate_function_expr(FunctionExpression* expr, const DataChunk& chunk, size_t row_index, std::string& result) {
    const auto& args = expr->get_arguments();
    FunctionType func_type = expr->get_function_type();
    
    switch (func_type) {
        case FunctionType::SIN: {
            if (args.size() != 1) {
                return Status::InvalidArgument("SIN function requires 1 argument");
            }
            
            std::string arg_value;
            Status status = evaluate_expression(args[0].get(), chunk, row_index, arg_value);
            if (!status.ok()) {
                return status;
            }
            
            double value = std::stod(arg_value);
            result = std::to_string(std::sin(value));
            return Status::OK();
        }
        
        case FunctionType::COS: {
            if (args.size() != 1) {
                return Status::InvalidArgument("COS function requires 1 argument");
            }
            
            std::string arg_value;
            Status status = evaluate_expression(args[0].get(), chunk, row_index, arg_value);
            if (!status.ok()) {
                return status;
            }
            
            double value = std::stod(arg_value);
            result = std::to_string(std::cos(value));
            return Status::OK();
        }
        
        case FunctionType::SUBSTR: {
            if (args.size() != 3) {
                return Status::InvalidArgument("SUBSTR function requires 3 arguments");
            }
            
            std::string str_value, start_value, length_value;
            Status status = evaluate_expression(args[0].get(), chunk, row_index, str_value);
            if (!status.ok()) return status;
            
            status = evaluate_expression(args[1].get(), chunk, row_index, start_value);
            if (!status.ok()) return status;
            
            status = evaluate_expression(args[2].get(), chunk, row_index, length_value);
            if (!status.ok()) return status;
            
            int start = std::stoi(start_value);
            int length = std::stoi(length_value);
            
            if (start < 0 || start >= static_cast<int>(str_value.length())) {
                result = "";
            } else {
                result = str_value.substr(start, length);
            }
            return Status::OK();
        }
        
        default:
            return Status::ExecutionError("Unsupported function type");
    }
}

int ExpressionEvaluator::find_column_index(const DataChunk& chunk, const std::string& column_name) {
    // Helper function to convert string to uppercase
    auto to_upper = [](const std::string& str) {
        std::string result = str;
        std::transform(result.begin(), result.end(), result.begin(),
                       [](unsigned char c) { return std::toupper(c); });
        return result;
    };

    std::string column_name_upper = to_upper(column_name);

    for (size_t i = 0; i < chunk.columns.size(); ++i) {
        std::string chunk_col_upper = to_upper(chunk.columns[i].name);
        if (chunk_col_upper == column_name_upper) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

bool ExpressionEvaluator::compare_values(const std::string& left, const std::string& right, BinaryOperatorType op, DataType type) {
    switch (op) {
        case BinaryOperatorType::EQUAL:
            if (type == DataType::STRING) {
                return left == right;
            } else {
                return std::stod(left) == std::stod(right);
            }
        case BinaryOperatorType::NOT_EQUAL:
            if (type == DataType::STRING) {
                return left != right;
            } else {
                return std::stod(left) != std::stod(right);
            }
        case BinaryOperatorType::LESS_THAN:
            if (type == DataType::STRING) {
                return left < right;
            } else {
                return std::stod(left) < std::stod(right);
            }
        case BinaryOperatorType::LESS_EQUAL:
            if (type == DataType::STRING) {
                return left <= right;
            } else {
                return std::stod(left) <= std::stod(right);
            }
        case BinaryOperatorType::GREATER_THAN:
            if (type == DataType::STRING) {
                return left > right;
            } else {
                return std::stod(left) > std::stod(right);
            }
        case BinaryOperatorType::GREATER_EQUAL:
            if (type == DataType::STRING) {
                return left >= right;
            } else {
                return std::stod(left) >= std::stod(right);
            }
        default:
            return false;
    }
}

std::string ExpressionEvaluator::apply_arithmetic(const std::string& left, const std::string& right, BinaryOperatorType op, DataType type) {
    double left_val = std::stod(left);
    double right_val = std::stod(right);
    double result_val;
    
    switch (op) {
        case BinaryOperatorType::ADD:
            result_val = left_val + right_val;
            break;
        case BinaryOperatorType::SUBTRACT:
            result_val = left_val - right_val;
            break;
        case BinaryOperatorType::MULTIPLY:
            result_val = left_val * right_val;
            break;
        case BinaryOperatorType::DIVIDE:
            if (right_val == 0) {
                return "0"; // 简单处理除零错误
            }
            result_val = left_val / right_val;
            break;
        default:
            return "0";
    }
    
    return std::to_string(result_val);
}

} // namespace minidb

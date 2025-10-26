#include "exec/operators/typed_expression_evaluator.h"
#include "storage/table.h"
#include "log/logger.h"
#include <algorithm>
#include <cmath>

namespace minidb {

// ============= 公共接口 =============

Status TypedExpressionEvaluator::evaluate(const DataChunk& chunk, std::vector<Value>& results) {
    results.clear();
    results.reserve(chunk.row_count);

    for (size_t i = 0; i < chunk.row_count; ++i) {
        Value result;
        Status status = evaluate_row(chunk, i, result);
        if (!status.ok()) {
            return status;
        }
        results.push_back(result);
    }

    return Status::OK();
}

Status TypedExpressionEvaluator::evaluate_row(const DataChunk& chunk, size_t row_index, Value& result) {
    // 根据表达式类型分发
    if (auto* literal = dynamic_cast<LiteralExpression*>(expression_)) {
        result = evaluate_literal(literal);
        return Status::OK();
    }

    if (auto* col_ref = dynamic_cast<ColumnRefExpression*>(expression_)) {
        result = evaluate_column_ref(col_ref, chunk, row_index);
        return Status::OK();
    }

    if (auto* binary = dynamic_cast<BinaryExpression*>(expression_)) {
        result = evaluate_binary_op(binary, chunk, row_index);
        return Status::OK();
    }

    if (auto* func = dynamic_cast<FunctionExpression*>(expression_)) {
        result = evaluate_function(func, chunk, row_index);
        return Status::OK();
    }

    return Status::ExecutionError("Unsupported expression type");
}

// ============= 表达式求值 =============

Value TypedExpressionEvaluator::evaluate_literal(LiteralExpression* expr) {
    // 根据类型解析字面量值
    const std::string& value_str = expr->get_value();
    DataType type = expr->get_data_type();

    switch (type) {
        case DataType::INT: {
            int64_t val = std::stoll(value_str);
            return Value::make_int(val);
        }
        case DataType::DECIMAL: {
            double val = std::stod(value_str);
            return Value::make_double(val);
        }
        case DataType::STRING:
            return Value::make_string(value_str);
        case DataType::BOOL: {
            bool val = (value_str == "true" || value_str == "TRUE" || value_str == "1");
            return Value::make_bool(val);
        }
        default:
            return Value::make_null();
    }
}

Value TypedExpressionEvaluator::evaluate_column_ref(ColumnRefExpression* expr,
                                                    const DataChunk& chunk,
                                                    size_t row_idx) {
    // 使用表名.列名查找
    int col_index = find_column_index(chunk, expr->get_table_name(), expr->get_column_name());

    if (col_index < 0) {
        LOG_ERROR("TypedExpressionEvaluator", "EvaluateColumnRef",
                 "Column not found: " + expr->get_table_name() + "." + expr->get_column_name());
        return Value::make_null();
    }

    const ColumnVector& column = chunk.columns[col_index];

    // 根据列类型读取值
    switch (column.type) {
        case DataType::INT:
            return Value::make_int(column.get_int(row_idx));
        case DataType::DECIMAL:
            return Value::make_double(column.get_decimal(row_idx));
        case DataType::STRING:
            return Value::make_string(column.get_string(row_idx));
        case DataType::BOOL:
            return Value::make_bool(column.get_bool(row_idx));
        default:
            return Value::make_null();
    }
}

Value TypedExpressionEvaluator::evaluate_binary_op(BinaryExpression* expr,
                                                   const DataChunk& chunk,
                                                   size_t row_idx) {
    // 递归求值左右子表达式
    TypedExpressionEvaluator left_eval(expr->get_left());
    TypedExpressionEvaluator right_eval(expr->get_right());

    Value left_val, right_val;
    Status status = left_eval.evaluate_row(chunk, row_idx, left_val);
    if (!status.ok()) {
        return Value::make_null();
    }

    status = right_eval.evaluate_row(chunk, row_idx, right_val);
    if (!status.ok()) {
        return Value::make_null();
    }

    BinaryOperatorType op = expr->get_operator();

    // 根据运算符类型分发
    if (op >= BinaryOperatorType::EQUAL && op <= BinaryOperatorType::GREATER_EQUAL) {
        return compute_comparison(left_val, right_val, op);
    }

    if (op == BinaryOperatorType::AND || op == BinaryOperatorType::OR) {
        return compute_logical(left_val, right_val, op);
    }

    // 算术运算
    return compute_arithmetic(left_val, right_val, op);
}

Value TypedExpressionEvaluator::evaluate_function(FunctionExpression* expr,
                                                  const DataChunk& chunk,
                                                  size_t row_idx) {
    const auto& args = expr->get_arguments();
    FunctionType func_type = expr->get_function_type();

    switch (func_type) {
        case FunctionType::SIN: {
            if (args.size() != 1) {
                return Value::make_null();
            }

            TypedExpressionEvaluator arg_eval(args[0].get());
            Value arg_val;
            Status status = arg_eval.evaluate_row(chunk, row_idx, arg_val);
            if (!status.ok()) {
                return Value::make_null();
            }

            double val = arg_val.as_double();
            return Value::make_double(std::sin(val));
        }

        case FunctionType::COS: {
            if (args.size() != 1) {
                return Value::make_null();
            }

            TypedExpressionEvaluator arg_eval(args[0].get());
            Value arg_val;
            Status status = arg_eval.evaluate_row(chunk, row_idx, arg_val);
            if (!status.ok()) {
                return Value::make_null();
            }

            double val = arg_val.as_double();
            return Value::make_double(std::cos(val));
        }

        case FunctionType::SUBSTR: {
            if (args.size() != 3) {
                return Value::make_null();
            }

            TypedExpressionEvaluator str_eval(args[0].get());
            TypedExpressionEvaluator start_eval(args[1].get());
            TypedExpressionEvaluator len_eval(args[2].get());

            Value str_val, start_val, len_val;
            Status status = str_eval.evaluate_row(chunk, row_idx, str_val);
            if (!status.ok()) return Value::make_null();

            status = start_eval.evaluate_row(chunk, row_idx, start_val);
            if (!status.ok()) return Value::make_null();

            status = len_eval.evaluate_row(chunk, row_idx, len_val);
            if (!status.ok()) return Value::make_null();

            std::string str = str_val.as_string();
            int64_t start = start_val.as_int();
            int64_t length = len_val.as_int();

            if (start < 0 || start >= static_cast<int64_t>(str.length())) {
                return Value::make_string("");
            }

            return Value::make_string(str.substr(start, length));
        }

        default:
            return Value::make_null();
    }
}

// ============= 二元运算 =============

Value TypedExpressionEvaluator::compute_arithmetic(const Value& left,
                                                   const Value& right,
                                                   BinaryOperatorType op) {
    // 根据结果类型选择计算方式
    // 如果任一值是 DECIMAL，使用浮点运算
    if (left.get_type() == DataType::DECIMAL || right.get_type() == DataType::DECIMAL) {
        double left_val = left.as_double();
        double right_val = right.as_double();

        switch (op) {
            case BinaryOperatorType::ADD:
                return Value::make_double(left_val + right_val);
            case BinaryOperatorType::SUBTRACT:
                return Value::make_double(left_val - right_val);
            case BinaryOperatorType::MULTIPLY:
                return Value::make_double(left_val * right_val);
            case BinaryOperatorType::DIVIDE:
                if (right_val == 0.0) {
                    return Value::make_double(0.0);  // 除零返回 0
                }
                return Value::make_double(left_val / right_val);
            default:
                return Value::make_null();
        }
    }

    // 整数运算
    int64_t left_val = left.as_int();
    int64_t right_val = right.as_int();

    switch (op) {
        case BinaryOperatorType::ADD:
            return Value::make_int(left_val + right_val);
        case BinaryOperatorType::SUBTRACT:
            return Value::make_int(left_val - right_val);
        case BinaryOperatorType::MULTIPLY:
            return Value::make_int(left_val * right_val);
        case BinaryOperatorType::DIVIDE:
            if (right_val == 0) {
                return Value::make_int(0);  // 除零返回 0
            }
            return Value::make_int(left_val / right_val);
        default:
            return Value::make_null();
    }
}

Value TypedExpressionEvaluator::compute_comparison(const Value& left,
                                                   const Value& right,
                                                   BinaryOperatorType op) {
    // 使用 Value 的重载运算符
    bool result;

    switch (op) {
        case BinaryOperatorType::EQUAL:
            result = (left == right);
            break;
        case BinaryOperatorType::NOT_EQUAL:
            result = (left != right);
            break;
        case BinaryOperatorType::LESS_THAN:
            result = (left < right);
            break;
        case BinaryOperatorType::LESS_EQUAL:
            result = (left <= right);
            break;
        case BinaryOperatorType::GREATER_THAN:
            result = (left > right);
            break;
        case BinaryOperatorType::GREATER_EQUAL:
            result = (left >= right);
            break;
        default:
            return Value::make_bool(false);
    }

    return Value::make_bool(result);
}

Value TypedExpressionEvaluator::compute_logical(const Value& left,
                                               const Value& right,
                                               BinaryOperatorType op) {
    bool left_bool = left.as_bool();
    bool right_bool = right.as_bool();

    switch (op) {
        case BinaryOperatorType::AND:
            return Value::make_bool(left_bool && right_bool);
        case BinaryOperatorType::OR:
            return Value::make_bool(left_bool || right_bool);
        default:
            return Value::make_bool(false);
    }
}

// ============= 辅助方法 =============

int TypedExpressionEvaluator::find_column_index(const DataChunk& chunk,
                                                const std::string& table_name,
                                                const std::string& column_name) {
    // 大小写不敏感的列名匹配
    auto to_upper = [](const std::string& str) {
        std::string result = str;
        std::transform(result.begin(), result.end(), result.begin(),
                      [](unsigned char c) { return std::toupper(c); });
        return result;
    };

    // 构建完整限定名：表名.列名
    std::string qualified_name = table_name + "." + column_name;
    std::string qualified_name_upper = to_upper(qualified_name);

    for (size_t i = 0; i < chunk.columns.size(); ++i) {
        std::string chunk_col_upper = to_upper(chunk.columns[i].name);
        if (chunk_col_upper == qualified_name_upper) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

} // namespace minidb

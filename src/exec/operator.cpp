#include "exec/operator.h"
#include "exec/operators/scan_operator.h"
#include "exec/operators/filter_operator.h"
#include "exec/operators/projection_operator.h"
#include "exec/operators/final_result_operator.h"
#include "common/utils.h"

namespace minidb {

std::unique_ptr<Operator> OperatorFactory::create_scan_operator(const std::string& table_name,
                                                               const std::vector<std::string>& columns) {
    // 注意：这里需要传入实际的Table对象，在实际使用时需要从TableManager获取
    return make_unique<ScanOperator>(table_name, columns, nullptr);
}

std::unique_ptr<Operator> OperatorFactory::create_filter_operator(std::unique_ptr<Expression> predicate) {
    return make_unique<FilterOperator>(std::move(predicate));
}

std::unique_ptr<Operator> OperatorFactory::create_projection_operator(const std::vector<std::string>& columns) {
    return make_unique<ProjectionOperator>(columns);
}

std::unique_ptr<Operator> OperatorFactory::create_final_result_operator() {
    return make_unique<FinalResultOperator>();
}

} // namespace minidb

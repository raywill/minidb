#include "exec/operators/scan_operator.h"
#include "exec/operators/filter_operator.h"
#include "exec/operators/projection_operator.h"
#include "exec/operators/final_result_operator.h"
#include "storage/table.h"
#include "sql/ast/ast_node.h"
#include "mem/arena.h"
#include "log/logger.h"
#include <iostream>
#include <cassert>

using namespace minidb;

// 创建测试表和数据
std::shared_ptr<Table> create_test_table() {
    system("rm -rf ./test_operator_data");
    system("mkdir -p ./test_operator_data/test_table");
    
    TableSchema schema("test_table");
    schema.add_column("id", DataType::INT);
    schema.add_column("name", DataType::STRING);
    schema.add_column("age", DataType::INT);
    schema.add_column("score", DataType::DECIMAL);
    
    auto table = std::shared_ptr<Table>(new Table("test_table", schema, "./test_operator_data/test_table"));
    Status status = table->initialize();
    assert(status.ok());
    
    // 插入测试数据
    std::vector<Row> rows;
    
    Row row1(4);
    row1.values[0] = "1"; row1.values[1] = "Alice"; row1.values[2] = "20"; row1.values[3] = "95.5";
    rows.push_back(row1);
    
    Row row2(4);
    row2.values[0] = "2"; row2.values[1] = "Bob"; row2.values[2] = "21"; row2.values[3] = "87.2";
    rows.push_back(row2);
    
    Row row3(4);
    row3.values[0] = "3"; row3.values[1] = "Charlie"; row3.values[2] = "19"; row3.values[3] = "92.0";
    rows.push_back(row3);
    
    Row row4(4);
    row4.values[0] = "4"; row4.values[1] = "David"; row4.values[2] = "22"; row4.values[3] = "88.5";
    rows.push_back(row4);
    
    status = table->insert_rows(rows);
    assert(status.ok());
    
    return table;
}

void test_scan_operator() {
    std::cout << "Testing ScanOperator..." << std::endl;
    
    auto table = create_test_table();
    
    // 创建扫描算子
    std::vector<std::string> columns = {"id", "name", "age"};
    ScanOperator scan_op("test_table", columns, table);
    
    ScopedArena arena;
    ExecutionContext context(arena.get(), 1);
    
    // 初始化算子
    Status status = scan_op.initialize(&context);
    assert(status.ok());
    assert(scan_op.get_state() == OperatorState::READY);
    
    // 验证输出列信息
    auto output_columns = scan_op.get_output_columns();
    auto output_types = scan_op.get_output_types();
    assert(output_columns.size() == 3);
    assert(output_types.size() == 3);
    assert(output_columns[0] == "id");
    assert(output_columns[1] == "name");
    assert(output_columns[2] == "age");
    assert(output_types[0] == DataType::INT);
    assert(output_types[1] == DataType::STRING);
    assert(output_types[2] == DataType::INT);
    
    // 获取数据
    DataChunk chunk;
    status = scan_op.get_next(&context, chunk);
    assert(status.ok());
    assert(chunk.row_count == 4);
    assert(chunk.columns.size() == 3);
    
    // 验证数据内容
    assert(chunk.columns[0].get_int(0) == 1);
    assert(chunk.columns[0].get_int(1) == 2);
    assert(chunk.columns[1].get_string(0) == "Alice");
    assert(chunk.columns[1].get_string(1) == "Bob");
    assert(chunk.columns[2].get_int(0) == 20);
    assert(chunk.columns[2].get_int(1) == 21);
    
    // 第二次调用应该返回空
    DataChunk chunk2;
    status = scan_op.get_next(&context, chunk2);
    assert(status.ok());
    assert(chunk2.empty());
    assert(scan_op.get_state() == OperatorState::FINISHED);
    
    // 测试重置
    status = scan_op.reset();
    assert(status.ok());
    assert(scan_op.get_state() == OperatorState::READY);
    
    std::cout << "ScanOperator test passed!" << std::endl;
}

void test_scan_operator_all_columns() {
    std::cout << "Testing ScanOperator all columns..." << std::endl;
    
    auto table = create_test_table();
    
    // 扫描所有列
    std::vector<std::string> all_columns = {"id", "name", "age", "score"};
    ScanOperator scan_op("test_table", all_columns, table);
    
    ScopedArena arena;
    ExecutionContext context(arena.get(), 2);
    
    Status status = scan_op.initialize(&context);
    assert(status.ok());
    
    DataChunk chunk;
    status = scan_op.get_next(&context, chunk);
    assert(status.ok());
    assert(chunk.columns.size() == 4);
    assert(chunk.row_count == 4);
    
    // 验证所有列的数据
    assert(chunk.columns[3].get_decimal(0) == 95.5);
    assert(chunk.columns[3].get_decimal(1) == 87.2);
    
    std::cout << "ScanOperator all columns test passed!" << std::endl;
}

void test_projection_operator() {
    std::cout << "Testing ProjectionOperator..." << std::endl;
    
    auto table = create_test_table();
    
    // 创建扫描算子
    std::vector<std::string> scan_columns = {"id", "name", "age", "score"};
    auto scan_op = std::unique_ptr<Operator>(new ScanOperator("test_table", scan_columns, table));
    
    // 创建投影算子
    std::vector<std::string> proj_columns = {"name", "score"};
    ProjectionOperator proj_op(proj_columns);
    proj_op.set_child(std::move(scan_op));
    
    ScopedArena arena;
    ExecutionContext context(arena.get(), 3);
    
    // 初始化
    Status status = proj_op.initialize(&context);
    assert(status.ok());
    
    // 验证输出列信息
    auto output_columns = proj_op.get_output_columns();
    auto output_types = proj_op.get_output_types();
    assert(output_columns.size() == 2);
    assert(output_columns[0] == "name");
    assert(output_columns[1] == "score");
    assert(output_types[0] == DataType::STRING);
    assert(output_types[1] == DataType::DECIMAL);
    
    // 获取投影后的数据
    DataChunk chunk;
    status = proj_op.get_next(&context, chunk);
    assert(status.ok());
    assert(chunk.columns.size() == 2);
    assert(chunk.row_count == 4);
    
    // 验证投影结果
    assert(chunk.columns[0].name == "name");
    assert(chunk.columns[1].name == "score");
    assert(chunk.columns[0].get_string(0) == "Alice");
    assert(chunk.columns[1].get_decimal(0) == 95.5);
    
    std::cout << "ProjectionOperator test passed!" << std::endl;
}

void test_projection_operator_star() {
    std::cout << "Testing ProjectionOperator with * selection..." << std::endl;
    
    auto table = create_test_table();
    
    // 创建扫描算子
    std::vector<std::string> scan_columns = {"id", "name", "age", "score"};
    auto scan_op = std::unique_ptr<Operator>(new ScanOperator("test_table", scan_columns, table));
    
    // 创建投影算子（使用*）
    std::vector<std::string> proj_columns = {"*"};
    ProjectionOperator proj_op(proj_columns);
    proj_op.set_child(std::move(scan_op));
    
    ScopedArena arena;
    ExecutionContext context(arena.get(), 4);
    
    // 初始化
    Status status = proj_op.initialize(&context);
    assert(status.ok());
    
    // 验证输出列信息（应该展开为所有列）
    auto output_columns = proj_op.get_output_columns();
    assert(output_columns.size() == 4);
    assert(output_columns[0] == "id");
    assert(output_columns[1] == "name");
    assert(output_columns[2] == "age");
    assert(output_columns[3] == "score");
    
    // 获取数据
    DataChunk chunk;
    status = proj_op.get_next(&context, chunk);
    assert(status.ok());
    assert(chunk.columns.size() == 4);
    
    std::cout << "ProjectionOperator * selection test passed!" << std::endl;
}

void test_final_result_operator() {
    std::cout << "Testing FinalResultOperator..." << std::endl;
    
    auto table = create_test_table();
    
    // 创建算子链：Scan -> Projection -> FinalResult
    auto scan_op = std::unique_ptr<Operator>(new ScanOperator("test_table", {"id", "name"}, table));
    
    auto proj_op = std::unique_ptr<Operator>(new ProjectionOperator({"name", "id"}));
    static_cast<PushOperator*>(proj_op.get())->set_child(std::move(scan_op));
    
    FinalResultOperator final_op;
    final_op.set_child(std::move(proj_op));
    
    ScopedArena arena;
    ExecutionContext context(arena.get(), 5);
    
    // 初始化
    Status status = final_op.initialize(&context);
    assert(status.ok());
    
    // 执行
    DataChunk chunk;
    status = final_op.get_next(&context, chunk);
    assert(status.ok());
    
    // 获取格式化结果
    std::string result_text = final_op.get_result_text();
    assert(!result_text.empty());
    
    // 验证结果格式
    assert(result_text.find("name") != std::string::npos);
    assert(result_text.find("id") != std::string::npos);
    assert(result_text.find("Alice") != std::string::npos);
    assert(result_text.find("Bob") != std::string::npos);
    assert(result_text.find("|") != std::string::npos); // 分隔符
    
    std::cout << "FinalResultOperator test passed!" << std::endl;
}

void test_operator_state_management() {
    std::cout << "Testing Operator state management..." << std::endl;
    
    auto table = create_test_table();
    
    ScanOperator scan_op("test_table", {"id"}, table);
    
    // 初始状态
    assert(scan_op.get_state() == OperatorState::READY);
    
    ScopedArena arena;
    ExecutionContext context(arena.get(), 6);
    
    // 初始化后状态
    Status status = scan_op.initialize(&context);
    assert(status.ok());
    assert(scan_op.get_state() == OperatorState::READY);
    
    // 执行中状态
    DataChunk chunk;
    status = scan_op.get_next(&context, chunk);
    assert(status.ok());
    // 注意：状态可能在执行完成后变为FINISHED
    
    // 重置状态
    status = scan_op.reset();
    assert(status.ok());
    assert(scan_op.get_state() == OperatorState::READY);
    
    std::cout << "Operator state management test passed!" << std::endl;
}

void test_operator_error_handling() {
    std::cout << "Testing Operator error handling..." << std::endl;
    
    // 测试无效表的扫描算子
    ScanOperator invalid_scan("nonexistent", {"id"}, nullptr);
    
    ScopedArena arena;
    ExecutionContext context(arena.get(), 7);
    
    Status status = invalid_scan.initialize(&context);
    assert(!status.ok());
    
    // 测试无子算子的投影算子
    ProjectionOperator proj_without_child({"id"});
    status = proj_without_child.initialize(&context);
    assert(!status.ok());
    assert(status.is_invalid_argument());
    
    // 测试无子算子的最终结果算子
    FinalResultOperator final_without_child;
    status = final_without_child.initialize(&context);
    assert(!status.ok());
    assert(status.is_invalid_argument());
    
    std::cout << "Operator error handling test passed!" << std::endl;
}

void test_data_chunk_operations() {
    std::cout << "Testing DataChunk operations..." << std::endl;
    
    DataChunk chunk;
    
    // 测试空chunk
    assert(chunk.empty());
    assert(chunk.row_count == 0);
    assert(chunk.columns.empty());
    
    // 添加列
    ColumnVector col1("id", DataType::INT);
    col1.append_int(1);
    col1.append_int(2);
    col1.append_int(3);
    
    ColumnVector col2("name", DataType::STRING);
    col2.append_string("Alice");
    col2.append_string("Bob");
    col2.append_string("Charlie");
    
    chunk.add_column(col1);
    chunk.add_column(col2);
    
    assert(!chunk.empty());
    assert(chunk.row_count == 3);
    assert(chunk.columns.size() == 2);
    
    // 测试清空
    chunk.clear();
    assert(chunk.empty());
    assert(chunk.row_count == 0);
    assert(chunk.columns.empty());
    
    std::cout << "DataChunk operations test passed!" << std::endl;
}

void test_operator_pipeline() {
    std::cout << "Testing Operator pipeline..." << std::endl;
    
    auto table = create_test_table();
    
    // 构建算子流水线：Scan -> Projection -> FinalResult
    auto scan_op = std::unique_ptr<Operator>(new ScanOperator("test_table", {"id", "name", "age", "score"}, table));
    
    auto proj_op = std::unique_ptr<Operator>(new ProjectionOperator({"name", "age"}));
    static_cast<PushOperator*>(proj_op.get())->set_child(std::move(scan_op));
    
    auto final_op = std::unique_ptr<Operator>(new FinalResultOperator());
    static_cast<PushOperator*>(final_op.get())->set_child(std::move(proj_op));
    
    ScopedArena arena;
    ExecutionContext context(arena.get(), 8);
    
    // 初始化整个流水线
    Status status = final_op->initialize(&context);
    assert(status.ok());
    
    // 执行流水线
    DataChunk result_chunk;
    status = final_op->get_next(&context, result_chunk);
    assert(status.ok());
    
    // 获取最终结果
    FinalResultOperator* final_result = static_cast<FinalResultOperator*>(final_op.get());
    std::string result_text = final_result->get_result_text();
    
    assert(!result_text.empty());
    assert(result_text.find("name") != std::string::npos);
    assert(result_text.find("age") != std::string::npos);
    assert(result_text.find("Alice") != std::string::npos);
    assert(result_text.find("20") != std::string::npos);
    
    std::cout << "Operator pipeline test passed!" << std::endl;
}

void test_operator_batching() {
    std::cout << "Testing Operator batching..." << std::endl;
    
    // 创建大量数据的表
    system("rm -rf ./test_batch_data");
    system("mkdir -p ./test_batch_data/batch_table");
    
    TableSchema schema("batch_table");
    schema.add_column("id", DataType::INT);
    schema.add_column("value", DataType::STRING);
    
    auto table = std::shared_ptr<Table>(new Table("batch_table", schema, "./test_batch_data/batch_table"));
    Status status = table->initialize();
    assert(status.ok());
    
    // 插入大量数据（超过默认批大小）
    const int num_rows = 2500; // 超过默认的1024批大小
    std::vector<Row> rows;
    
    for (int i = 0; i < num_rows; ++i) {
        Row row(2);
        row.values[0] = std::to_string(i);
        row.values[1] = "value_" + std::to_string(i);
        rows.push_back(row);
        
        // 批量插入
        if (rows.size() == 500 || i == num_rows - 1) {
            status = table->insert_rows(rows);
            assert(status.ok());
            rows.clear();
        }
    }
    
    // 创建扫描算子
    ScanOperator scan_op("batch_table", {"id", "value"}, table);
    
    ScopedArena arena;
    ExecutionContext context(arena.get(), 9);
    
    status = scan_op.initialize(&context);
    assert(status.ok());
    
    // 应该需要多次调用get_next来获取所有数据
    int total_rows = 0;
    int batch_count = 0;
    
    while (true) {
        DataChunk chunk;
        status = scan_op.get_next(&context, chunk);
        assert(status.ok());
        
        if (chunk.empty()) {
            break;
        }
        
        total_rows += chunk.row_count;
        batch_count++;
        
        // 验证批大小不超过默认值
        assert(chunk.row_count <= DEFAULT_BATCH_SIZE);
    }
    
    assert(total_rows == num_rows);
    assert(batch_count >= 3); // 应该有多个批次
    
    // 清理
    system("rm -rf ./test_batch_data");
    
    std::cout << "Operator batching test passed!" << std::endl;
}

void test_expression_evaluator() {
    std::cout << "Testing ExpressionEvaluator..." << std::endl;
    
    // 创建测试数据块
    DataChunk chunk;
    chunk.row_count = 3;
    
    ColumnVector id_col("id", DataType::INT);
    id_col.append_int(1);
    id_col.append_int(2);
    id_col.append_int(3);
    
    ColumnVector age_col("age", DataType::INT);
    age_col.append_int(20);
    age_col.append_int(18);
    age_col.append_int(25);
    
    chunk.add_column(id_col);
    chunk.add_column(age_col);
    
    // 创建比较表达式：age > 19
    auto left_expr = std::unique_ptr<Expression>(new ColumnRefExpression("age"));
    auto right_expr = std::unique_ptr<Expression>(new LiteralExpression(DataType::INT, "19"));
    auto comparison_expr = std::unique_ptr<Expression>(
        new BinaryExpression(BinaryOperatorType::GREATER_THAN, std::move(left_expr), std::move(right_expr)));
    
    // 创建表达式求值器
    ExpressionEvaluator evaluator(comparison_expr.get());
    
    // 求值
    std::vector<bool> results;
    Status status = evaluator.evaluate(chunk, results);
    assert(status.ok());
    assert(results.size() == 3);
    
    // 验证结果：age > 19 -> [true, false, true]
    assert(results[0] == true);  // 20 > 19
    assert(results[1] == false); // 18 > 19
    assert(results[2] == true);  // 25 > 19
    
    std::cout << "ExpressionEvaluator test passed!" << std::endl;
}

void test_operator_memory_management() {
    std::cout << "Testing Operator memory management..." << std::endl;
    
    auto table = create_test_table();
    
    // 使用自定义Arena
    PoolAllocator custom_allocator(64 * 1024); // 64KB
    Arena custom_arena(&custom_allocator);
    ExecutionContext context(&custom_arena, 10);
    
    // 创建算子
    ScanOperator scan_op("test_table", {"id", "name"}, table);
    
    size_t initial_bytes = custom_arena.allocated_bytes();
    
    // 初始化和执行
    Status status = scan_op.initialize(&context);
    assert(status.ok());
    
    DataChunk chunk;
    status = scan_op.get_next(&context, chunk);
    assert(status.ok());
    
    // 验证内存使用
    size_t used_bytes = custom_arena.allocated_bytes();
    assert(used_bytes >= initial_bytes);
    
    // 重置Arena
    custom_arena.reset();
    assert(custom_arena.allocated_bytes() == 0);
    
    std::cout << "Operator memory management test passed!" << std::endl;
}

int main() {
    try {
        // 设置日志级别
        Logger::instance()->set_level(LogLevel::ERROR);
        
        test_scan_operator();
        test_scan_operator_all_columns();
        test_projection_operator();
        test_projection_operator_star();
        test_final_result_operator();
        test_operator_state_management();
        test_operator_error_handling();
        test_data_chunk_operations();
        test_operator_pipeline();
        test_operator_batching();
        test_expression_evaluator();
        test_operator_memory_management();
        
        // 清理测试数据
        system("rm -rf ./test_operator_data");
        
        std::cout << "\n🎉 All operator tests passed!" << std::endl;
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "❌ Operator test failed: " << e.what() << std::endl;
        system("rm -rf ./test_operator_data");
        return 1;
    }
}

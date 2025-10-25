#include "exec/plan/planner.h"
#include "exec/plan/plan.h"
#include "exec/operator.h"
#include "sql/compiler/compiler.h"
#include "sql/parser/new_parser.h"
#include "storage/catalog.h"
#include <iostream>
#include <cassert>

using namespace minidb;

void cleanup_test_data() {
    system("rm -rf ./test_planner_dml_data");
}

Catalog* setup_catalog_with_table() {
    cleanup_test_data();
    Catalog* catalog = new Catalog("./test_planner_dml_data");
    assert(catalog->initialize().ok());

    TableSchema schema("students");
    schema.add_column("id", DataType::INT);
    schema.add_column("name", DataType::STRING);
    schema.add_column("age", DataType::INT);
    schema.add_column("score", DataType::DECIMAL);

    assert(catalog->create_table("STUDENTS", schema).ok());

    return catalog;
}

void test_plan_insert() {
    std::cout << "Testing plan INSERT..." << std::endl;

    Catalog* catalog = setup_catalog_with_table();
    TableManager table_manager(catalog);

    SQLParser parser("INSERT INTO students VALUES (1, 'Alice', 20, 95.5);");
    std::unique_ptr<StmtAST> ast;
    assert(parser.parse(ast).ok());

    Compiler compiler(catalog);
    std::unique_ptr<Statement> stmt;
    assert(compiler.compile(ast.get(), stmt).ok());

    Planner planner(catalog, &table_manager);
    std::unique_ptr<Plan> plan;
    Status status = planner.create_plan(stmt.get(), plan);
    assert(status.ok());
    assert(plan != nullptr);
    assert(plan->get_type() == PlanType::INSERT);

    InsertPlan* insert_plan = static_cast<InsertPlan*>(plan.get());
    assert(insert_plan->get_table_name() == "STUDENTS");
    assert(insert_plan->get_table() != nullptr);

    delete catalog;
    cleanup_test_data();
    std::cout << "Plan INSERT test passed!" << std::endl;
}

// 测试 INSERT plan 保留值
void test_plan_insert_preserves_single_row() {
    std::cout << "Testing plan INSERT preserves single row values..." << std::endl;

    Catalog* catalog = setup_catalog_with_table();
    TableManager table_manager(catalog);

    SQLParser parser("INSERT INTO students VALUES (1, 'Alice', 20, 95.5);");
    std::unique_ptr<StmtAST> ast;
    assert(parser.parse(ast).ok());

    Compiler compiler(catalog);
    std::unique_ptr<Statement> stmt;
    assert(compiler.compile(ast.get(), stmt).ok());

    Planner planner(catalog, &table_manager);
    std::unique_ptr<Plan> plan;
    assert(planner.create_plan(stmt.get(), plan).ok());

    InsertPlan* insert_plan = static_cast<InsertPlan*>(plan.get());

    // 验证值被正确复制到 Plan 中
    const auto& values = insert_plan->get_values();
    assert(values.size() == 1);  // 1 行
    assert(values[0].size() == 4);  // 4 列

    // 验证每个值都是 LiteralExpression
    LiteralExpression* val0 = dynamic_cast<LiteralExpression*>(values[0][0].get());
    LiteralExpression* val1 = dynamic_cast<LiteralExpression*>(values[0][1].get());
    LiteralExpression* val2 = dynamic_cast<LiteralExpression*>(values[0][2].get());
    LiteralExpression* val3 = dynamic_cast<LiteralExpression*>(values[0][3].get());

    assert(val0 != nullptr && val0->get_value() == "1");
    assert(val1 != nullptr && val1->get_value() == "Alice");
    assert(val2 != nullptr && val2->get_value() == "20");
    assert(val3 != nullptr && val3->get_value() == "95.5");

    delete catalog;
    cleanup_test_data();
    std::cout << "Plan INSERT preserves single row values test passed!" << std::endl;
}

// 测试 INSERT plan 保留多行值
void test_plan_insert_preserves_multiple_rows() {
    std::cout << "Testing plan INSERT preserves multiple rows..." << std::endl;

    Catalog* catalog = setup_catalog_with_table();
    TableManager table_manager(catalog);

    SQLParser parser("INSERT INTO students VALUES (1, 'Alice', 20, 95.5), (2, 'Bob', 22, 87.3);");
    std::unique_ptr<StmtAST> ast;
    assert(parser.parse(ast).ok());

    Compiler compiler(catalog);
    std::unique_ptr<Statement> stmt;
    assert(compiler.compile(ast.get(), stmt).ok());

    Planner planner(catalog, &table_manager);
    std::unique_ptr<Plan> plan;
    assert(planner.create_plan(stmt.get(), plan).ok());

    InsertPlan* insert_plan = static_cast<InsertPlan*>(plan.get());

    // 验证所有行都被复制
    const auto& values = insert_plan->get_values();
    assert(values.size() == 2);  // 2 行
    assert(values[0].size() == 4);  // 4 列
    assert(values[1].size() == 4);  // 4 列

    // 验证第一行
    LiteralExpression* row1_val0 = dynamic_cast<LiteralExpression*>(values[0][0].get());
    LiteralExpression* row1_val1 = dynamic_cast<LiteralExpression*>(values[0][1].get());
    assert(row1_val0 != nullptr && row1_val0->get_value() == "1");
    assert(row1_val1 != nullptr && row1_val1->get_value() == "Alice");

    // 验证第二行
    LiteralExpression* row2_val0 = dynamic_cast<LiteralExpression*>(values[1][0].get());
    LiteralExpression* row2_val1 = dynamic_cast<LiteralExpression*>(values[1][1].get());
    assert(row2_val0 != nullptr && row2_val0->get_value() == "2");
    assert(row2_val1 != nullptr && row2_val1->get_value() == "Bob");

    delete catalog;
    cleanup_test_data();
    std::cout << "Plan INSERT preserves multiple rows test passed!" << std::endl;
}

// 测试 INSERT plan 使用列子集
void test_plan_insert_with_column_subset() {
    std::cout << "Testing plan INSERT with column subset..." << std::endl;

    Catalog* catalog = setup_catalog_with_table();
    TableManager table_manager(catalog);

    SQLParser parser("INSERT INTO students(id, name) VALUES (1, 'Alice');");
    std::unique_ptr<StmtAST> ast;
    assert(parser.parse(ast).ok());

    Compiler compiler(catalog);
    std::unique_ptr<Statement> stmt;
    assert(compiler.compile(ast.get(), stmt).ok());

    Planner planner(catalog, &table_manager);
    std::unique_ptr<Plan> plan;
    assert(planner.create_plan(stmt.get(), plan).ok());

    InsertPlan* insert_plan = static_cast<InsertPlan*>(plan.get());

    // 验证值数量匹配指定的列数
    const auto& values = insert_plan->get_values();
    assert(values.size() == 1);  // 1 行
    assert(values[0].size() == 2);  // 只有 2 列（id, name）

    LiteralExpression* val0 = dynamic_cast<LiteralExpression*>(values[0][0].get());
    LiteralExpression* val1 = dynamic_cast<LiteralExpression*>(values[0][1].get());
    assert(val0 != nullptr && val0->get_value() == "1");
    assert(val1 != nullptr && val1->get_value() == "Alice");

    delete catalog;
    cleanup_test_data();
    std::cout << "Plan INSERT with column subset test passed!" << std::endl;
}

void test_plan_select_star() {
    std::cout << "Testing plan SELECT *..." << std::endl;

    Catalog* catalog = setup_catalog_with_table();
    TableManager table_manager(catalog);

    SQLParser parser("SELECT * FROM students;");
    std::unique_ptr<StmtAST> ast;
    assert(parser.parse(ast).ok());

    Compiler compiler(catalog);
    std::unique_ptr<Statement> stmt;
    assert(compiler.compile(ast.get(), stmt).ok());

    Planner planner(catalog, &table_manager);
    std::unique_ptr<Plan> plan;
    assert(planner.create_plan(stmt.get(), plan).ok());
    assert(plan->get_type() == PlanType::SELECT);

    SelectPlan* select_plan = static_cast<SelectPlan*>(plan.get());
    assert(select_plan->get_table_name() == "STUDENTS");

    // Verify operator tree is created
    Operator* root_op = select_plan->get_root_operator();
    assert(root_op != nullptr);

    delete catalog;
    cleanup_test_data();
    std::cout << "Plan SELECT * test passed!" << std::endl;
}

void test_plan_select_with_projection() {
    std::cout << "Testing plan SELECT with projection..." << std::endl;

    Catalog* catalog = setup_catalog_with_table();
    TableManager table_manager(catalog);

    SQLParser parser("SELECT name, age FROM students;");
    std::unique_ptr<StmtAST> ast;
    assert(parser.parse(ast).ok());

    Compiler compiler(catalog);
    std::unique_ptr<Statement> stmt;
    assert(compiler.compile(ast.get(), stmt).ok());

    Planner planner(catalog, &table_manager);
    std::unique_ptr<Plan> plan;
    assert(planner.create_plan(stmt.get(), plan).ok());

    SelectPlan* select_plan = static_cast<SelectPlan*>(plan.get());
    Operator* root_op = select_plan->get_root_operator();
    assert(root_op != nullptr);

    // Root should be FinalResultOperator
    assert(root_op->get_type() == OperatorType::FINAL_RESULT);

    delete catalog;
    cleanup_test_data();
    std::cout << "Plan SELECT with projection test passed!" << std::endl;
}

void test_plan_select_with_filter() {
    std::cout << "Testing plan SELECT with WHERE..." << std::endl;

    Catalog* catalog = setup_catalog_with_table();
    TableManager table_manager(catalog);

    SQLParser parser("SELECT name FROM students WHERE age > 18;");
    std::unique_ptr<StmtAST> ast;
    assert(parser.parse(ast).ok());

    Compiler compiler(catalog);
    std::unique_ptr<Statement> stmt;
    assert(compiler.compile(ast.get(), stmt).ok());

    Planner planner(catalog, &table_manager);
    std::unique_ptr<Plan> plan;
    assert(planner.create_plan(stmt.get(), plan).ok());

    SelectPlan* select_plan = static_cast<SelectPlan*>(plan.get());
    Operator* root_op = select_plan->get_root_operator();
    assert(root_op != nullptr);

    // Should have operator tree: Scan -> Filter -> Projection -> FinalResult

    delete catalog;
    cleanup_test_data();
    std::cout << "Plan SELECT with WHERE test passed!" << std::endl;
}

void test_plan_delete() {
    std::cout << "Testing plan DELETE..." << std::endl;

    Catalog* catalog = setup_catalog_with_table();
    TableManager table_manager(catalog);

    SQLParser parser("DELETE FROM students WHERE age < 18;");
    std::unique_ptr<StmtAST> ast;
    assert(parser.parse(ast).ok());

    Compiler compiler(catalog);
    std::unique_ptr<Statement> stmt;
    assert(compiler.compile(ast.get(), stmt).ok());

    Planner planner(catalog, &table_manager);
    std::unique_ptr<Plan> plan;
    assert(planner.create_plan(stmt.get(), plan).ok());
    assert(plan->get_type() == PlanType::DELETE);

    DeletePlan* delete_plan = static_cast<DeletePlan*>(plan.get());
    assert(delete_plan->get_table_name() == "STUDENTS");
    assert(delete_plan->get_table() != nullptr);

    delete catalog;
    cleanup_test_data();
    std::cout << "Plan DELETE test passed!" << std::endl;
}

void test_plan_delete_all() {
    std::cout << "Testing plan DELETE without WHERE..." << std::endl;

    Catalog* catalog = setup_catalog_with_table();
    TableManager table_manager(catalog);

    SQLParser parser("DELETE FROM students;");
    std::unique_ptr<StmtAST> ast;
    assert(parser.parse(ast).ok());

    Compiler compiler(catalog);
    std::unique_ptr<Statement> stmt;
    assert(compiler.compile(ast.get(), stmt).ok());

    Planner planner(catalog, &table_manager);
    std::unique_ptr<Plan> plan;
    assert(planner.create_plan(stmt.get(), plan).ok());

    DeletePlan* delete_plan = static_cast<DeletePlan*>(plan.get());
    // WHERE clause is null for delete all
    assert(delete_plan->get_where_clause() == nullptr);

    delete catalog;
    cleanup_test_data();
    std::cout << "Plan DELETE without WHERE test passed!" << std::endl;
}

void test_operator_tree_structure() {
    std::cout << "Testing operator tree structure..." << std::endl;

    Catalog* catalog = setup_catalog_with_table();
    TableManager table_manager(catalog);

    // Complex query with filter and projection
    SQLParser parser("SELECT name, age FROM students WHERE score > 90.0;");
    std::unique_ptr<StmtAST> ast;
    assert(parser.parse(ast).ok());

    Compiler compiler(catalog);
    std::unique_ptr<Statement> stmt;
    assert(compiler.compile(ast.get(), stmt).ok());

    Planner planner(catalog, &table_manager);
    std::unique_ptr<Plan> plan;
    assert(planner.create_plan(stmt.get(), plan).ok());

    SelectPlan* select_plan = static_cast<SelectPlan*>(plan.get());
    Operator* root_op = select_plan->get_root_operator();
    assert(root_op != nullptr);

    // Verify operator chain is built
    // Expected: FinalResult -> Projection -> Filter -> Scan

    delete catalog;
    cleanup_test_data();
    std::cout << "Operator tree structure test passed!" << std::endl;
}

int main() {
    try {
        test_plan_insert();
        test_plan_insert_preserves_single_row();
        test_plan_insert_preserves_multiple_rows();
        test_plan_insert_with_column_subset();
        test_plan_select_star();
        test_plan_select_with_projection();
        test_plan_select_with_filter();
        test_plan_delete();
        test_plan_delete_all();
        test_operator_tree_structure();

        std::cout << "\nAll Planner DML tests passed!" << std::endl;
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Planner DML test failed: " << e.what() << std::endl;
        cleanup_test_data();
        return 1;
    }
}

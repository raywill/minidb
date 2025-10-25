#include "exec/plan/planner.h"
#include "exec/plan/plan.h"
#include "sql/compiler/compiler.h"
#include "sql/parser/new_parser.h"
#include "storage/catalog.h"
#include <iostream>
#include <cassert>

using namespace minidb;

void cleanup_test_data() {
    system("rm -rf ./test_planner_ddl_data");
}

void test_plan_create_table() {
    std::cout << "Testing plan CREATE TABLE..." << std::endl;

    cleanup_test_data();
    Catalog catalog("./test_planner_ddl_data");
    assert(catalog.initialize().ok());
    TableManager table_manager(&catalog);

    // Parse and compile
    SQLParser parser("CREATE TABLE users(id INT, name STRING);");
    std::unique_ptr<StmtAST> ast;
    assert(parser.parse(ast).ok());

    Compiler compiler(&catalog);
    std::unique_ptr<Statement> stmt;
    assert(compiler.compile(ast.get(), stmt).ok());

    // Create plan
    Planner planner(&catalog, &table_manager);
    std::unique_ptr<Plan> plan;
    Status status = planner.create_plan(stmt.get(), plan);
    assert(status.ok());
    assert(plan != nullptr);
    assert(plan->get_type() == PlanType::CREATE_TABLE);

    // Verify plan details
    CreateTablePlan* create_plan = static_cast<CreateTablePlan*>(plan.get());
    assert(create_plan->get_table_name() == "USERS");
    assert(create_plan->get_columns().size() == 2);
    assert(create_plan->get_columns()[0].name == "ID");
    assert(create_plan->get_columns()[1].name == "NAME");

    cleanup_test_data();
    std::cout << "Plan CREATE TABLE test passed!" << std::endl;
}

void test_plan_create_table_if_not_exists() {
    std::cout << "Testing plan CREATE TABLE IF NOT EXISTS..." << std::endl;

    cleanup_test_data();
    Catalog catalog("./test_planner_ddl_data");
    assert(catalog.initialize().ok());
    TableManager table_manager(&catalog);

    SQLParser parser("CREATE TABLE IF NOT EXISTS products(id INT);");
    std::unique_ptr<StmtAST> ast;
    assert(parser.parse(ast).ok());

    Compiler compiler(&catalog);
    std::unique_ptr<Statement> stmt;
    assert(compiler.compile(ast.get(), stmt).ok());

    Planner planner(&catalog, &table_manager);
    std::unique_ptr<Plan> plan;
    assert(planner.create_plan(stmt.get(), plan).ok());

    CreateTablePlan* create_plan = static_cast<CreateTablePlan*>(plan.get());
    assert(create_plan->get_if_not_exists() == true);

    cleanup_test_data();
    std::cout << "Plan CREATE TABLE IF NOT EXISTS test passed!" << std::endl;
}

void test_plan_drop_table() {
    std::cout << "Testing plan DROP TABLE..." << std::endl;

    cleanup_test_data();
    Catalog catalog("./test_planner_ddl_data");
    assert(catalog.initialize().ok());
    TableManager table_manager(&catalog);

    SQLParser parser("DROP TABLE users;");
    std::unique_ptr<StmtAST> ast;
    assert(parser.parse(ast).ok());

    Compiler compiler(&catalog);
    std::unique_ptr<Statement> stmt;
    assert(compiler.compile(ast.get(), stmt).ok());

    Planner planner(&catalog, &table_manager);
    std::unique_ptr<Plan> plan;
    assert(planner.create_plan(stmt.get(), plan).ok());
    assert(plan->get_type() == PlanType::DROP_TABLE);

    DropTablePlan* drop_plan = static_cast<DropTablePlan*>(plan.get());
    assert(drop_plan->get_table_name() == "USERS");
    assert(drop_plan->get_if_exists() == false);

    cleanup_test_data();
    std::cout << "Plan DROP TABLE test passed!" << std::endl;
}

void test_plan_drop_table_if_exists() {
    std::cout << "Testing plan DROP TABLE IF EXISTS..." << std::endl;

    cleanup_test_data();
    Catalog catalog("./test_planner_ddl_data");
    assert(catalog.initialize().ok());
    TableManager table_manager(&catalog);

    SQLParser parser("DROP TABLE IF EXISTS temp;");
    std::unique_ptr<StmtAST> ast;
    assert(parser.parse(ast).ok());

    Compiler compiler(&catalog);
    std::unique_ptr<Statement> stmt;
    assert(compiler.compile(ast.get(), stmt).ok());

    Planner planner(&catalog, &table_manager);
    std::unique_ptr<Plan> plan;
    assert(planner.create_plan(stmt.get(), plan).ok());

    DropTablePlan* drop_plan = static_cast<DropTablePlan*>(plan.get());
    assert(drop_plan->get_if_exists() == true);

    cleanup_test_data();
    std::cout << "Plan DROP TABLE IF EXISTS test passed!" << std::endl;
}

int main() {
    try {
        test_plan_create_table();
        test_plan_create_table_if_not_exists();
        test_plan_drop_table();
        test_plan_drop_table_if_exists();

        std::cout << "\nAll Planner DDL tests passed!" << std::endl;
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Planner DDL test failed: " << e.what() << std::endl;
        cleanup_test_data();
        return 1;
    }
}

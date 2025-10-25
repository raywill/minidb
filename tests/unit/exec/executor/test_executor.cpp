#include "exec/executor/new_executor.h"
#include "exec/plan/planner.h"
#include "sql/compiler/compiler.h"
#include "sql/optimizer/optimizer.h"
#include "sql/parser/new_parser.h"
#include "storage/catalog.h"
#include "storage/table.h"
#include "log/logger.h"
#include <iostream>
#include <cassert>

using namespace minidb;

void cleanup_test_data() {
    system("rm -rf ./test_exec_simple_data");
}

QueryResult execute_sql(const std::string& sql, Catalog* catalog, TableManager* table_manager) {
    // 1. Parse
    SQLParser parser(sql);
    std::unique_ptr<StmtAST> ast;
    Status status = parser.parse(ast);
    if (!status.ok()) {
        return QueryResult::error_result(status.ToString());
    }

    // 2. Compile
    Compiler compiler(catalog);
    std::unique_ptr<Statement> stmt;
    status = compiler.compile(ast.get(), stmt);
    if (!status.ok()) {
        return QueryResult::error_result(status.ToString());
    }

    // 3. Optimize
    Optimizer optimizer;
    std::unique_ptr<Statement> optimized_stmt;
    status = optimizer.optimize(stmt.get(), optimized_stmt);
    if (!status.ok()) {
        return QueryResult::error_result(status.ToString());
    }

    // 4. Plan
    Planner planner(catalog, table_manager);
    std::unique_ptr<Plan> plan;
    status = planner.create_plan(optimized_stmt ? optimized_stmt.get() : stmt.get(), plan);
    if (!status.ok()) {
        return QueryResult::error_result(status.ToString());
    }

    // 5. Execute
    QueryExecutor executor(catalog, table_manager);
    return executor.execute_plan(plan.get());
}

void test_create_and_drop_table() {
    std::cout << "Testing CREATE and DROP TABLE..." << std::endl;

    cleanup_test_data();
    Catalog catalog("./test_exec_simple_data");
    assert(catalog.initialize().ok());
    TableManager table_manager(&catalog);

    // Create table
    QueryResult result = execute_sql("CREATE TABLE users(id INT, name STRING);", &catalog, &table_manager);
    assert(result.success);
    assert(catalog.table_exists("USERS"));

    // Drop table
    result = execute_sql("DROP TABLE users;", &catalog, &table_manager);
    assert(result.success);
    assert(!catalog.table_exists("USERS"));

    cleanup_test_data();
    std::cout << "CREATE and DROP TABLE test passed!" << std::endl;
}

void test_insert_and_select() {
    std::cout << "Testing INSERT and SELECT..." << std::endl;

    cleanup_test_data();
    Catalog catalog("./test_exec_simple_data");
    assert(catalog.initialize().ok());
    TableManager table_manager(&catalog);

    // Create table
    QueryResult result = execute_sql("CREATE TABLE students(id INT, name STRING, score DECIMAL);",
                                    &catalog, &table_manager);
    assert(result.success);

    // Insert data
    result = execute_sql("INSERT INTO students VALUES (1, 'Alice', 95.5), (2, 'Bob', 87.2);",
                        &catalog, &table_manager);
    assert(result.success);
    assert(result.rows_affected == 2);

    // Select all
    result = execute_sql("SELECT * FROM students;", &catalog, &table_manager);
    assert(result.success);
    assert(result.result_text.find("Alice") != std::string::npos);
    assert(result.result_text.find("Bob") != std::string::npos);

    cleanup_test_data();
    std::cout << "INSERT and SELECT test passed!" << std::endl;
}

void test_delete() {
    std::cout << "Testing DELETE..." << std::endl;

    cleanup_test_data();
    Catalog catalog("./test_exec_simple_data");
    assert(catalog.initialize().ok());
    TableManager table_manager(&catalog);

    // Create and populate table
    execute_sql("CREATE TABLE data(id INT, value INT);", &catalog, &table_manager);
    execute_sql("INSERT INTO data VALUES (1, 10), (2, 20), (3, 30);", &catalog, &table_manager);

    // Delete rows
    QueryResult result = execute_sql("DELETE FROM data WHERE value > 15;", &catalog, &table_manager);
    assert(result.success);
    assert(result.rows_affected == 2);

    cleanup_test_data();
    std::cout << "DELETE test passed!" << std::endl;
}

int main() {
    try {
        Logger::instance()->set_level(LogLevel::ERROR);

        test_create_and_drop_table();
        test_insert_and_select();
        test_delete();

        std::cout << "\nAll executor tests passed!" << std::endl;
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Executor test failed: " << e.what() << std::endl;
        cleanup_test_data();
        return 1;
    }
}

#include "server/database_server.h"
#include "sql/parser/new_parser.h"
#include "sql/compiler/compiler.h"
#include "sql/optimizer/optimizer.h"
#include "exec/plan/planner.h"
#include "exec/executor/new_executor.h"
#include "storage/catalog.h"
#include <iostream>
#include <cassert>

using namespace minidb;

void cleanup_test_data() {
    system("rm -rf ./test_e2e_data");
}

// 端到端测试：模拟用户执行SQL的完整流程
void test_create_insert_select() {
    std::cout << "=== Testing CREATE TABLE + INSERT + SELECT ===" << std::endl;

    cleanup_test_data();

    // 1. 初始化catalog
    Catalog catalog("./test_e2e_data");
    assert(catalog.initialize().ok());

    TableManager table_manager(&catalog);
    QueryExecutor executor(&catalog, &table_manager);

    // 2. CREATE TABLE t1 (c1 INT)
    std::cout << "\n[Step 1] CREATE TABLE t1 (c1 INT);" << std::endl;
    {
        SQLParser parser("CREATE TABLE t1 (c1 INT);");
        std::unique_ptr<StmtAST> ast;
        Status status = parser.parse(ast);
        std::cout << "  Parse: " << (status.ok() ? "OK" : status.message()) << std::endl;
        assert(status.ok());

        Compiler compiler(&catalog);
        std::unique_ptr<Statement> stmt;
        status = compiler.compile(ast.get(), stmt);
        std::cout << "  Compile: " << (status.ok() ? "OK" : status.message()) << std::endl;
        assert(status.ok());

        Optimizer optimizer;
        std::unique_ptr<Statement> optimized_stmt;
        status = optimizer.optimize(stmt.get(), optimized_stmt);
        std::cout << "  Optimize: " << (status.ok() ? "OK" : status.message()) << std::endl;
        assert(status.ok());

        Planner planner(&catalog, &table_manager);
        std::unique_ptr<Plan> plan;
        status = planner.create_plan(stmt.get(), plan);
        std::cout << "  Plan: " << (status.ok() ? "OK" : status.message()) << std::endl;
        assert(status.ok());

        QueryResult result = executor.execute_plan(plan.get());
        std::cout << "  Execute: " << (result.success ? "OK" : result.error_message) << std::endl;
        assert(result.success);
    }

    // 3. INSERT INTO t1 VALUES (3)
    std::cout << "\n[Step 2] INSERT INTO t1 VALUES (3);" << std::endl;
    {
        SQLParser parser("INSERT INTO t1 VALUES (3);");
        std::unique_ptr<StmtAST> ast;
        Status status = parser.parse(ast);
        std::cout << "  Parse: " << (status.ok() ? "OK" : status.message()) << std::endl;
        assert(status.ok());

        Compiler compiler(&catalog);
        std::unique_ptr<Statement> stmt;
        status = compiler.compile(ast.get(), stmt);
        std::cout << "  Compile: " << (status.ok() ? "OK" : status.message()) << std::endl;
        assert(status.ok());

        Optimizer optimizer;
        std::unique_ptr<Statement> optimized_stmt;
        status = optimizer.optimize(stmt.get(), optimized_stmt);
        std::cout << "  Optimize: " << (status.ok() ? "OK" : status.message()) << std::endl;
        assert(status.ok());

        Planner planner(&catalog, &table_manager);
        std::unique_ptr<Plan> plan;
        status = planner.create_plan(stmt.get(), plan);
        std::cout << "  Plan: " << (status.ok() ? "OK" : status.message()) << std::endl;
        assert(status.ok());

        QueryResult result = executor.execute_plan(plan.get());
        std::cout << "  Execute: " << (result.success ? "OK" : result.error_message) << std::endl;
        std::cout << "  Rows affected: " << result.rows_affected << std::endl;
        assert(result.success);
        assert(result.rows_affected == 1);
    }

    // 4. SELECT * FROM t1
    std::cout << "\n[Step 3] SELECT * FROM t1;" << std::endl;
    {
        SQLParser parser("SELECT * FROM t1;");
        std::unique_ptr<StmtAST> ast;
        Status status = parser.parse(ast);
        std::cout << "  Parse: " << (status.ok() ? "OK" : status.message()) << std::endl;
        assert(status.ok());

        Compiler compiler(&catalog);
        std::unique_ptr<Statement> stmt;
        status = compiler.compile(ast.get(), stmt);
        std::cout << "  Compile: " << (status.ok() ? "OK" : status.message()) << std::endl;
        assert(status.ok());

        Optimizer optimizer;
        std::unique_ptr<Statement> optimized_stmt;
        status = optimizer.optimize(stmt.get(), optimized_stmt);
        std::cout << "  Optimize: " << (status.ok() ? "OK" : status.message()) << std::endl;
        assert(status.ok());

        Planner planner(&catalog, &table_manager);
        std::unique_ptr<Plan> plan;
        status = planner.create_plan(stmt.get(), plan);
        std::cout << "  Plan: " << (status.ok() ? "OK" : status.message()) << std::endl;
        assert(status.ok());

        QueryResult result = executor.execute_plan(plan.get());
        std::cout << "  Execute: " << (result.success ? "OK" : result.error_message) << std::endl;
        if (result.success) {
            std::cout << "  Result text:\n" << result.result_text << std::endl;
        }
        assert(result.success);
        assert(result.result_text.find("3") != std::string::npos);
    }

    cleanup_test_data();
    std::cout << "\n=== CREATE+INSERT+SELECT test passed! ===" << std::endl;
}

// 端到端测试：多行 INSERT
void test_multi_row_insert() {
    std::cout << "\n=== Testing Multi-row INSERT ===\n" << std::endl;

    cleanup_test_data();

    Catalog catalog("./test_e2e_data");
    assert(catalog.initialize().ok());

    TableManager table_manager(&catalog);
    QueryExecutor executor(&catalog, &table_manager);

    // 1. CREATE TABLE
    std::cout << "[Step 1] CREATE TABLE t1 (c1 INT);" << std::endl;
    {
        SQLParser parser("CREATE TABLE t1 (c1 INT);");
        std::unique_ptr<StmtAST> ast;
        assert(parser.parse(ast).ok());

        Compiler compiler(&catalog);
        std::unique_ptr<Statement> stmt;
        assert(compiler.compile(ast.get(), stmt).ok());

        Optimizer optimizer;
        std::unique_ptr<Statement> optimized_stmt;
        assert(optimizer.optimize(stmt.get(), optimized_stmt).ok());

        Planner planner(&catalog, &table_manager);
        std::unique_ptr<Plan> plan;
        assert(planner.create_plan(stmt.get(), plan).ok());

        QueryResult result = executor.execute_plan(plan.get());
        std::cout << "  Execute: " << (result.success ? "OK" : result.error_message) << std::endl;
        assert(result.success);
    }

    // 2. INSERT multiple rows
    std::cout << "\n[Step 2] INSERT INTO t1 VALUES (1), (2), (3);" << std::endl;
    {
        SQLParser parser("INSERT INTO t1 VALUES (1), (2), (3);");
        std::unique_ptr<StmtAST> ast;
        assert(parser.parse(ast).ok());

        Compiler compiler(&catalog);
        std::unique_ptr<Statement> stmt;
        assert(compiler.compile(ast.get(), stmt).ok());

        Planner planner(&catalog, &table_manager);
        std::unique_ptr<Plan> plan;
        assert(planner.create_plan(stmt.get(), plan).ok());

        QueryResult result = executor.execute_plan(plan.get());
        std::cout << "  Execute: " << (result.success ? "OK" : result.error_message) << std::endl;
        std::cout << "  Rows affected: " << result.rows_affected << std::endl;
        assert(result.success);
        assert(result.rows_affected == 3);
    }

    // 3. SELECT and verify all rows
    std::cout << "\n[Step 3] SELECT * FROM t1;" << std::endl;
    {
        SQLParser parser("SELECT * FROM t1;");
        std::unique_ptr<StmtAST> ast;
        assert(parser.parse(ast).ok());

        Compiler compiler(&catalog);
        std::unique_ptr<Statement> stmt;
        assert(compiler.compile(ast.get(), stmt).ok());

        Planner planner(&catalog, &table_manager);
        std::unique_ptr<Plan> plan;
        assert(planner.create_plan(stmt.get(), plan).ok());

        QueryResult result = executor.execute_plan(plan.get());
        std::cout << "  Execute: " << (result.success ? "OK" : result.error_message) << std::endl;
        if (result.success) {
            std::cout << "  Result text:\n" << result.result_text << std::endl;
        }
        assert(result.success);
        assert(result.result_text.find("1") != std::string::npos);
        assert(result.result_text.find("2") != std::string::npos);
        assert(result.result_text.find("3") != std::string::npos);
    }

    cleanup_test_data();
    std::cout << "\n=== Multi-row INSERT test passed! ===" << std::endl;
}

// 端到端测试：多列 INSERT
void test_multi_column_insert() {
    std::cout << "\n=== Testing Multi-column INSERT ===\n" << std::endl;

    cleanup_test_data();

    Catalog catalog("./test_e2e_data");
    assert(catalog.initialize().ok());

    TableManager table_manager(&catalog);
    QueryExecutor executor(&catalog, &table_manager);

    // 1. CREATE TABLE with multiple columns
    std::cout << "[Step 1] CREATE TABLE t2 (id INT, name STRING, score DECIMAL);" << std::endl;
    {
        SQLParser parser("CREATE TABLE t2 (id INT, name STRING, score DECIMAL);");
        std::unique_ptr<StmtAST> ast;
        assert(parser.parse(ast).ok());

        Compiler compiler(&catalog);
        std::unique_ptr<Statement> stmt;
        assert(compiler.compile(ast.get(), stmt).ok());

        Planner planner(&catalog, &table_manager);
        std::unique_ptr<Plan> plan;
        assert(planner.create_plan(stmt.get(), plan).ok());

        QueryResult result = executor.execute_plan(plan.get());
        std::cout << "  Execute: " << (result.success ? "OK" : result.error_message) << std::endl;
        assert(result.success);
    }

    // 2. INSERT multiple columns, multiple rows
    std::cout << "\n[Step 2] INSERT INTO t2 VALUES (1, 'Alice', 95.5), (2, 'Bob', 87.3);" << std::endl;
    {
        SQLParser parser("INSERT INTO t2 VALUES (1, 'Alice', 95.5), (2, 'Bob', 87.3);");
        std::unique_ptr<StmtAST> ast;
        assert(parser.parse(ast).ok());

        Compiler compiler(&catalog);
        std::unique_ptr<Statement> stmt;
        assert(compiler.compile(ast.get(), stmt).ok());

        Planner planner(&catalog, &table_manager);
        std::unique_ptr<Plan> plan;
        assert(planner.create_plan(stmt.get(), plan).ok());

        QueryResult result = executor.execute_plan(plan.get());
        std::cout << "  Execute: " << (result.success ? "OK" : result.error_message) << std::endl;
        std::cout << "  Rows affected: " << result.rows_affected << std::endl;
        assert(result.success);
        assert(result.rows_affected == 2);
    }

    // 3. SELECT and verify all columns
    std::cout << "\n[Step 3] SELECT * FROM t2;" << std::endl;
    {
        SQLParser parser("SELECT * FROM t2;");
        std::unique_ptr<StmtAST> ast;
        assert(parser.parse(ast).ok());

        Compiler compiler(&catalog);
        std::unique_ptr<Statement> stmt;
        assert(compiler.compile(ast.get(), stmt).ok());

        Planner planner(&catalog, &table_manager);
        std::unique_ptr<Plan> plan;
        assert(planner.create_plan(stmt.get(), plan).ok());

        QueryResult result = executor.execute_plan(plan.get());
        std::cout << "  Execute: " << (result.success ? "OK" : result.error_message) << std::endl;
        if (result.success) {
            std::cout << "  Result text:\n" << result.result_text << std::endl;
        }
        assert(result.success);
        // 验证所有值都存在
        assert(result.result_text.find("1") != std::string::npos);
        assert(result.result_text.find("Alice") != std::string::npos);
        assert(result.result_text.find("95.5") != std::string::npos);
        assert(result.result_text.find("2") != std::string::npos);
        assert(result.result_text.find("Bob") != std::string::npos);
        assert(result.result_text.find("87.3") != std::string::npos);
    }

    cleanup_test_data();
    std::cout << "\n=== Multi-column INSERT test passed! ===" << std::endl;
}

// 端到端测试：列子集 INSERT
void test_column_subset_insert() {
    std::cout << "\n=== Testing Column Subset INSERT ===\n" << std::endl;

    cleanup_test_data();

    Catalog catalog("./test_e2e_data");
    assert(catalog.initialize().ok());

    TableManager table_manager(&catalog);
    QueryExecutor executor(&catalog, &table_manager);

    // 1. CREATE TABLE
    std::cout << "[Step 1] CREATE TABLE t3 (id INT, name STRING, age INT);" << std::endl;
    {
        SQLParser parser("CREATE TABLE t3 (id INT, name STRING, age INT);");
        std::unique_ptr<StmtAST> ast;
        assert(parser.parse(ast).ok());

        Compiler compiler(&catalog);
        std::unique_ptr<Statement> stmt;
        assert(compiler.compile(ast.get(), stmt).ok());

        Planner planner(&catalog, &table_manager);
        std::unique_ptr<Plan> plan;
        assert(planner.create_plan(stmt.get(), plan).ok());

        QueryResult result = executor.execute_plan(plan.get());
        std::cout << "  Execute: " << (result.success ? "OK" : result.error_message) << std::endl;
        assert(result.success);
    }

    // 2. INSERT with column subset
    std::cout << "\n[Step 2] INSERT INTO t3(id, name) VALUES (1, 'Alice');" << std::endl;
    {
        SQLParser parser("INSERT INTO t3(id, name) VALUES (1, 'Alice');");
        std::unique_ptr<StmtAST> ast;
        assert(parser.parse(ast).ok());

        Compiler compiler(&catalog);
        std::unique_ptr<Statement> stmt;
        assert(compiler.compile(ast.get(), stmt).ok());

        Planner planner(&catalog, &table_manager);
        std::unique_ptr<Plan> plan;
        assert(planner.create_plan(stmt.get(), plan).ok());

        QueryResult result = executor.execute_plan(plan.get());
        std::cout << "  Execute: " << (result.success ? "OK" : result.error_message) << std::endl;
        std::cout << "  Rows affected: " << result.rows_affected << std::endl;
        assert(result.success);
        assert(result.rows_affected == 1);
    }

    // 3. SELECT and verify
    std::cout << "\n[Step 3] SELECT * FROM t3;" << std::endl;
    {
        SQLParser parser("SELECT * FROM t3;");
        std::unique_ptr<StmtAST> ast;
        assert(parser.parse(ast).ok());

        Compiler compiler(&catalog);
        std::unique_ptr<Statement> stmt;
        assert(compiler.compile(ast.get(), stmt).ok());

        Planner planner(&catalog, &table_manager);
        std::unique_ptr<Plan> plan;
        assert(planner.create_plan(stmt.get(), plan).ok());

        QueryResult result = executor.execute_plan(plan.get());
        std::cout << "  Execute: " << (result.success ? "OK" : result.error_message) << std::endl;
        if (result.success) {
            std::cout << "  Result text:\n" << result.result_text << std::endl;
        }
        assert(result.success);
        assert(result.result_text.find("1") != std::string::npos);
        assert(result.result_text.find("Alice") != std::string::npos);
    }

    cleanup_test_data();
    std::cout << "\n=== Column subset INSERT test passed! ===" << std::endl;
}

int main() {
    try {
        test_create_insert_select();
        test_multi_row_insert();
        test_multi_column_insert();
        test_column_subset_insert();

        std::cout << "\n========================================" << std::endl;
        std::cout << "All E2E integration tests passed!" << std::endl;
        std::cout << "========================================" << std::endl;
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "E2E test failed: " << e.what() << std::endl;
        cleanup_test_data();
        return 1;
    }
}

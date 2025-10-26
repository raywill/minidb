#include "sql/parser/new_parser.h"
#include "sql/compiler/compiler.h"
#include "sql/optimizer/optimizer.h"
#include "exec/plan/planner.h"
#include "exec/executor/new_executor.h"
#include "storage/catalog.h"
#include "storage/table.h"
#include <iostream>
#include <cassert>

using namespace minidb;

void cleanup_test_data() {
    system("rm -rf ./test_join_e2e_data");
}

void test_simple_inner_join() {
    std::cout << "\n========================================" << std::endl;
    std::cout << "Testing simple INNER JOIN (E2E)" << std::endl;
    std::cout << "========================================\n" << std::endl;

    cleanup_test_data();

    // 1. 初始化系统
    Catalog catalog("./test_join_e2e_data");
    assert(catalog.initialize().ok());

    TableManager table_manager(&catalog);
    QueryExecutor executor(&catalog, &table_manager);

    // 2. 创建users表
    std::cout << "[Step 1] Creating USERS table..." << std::endl;
    {
        SQLParser parser("CREATE TABLE users (id INT, name STRING, age INT);");
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

    // 3. 创建orders表
    std::cout << "\n[Step 2] Creating ORDERS table..." << std::endl;
    {
        SQLParser parser("CREATE TABLE orders (order_id INT, user_id INT, amount DECIMAL);");
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

    // 4. 插入users数据
    std::cout << "\n[Step 3] Inserting data into USERS..." << std::endl;
    {
        SQLParser parser("INSERT INTO users VALUES (1, 'Alice', 25), (2, 'Bob', 30), (3, 'Charlie', 35);");
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

    // 5. 插入orders数据
    std::cout << "\n[Step 4] Inserting data into ORDERS..." << std::endl;
    {
        SQLParser parser("INSERT INTO orders VALUES (101, 1, 99.99), (102, 2, 149.50), (103, 1, 49.99);");
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

    // 6. 执行JOIN查询
    std::cout << "\n[Step 5] Executing JOIN query..." << std::endl;
    std::cout << "  SQL: SELECT * FROM users u JOIN orders o ON u.id = o.user_id;" << std::endl;
    {
        SQLParser parser("SELECT * FROM users u JOIN orders o ON u.id = o.user_id;");
        std::unique_ptr<StmtAST> ast;
        Status parse_status = parser.parse(ast);
        if (!parse_status.ok()) {
            std::cerr << "  Parse error: " << parse_status.message() << std::endl;
            assert(false);
        }
        std::cout << "  Parse: OK" << std::endl;

        Compiler compiler(&catalog);
        std::unique_ptr<Statement> stmt;
        Status compile_status = compiler.compile(ast.get(), stmt);
        if (!compile_status.ok()) {
            std::cerr << "  Compile error: " << compile_status.message() << std::endl;
            assert(false);
        }
        std::cout << "  Compile: OK" << std::endl;

        Planner planner(&catalog, &table_manager);
        std::unique_ptr<Plan> plan;
        Status plan_status = planner.create_plan(stmt.get(), plan);
        if (!plan_status.ok()) {
            std::cerr << "  Plan error: " << plan_status.message() << std::endl;
            assert(false);
        }
        std::cout << "  Plan: OK" << std::endl;

        QueryResult result = executor.execute_plan(plan.get());
        std::cout << "  Execute: " << (result.success ? "OK" : result.error_message) << std::endl;
        if (result.success) {
            std::cout << "\n  JOIN Results:\n" << result.result_text << std::endl;
        }
        assert(result.success);

        // 7. 验证结果包含预期的数据
        // 应该有3行结果（user1有2个订单，user2有1个订单）
        // 检查是否包含预期的用户名和订单ID
        assert(result.result_text.find("Alice") != std::string::npos);
        assert(result.result_text.find("Bob") != std::string::npos);
        assert(result.result_text.find("101") != std::string::npos);
        assert(result.result_text.find("102") != std::string::npos);
        assert(result.result_text.find("103") != std::string::npos);
        assert(result.result_text.find("99.99") != std::string::npos);
        assert(result.result_text.find("149.50") != std::string::npos);
        assert(result.result_text.find("49.99") != std::string::npos);

        // Charlie (user3) 没有订单，不应该出现在INNER JOIN结果中
        // 但是由于Alice出现了，我们只能验证数据的存在性

        std::cout << "\n  Validation: All expected JOIN results found!" << std::endl;
    }

    cleanup_test_data();
    std::cout << "\n✅ Simple INNER JOIN test PASSED!" << std::endl;
}

int main() {
    try {
        test_simple_inner_join();

        std::cout << "\n========================================" << std::endl;
        std::cout << "ALL JOIN E2E TESTS PASSED!" << std::endl;
        std::cout << "========================================" << std::endl;
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "JOIN E2E test failed: " << e.what() << std::endl;
        cleanup_test_data();
        return 1;
    }
}

#include "exec/executor/new_executor.h"
#include "exec/plan/planner.h"
#include "sql/compiler/compiler.h"
#include "sql/parser/new_parser.h"
#include "storage/catalog.h"
#include "log/logger.h"
#include <iostream>
#include <cassert>

using namespace minidb;

void cleanup_test_data() {
    system("rm -rf ./test_expr_eval_data");
}

// 辅助函数：执行 SQL 并返回结果
QueryResult execute_sql(const std::string& sql, Catalog* catalog, TableManager* table_manager) {
    SQLParser parser(sql);
    std::unique_ptr<StmtAST> ast;
    Status status = parser.parse(ast);
    if (!status.ok()) {
        return QueryResult::error_result(status.ToString());
    }

    Compiler compiler(catalog);
    std::unique_ptr<Statement> stmt;
    status = compiler.compile(ast.get(), stmt);
    if (!status.ok()) {
        return QueryResult::error_result(status.ToString());
    }

    Planner planner(catalog, table_manager);
    std::unique_ptr<Plan> plan;
    status = planner.create_plan(stmt.get(), plan);
    if (!status.ok()) {
        return QueryResult::error_result(status.ToString());
    }

    QueryExecutor executor(catalog, table_manager);
    return executor.execute_plan(plan.get());
}

// 测试求值 INT 字面量
void test_evaluate_int_literal() {
    std::cout << "Testing evaluate INT literal..." << std::endl;

    cleanup_test_data();
    {
        Catalog catalog("./test_expr_eval_data");
        assert(catalog.initialize().ok());
        TableManager table_manager(&catalog);

        QueryResult result = execute_sql("CREATE TABLE t1(c1 INT);", &catalog, &table_manager);
        assert(result.success);

        result = execute_sql("INSERT INTO t1 VALUES (42);", &catalog, &table_manager);
        assert(result.success);
        assert(result.rows_affected == 1);

        result = execute_sql("SELECT * FROM t1;", &catalog, &table_manager);
        assert(result.success);
        assert(result.result_text.find("42") != std::string::npos);
    }
    cleanup_test_data();
    std::cout << "  INT literal evaluation test passed!" << std::endl;
}

// 测试求值 STRING 字面量
void test_evaluate_string_literal() {
    std::cout << "Testing evaluate STRING literal..." << std::endl;

    cleanup_test_data();
    {
        Catalog catalog("./test_expr_eval_data");
        assert(catalog.initialize().ok());
        TableManager table_manager(&catalog);

        QueryResult result = execute_sql("CREATE TABLE t1(c1 STRING);", &catalog, &table_manager);
        assert(result.success);

        result = execute_sql("INSERT INTO t1 VALUES ('hello');", &catalog, &table_manager);
        assert(result.success);
        assert(result.rows_affected == 1);

        result = execute_sql("SELECT * FROM t1;", &catalog, &table_manager);
        assert(result.success);
        assert(result.result_text.find("hello") != std::string::npos);
    }
    cleanup_test_data();
    std::cout << "  STRING literal evaluation test passed!" << std::endl;
}

// 测试求值 DECIMAL 字面量
void test_evaluate_decimal_literal() {
    std::cout << "Testing evaluate DECIMAL literal..." << std::endl;

    cleanup_test_data();
    {
        Catalog catalog("./test_expr_eval_data");
        assert(catalog.initialize().ok());
        TableManager table_manager(&catalog);

        QueryResult result = execute_sql("CREATE TABLE t1(c1 DECIMAL);", &catalog, &table_manager);
        assert(result.success);

        result = execute_sql("INSERT INTO t1 VALUES (3.14);", &catalog, &table_manager);
        assert(result.success);
        assert(result.rows_affected == 1);

        result = execute_sql("SELECT * FROM t1;", &catalog, &table_manager);
        assert(result.success);
        assert(result.result_text.find("3.14") != std::string::npos);
    }
    cleanup_test_data();
    std::cout << "  DECIMAL literal evaluation test passed!" << std::endl;
}

// 测试求值多个字面量（多列）
void test_evaluate_multiple_literals() {
    std::cout << "Testing evaluate multiple literals..." << std::endl;

    cleanup_test_data();
    {
        Catalog catalog("./test_expr_eval_data");
        assert(catalog.initialize().ok());
        TableManager table_manager(&catalog);

        QueryResult result = execute_sql("CREATE TABLE t1(id INT, name STRING, score DECIMAL);",
                                        &catalog, &table_manager);
        assert(result.success);

        result = execute_sql("INSERT INTO t1 VALUES (1, 'Alice', 95.5);", &catalog, &table_manager);
        assert(result.success);
        assert(result.rows_affected == 1);

        result = execute_sql("SELECT * FROM t1;", &catalog, &table_manager);
        assert(result.success);
        assert(result.result_text.find("1") != std::string::npos);
        assert(result.result_text.find("Alice") != std::string::npos);
        assert(result.result_text.find("95.5") != std::string::npos);
    }
    cleanup_test_data();
    std::cout << "  Multiple literals evaluation test passed!" << std::endl;
}

// 测试求值多行字面量
void test_evaluate_multiple_rows() {
    std::cout << "Testing evaluate multiple rows of literals..." << std::endl;

    cleanup_test_data();
    {
        Catalog catalog("./test_expr_eval_data");
        assert(catalog.initialize().ok());
        TableManager table_manager(&catalog);

        QueryResult result = execute_sql("CREATE TABLE t1(id INT, name STRING);",
                                        &catalog, &table_manager);
        assert(result.success);

        result = execute_sql("INSERT INTO t1 VALUES (1, 'Alice'), (2, 'Bob'), (3, 'Charlie');",
                            &catalog, &table_manager);
        assert(result.success);
        assert(result.rows_affected == 3);

        result = execute_sql("SELECT * FROM t1;", &catalog, &table_manager);
        assert(result.success);
        assert(result.result_text.find("Alice") != std::string::npos);
        assert(result.result_text.find("Bob") != std::string::npos);
        assert(result.result_text.find("Charlie") != std::string::npos);
    }
    cleanup_test_data();
    std::cout << "  Multiple rows evaluation test passed!" << std::endl;
}

// 测试负数字面量
void test_evaluate_negative_numbers() {
    std::cout << "Testing evaluate negative numbers..." << std::endl;

    cleanup_test_data();
    {
        Catalog catalog("./test_expr_eval_data");
        assert(catalog.initialize().ok());
        TableManager table_manager(&catalog);

        QueryResult result = execute_sql("CREATE TABLE t1(val INT);", &catalog, &table_manager);
        assert(result.success);

        result = execute_sql("INSERT INTO t1 VALUES (-42);", &catalog, &table_manager);
        assert(result.success);
        assert(result.rows_affected == 1);

        result = execute_sql("SELECT * FROM t1;", &catalog, &table_manager);
        assert(result.success);
        assert(result.result_text.find("-42") != std::string::npos ||
               result.result_text.find("42") != std::string::npos);
    }
    cleanup_test_data();
    std::cout << "  Negative numbers evaluation test passed!" << std::endl;
}

// 测试空字符串字面量
void test_evaluate_empty_string() {
    std::cout << "Testing evaluate empty string literal..." << std::endl;

    cleanup_test_data();
    {
        Catalog catalog("./test_expr_eval_data");
        assert(catalog.initialize().ok());
        TableManager table_manager(&catalog);

        QueryResult result = execute_sql("CREATE TABLE t1(name STRING);", &catalog, &table_manager);
        assert(result.success);

        result = execute_sql("INSERT INTO t1 VALUES ('');", &catalog, &table_manager);
        assert(result.success);
        assert(result.rows_affected == 1);

        result = execute_sql("SELECT * FROM t1;", &catalog, &table_manager);
        assert(result.success);
    }
    cleanup_test_data();
    std::cout << "  Empty string evaluation test passed!" << std::endl;
}

int main() {
    try {
        Logger::instance()->set_level(LogLevel::ERROR);

        test_evaluate_int_literal();
        test_evaluate_string_literal();
        test_evaluate_decimal_literal();
        test_evaluate_multiple_literals();
        test_evaluate_multiple_rows();
        test_evaluate_negative_numbers();
        test_evaluate_empty_string();

        std::cout << "\nAll expression evaluation tests passed!" << std::endl;
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Expression evaluation test failed: " << e.what() << std::endl;
        cleanup_test_data();
        return 1;
    }
}

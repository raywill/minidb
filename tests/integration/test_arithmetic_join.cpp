#include "sql/parser/new_parser.h"
#include "sql/compiler/compiler.h"
#include "exec/plan/planner.h"
#include "exec/executor/new_executor.h"
#include "storage/catalog.h"
#include "storage/table.h"
#include <iostream>
#include <cassert>

using namespace minidb;

int main() {
    system("rm -rf ./test_arithmetic_join_data");
    
    Catalog catalog("./test_arithmetic_join_data");
    assert(catalog.initialize().ok());
    TableManager table_manager(&catalog);
    QueryExecutor executor(&catalog, &table_manager);

    std::cout << "Creating tables..." << std::endl;
    
    {
        SQLParser parser("CREATE TABLE t1 (c1 INT);");
        std::unique_ptr<StmtAST> ast;
        assert(parser.parse(ast).ok());
        Compiler compiler(&catalog);
        std::unique_ptr<Statement> stmt;
        assert(compiler.compile(ast.get(), stmt).ok());
        Planner planner(&catalog, &table_manager);
        std::unique_ptr<Plan> plan;
        assert(planner.create_plan(stmt.get(), plan).ok());
        QueryResult result = executor.execute_plan(plan.get());
        assert(result.success);
    }

    {
        SQLParser parser("CREATE TABLE t2 (c2 INT);");
        std::unique_ptr<StmtAST> ast;
        assert(parser.parse(ast).ok());
        Compiler compiler(&catalog);
        std::unique_ptr<Statement> stmt;
        assert(compiler.compile(ast.get(), stmt).ok());
        Planner planner(&catalog, &table_manager);
        std::unique_ptr<Plan> plan;
        assert(planner.create_plan(stmt.get(), plan).ok());
        QueryResult result = executor.execute_plan(plan.get());
        assert(result.success);
    }

    std::cout << "Inserting data..." << std::endl;

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
        assert(result.success);
        std::cout << "  t1: " << result.rows_affected << " rows inserted" << std::endl;
    }

    {
        SQLParser parser("INSERT INTO t2 VALUES (1), (2), (3), (4);");
        std::unique_ptr<StmtAST> ast;
        assert(parser.parse(ast).ok());
        Compiler compiler(&catalog);
        std::unique_ptr<Statement> stmt;
        assert(compiler.compile(ast.get(), stmt).ok());
        Planner planner(&catalog, &table_manager);
        std::unique_ptr<Plan> plan;
        assert(planner.create_plan(stmt.get(), plan).ok());
        QueryResult result = executor.execute_plan(plan.get());
        assert(result.success);
        std::cout << "  t2: " << result.rows_affected << " rows inserted" << std::endl;
    }

    std::cout << "\nExecuting JOIN query: SELECT * FROM t1 JOIN t2 ON t1.c1 = t2.c2 + 1" << std::endl;

    {
        SQLParser parser("SELECT * FROM t1 JOIN t2 ON t1.c1 = t2.c2 + 1;");
        std::unique_ptr<StmtAST> ast;
        assert(parser.parse(ast).ok());
        Compiler compiler(&catalog);
        std::unique_ptr<Statement> stmt;
        assert(compiler.compile(ast.get(), stmt).ok());
        Planner planner(&catalog, &table_manager);
        std::unique_ptr<Plan> plan;
        assert(planner.create_plan(stmt.get(), plan).ok());
        QueryResult result = executor.execute_plan(plan.get());
        
        std::cout << "\nResult:" << std::endl;
        std::cout << result.result_text << std::endl;
        
        assert(result.success);
        // 应该有2行: (2,1) 和 (3,2)
        assert(result.result_text.find("2") != std::string::npos);
        assert(result.result_text.find("3") != std::string::npos);
        
        std::cout << "\n✅ Arithmetic JOIN test PASSED!" << std::endl;
    }

    system("rm -rf ./test_arithmetic_join_data");
    return 0;
}

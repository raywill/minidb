#include "sql/compiler/compiler.h"
#include "sql/parser/new_parser.h"
#include "sql/ast/ast.h"
#include "storage/catalog.h"
#include <iostream>
#include <cassert>

using namespace minidb;

void cleanup_test_data() {
    system("rm -rf ./test_compiler_join_data");
}

Catalog* setup_catalog_with_tables() {
    cleanup_test_data();
    Catalog* catalog = new Catalog("./test_compiler_join_data");
    assert(catalog->initialize().ok());

    // Create users table
    TableSchema users_schema("users");
    users_schema.add_column("id", DataType::INT);
    users_schema.add_column("name", DataType::STRING);
    users_schema.add_column("age", DataType::INT);
    assert(catalog->create_table("USERS", users_schema).ok());

    // Create orders table
    TableSchema orders_schema("orders");
    orders_schema.add_column("order_id", DataType::INT);
    orders_schema.add_column("user_id", DataType::INT);
    orders_schema.add_column("total", DataType::DECIMAL);
    assert(catalog->create_table("ORDERS", orders_schema).ok());

    return catalog;
}

void test_compile_simple_join() {
    std::cout << "Testing compile simple JOIN..." << std::endl;

    Catalog* catalog = setup_catalog_with_tables();

    SQLParser parser("SELECT * FROM users u JOIN orders o ON u.id = o.user_id;");
    std::unique_ptr<StmtAST> ast;
    Status parse_status = parser.parse(ast);
    if (!parse_status.ok()) {
        std::cerr << "Parse failed: " << parse_status.message() << std::endl;
    }
    assert(parse_status.ok());

    Compiler compiler(catalog);
    std::unique_ptr<Statement> stmt;
    Status compile_status = compiler.compile(ast.get(), stmt);
    if (!compile_status.ok()) {
        std::cerr << "Compile failed: " << compile_status.message() << std::endl;
    }
    assert(compile_status.ok());
    assert(stmt->get_type() == StatementType::SELECT);

    SelectStatement* select_stmt = static_cast<SelectStatement*>(stmt.get());
    assert(select_stmt->get_table_name() == "USERS");
    assert(select_stmt->get_table_alias() == "U");
    assert(select_stmt->has_joins());
    assert(select_stmt->get_joins().size() == 1);

    const auto& join = select_stmt->get_joins()[0];
    assert(join.table_name == "ORDERS");
    assert(join.table_alias == "O");
    assert(join.join_type == JoinType::INNER);
    assert(join.condition != nullptr);

    delete catalog;
    cleanup_test_data();
    std::cout << "Compile simple JOIN test passed!" << std::endl;
}

void test_compile_join_with_qualified_columns() {
    std::cout << "Testing compile JOIN with qualified columns..." << std::endl;

    Catalog* catalog = setup_catalog_with_tables();

    SQLParser parser("SELECT u.name, o.total FROM users u JOIN orders o ON u.id = o.user_id;");
    std::unique_ptr<StmtAST> ast;
    assert(parser.parse(ast).ok());

    Compiler compiler(catalog);
    std::unique_ptr<Statement> stmt;
    Status compile_status = compiler.compile(ast.get(), stmt);
    if (!compile_status.ok()) {
        std::cerr << "Compile failed: " << compile_status.message() << std::endl;
    }
    assert(compile_status.ok());

    SelectStatement* select_stmt = static_cast<SelectStatement*>(stmt.get());
    assert(select_stmt->get_select_columns().size() == 2);

    // Column names are stored as they appear in the schema
    std::cout << "  Column 0: " << select_stmt->get_select_columns()[0] << std::endl;
    std::cout << "  Column 1: " << select_stmt->get_select_columns()[1] << std::endl;

    delete catalog;
    cleanup_test_data();
    std::cout << "Compile JOIN with qualified columns test passed!" << std::endl;
}

void test_compile_join_with_where() {
    std::cout << "Testing compile JOIN with WHERE..." << std::endl;

    Catalog* catalog = setup_catalog_with_tables();

    SQLParser parser("SELECT * FROM users u JOIN orders o ON u.id = o.user_id WHERE u.age > 18;");
    std::unique_ptr<StmtAST> ast;
    assert(parser.parse(ast).ok());

    Compiler compiler(catalog);
    std::unique_ptr<Statement> stmt;
    Status compile_status = compiler.compile(ast.get(), stmt);
    if (!compile_status.ok()) {
        std::cerr << "Compile failed: " << compile_status.message() << std::endl;
    }
    assert(compile_status.ok());

    SelectStatement* select_stmt = static_cast<SelectStatement*>(stmt.get());
    assert(select_stmt->has_joins());
    assert(select_stmt->get_where_clause() != nullptr);

    delete catalog;
    cleanup_test_data();
    std::cout << "Compile JOIN with WHERE test passed!" << std::endl;
}

void test_compile_join_ambiguous_column() {
    std::cout << "Testing compile JOIN with ambiguous column (should fail)..." << std::endl;

    Catalog* catalog = new Catalog("./test_compiler_join_data");
    assert(catalog->initialize().ok());

    // Create two tables that both have an 'id' column
    TableSchema t1("t1");
    t1.add_column("id", DataType::INT);
    t1.add_column("name", DataType::STRING);
    assert(catalog->create_table("T1", t1).ok());

    TableSchema t2("t2");
    t2.add_column("id", DataType::INT);
    t2.add_column("value", DataType::STRING);
    assert(catalog->create_table("T2", t2).ok());

    // Both tables have 'id' column, but not qualified in WHERE - should fail
    SQLParser parser("SELECT * FROM t1 JOIN t2 ON t1.id = t2.id WHERE id > 10;");
    std::unique_ptr<StmtAST> ast;
    assert(parser.parse(ast).ok());

    Compiler compiler(catalog);
    std::unique_ptr<Statement> stmt;
    Status compile_status = compiler.compile(ast.get(), stmt);

    // Should fail due to ambiguous column 'id'
    assert(!compile_status.ok());
    std::cout << "  Expected error: " << compile_status.message() << std::endl;

    delete catalog;
    cleanup_test_data();
    std::cout << "Compile JOIN with ambiguous column test passed (correctly failed)!" << std::endl;
}

void test_compile_join_table_not_found() {
    std::cout << "Testing compile JOIN with non-existent table (should fail)..." << std::endl;

    Catalog* catalog = setup_catalog_with_tables();

    SQLParser parser("SELECT * FROM users u JOIN nonexistent n ON u.id = n.id;");
    std::unique_ptr<StmtAST> ast;
    assert(parser.parse(ast).ok());

    Compiler compiler(catalog);
    std::unique_ptr<Statement> stmt;
    Status compile_status = compiler.compile(ast.get(), stmt);

    // Should fail due to non-existent table
    assert(!compile_status.ok());
    std::cout << "  Expected error: " << compile_status.message() << std::endl;

    delete catalog;
    cleanup_test_data();
    std::cout << "Compile JOIN with non-existent table test passed (correctly failed)!" << std::endl;
}

void test_compile_left_join() {
    std::cout << "Testing compile LEFT JOIN..." << std::endl;

    Catalog* catalog = setup_catalog_with_tables();

    SQLParser parser("SELECT * FROM users u LEFT JOIN orders o ON u.id = o.user_id;");
    std::unique_ptr<StmtAST> ast;
    assert(parser.parse(ast).ok());

    Compiler compiler(catalog);
    std::unique_ptr<Statement> stmt;
    Status compile_status = compiler.compile(ast.get(), stmt);
    if (!compile_status.ok()) {
        std::cerr << "Compile failed: " << compile_status.message() << std::endl;
    }
    assert(compile_status.ok());

    SelectStatement* select_stmt = static_cast<SelectStatement*>(stmt.get());
    assert(select_stmt->has_joins());
    assert(select_stmt->get_joins()[0].join_type == JoinType::LEFT_OUTER);

    delete catalog;
    cleanup_test_data();
    std::cout << "Compile LEFT JOIN test passed!" << std::endl;
}

int main() {
    try {
        test_compile_simple_join();
        test_compile_join_with_qualified_columns();
        test_compile_join_with_where();
        test_compile_join_ambiguous_column();
        test_compile_join_table_not_found();
        test_compile_left_join();

        std::cout << "\n========================================" << std::endl;
        std::cout << "All Compiler JOIN tests passed!" << std::endl;
        std::cout << "========================================" << std::endl;
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Compiler JOIN test failed: " << e.what() << std::endl;
        cleanup_test_data();
        return 1;
    }
}

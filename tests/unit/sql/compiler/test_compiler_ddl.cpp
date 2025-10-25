#include "sql/compiler/compiler.h"
#include "sql/parser/new_parser.h"
#include "sql/ast/ast.h"
#include "storage/catalog.h"
#include <iostream>
#include <cassert>

using namespace minidb;

void cleanup_test_data() {
    system("rm -rf ./test_compiler_ddl_data");
}

void test_compile_create_table() {
    std::cout << "Testing compile CREATE TABLE..." << std::endl;

    cleanup_test_data();
    Catalog catalog("./test_compiler_ddl_data");
    assert(catalog.initialize().ok());

    // Parse AST
    SQLParser parser("CREATE TABLE users(id INT, name STRING, age INT);");
    std::unique_ptr<StmtAST> ast;
    Status status = parser.parse(ast);
    assert(status.ok());

    // Compile to Statement
    Compiler compiler(&catalog);
    std::unique_ptr<Statement> stmt;
    status = compiler.compile(ast.get(), stmt);
    assert(status.ok());
    assert(stmt != nullptr);
    assert(stmt->get_type() == StatementType::CREATE_TABLE);

    // Verify Statement details
    CreateTableStatement* create_stmt = static_cast<CreateTableStatement*>(stmt.get());
    assert(create_stmt->get_table_name() == "USERS");
    assert(create_stmt->get_columns().size() == 3);
    assert(create_stmt->get_columns()[0].name == "ID");
    assert(create_stmt->get_columns()[0].type == DataType::INT);
    assert(create_stmt->get_columns()[1].name == "NAME");
    assert(create_stmt->get_columns()[1].type == DataType::STRING);

    cleanup_test_data();
    std::cout << "Compile CREATE TABLE test passed!" << std::endl;
}

void test_compile_create_table_if_not_exists() {
    std::cout << "Testing compile CREATE TABLE IF NOT EXISTS..." << std::endl;

    cleanup_test_data();
    Catalog catalog("./test_compiler_ddl_data");
    assert(catalog.initialize().ok());

    SQLParser parser("CREATE TABLE IF NOT EXISTS products(id INT, price DECIMAL);");
    std::unique_ptr<StmtAST> ast;
    assert(parser.parse(ast).ok());

    Compiler compiler(&catalog);
    std::unique_ptr<Statement> stmt;
    assert(compiler.compile(ast.get(), stmt).ok());

    CreateTableStatement* create_stmt = static_cast<CreateTableStatement*>(stmt.get());
    assert(create_stmt->get_if_not_exists() == true);

    cleanup_test_data();
    std::cout << "Compile CREATE TABLE IF NOT EXISTS test passed!" << std::endl;
}

void test_compile_drop_table() {
    std::cout << "Testing compile DROP TABLE..." << std::endl;

    cleanup_test_data();
    Catalog catalog("./test_compiler_ddl_data");
    assert(catalog.initialize().ok());

    SQLParser parser("DROP TABLE users;");
    std::unique_ptr<StmtAST> ast;
    assert(parser.parse(ast).ok());

    Compiler compiler(&catalog);
    std::unique_ptr<Statement> stmt;
    assert(compiler.compile(ast.get(), stmt).ok());
    assert(stmt->get_type() == StatementType::DROP_TABLE);

    DropTableStatement* drop_stmt = static_cast<DropTableStatement*>(stmt.get());
    assert(drop_stmt->get_table_name() == "USERS");
    assert(drop_stmt->get_if_exists() == false);

    cleanup_test_data();
    std::cout << "Compile DROP TABLE test passed!" << std::endl;
}

void test_compile_drop_table_if_exists() {
    std::cout << "Testing compile DROP TABLE IF EXISTS..." << std::endl;

    cleanup_test_data();
    Catalog catalog("./test_compiler_ddl_data");
    assert(catalog.initialize().ok());

    SQLParser parser("DROP TABLE IF EXISTS temp_table;");
    std::unique_ptr<StmtAST> ast;
    assert(parser.parse(ast).ok());

    Compiler compiler(&catalog);
    std::unique_ptr<Statement> stmt;
    assert(compiler.compile(ast.get(), stmt).ok());

    DropTableStatement* drop_stmt = static_cast<DropTableStatement*>(stmt.get());
    assert(drop_stmt->get_if_exists() == true);

    cleanup_test_data();
    std::cout << "Compile DROP TABLE IF EXISTS test passed!" << std::endl;
}

void test_compile_all_data_types() {
    std::cout << "Testing compile CREATE TABLE with all data types..." << std::endl;

    cleanup_test_data();
    Catalog catalog("./test_compiler_ddl_data");
    assert(catalog.initialize().ok());

    SQLParser parser("CREATE TABLE all_types(i INT, s STRING, b BOOL, d DECIMAL);");
    std::unique_ptr<StmtAST> ast;
    assert(parser.parse(ast).ok());

    Compiler compiler(&catalog);
    std::unique_ptr<Statement> stmt;
    assert(compiler.compile(ast.get(), stmt).ok());

    CreateTableStatement* create_stmt = static_cast<CreateTableStatement*>(stmt.get());
    const auto& cols = create_stmt->get_columns();
    assert(cols.size() == 4);
    assert(cols[0].type == DataType::INT);
    assert(cols[1].type == DataType::STRING);
    assert(cols[2].type == DataType::BOOL);
    assert(cols[3].type == DataType::DECIMAL);

    cleanup_test_data();
    std::cout << "Compile all data types test passed!" << std::endl;
}

int main() {
    try {
        test_compile_create_table();
        test_compile_create_table_if_not_exists();
        test_compile_drop_table();
        test_compile_drop_table_if_exists();
        test_compile_all_data_types();

        std::cout << "\nAll Compiler DDL tests passed!" << std::endl;
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Compiler DDL test failed: " << e.what() << std::endl;
        cleanup_test_data();
        return 1;
    }
}

#include "sql/compiler/compiler.h"
#include "sql/parser/new_parser.h"
#include "sql/ast/ast.h"
#include "storage/catalog.h"
#include <iostream>
#include <cassert>

using namespace minidb;

void cleanup_test_data() {
    system("rm -rf ./test_compiler_dml_data");
}

Catalog* setup_catalog_with_table() {
    cleanup_test_data();
    Catalog* catalog = new Catalog("./test_compiler_dml_data");
    assert(catalog->initialize().ok());

    // Create a test table
    TableSchema schema("students");
    schema.add_column("id", DataType::INT);
    schema.add_column("name", DataType::STRING);
    schema.add_column("age", DataType::INT);
    schema.add_column("score", DataType::DECIMAL);

    assert(catalog->create_table("STUDENTS", schema).ok());

    return catalog;
}

void test_compile_insert() {
    std::cout << "Testing compile INSERT..." << std::endl;

    Catalog* catalog = setup_catalog_with_table();

    SQLParser parser("INSERT INTO students VALUES (1, 'Alice', 20, 95.5);");
    std::unique_ptr<StmtAST> ast;
    assert(parser.parse(ast).ok());

    Compiler compiler(catalog);
    std::unique_ptr<Statement> stmt;
    Status status = compiler.compile(ast.get(), stmt);
    assert(status.ok());
    assert(stmt->get_type() == StatementType::INSERT);

    InsertStatement* insert_stmt = static_cast<InsertStatement*>(stmt.get());
    assert(insert_stmt->get_table_name() == "STUDENTS");
    assert(insert_stmt->get_values().size() == 1);
    assert(insert_stmt->get_values()[0].size() == 4);

    // Verify column indices are resolved
    assert(insert_stmt->get_column_indices().size() == 4);

    delete catalog;
    cleanup_test_data();
    std::cout << "Compile INSERT test passed!" << std::endl;
}

void test_compile_insert_multiple_rows() {
    std::cout << "Testing compile INSERT multiple rows..." << std::endl;

    Catalog* catalog = setup_catalog_with_table();

    SQLParser parser("INSERT INTO students VALUES (1, 'Alice', 20, 95.5), (2, 'Bob', 21, 87.0);");
    std::unique_ptr<StmtAST> ast;
    assert(parser.parse(ast).ok());

    Compiler compiler(catalog);
    std::unique_ptr<Statement> stmt;
    assert(compiler.compile(ast.get(), stmt).ok());

    InsertStatement* insert_stmt = static_cast<InsertStatement*>(stmt.get());
    assert(insert_stmt->get_values().size() == 2);

    delete catalog;
    cleanup_test_data();
    std::cout << "Compile INSERT multiple rows test passed!" << std::endl;
}

void test_compile_select_star() {
    std::cout << "Testing compile SELECT *..." << std::endl;

    Catalog* catalog = setup_catalog_with_table();

    SQLParser parser("SELECT * FROM students;");
    std::unique_ptr<StmtAST> ast;
    assert(parser.parse(ast).ok());

    Compiler compiler(catalog);
    std::unique_ptr<Statement> stmt;
    assert(compiler.compile(ast.get(), stmt).ok());
    assert(stmt->get_type() == StatementType::SELECT);

    SelectStatement* select_stmt = static_cast<SelectStatement*>(stmt.get());
    assert(select_stmt->get_table_name() == "STUDENTS");

    // * should expand to all columns
    assert(select_stmt->get_select_columns().size() == 4);
    assert(select_stmt->get_select_columns()[0] == "id");
    assert(select_stmt->get_select_columns()[1] == "name");
    assert(select_stmt->get_select_columns()[2] == "age");
    assert(select_stmt->get_select_columns()[3] == "score");

    delete catalog;
    cleanup_test_data();
    std::cout << "Compile SELECT * test passed!" << std::endl;
}

void test_compile_select_specific_columns() {
    std::cout << "Testing compile SELECT specific columns..." << std::endl;

    Catalog* catalog = setup_catalog_with_table();

    SQLParser parser("SELECT name, age FROM students;");
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
    assert(select_stmt->get_select_columns()[0] == "name");
    assert(select_stmt->get_select_columns()[1] == "age");

    // Verify column indices are resolved
    assert(select_stmt->get_select_column_indices().size() == 2);
    assert(select_stmt->get_select_column_indices()[0] == 1); // name is column 1
    assert(select_stmt->get_select_column_indices()[1] == 2); // age is column 2

    delete catalog;
    cleanup_test_data();
    std::cout << "Compile SELECT specific columns test passed!" << std::endl;
}

void test_compile_select_with_where() {
    std::cout << "Testing compile SELECT with WHERE..." << std::endl;

    Catalog* catalog = setup_catalog_with_table();

    SQLParser parser("SELECT name FROM students WHERE age > 18;");
    std::unique_ptr<StmtAST> ast;
    assert(parser.parse(ast).ok());

    Compiler compiler(catalog);
    std::unique_ptr<Statement> stmt;
    assert(compiler.compile(ast.get(), stmt).ok());

    SelectStatement* select_stmt = static_cast<SelectStatement*>(stmt.get());
    assert(select_stmt->get_where_clause() != nullptr);

    delete catalog;
    cleanup_test_data();
    std::cout << "Compile SELECT with WHERE test passed!" << std::endl;
}

void test_compile_delete() {
    std::cout << "Testing compile DELETE..." << std::endl;

    Catalog* catalog = setup_catalog_with_table();

    SQLParser parser("DELETE FROM students WHERE age < 18;");
    std::unique_ptr<StmtAST> ast;
    assert(parser.parse(ast).ok());

    Compiler compiler(catalog);
    std::unique_ptr<Statement> stmt;
    assert(compiler.compile(ast.get(), stmt).ok());
    assert(stmt->get_type() == StatementType::DELETE);

    DeleteStatement* delete_stmt = static_cast<DeleteStatement*>(stmt.get());
    assert(delete_stmt->get_table_name() == "STUDENTS");
    assert(delete_stmt->get_where_clause() != nullptr);

    delete catalog;
    cleanup_test_data();
    std::cout << "Compile DELETE test passed!" << std::endl;
}

void test_compile_delete_all() {
    std::cout << "Testing compile DELETE without WHERE..." << std::endl;

    Catalog* catalog = setup_catalog_with_table();

    SQLParser parser("DELETE FROM students;");
    std::unique_ptr<StmtAST> ast;
    assert(parser.parse(ast).ok());

    Compiler compiler(catalog);
    std::unique_ptr<Statement> stmt;
    assert(compiler.compile(ast.get(), stmt).ok());

    DeleteStatement* delete_stmt = static_cast<DeleteStatement*>(stmt.get());
    assert(delete_stmt->get_where_clause() == nullptr);

    delete catalog;
    cleanup_test_data();
    std::cout << "Compile DELETE without WHERE test passed!" << std::endl;
}

int main() {
    try {
        test_compile_insert();
        test_compile_insert_multiple_rows();
        test_compile_select_star();
        test_compile_select_specific_columns();
        test_compile_select_with_where();
        test_compile_delete();
        test_compile_delete_all();

        std::cout << "\nAll Compiler DML tests passed!" << std::endl;
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Compiler DML test failed: " << e.what() << std::endl;
        cleanup_test_data();
        return 1;
    }
}

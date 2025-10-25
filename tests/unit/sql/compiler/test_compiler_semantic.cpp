#include "sql/compiler/compiler.h"
#include "sql/parser/new_parser.h"
#include "sql/ast/ast.h"
#include "storage/catalog.h"
#include <iostream>
#include <cassert>

using namespace minidb;

void cleanup_test_data() {
    system("rm -rf ./test_compiler_semantic_data");
}

Catalog* setup_catalog_with_table() {
    cleanup_test_data();
    Catalog* catalog = new Catalog("./test_compiler_semantic_data");
    assert(catalog->initialize().ok());

    TableSchema schema("users");
    schema.add_column("id", DataType::INT);
    schema.add_column("name", DataType::STRING);
    schema.add_column("age", DataType::INT);

    assert(catalog->create_table("USERS", schema).ok());

    return catalog;
}

void test_table_not_found_error() {
    std::cout << "Testing table not found error..." << std::endl;

    cleanup_test_data();
    Catalog catalog("./test_compiler_semantic_data");
    assert(catalog.initialize().ok());

    // Try to select from non-existent table
    SQLParser parser("SELECT * FROM nonexistent;");
    std::unique_ptr<StmtAST> ast;
    assert(parser.parse(ast).ok());

    Compiler compiler(&catalog);
    std::unique_ptr<Statement> stmt;
    Status status = compiler.compile(ast.get(), stmt);

    // Should fail with table not found
    if (status.ok()) {
        std::cerr << "ERROR: Compilation should have failed but succeeded!" << std::endl;
    }
    if (!status.is_not_found()) {
        std::cerr << "ERROR: Expected NOT_FOUND, got: " << status.message() << std::endl;
        std::cerr << "Status is_not_found(): " << (status.is_not_found() ? "true" : "false") << std::endl;
    }
    assert(!status.ok());
    assert(status.is_not_found());

    cleanup_test_data();
    std::cout << "Table not found error test passed!" << std::endl;
}

void test_column_not_found_error() {
    std::cout << "Testing column not found error..." << std::endl;

    Catalog* catalog = setup_catalog_with_table();

    // Try to select non-existent column
    SQLParser parser("SELECT nonexistent_column FROM users;");
    std::unique_ptr<StmtAST> ast;
    assert(parser.parse(ast).ok());

    Compiler compiler(catalog);
    std::unique_ptr<Statement> stmt;
    Status status = compiler.compile(ast.get(), stmt);

    // Should fail with column not found
    assert(!status.ok());

    delete catalog;
    cleanup_test_data();
    std::cout << "Column not found error test passed!" << std::endl;
}

void test_column_resolution() {
    std::cout << "Testing column index resolution..." << std::endl;

    Catalog* catalog = setup_catalog_with_table();

    SQLParser parser("SELECT name, age FROM users;");
    std::unique_ptr<StmtAST> ast;
    assert(parser.parse(ast).ok());

    Compiler compiler(catalog);
    std::unique_ptr<Statement> stmt;
    Status status = compiler.compile(ast.get(), stmt);
    assert(status.ok());

    SelectStatement* select_stmt = static_cast<SelectStatement*>(stmt.get());

    // Verify column indices are correctly resolved
    const auto& indices = select_stmt->get_select_column_indices();
    assert(indices.size() == 2);
    assert(indices[0] == 1); // name is column 1 (0-indexed)
    assert(indices[1] == 2); // age is column 2

    delete catalog;
    cleanup_test_data();
    std::cout << "Column resolution test passed!" << std::endl;
}

void test_where_clause_column_resolution() {
    std::cout << "Testing WHERE clause column resolution..." << std::endl;

    Catalog* catalog = setup_catalog_with_table();

    SQLParser parser("SELECT * FROM users WHERE age > 18;");
    std::unique_ptr<StmtAST> ast;
    assert(parser.parse(ast).ok());

    Compiler compiler(catalog);
    std::unique_ptr<Statement> stmt;
    Status status = compiler.compile(ast.get(), stmt);
    assert(status.ok());

    SelectStatement* select_stmt = static_cast<SelectStatement*>(stmt.get());
    Expression* where_expr = select_stmt->get_where_clause();
    assert(where_expr != nullptr);

    // WHERE clause should contain resolved column references
    BinaryExpression* bin_expr = dynamic_cast<BinaryExpression*>(where_expr);
    assert(bin_expr != nullptr);

    ColumnRefExpression* col_ref = dynamic_cast<ColumnRefExpression*>(bin_expr->get_left());
    assert(col_ref != nullptr);
    assert(col_ref->get_column_index() == 2); // age is column 2

    delete catalog;
    cleanup_test_data();
    std::cout << "WHERE clause column resolution test passed!" << std::endl;
}

void test_insert_column_count_mismatch() {
    std::cout << "Testing INSERT column count validation..." << std::endl;

    Catalog* catalog = setup_catalog_with_table();

    // Table has 3 columns, but INSERT has 2 values
    SQLParser parser("INSERT INTO users VALUES (1, 'Alice');");
    std::unique_ptr<StmtAST> ast;
    assert(parser.parse(ast).ok());

    Compiler compiler(catalog);
    std::unique_ptr<Statement> stmt;
    Status status = compiler.compile(ast.get(), stmt);

    // Should fail with column count mismatch
    assert(!status.ok());

    delete catalog;
    cleanup_test_data();
    std::cout << "INSERT column count validation test passed!" << std::endl;
}

void test_case_insensitive_names() {
    std::cout << "Testing case-insensitive table and column names..." << std::endl;

    Catalog* catalog = setup_catalog_with_table();

    // Use different cases
    SQLParser parser1("SELECT NAME FROM UsErS;");
    std::unique_ptr<StmtAST> ast1;
    assert(parser1.parse(ast1).ok());

    Compiler compiler(catalog);
    std::unique_ptr<Statement> stmt1;
    Status status = compiler.compile(ast1.get(), stmt1);
    assert(status.ok());

    // Verify it found the table and column
    SelectStatement* select_stmt = static_cast<SelectStatement*>(stmt1.get());
    assert(select_stmt->get_table_name() == "USERS");
    assert(select_stmt->get_select_columns()[0] == "name");

    delete catalog;
    cleanup_test_data();
    std::cout << "Case-insensitive names test passed!" << std::endl;
}

void test_star_expansion() {
    std::cout << "Testing * expansion to all columns..." << std::endl;

    Catalog* catalog = setup_catalog_with_table();

    SQLParser parser("SELECT * FROM users;");
    std::unique_ptr<StmtAST> ast;
    assert(parser.parse(ast).ok());

    Compiler compiler(catalog);
    std::unique_ptr<Statement> stmt;
    assert(compiler.compile(ast.get(), stmt).ok());

    SelectStatement* select_stmt = static_cast<SelectStatement*>(stmt.get());

    // * should expand to all 3 columns
    assert(select_stmt->get_select_columns().size() == 3);
    assert(select_stmt->get_select_column_indices().size() == 3);
    assert(select_stmt->get_select_column_indices()[0] == 0);
    assert(select_stmt->get_select_column_indices()[1] == 1);
    assert(select_stmt->get_select_column_indices()[2] == 2);

    delete catalog;
    cleanup_test_data();
    std::cout << "Star expansion test passed!" << std::endl;
}

int main() {
    try {
        test_table_not_found_error();
        test_column_not_found_error();
        test_column_resolution();
        test_where_clause_column_resolution();
        test_insert_column_count_mismatch();
        test_case_insensitive_names();
        test_star_expansion();

        std::cout << "\nAll Compiler semantic analysis tests passed!" << std::endl;
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Compiler semantic test failed: " << e.what() << std::endl;
        cleanup_test_data();
        return 1;
    }
}

#include "sql/parser/new_parser.h"
#include "sql/ast/ast.h"
#include <iostream>
#include <cassert>

using namespace minidb;

void test_insert_single_row() {
    std::cout << "Testing INSERT single row..." << std::endl;

    SQLParser parser("INSERT INTO users VALUES (1, 'Alice');");
    std::unique_ptr<StmtAST> ast;
    Status status = parser.parse(ast);
    assert(status.ok());

    InsertAST* insert_ast = static_cast<InsertAST*>(ast.get());
    assert(insert_ast->get_table_name() == "USERS");
    assert(insert_ast->get_values().size() == 1);
    assert(insert_ast->get_values()[0].size() == 2);

    std::cout << "INSERT single row test passed!" << std::endl;
}

void test_insert_multiple_rows() {
    std::cout << "Testing INSERT multiple rows..." << std::endl;

    SQLParser parser("INSERT INTO users VALUES (1, 'Alice'), (2, 'Bob'), (3, 'Charlie');");
    std::unique_ptr<StmtAST> ast;
    Status status = parser.parse(ast);
    assert(status.ok());

    InsertAST* insert_ast = static_cast<InsertAST*>(ast.get());
    assert(insert_ast->get_values().size() == 3);

    std::cout << "INSERT multiple rows test passed!" << std::endl;
}

void test_insert_different_types() {
    std::cout << "Testing INSERT with different data types..." << std::endl;

    SQLParser parser("INSERT INTO test VALUES (42, 'string', true, 3.14);");
    std::unique_ptr<StmtAST> ast;
    Status status = parser.parse(ast);
    assert(status.ok());

    InsertAST* insert_ast = static_cast<InsertAST*>(ast.get());
    const auto& values = insert_ast->get_values()[0];
    assert(values.size() == 4);
    assert(values[0]->get_type() == ASTType::LITERAL);

    std::cout << "INSERT different types test passed!" << std::endl;
}

void test_select_star() {
    std::cout << "Testing SELECT *..." << std::endl;

    SQLParser parser("SELECT * FROM users;");
    std::unique_ptr<StmtAST> ast;
    Status status = parser.parse(ast);
    assert(status.ok());

    SelectAST* select_ast = static_cast<SelectAST*>(ast.get());
    assert(select_ast->get_select_list().size() == 1);
    assert(select_ast->get_from_table()->get_table_name() == "USERS");
    assert(select_ast->get_where_clause() == nullptr);

    std::cout << "SELECT * test passed!" << std::endl;
}

void test_select_columns() {
    std::cout << "Testing SELECT specific columns..." << std::endl;

    SQLParser parser("SELECT id, name, age FROM users;");
    std::unique_ptr<StmtAST> ast;
    Status status = parser.parse(ast);
    assert(status.ok());

    SelectAST* select_ast = static_cast<SelectAST*>(ast.get());
    assert(select_ast->get_select_list().size() == 3);

    std::cout << "SELECT columns test passed!" << std::endl;
}

void test_select_with_where() {
    std::cout << "Testing SELECT with WHERE..." << std::endl;

    SQLParser parser("SELECT name FROM users WHERE age > 18;");
    std::unique_ptr<StmtAST> ast;
    Status status = parser.parse(ast);
    assert(status.ok());

    SelectAST* select_ast = static_cast<SelectAST*>(ast.get());
    assert(select_ast->get_where_clause() != nullptr);
    assert(select_ast->get_where_clause()->get_type() == ASTType::BINARY_OP);

    std::cout << "SELECT with WHERE test passed!" << std::endl;
}

void test_delete_without_where() {
    std::cout << "Testing DELETE without WHERE..." << std::endl;

    SQLParser parser("DELETE FROM users;");
    std::unique_ptr<StmtAST> ast;
    Status status = parser.parse(ast);
    assert(status.ok());

    DeleteAST* delete_ast = static_cast<DeleteAST*>(ast.get());
    assert(delete_ast->get_from_table()->get_table_name() == "USERS");
    assert(delete_ast->get_where_clause() == nullptr);

    std::cout << "DELETE without WHERE test passed!" << std::endl;
}

void test_delete_with_where() {
    std::cout << "Testing DELETE with WHERE..." << std::endl;

    SQLParser parser("DELETE FROM users WHERE age < 18;");
    std::unique_ptr<StmtAST> ast;
    Status status = parser.parse(ast);
    assert(status.ok());

    DeleteAST* delete_ast = static_cast<DeleteAST*>(ast.get());
    assert(delete_ast->get_where_clause() != nullptr);

    std::cout << "DELETE with WHERE test passed!" << std::endl;
}

int main() {
    try {
        test_insert_single_row();
        test_insert_multiple_rows();
        test_insert_different_types();
        test_select_star();
        test_select_columns();
        test_select_with_where();
        test_delete_without_where();
        test_delete_with_where();

        std::cout << "\nAll DML parser tests passed!" << std::endl;
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "DML parser test failed: " << e.what() << std::endl;
        return 1;
    }
}

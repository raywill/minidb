#include "sql/parser/new_parser.h"
#include "sql/ast/ast.h"
#include <iostream>
#include <cassert>

using namespace minidb;

void test_create_table_basic() {
    std::cout << "Testing CREATE TABLE basic..." << std::endl;

    SQLParser parser("CREATE TABLE users(id INT, name STRING);");
    std::unique_ptr<StmtAST> ast;
    Status status = parser.parse(ast);
    assert(status.ok());
    assert(ast->get_type() == ASTType::CREATE_TABLE);

    CreateTableAST* create_ast = static_cast<CreateTableAST*>(ast.get());
    assert(create_ast->get_table_name() == "USERS");
    assert(create_ast->get_columns().size() == 2);
    assert(!create_ast->get_if_not_exists());

    std::cout << "CREATE TABLE basic test passed!" << std::endl;
}

void test_create_table_if_not_exists() {
    std::cout << "Testing CREATE TABLE IF NOT EXISTS..." << std::endl;

    SQLParser parser("CREATE TABLE IF NOT EXISTS products(id INT, name STRING, price DECIMAL);");
    std::unique_ptr<StmtAST> ast;
    Status status = parser.parse(ast);
    assert(status.ok());

    CreateTableAST* create_ast = static_cast<CreateTableAST*>(ast.get());
    assert(create_ast->get_table_name() == "PRODUCTS");
    assert(create_ast->get_columns().size() == 3);
    assert(create_ast->get_if_not_exists());

    std::cout << "CREATE TABLE IF NOT EXISTS test passed!" << std::endl;
}

void test_create_table_all_types() {
    std::cout << "Testing CREATE TABLE with all data types..." << std::endl;

    SQLParser parser("CREATE TABLE all_types(int_col INT, str_col STRING, bool_col BOOL, dec_col DECIMAL);");
    std::unique_ptr<StmtAST> ast;
    Status status = parser.parse(ast);
    assert(status.ok());

    CreateTableAST* create_ast = static_cast<CreateTableAST*>(ast.get());
    const auto& columns = create_ast->get_columns();
    assert(columns.size() == 4);
    assert(columns[0]->get_data_type() == DataType::INT);
    assert(columns[1]->get_data_type() == DataType::STRING);
    assert(columns[2]->get_data_type() == DataType::BOOL);
    assert(columns[3]->get_data_type() == DataType::DECIMAL);

    std::cout << "CREATE TABLE all types test passed!" << std::endl;
}

void test_drop_table_basic() {
    std::cout << "Testing DROP TABLE basic..." << std::endl;

    SQLParser parser("DROP TABLE users;");
    std::unique_ptr<StmtAST> ast;
    Status status = parser.parse(ast);
    assert(status.ok());

    DropTableAST* drop_ast = static_cast<DropTableAST*>(ast.get());
    assert(drop_ast->get_table_name() == "USERS");
    assert(!drop_ast->get_if_exists());

    std::cout << "DROP TABLE basic test passed!" << std::endl;
}

void test_drop_table_if_exists() {
    std::cout << "Testing DROP TABLE IF EXISTS..." << std::endl;

    SQLParser parser("DROP TABLE IF EXISTS temp_table;");
    std::unique_ptr<StmtAST> ast;
    Status status = parser.parse(ast);
    assert(status.ok());

    DropTableAST* drop_ast = static_cast<DropTableAST*>(ast.get());
    assert(drop_ast->get_table_name() == "TEMP_TABLE");
    assert(drop_ast->get_if_exists());

    std::cout << "DROP TABLE IF EXISTS test passed!" << std::endl;
}

void test_case_sensitivity() {
    std::cout << "Testing case sensitivity..." << std::endl;

    std::vector<std::string> case_variations = {
        "create table test(id int);",
        "CREATE TABLE test(id INT);",
        "Create Table test(Id Int);",
    };

    for (const std::string& sql : case_variations) {
        SQLParser parser(sql);
        std::unique_ptr<StmtAST> ast;
        Status status = parser.parse(ast);
        assert(status.ok());
        assert(ast->get_type() == ASTType::CREATE_TABLE);

        CreateTableAST* create_ast = static_cast<CreateTableAST*>(ast.get());
        assert(create_ast->get_table_name() == "TEST");
    }

    std::cout << "Case sensitivity test passed!" << std::endl;
}

int main() {
    try {
        test_create_table_basic();
        test_create_table_if_not_exists();
        test_create_table_all_types();
        test_drop_table_basic();
        test_drop_table_if_exists();
        test_case_sensitivity();

        std::cout << "\nAll DDL parser tests passed!" << std::endl;
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "DDL parser test failed: " << e.what() << std::endl;
        return 1;
    }
}

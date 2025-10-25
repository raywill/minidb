#include "sql/parser/new_parser.h"
#include "sql/parser/tokenizer.h"
#include "sql/ast/ast.h"
#include <iostream>
#include <cassert>

using namespace minidb;

void test_tokenizer() {
    std::cout << "Testing tokenizer..." << std::endl;

    std::string sql = "SELECT id, name FROM student WHERE age > 18;";
    Tokenizer tokenizer(sql);

    auto tokens = tokenizer.tokenize_all();

    assert(tokens.size() >= 10);
    assert(tokens[0].type == TokenType::SELECT);
    assert(tokens[1].type == TokenType::IDENTIFIER);
    assert(tokens[1].value == "ID");
    assert(tokens[2].type == TokenType::COMMA);
    assert(tokens[3].type == TokenType::IDENTIFIER);
    assert(tokens[3].value == "NAME");
    assert(tokens[4].type == TokenType::FROM);

    std::cout << "Tokenizer test passed!" << std::endl;
}

void test_create_table_parser() {
    std::cout << "Testing CREATE TABLE parser..." << std::endl;

    std::string sql = "CREATE TABLE student(id INT, name STRING, age INT);";
    SQLParser parser(sql);

    std::unique_ptr<StmtAST> ast;
    Status status = parser.parse(ast);

    assert(status.ok());
    assert(ast != nullptr);
    assert(ast->get_type() == ASTType::CREATE_TABLE);

    CreateTableAST* create_ast = static_cast<CreateTableAST*>(ast.get());
    assert(create_ast->get_table_name() == "STUDENT");
    assert(create_ast->get_columns().size() == 3);

    std::cout << "CREATE TABLE parser test passed!" << std::endl;
}

void test_select_parser() {
    std::cout << "Testing SELECT parser..." << std::endl;

    std::string sql = "SELECT id, name FROM student WHERE age > 18;";
    SQLParser parser(sql);

    std::unique_ptr<StmtAST> ast;
    Status status = parser.parse(ast);

    assert(status.ok());
    assert(ast != nullptr);
    assert(ast->get_type() == ASTType::SELECT);

    SelectAST* select_ast = static_cast<SelectAST*>(ast.get());
    assert(select_ast->get_select_list().size() == 2);
    assert(select_ast->get_from_table()->get_table_name() == "STUDENT");
    assert(select_ast->get_where_clause() != nullptr);

    std::cout << "SELECT parser test passed!" << std::endl;
}

void test_insert_parser() {
    std::cout << "Testing INSERT parser..." << std::endl;

    std::string sql = "INSERT INTO student VALUES (1, 'Alice', 20), (2, 'Bob', 21);";
    SQLParser parser(sql);

    std::unique_ptr<StmtAST> ast;
    Status status = parser.parse(ast);

    assert(status.ok());
    assert(ast != nullptr);
    assert(ast->get_type() == ASTType::INSERT);

    InsertAST* insert_ast = static_cast<InsertAST*>(ast.get());
    assert(insert_ast->get_table_name() == "STUDENT");
    assert(insert_ast->get_values().size() == 2);
    assert(insert_ast->get_values()[0].size() == 3);

    std::cout << "INSERT parser test passed!" << std::endl;
}

void test_delete_parser() {
    std::cout << "Testing DELETE parser..." << std::endl;

    std::string sql = "DELETE FROM student WHERE age < 18;";
    SQLParser parser(sql);

    std::unique_ptr<StmtAST> ast;
    Status status = parser.parse(ast);

    assert(status.ok());
    assert(ast != nullptr);
    assert(ast->get_type() == ASTType::DELETE);

    DeleteAST* delete_ast = static_cast<DeleteAST*>(ast.get());
    assert(delete_ast->get_from_table()->get_table_name() == "STUDENT");
    assert(delete_ast->get_where_clause() != nullptr);

    std::cout << "DELETE parser test passed!" << std::endl;
}

void test_expression_parser() {
    std::cout << "Testing expression parser..." << std::endl;

    std::string sql = "SELECT sin(age * 3.14 / 180) FROM student WHERE name = 'Alice' AND age >= 18;";
    SQLParser parser(sql);

    std::unique_ptr<StmtAST> ast;
    Status status = parser.parse(ast);

    assert(status.ok());
    assert(ast != nullptr);

    SelectAST* select_ast = static_cast<SelectAST*>(ast.get());

    // Check function expression
    const auto& select_list = select_ast->get_select_list();
    assert(select_list.size() == 1);
    assert(select_list[0]->get_type() == ASTType::FUNCTION_CALL);

    // Check compound WHERE condition
    ExprAST* where_clause = select_ast->get_where_clause();
    assert(where_clause != nullptr);
    assert(where_clause->get_type() == ASTType::BINARY_OP);

    std::cout << "Expression parser test passed!" << std::endl;
}

int main() {
    try {
        test_tokenizer();
        test_create_table_parser();
        test_select_parser();
        test_insert_parser();
        test_delete_parser();
        test_expression_parser();

        std::cout << "All parser tests passed!" << std::endl;
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Parser test failed with exception: " << e.what() << std::endl;
        return 1;
    }
}

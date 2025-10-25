#include "sql/parser/parser.h"
#include "sql/parser/tokenizer.h"
#include "sql/ast/statements.h"
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
    Parser parser(sql);
    
    std::unique_ptr<Statement> stmt;
    Status status = parser.parse(stmt);
    
    assert(status.ok());
    assert(stmt != nullptr);
    assert(stmt->get_type() == ASTNodeType::CREATE_TABLE_STMT);
    
    CreateTableStatement* create_stmt = static_cast<CreateTableStatement*>(stmt.get());
    assert(create_stmt->get_table_name() == "STUDENT");
    assert(create_stmt->get_columns().size() == 3);
    
    std::cout << "CREATE TABLE parser test passed!" << std::endl;
}

void test_select_parser() {
    std::cout << "Testing SELECT parser..." << std::endl;
    
    std::string sql = "SELECT id, name FROM student WHERE age > 18;";
    Parser parser(sql);
    
    std::unique_ptr<Statement> stmt;
    Status status = parser.parse(stmt);
    
    assert(status.ok());
    assert(stmt != nullptr);
    assert(stmt->get_type() == ASTNodeType::SELECT_STMT);
    
    SelectStatement* select_stmt = static_cast<SelectStatement*>(stmt.get());
    assert(select_stmt->get_select_list().size() == 2);
    assert(select_stmt->get_from_table() != nullptr);
    assert(select_stmt->get_where_clause() != nullptr);
    
    std::cout << "SELECT parser test passed!" << std::endl;
}

void test_insert_parser() {
    std::cout << "Testing INSERT parser..." << std::endl;
    
    std::string sql = "INSERT INTO student VALUES (1, 'Alice', 20), (2, 'Bob', 21);";
    Parser parser(sql);
    
    std::unique_ptr<Statement> stmt;
    Status status = parser.parse(stmt);
    
    assert(status.ok());
    assert(stmt != nullptr);
    assert(stmt->get_type() == ASTNodeType::INSERT_STMT);
    
    InsertStatement* insert_stmt = static_cast<InsertStatement*>(stmt.get());
    assert(insert_stmt->get_table_name() == "STUDENT");
    assert(insert_stmt->get_values().size() == 2);
    assert(insert_stmt->get_values()[0].size() == 3);
    
    std::cout << "INSERT parser test passed!" << std::endl;
}

void test_delete_parser() {
    std::cout << "Testing DELETE parser..." << std::endl;
    
    std::string sql = "DELETE FROM student WHERE age < 18;";
    Parser parser(sql);
    
    std::unique_ptr<Statement> stmt;
    Status status = parser.parse(stmt);
    
    assert(status.ok());
    assert(stmt != nullptr);
    assert(stmt->get_type() == ASTNodeType::DELETE_STMT);
    
    DeleteStatement* delete_stmt = static_cast<DeleteStatement*>(stmt.get());
    assert(delete_stmt->get_from_table() != nullptr);
    assert(delete_stmt->get_where_clause() != nullptr);
    
    std::cout << "DELETE parser test passed!" << std::endl;
}

void test_expression_parser() {
    std::cout << "Testing expression parser..." << std::endl;
    
    std::string sql = "SELECT sin(age * 3.14 / 180) FROM student WHERE name = 'Alice' AND age >= 18;";
    Parser parser(sql);
    
    std::unique_ptr<Statement> stmt;
    Status status = parser.parse(stmt);
    
    assert(status.ok());
    assert(stmt != nullptr);
    
    SelectStatement* select_stmt = static_cast<SelectStatement*>(stmt.get());
    
    // 检查函数表达式
    const auto& select_list = select_stmt->get_select_list();
    assert(select_list.size() == 1);
    assert(select_list[0]->get_type() == ASTNodeType::FUNCTION_EXPR);
    
    // 检查复合WHERE条件
    Expression* where_clause = select_stmt->get_where_clause();
    assert(where_clause != nullptr);
    assert(where_clause->get_type() == ASTNodeType::BINARY_EXPR);
    
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

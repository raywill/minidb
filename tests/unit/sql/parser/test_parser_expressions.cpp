#include "sql/parser/new_parser.h"
#include "sql/ast/ast.h"
#include <iostream>
#include <cassert>

using namespace minidb;

void test_arithmetic_expressions() {
    std::cout << "Testing arithmetic expressions..." << std::endl;

    SQLParser parser("SELECT age + 5, score * 2, (age - 1) / 2 FROM users;");
    std::unique_ptr<StmtAST> ast;
    Status status = parser.parse(ast);
    assert(status.ok());

    SelectAST* select_ast = static_cast<SelectAST*>(ast.get());
    const auto& select_list = select_ast->get_select_list();
    assert(select_list.size() == 3);
    assert(select_list[0]->get_type() == ASTType::BINARY_OP);
    assert(select_list[1]->get_type() == ASTType::BINARY_OP);

    std::cout << "Arithmetic expressions test passed!" << std::endl;
}

void test_comparison_expressions() {
    std::cout << "Testing comparison expressions..." << std::endl;

    SQLParser parser("SELECT * FROM users WHERE age > 18 AND score >= 90;");
    std::unique_ptr<StmtAST> ast;
    Status status = parser.parse(ast);
    assert(status.ok());

    SelectAST* select_ast = static_cast<SelectAST*>(ast.get());
    ExprAST* where_clause = select_ast->get_where_clause();
    assert(where_clause != nullptr);
    assert(where_clause->get_type() == ASTType::BINARY_OP);

    std::cout << "Comparison expressions test passed!" << std::endl;
}

void test_logical_expressions() {
    std::cout << "Testing logical expressions..." << std::endl;

    SQLParser parser("SELECT * FROM users WHERE age > 18 AND (name = 'Alice' OR score >= 90);");
    std::unique_ptr<StmtAST> ast;
    Status status = parser.parse(ast);
    assert(status.ok());

    SelectAST* select_ast = static_cast<SelectAST*>(ast.get());
    ExprAST* where_clause = select_ast->get_where_clause();
    assert(where_clause != nullptr);
    assert(where_clause->get_type() == ASTType::BINARY_OP);

    std::cout << "Logical expressions test passed!" << std::endl;
}

void test_function_sin() {
    std::cout << "Testing SIN function..." << std::endl;

    SQLParser parser("SELECT sin(age) FROM users;");
    std::unique_ptr<StmtAST> ast;
    Status status = parser.parse(ast);
    assert(status.ok());

    SelectAST* select_ast = static_cast<SelectAST*>(ast.get());
    const auto& select_list = select_ast->get_select_list();
    assert(select_list[0]->get_type() == ASTType::FUNCTION_CALL);

    FunctionCallAST* func_ast = static_cast<FunctionCallAST*>(select_list[0].get());
    assert(func_ast->get_func_type() == FuncType::SIN);
    assert(func_ast->get_args().size() == 1);

    std::cout << "SIN function test passed!" << std::endl;
}

void test_function_cos() {
    std::cout << "Testing COS function..." << std::endl;

    SQLParser parser("SELECT cos(age) FROM users;");
    std::unique_ptr<StmtAST> ast;
    Status status = parser.parse(ast);
    assert(status.ok());

    SelectAST* select_ast = static_cast<SelectAST*>(ast.get());
    const auto& select_list = select_ast->get_select_list();
    FunctionCallAST* func_ast = static_cast<FunctionCallAST*>(select_list[0].get());
    assert(func_ast->get_func_type() == FuncType::COS);

    std::cout << "COS function test passed!" << std::endl;
}

void test_function_substr() {
    std::cout << "Testing SUBSTR function..." << std::endl;

    SQLParser parser("SELECT substr(name, 0, 5) FROM users;");
    std::unique_ptr<StmtAST> ast;
    Status status = parser.parse(ast);
    assert(status.ok());

    SelectAST* select_ast = static_cast<SelectAST*>(ast.get());
    const auto& select_list = select_ast->get_select_list();
    FunctionCallAST* func_ast = static_cast<FunctionCallAST*>(select_list[0].get());
    assert(func_ast->get_func_type() == FuncType::SUBSTR);
    assert(func_ast->get_args().size() == 3);

    std::cout << "SUBSTR function test passed!" << std::endl;
}

void test_nested_functions() {
    std::cout << "Testing nested functions..." << std::endl;

    SQLParser parser("SELECT sin(cos(age)) FROM users;");
    std::unique_ptr<StmtAST> ast;
    Status status = parser.parse(ast);
    assert(status.ok());

    SelectAST* select_ast = static_cast<SelectAST*>(ast.get());
    const auto& select_list = select_ast->get_select_list();
    FunctionCallAST* sin_ast = static_cast<FunctionCallAST*>(select_list[0].get());
    assert(sin_ast->get_func_type() == FuncType::SIN);

    const auto& sin_args = sin_ast->get_args();
    assert(sin_args.size() == 1);
    assert(sin_args[0]->get_type() == ASTType::FUNCTION_CALL);

    std::cout << "Nested functions test passed!" << std::endl;
}

void test_operator_precedence() {
    std::cout << "Testing operator precedence..." << std::endl;

    SQLParser parser("SELECT a + b * c FROM test;");
    std::unique_ptr<StmtAST> ast;
    Status status = parser.parse(ast);
    assert(status.ok());

    SelectAST* select_ast = static_cast<SelectAST*>(ast.get());
    const auto& select_list = select_ast->get_select_list();
    assert(select_list[0]->get_type() == ASTType::BINARY_OP);

    // The multiplication should be evaluated before addition
    BinaryOpAST* add_ast = static_cast<BinaryOpAST*>(select_list[0].get());
    assert(add_ast->get_op() == BinaryOp::ADD);

    std::cout << "Operator precedence test passed!" << std::endl;
}

int main() {
    try {
        test_arithmetic_expressions();
        test_comparison_expressions();
        test_logical_expressions();
        test_function_sin();
        test_function_cos();
        test_function_substr();
        test_nested_functions();
        test_operator_precedence();

        std::cout << "\nAll expression parser tests passed!" << std::endl;
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Expression parser test failed: " << e.what() << std::endl;
        return 1;
    }
}

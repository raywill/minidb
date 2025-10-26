#include "sql/parser/new_parser.h"
#include "sql/ast/ast.h"
#include <iostream>
#include <cassert>

using namespace minidb;

void test_simple_join() {
    std::cout << "Testing simple JOIN..." << std::endl;

    SQLParser parser("SELECT * FROM t1 JOIN t2 ON t1.id = t2.id;");
    std::unique_ptr<StmtAST> ast;
    Status status = parser.parse(ast);

    assert(status.ok());
    assert(ast != nullptr);

    SelectAST* select_ast = static_cast<SelectAST*>(ast.get());
    assert(select_ast->get_from_table()->get_table_name() == "T1");
    assert(select_ast->get_joins().size() == 1);

    const auto& join = select_ast->get_joins()[0];
    assert(join->get_join_type() == JoinType::INNER);
    assert(join->get_right_table()->get_table_name() == "T2");
    assert(join->get_condition() != nullptr);

    std::cout << "Simple JOIN test passed!" << std::endl;
    std::cout << "AST: " << select_ast->to_string() << std::endl;
}

void test_inner_join() {
    std::cout << "\nTesting INNER JOIN..." << std::endl;

    SQLParser parser("SELECT * FROM t1 INNER JOIN t2 ON t1.id = t2.user_id;");
    std::unique_ptr<StmtAST> ast;
    Status status = parser.parse(ast);

    assert(status.ok());
    assert(ast != nullptr);

    SelectAST* select_ast = static_cast<SelectAST*>(ast.get());
    assert(select_ast->get_joins().size() == 1);
    assert(select_ast->get_joins()[0]->get_join_type() == JoinType::INNER);

    std::cout << "INNER JOIN test passed!" << std::endl;
    std::cout << "AST: " << select_ast->to_string() << std::endl;
}

void test_left_join() {
    std::cout << "\nTesting LEFT JOIN..." << std::endl;

    SQLParser parser("SELECT * FROM t1 LEFT JOIN t2 ON t1.id = t2.id;");
    std::unique_ptr<StmtAST> ast;
    Status status = parser.parse(ast);

    assert(status.ok());
    assert(ast != nullptr);

    SelectAST* select_ast = static_cast<SelectAST*>(ast.get());
    assert(select_ast->get_joins().size() == 1);
    assert(select_ast->get_joins()[0]->get_join_type() == JoinType::LEFT_OUTER);

    std::cout << "LEFT JOIN test passed!" << std::endl;
    std::cout << "AST: " << select_ast->to_string() << std::endl;
}

void test_join_with_alias() {
    std::cout << "\nTesting JOIN with table aliases..." << std::endl;

    SQLParser parser("SELECT u.name FROM users u JOIN orders o ON u.id = o.user_id;");
    std::unique_ptr<StmtAST> ast;
    Status status = parser.parse(ast);

    assert(status.ok());
    assert(ast != nullptr);

    SelectAST* select_ast = static_cast<SelectAST*>(ast.get());

    // Check FROM table alias
    assert(select_ast->get_from_table()->get_table_name() == "USERS");
    assert(select_ast->get_from_table()->has_alias());
    assert(select_ast->get_from_table()->get_alias() == "U");

    // Check JOIN table alias
    assert(select_ast->get_joins().size() == 1);
    const auto& join = select_ast->get_joins()[0];
    assert(join->get_right_table()->get_table_name() == "ORDERS");
    assert(join->get_right_table()->has_alias());
    assert(join->get_right_table()->get_alias() == "O");

    std::cout << "JOIN with alias test passed!" << std::endl;
    std::cout << "AST: " << select_ast->to_string() << std::endl;
}

void test_join_with_as_keyword() {
    std::cout << "\nTesting JOIN with AS keyword..." << std::endl;

    SQLParser parser("SELECT u.name FROM users AS u JOIN orders AS o ON u.id = o.user_id;");
    std::unique_ptr<StmtAST> ast;
    Status status = parser.parse(ast);

    assert(status.ok());
    assert(ast != nullptr);

    SelectAST* select_ast = static_cast<SelectAST*>(ast.get());
    assert(select_ast->get_from_table()->get_alias() == "U");
    assert(select_ast->get_joins()[0]->get_right_table()->get_alias() == "O");

    std::cout << "JOIN with AS keyword test passed!" << std::endl;
}

void test_multiple_joins() {
    std::cout << "\nTesting multiple JOINs..." << std::endl;

    SQLParser parser("SELECT * FROM t1 JOIN t2 ON t1.id = t2.id JOIN t3 ON t2.id = t3.id;");
    std::unique_ptr<StmtAST> ast;
    Status status = parser.parse(ast);

    assert(status.ok());
    assert(ast != nullptr);

    SelectAST* select_ast = static_cast<SelectAST*>(ast.get());
    assert(select_ast->get_joins().size() == 2);
    assert(select_ast->get_joins()[0]->get_right_table()->get_table_name() == "T2");
    assert(select_ast->get_joins()[1]->get_right_table()->get_table_name() == "T3");

    std::cout << "Multiple JOINs test passed!" << std::endl;
    std::cout << "AST: " << select_ast->to_string() << std::endl;
}

void test_join_with_where() {
    std::cout << "\nTesting JOIN with WHERE clause..." << std::endl;

    SQLParser parser("SELECT * FROM t1 JOIN t2 ON t1.id = t2.id WHERE t1.age > 18;");
    std::unique_ptr<StmtAST> ast;
    Status status = parser.parse(ast);

    assert(status.ok());
    assert(ast != nullptr);

    SelectAST* select_ast = static_cast<SelectAST*>(ast.get());
    assert(select_ast->get_joins().size() == 1);
    assert(select_ast->get_where_clause() != nullptr);

    std::cout << "JOIN with WHERE test passed!" << std::endl;
    std::cout << "AST: " << select_ast->to_string() << std::endl;
}

void test_join_with_specific_columns() {
    std::cout << "\nTesting JOIN with specific columns..." << std::endl;

    SQLParser parser("SELECT t1.name, t2.score FROM t1 JOIN t2 ON t1.id = t2.user_id;");
    std::unique_ptr<StmtAST> ast;
    Status status = parser.parse(ast);

    assert(status.ok());
    assert(ast != nullptr);

    SelectAST* select_ast = static_cast<SelectAST*>(ast.get());
    assert(select_ast->get_select_list().size() == 2);
    assert(select_ast->get_joins().size() == 1);

    // Verify qualified column references
    ColumnRefAST* col1 = static_cast<ColumnRefAST*>(select_ast->get_select_list()[0].get());
    assert(col1->get_table_name() == "T1");
    assert(col1->get_column_name() == "NAME");

    ColumnRefAST* col2 = static_cast<ColumnRefAST*>(select_ast->get_select_list()[1].get());
    assert(col2->get_table_name() == "T2");
    assert(col2->get_column_name() == "SCORE");

    std::cout << "JOIN with specific columns test passed!" << std::endl;
    std::cout << "AST: " << select_ast->to_string() << std::endl;
}

void test_qualified_column_in_condition() {
    std::cout << "\nTesting qualified columns in JOIN condition..." << std::endl;

    SQLParser parser("SELECT * FROM users u JOIN orders o ON u.id = o.user_id AND u.status = 'active';");
    std::unique_ptr<StmtAST> ast;
    Status status = parser.parse(ast);

    assert(status.ok());
    assert(ast != nullptr);

    SelectAST* select_ast = static_cast<SelectAST*>(ast.get());
    assert(select_ast->get_joins().size() == 1);

    // Check that the condition is parsed correctly
    const auto& join = select_ast->get_joins()[0];
    assert(join->get_condition() != nullptr);
    assert(join->get_condition()->get_type() == ASTType::BINARY_OP);

    std::cout << "Qualified column in condition test passed!" << std::endl;
}

int main() {
    try {
        test_simple_join();
        test_inner_join();
        test_left_join();
        test_join_with_alias();
        test_join_with_as_keyword();
        test_multiple_joins();
        test_join_with_where();
        test_join_with_specific_columns();
        test_qualified_column_in_condition();

        std::cout << "\n========================================" << std::endl;
        std::cout << "All JOIN parser tests passed!" << std::endl;
        std::cout << "========================================" << std::endl;
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "JOIN parser test failed: " << e.what() << std::endl;
        return 1;
    }
}

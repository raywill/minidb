#include "sql/compiler/statement.h"
#include "common/utils.h"
#include <iostream>
#include <cassert>

using namespace minidb;

// 测试 LiteralExpression clone
void test_literal_expression_clone() {
    std::cout << "Testing LiteralExpression clone..." << std::endl;

    auto orig = make_unique<LiteralExpression>(DataType::INT, "42");
    auto cloned = orig->clone();

    auto* lit = dynamic_cast<LiteralExpression*>(cloned.get());
    assert(lit != nullptr);
    assert(lit->get_data_type() == DataType::INT);
    assert(lit->get_value() == "42");
    assert(lit != orig.get());  // 确保是新对象

    std::cout << "  LiteralExpression clone test passed!" << std::endl;
}

// 测试 ColumnRefExpression clone
void test_column_ref_expression_clone() {
    std::cout << "Testing ColumnRefExpression clone..." << std::endl;

    auto orig = make_unique<ColumnRefExpression>("users", "id", 0);
    auto cloned = orig->clone();

    auto* col_ref = dynamic_cast<ColumnRefExpression*>(cloned.get());
    assert(col_ref != nullptr);
    assert(col_ref->get_table_name() == "users");
    assert(col_ref->get_column_name() == "id");
    assert(col_ref->get_column_index() == 0);
    assert(col_ref != orig.get());  // 确保是新对象

    std::cout << "  ColumnRefExpression clone test passed!" << std::endl;
}

// 测试 BinaryExpression 递归 clone
void test_binary_expression_clone() {
    std::cout << "Testing BinaryExpression recursive clone..." << std::endl;

    auto left = make_unique<LiteralExpression>(DataType::INT, "3");
    auto right = make_unique<LiteralExpression>(DataType::INT, "5");
    auto orig = make_unique<BinaryExpression>(
        BinaryOperatorType::ADD, std::move(left), std::move(right));

    auto cloned = orig->clone();
    auto* bin = dynamic_cast<BinaryExpression*>(cloned.get());
    assert(bin != nullptr);
    assert(bin->get_operator() == BinaryOperatorType::ADD);
    assert(bin != orig.get());  // 确保是新对象

    // 验证子表达式也被正确克隆
    auto* cloned_left = dynamic_cast<LiteralExpression*>(bin->get_left());
    auto* cloned_right = dynamic_cast<LiteralExpression*>(bin->get_right());
    assert(cloned_left != nullptr);
    assert(cloned_right != nullptr);
    assert(cloned_left->get_value() == "3");
    assert(cloned_right->get_value() == "5");

    // 确保子表达式是新对象
    assert(cloned_left != orig->get_left());
    assert(cloned_right != orig->get_right());

    std::cout << "  BinaryExpression clone test passed!" << std::endl;
}

// 测试嵌套 BinaryExpression clone
void test_nested_binary_expression_clone() {
    std::cout << "Testing nested BinaryExpression clone..." << std::endl;

    // (3 + 5) * 2
    auto left_left = make_unique<LiteralExpression>(DataType::INT, "3");
    auto left_right = make_unique<LiteralExpression>(DataType::INT, "5");
    auto left = make_unique<BinaryExpression>(
        BinaryOperatorType::ADD, std::move(left_left), std::move(left_right));

    auto right = make_unique<LiteralExpression>(DataType::INT, "2");
    auto orig = make_unique<BinaryExpression>(
        BinaryOperatorType::MULTIPLY, std::move(left), std::move(right));

    auto cloned = orig->clone();
    auto* bin = dynamic_cast<BinaryExpression*>(cloned.get());
    assert(bin != nullptr);
    assert(bin->get_operator() == BinaryOperatorType::MULTIPLY);

    // 验证左侧嵌套表达式
    auto* cloned_left = dynamic_cast<BinaryExpression*>(bin->get_left());
    assert(cloned_left != nullptr);
    assert(cloned_left->get_operator() == BinaryOperatorType::ADD);

    auto* cloned_left_left = dynamic_cast<LiteralExpression*>(cloned_left->get_left());
    assert(cloned_left_left != nullptr);
    assert(cloned_left_left->get_value() == "3");

    std::cout << "  Nested BinaryExpression clone test passed!" << std::endl;
}

// 测试 FunctionExpression clone
void test_function_expression_clone() {
    std::cout << "Testing FunctionExpression clone..." << std::endl;

    std::vector<std::unique_ptr<Expression>> args;
    args.push_back(make_unique<LiteralExpression>(DataType::INT, "45"));

    auto orig = make_unique<FunctionExpression>(FunctionType::SIN, std::move(args));
    auto cloned = orig->clone();

    auto* func = dynamic_cast<FunctionExpression*>(cloned.get());
    assert(func != nullptr);
    assert(func->get_function_type() == FunctionType::SIN);
    assert(func->get_arguments().size() == 1);
    assert(func != orig.get());  // 确保是新对象

    // 验证参数也被正确克隆
    auto* cloned_arg = dynamic_cast<LiteralExpression*>(func->get_arguments()[0].get());
    assert(cloned_arg != nullptr);
    assert(cloned_arg->get_value() == "45");
    assert(cloned_arg != orig->get_arguments()[0].get());  // 确保参数是新对象

    std::cout << "  FunctionExpression clone test passed!" << std::endl;
}

// 测试 FunctionExpression 多参数 clone
void test_function_expression_multi_args_clone() {
    std::cout << "Testing FunctionExpression with multiple args clone..." << std::endl;

    std::vector<std::unique_ptr<Expression>> args;
    args.push_back(make_unique<LiteralExpression>(DataType::STRING, "hello"));
    args.push_back(make_unique<LiteralExpression>(DataType::INT, "1"));
    args.push_back(make_unique<LiteralExpression>(DataType::INT, "3"));

    auto orig = make_unique<FunctionExpression>(FunctionType::SUBSTR, std::move(args));
    auto cloned = orig->clone();

    auto* func = dynamic_cast<FunctionExpression*>(cloned.get());
    assert(func != nullptr);
    assert(func->get_function_type() == FunctionType::SUBSTR);
    assert(func->get_arguments().size() == 3);

    // 验证所有参数
    auto* arg0 = dynamic_cast<LiteralExpression*>(func->get_arguments()[0].get());
    auto* arg1 = dynamic_cast<LiteralExpression*>(func->get_arguments()[1].get());
    auto* arg2 = dynamic_cast<LiteralExpression*>(func->get_arguments()[2].get());

    assert(arg0 != nullptr && arg0->get_value() == "hello");
    assert(arg1 != nullptr && arg1->get_value() == "1");
    assert(arg2 != nullptr && arg2->get_value() == "3");

    std::cout << "  FunctionExpression multi-args clone test passed!" << std::endl;
}

// 测试不同数据类型的 LiteralExpression clone
void test_literal_expression_various_types() {
    std::cout << "Testing LiteralExpression clone with various data types..." << std::endl;

    // INT
    auto int_expr = make_unique<LiteralExpression>(DataType::INT, "123");
    auto cloned_int = int_expr->clone();
    auto* int_lit = dynamic_cast<LiteralExpression*>(cloned_int.get());
    assert(int_lit->get_data_type() == DataType::INT);
    assert(int_lit->get_value() == "123");

    // STRING
    auto str_expr = make_unique<LiteralExpression>(DataType::STRING, "test");
    auto cloned_str = str_expr->clone();
    auto* str_lit = dynamic_cast<LiteralExpression*>(cloned_str.get());
    assert(str_lit->get_data_type() == DataType::STRING);
    assert(str_lit->get_value() == "test");

    // DECIMAL
    auto dec_expr = make_unique<LiteralExpression>(DataType::DECIMAL, "3.14");
    auto cloned_dec = dec_expr->clone();
    auto* dec_lit = dynamic_cast<LiteralExpression*>(cloned_dec.get());
    assert(dec_lit->get_data_type() == DataType::DECIMAL);
    assert(dec_lit->get_value() == "3.14");

    // BOOL
    auto bool_expr = make_unique<LiteralExpression>(DataType::BOOL, "true");
    auto cloned_bool = bool_expr->clone();
    auto* bool_lit = dynamic_cast<LiteralExpression*>(cloned_bool.get());
    assert(bool_lit->get_data_type() == DataType::BOOL);
    assert(bool_lit->get_value() == "true");

    std::cout << "  LiteralExpression various types clone test passed!" << std::endl;
}

int main() {
    try {
        test_literal_expression_clone();
        test_column_ref_expression_clone();
        test_binary_expression_clone();
        test_nested_binary_expression_clone();
        test_function_expression_clone();
        test_function_expression_multi_args_clone();
        test_literal_expression_various_types();

        std::cout << "\nAll Expression clone tests passed!" << std::endl;
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Expression clone test failed: " << e.what() << std::endl;
        return 1;
    }
}

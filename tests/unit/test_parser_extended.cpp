#include "sql/parser/parser.h"
#include "sql/parser/tokenizer.h"
#include "sql/ast/statements.h"
#include "log/logger.h"
#include <iostream>
#include <cassert>
#include <vector>

using namespace minidb;

void test_tokenizer_comprehensive() {
    std::cout << "Testing Tokenizer comprehensive..." << std::endl;
    
    // 测试复杂SQL语句
    std::string complex_sql = "SELECT id, name, sin(age * 3.14 / 180) FROM student WHERE age >= 18 AND name != 'test' OR score <= 95.5;";
    
    Tokenizer tokenizer(complex_sql);
    auto tokens = tokenizer.tokenize_all();
    
    // 验证token数量和类型
    assert(tokens.size() > 20);
    
    // 验证关键token
    assert(tokens[0].type == TokenType::SELECT);
    assert(tokens[1].type == TokenType::IDENTIFIER);
    assert(tokens[1].value == "ID");
    assert(tokens[2].type == TokenType::COMMA);
    assert(tokens[3].type == TokenType::IDENTIFIER);
    assert(tokens[3].value == "NAME");
    
    // 查找函数token
    bool found_sin = false;
    bool found_multiply = false;
    bool found_divide = false;
    
    for (const auto& token : tokens) {
        if (token.type == TokenType::SIN) found_sin = true;
        if (token.type == TokenType::MULTIPLY) found_multiply = true;
        if (token.type == TokenType::DIVIDE) found_divide = true;
    }
    
    assert(found_sin);
    assert(found_multiply);
    assert(found_divide);
    
    std::cout << "Tokenizer comprehensive test passed!" << std::endl;
}

void test_tokenizer_edge_cases() {
    std::cout << "Testing Tokenizer edge cases..." << std::endl;
    
    // 测试空字符串
    Tokenizer empty_tokenizer("");
    auto empty_tokens = empty_tokenizer.tokenize_all();
    assert(empty_tokens.empty());
    
    // 测试只有空白字符
    Tokenizer whitespace_tokenizer("   \t\n  ");
    auto whitespace_tokens = whitespace_tokenizer.tokenize_all();
    assert(whitespace_tokens.empty());
    
    // 测试字符串字面量
    Tokenizer string_tokenizer("'hello world' \"quoted string\" 'with\\nnewline'");
    auto string_tokens = string_tokenizer.tokenize_all();
    assert(string_tokens.size() == 3);
    assert(string_tokens[0].type == TokenType::STRING);
    assert(string_tokens[0].value == "hello world");
    assert(string_tokens[1].type == TokenType::STRING);
    assert(string_tokens[1].value == "quoted string");
    assert(string_tokens[2].type == TokenType::STRING);
    assert(string_tokens[2].value == "with\nnewline");
    
    // 测试数字
    Tokenizer number_tokenizer("123 45.67 0 999.999");
    auto number_tokens = number_tokenizer.tokenize_all();
    assert(number_tokens.size() == 4);
    assert(number_tokens[0].type == TokenType::INTEGER);
    assert(number_tokens[0].value == "123");
    assert(number_tokens[1].type == TokenType::DECIMAL);
    assert(number_tokens[1].value == "45.67");
    assert(number_tokens[2].type == TokenType::INTEGER);
    assert(number_tokens[2].value == "0");
    assert(number_tokens[3].type == TokenType::DECIMAL);
    assert(number_tokens[3].value == "999.999");
    
    std::cout << "Tokenizer edge cases test passed!" << std::endl;
}

void test_parser_create_table_variations() {
    std::cout << "Testing Parser CREATE TABLE variations..." << std::endl;
    
    // 基本CREATE TABLE
    {
        Parser parser("CREATE TABLE users(id INT, name STRING);");
        std::unique_ptr<Statement> stmt;
        Status status = parser.parse(stmt);
        assert(status.ok());
        assert(stmt->get_type() == ASTNodeType::CREATE_TABLE_STMT);
        
        CreateTableStatement* create_stmt = static_cast<CreateTableStatement*>(stmt.get());
        assert(create_stmt->get_table_name() == "USERS");
        assert(create_stmt->get_columns().size() == 2);
        assert(!create_stmt->get_if_not_exists());
    }
    
    // CREATE TABLE IF NOT EXISTS
    {
        Parser parser("CREATE TABLE IF NOT EXISTS products(id INT, name STRING, price DECIMAL);");
        std::unique_ptr<Statement> stmt;
        Status status = parser.parse(stmt);
        assert(status.ok());
        
        CreateTableStatement* create_stmt = static_cast<CreateTableStatement*>(stmt.get());
        assert(create_stmt->get_table_name() == "PRODUCTS");
        assert(create_stmt->get_columns().size() == 3);
        assert(create_stmt->get_if_not_exists());
    }
    
    // 所有数据类型
    {
        Parser parser("CREATE TABLE all_types(int_col INT, str_col STRING, bool_col BOOL, dec_col DECIMAL);");
        std::unique_ptr<Statement> stmt;
        Status status = parser.parse(stmt);
        assert(status.ok());
        
        CreateTableStatement* create_stmt = static_cast<CreateTableStatement*>(stmt.get());
        const auto& columns = create_stmt->get_columns();
        assert(columns.size() == 4);
        assert(columns[0]->get_data_type() == DataType::INT);
        assert(columns[1]->get_data_type() == DataType::STRING);
        assert(columns[2]->get_data_type() == DataType::BOOL);
        assert(columns[3]->get_data_type() == DataType::DECIMAL);
    }
    
    std::cout << "Parser CREATE TABLE variations test passed!" << std::endl;
}

void test_parser_insert_variations() {
    std::cout << "Testing Parser INSERT variations..." << std::endl;
    
    // 单行INSERT
    {
        Parser parser("INSERT INTO users VALUES (1, 'Alice');");
        std::unique_ptr<Statement> stmt;
        Status status = parser.parse(stmt);
        assert(status.ok());
        
        InsertStatement* insert_stmt = static_cast<InsertStatement*>(stmt.get());
        assert(insert_stmt->get_table_name() == "USERS");
        assert(insert_stmt->get_values().size() == 1);
        assert(insert_stmt->get_values()[0].size() == 2);
    }
    
    // 多行INSERT
    {
        Parser parser("INSERT INTO users VALUES (1, 'Alice'), (2, 'Bob'), (3, 'Charlie');");
        std::unique_ptr<Statement> stmt;
        Status status = parser.parse(stmt);
        assert(status.ok());
        
        InsertStatement* insert_stmt = static_cast<InsertStatement*>(stmt.get());
        assert(insert_stmt->get_values().size() == 3);
    }
    
    // 不同数据类型的VALUES
    {
        Parser parser("INSERT INTO test VALUES (42, 'string', true, 3.14);");
        std::unique_ptr<Statement> stmt;
        Status status = parser.parse(stmt);
        assert(status.ok());
        
        InsertStatement* insert_stmt = static_cast<InsertStatement*>(stmt.get());
        const auto& values = insert_stmt->get_values()[0];
        assert(values.size() == 4);
        
        // 验证字面量类型
        assert(values[0]->get_type() == ASTNodeType::LITERAL_EXPR);
        assert(values[1]->get_type() == ASTNodeType::LITERAL_EXPR);
        assert(values[2]->get_type() == ASTNodeType::LITERAL_EXPR);
        assert(values[3]->get_type() == ASTNodeType::LITERAL_EXPR);
    }
    
    std::cout << "Parser INSERT variations test passed!" << std::endl;
}

void test_parser_select_variations() {
    std::cout << "Testing Parser SELECT variations..." << std::endl;
    
    // SELECT *
    {
        Parser parser("SELECT * FROM users;");
        std::unique_ptr<Statement> stmt;
        Status status = parser.parse(stmt);
        assert(status.ok());
        
        SelectStatement* select_stmt = static_cast<SelectStatement*>(stmt.get());
        assert(select_stmt->get_select_list().size() == 1);
        assert(select_stmt->get_from_table() != nullptr);
        assert(select_stmt->get_where_clause() == nullptr);
    }
    
    // SELECT specific columns
    {
        Parser parser("SELECT id, name, age FROM users;");
        std::unique_ptr<Statement> stmt;
        Status status = parser.parse(stmt);
        assert(status.ok());
        
        SelectStatement* select_stmt = static_cast<SelectStatement*>(stmt.get());
        assert(select_stmt->get_select_list().size() == 3);
    }
    
    // SELECT with WHERE
    {
        Parser parser("SELECT name FROM users WHERE age > 18;");
        std::unique_ptr<Statement> stmt;
        Status status = parser.parse(stmt);
        assert(status.ok());
        
        SelectStatement* select_stmt = static_cast<SelectStatement*>(stmt.get());
        assert(select_stmt->get_where_clause() != nullptr);
        assert(select_stmt->get_where_clause()->get_type() == ASTNodeType::BINARY_EXPR);
    }
    
    // SELECT with functions
    {
        Parser parser("SELECT sin(age), cos(score), substr(name, 0, 3) FROM users;");
        std::unique_ptr<Statement> stmt;
        Status status = parser.parse(stmt);
        assert(status.ok());
        
        SelectStatement* select_stmt = static_cast<SelectStatement*>(stmt.get());
        const auto& select_list = select_stmt->get_select_list();
        assert(select_list.size() == 3);
        assert(select_list[0]->get_type() == ASTNodeType::FUNCTION_EXPR);
        assert(select_list[1]->get_type() == ASTNodeType::FUNCTION_EXPR);
        assert(select_list[2]->get_type() == ASTNodeType::FUNCTION_EXPR);
    }
    
    std::cout << "Parser SELECT variations test passed!" << std::endl;
}

void test_parser_expression_parsing() {
    std::cout << "Testing Parser expression parsing..." << std::endl;
    
    // 测试算术表达式
    {
        Parser parser("SELECT age + 5, score * 2, (age - 1) / 2 FROM users;");
        std::unique_ptr<Statement> stmt;
        Status status = parser.parse(stmt);
        assert(status.ok());
        
        SelectStatement* select_stmt = static_cast<SelectStatement*>(stmt.get());
        const auto& select_list = select_stmt->get_select_list();
        assert(select_list.size() == 3);
        
        // 验证二元表达式
        assert(select_list[0]->get_type() == ASTNodeType::BINARY_EXPR);
        assert(select_list[1]->get_type() == ASTNodeType::BINARY_EXPR);
        assert(select_list[2]->get_type() == ASTNodeType::BINARY_EXPR);
    }
    
    // 测试复杂WHERE条件
    {
        Parser parser("SELECT * FROM users WHERE age > 18 AND (name = 'Alice' OR score >= 90);");
        std::unique_ptr<Statement> stmt;
        Status status = parser.parse(stmt);
        assert(status.ok());
        
        SelectStatement* select_stmt = static_cast<SelectStatement*>(stmt.get());
        Expression* where_clause = select_stmt->get_where_clause();
        assert(where_clause != nullptr);
        assert(where_clause->get_type() == ASTNodeType::BINARY_EXPR);
        
        BinaryExpression* and_expr = static_cast<BinaryExpression*>(where_clause);
        assert(and_expr->get_operator() == BinaryOperatorType::AND);
    }
    
    // 测试函数表达式
    {
        Parser parser("SELECT sin(age * 3.14 / 180) FROM users;");
        std::unique_ptr<Statement> stmt;
        Status status = parser.parse(stmt);
        assert(status.ok());
        
        SelectStatement* select_stmt = static_cast<SelectStatement*>(stmt.get());
        const auto& select_list = select_stmt->get_select_list();
        assert(select_list.size() == 1);
        assert(select_list[0]->get_type() == ASTNodeType::FUNCTION_EXPR);
        
        FunctionExpression* func_expr = static_cast<FunctionExpression*>(select_list[0].get());
        assert(func_expr->get_function_type() == FunctionType::SIN);
        assert(func_expr->get_arguments().size() == 1);
    }
    
    std::cout << "Parser expression parsing test passed!" << std::endl;
}

void test_parser_delete_variations() {
    std::cout << "Testing Parser DELETE variations..." << std::endl;
    
    // DELETE without WHERE
    {
        Parser parser("DELETE FROM users;");
        std::unique_ptr<Statement> stmt;
        Status status = parser.parse(stmt);
        assert(status.ok());
        
        DeleteStatement* delete_stmt = static_cast<DeleteStatement*>(stmt.get());
        assert(delete_stmt->get_from_table() != nullptr);
        assert(delete_stmt->get_where_clause() == nullptr);
    }
    
    // DELETE with WHERE
    {
        Parser parser("DELETE FROM users WHERE age < 18;");
        std::unique_ptr<Statement> stmt;
        Status status = parser.parse(stmt);
        assert(status.ok());
        
        DeleteStatement* delete_stmt = static_cast<DeleteStatement*>(stmt.get());
        assert(delete_stmt->get_where_clause() != nullptr);
    }
    
    // DELETE with complex WHERE
    {
        Parser parser("DELETE FROM users WHERE age < 18 AND active = false;");
        std::unique_ptr<Statement> stmt;
        Status status = parser.parse(stmt);
        assert(status.ok());
        
        DeleteStatement* delete_stmt = static_cast<DeleteStatement*>(stmt.get());
        Expression* where_clause = delete_stmt->get_where_clause();
        assert(where_clause->get_type() == ASTNodeType::BINARY_EXPR);
        
        BinaryExpression* and_expr = static_cast<BinaryExpression*>(where_clause);
        assert(and_expr->get_operator() == BinaryOperatorType::AND);
    }
    
    std::cout << "Parser DELETE variations test passed!" << std::endl;
}

void test_parser_drop_table_variations() {
    std::cout << "Testing Parser DROP TABLE variations..." << std::endl;
    
    // 基本DROP TABLE
    {
        Parser parser("DROP TABLE users;");
        std::unique_ptr<Statement> stmt;
        Status status = parser.parse(stmt);
        assert(status.ok());
        
        DropTableStatement* drop_stmt = static_cast<DropTableStatement*>(stmt.get());
        assert(drop_stmt->get_table_name() == "USERS");
        assert(!drop_stmt->get_if_exists());
    }
    
    // DROP TABLE IF EXISTS
    {
        Parser parser("DROP TABLE IF EXISTS temp_table;");
        std::unique_ptr<Statement> stmt;
        Status status = parser.parse(stmt);
        assert(status.ok());
        
        DropTableStatement* drop_stmt = static_cast<DropTableStatement*>(stmt.get());
        assert(drop_stmt->get_table_name() == "TEMP_TABLE");
        assert(drop_stmt->get_if_exists());
    }
    
    std::cout << "Parser DROP TABLE variations test passed!" << std::endl;
}

void test_parser_error_handling() {
    std::cout << "Testing Parser error handling..." << std::endl;
    
    // 语法错误测试
    std::vector<std::string> invalid_sqls = {
        "INVALID SQL SYNTAX",
        "SELECT FROM;", // 缺少列
        "CREATE TABLE;", // 缺少表名
        "INSERT INTO;", // 不完整
        "DELETE;", // 缺少FROM
        "SELECT * FROM", // 缺少表名
        "CREATE TABLE test(;", // 语法错误
        "INSERT INTO test VALUES;", // 缺少值
        "SELECT * FROM test WHERE;", // 缺少条件
    };
    
    for (const std::string& sql : invalid_sqls) {
        Parser parser(sql);
        std::unique_ptr<Statement> stmt;
        Status status = parser.parse(stmt);
        assert(!status.ok());
        assert(status.is_parse_error());
        std::cout << "Expected error for: " << sql << " -> " << status.ToString() << std::endl;
    }
    
    std::cout << "Parser error handling test passed!" << std::endl;
}

void test_parser_operator_precedence() {
    std::cout << "Testing Parser operator precedence..." << std::endl;
    
    // 测试算术操作符优先级
    {
        Parser parser("SELECT a + b * c FROM test;");
        std::unique_ptr<Statement> stmt;
        Status status = parser.parse(stmt);
        assert(status.ok());
        
        SelectStatement* select_stmt = static_cast<SelectStatement*>(stmt.get());
        const auto& select_list = select_stmt->get_select_list();
        assert(select_list[0]->get_type() == ASTNodeType::BINARY_EXPR);
        
        BinaryExpression* add_expr = static_cast<BinaryExpression*>(select_list[0].get());
        assert(add_expr->get_operator() == BinaryOperatorType::ADD);
        
        // 右操作数应该是乘法表达式
        assert(add_expr->get_right()->get_type() == ASTNodeType::BINARY_EXPR);
        BinaryExpression* mul_expr = static_cast<BinaryExpression*>(add_expr->get_right());
        assert(mul_expr->get_operator() == BinaryOperatorType::MULTIPLY);
    }
    
    // 测试比较和逻辑操作符优先级
    {
        Parser parser("SELECT * FROM test WHERE a > 5 AND b < 10 OR c = 0;");
        std::unique_ptr<Statement> stmt;
        Status status = parser.parse(stmt);
        assert(status.ok());
        
        SelectStatement* select_stmt = static_cast<SelectStatement*>(stmt.get());
        Expression* where_clause = select_stmt->get_where_clause();
        assert(where_clause->get_type() == ASTNodeType::BINARY_EXPR);
        
        BinaryExpression* or_expr = static_cast<BinaryExpression*>(where_clause);
        assert(or_expr->get_operator() == BinaryOperatorType::OR);
        
        // 左操作数应该是AND表达式
        assert(or_expr->get_left()->get_type() == ASTNodeType::BINARY_EXPR);
        BinaryExpression* and_expr = static_cast<BinaryExpression*>(or_expr->get_left());
        assert(and_expr->get_operator() == BinaryOperatorType::AND);
    }
    
    std::cout << "Parser operator precedence test passed!" << std::endl;
}

void test_parser_function_calls() {
    std::cout << "Testing Parser function calls..." << std::endl;
    
    // SIN函数
    {
        Parser parser("SELECT sin(age) FROM users;");
        std::unique_ptr<Statement> stmt;
        Status status = parser.parse(stmt);
        assert(status.ok());
        
        SelectStatement* select_stmt = static_cast<SelectStatement*>(stmt.get());
        const auto& select_list = select_stmt->get_select_list();
        assert(select_list[0]->get_type() == ASTNodeType::FUNCTION_EXPR);
        
        FunctionExpression* func_expr = static_cast<FunctionExpression*>(select_list[0].get());
        assert(func_expr->get_function_type() == FunctionType::SIN);
        assert(func_expr->get_arguments().size() == 1);
    }
    
    // SUBSTR函数
    {
        Parser parser("SELECT substr(name, 0, 5) FROM users;");
        std::unique_ptr<Statement> stmt;
        Status status = parser.parse(stmt);
        assert(status.ok());
        
        SelectStatement* select_stmt = static_cast<SelectStatement*>(stmt.get());
        const auto& select_list = select_stmt->get_select_list();
        FunctionExpression* func_expr = static_cast<FunctionExpression*>(select_list[0].get());
        assert(func_expr->get_function_type() == FunctionType::SUBSTR);
        assert(func_expr->get_arguments().size() == 3);
    }
    
    // 嵌套函数调用
    {
        Parser parser("SELECT sin(cos(age)) FROM users;");
        std::unique_ptr<Statement> stmt;
        Status status = parser.parse(stmt);
        assert(status.ok());
        
        SelectStatement* select_stmt = static_cast<SelectStatement*>(stmt.get());
        const auto& select_list = select_stmt->get_select_list();
        FunctionExpression* sin_expr = static_cast<FunctionExpression*>(select_list[0].get());
        assert(sin_expr->get_function_type() == FunctionType::SIN);
        
        const auto& sin_args = sin_expr->get_arguments();
        assert(sin_args.size() == 1);
        assert(sin_args[0]->get_type() == ASTNodeType::FUNCTION_EXPR);
        
        FunctionExpression* cos_expr = static_cast<FunctionExpression*>(sin_args[0].get());
        assert(cos_expr->get_function_type() == FunctionType::COS);
    }
    
    std::cout << "Parser function calls test passed!" << std::endl;
}

void test_parser_case_sensitivity() {
    std::cout << "Testing Parser case sensitivity..." << std::endl;
    
    // 测试关键字大小写不敏感
    std::vector<std::string> case_variations = {
        "create table test(id int);",
        "CREATE TABLE test(id INT);",
        "Create Table test(Id Int);",
        "CrEaTe TaBlE test(ID iNt);",
    };
    
    for (const std::string& sql : case_variations) {
        Parser parser(sql);
        std::unique_ptr<Statement> stmt;
        Status status = parser.parse(stmt);
        assert(status.ok());
        assert(stmt->get_type() == ASTNodeType::CREATE_TABLE_STMT);
        
        CreateTableStatement* create_stmt = static_cast<CreateTableStatement*>(stmt.get());
        assert(create_stmt->get_table_name() == "TEST"); // 应该转换为大写
    }
    
    std::cout << "Parser case sensitivity test passed!" << std::endl;
}

void test_ast_node_to_string() {
    std::cout << "Testing AST node to_string methods..." << std::endl;
    
    // 测试各种AST节点的字符串表示
    Parser parser("CREATE TABLE test(id INT, name STRING);");
    std::unique_ptr<Statement> stmt;
    Status status = parser.parse(stmt);
    assert(status.ok());
    
    std::string ast_string = stmt->to_string();
    assert(ast_string.find("CreateTable") != std::string::npos);
    assert(ast_string.find("TEST") != std::string::npos);
    assert(ast_string.find("ColumnDef") != std::string::npos);
    
    // 测试SELECT语句
    Parser select_parser("SELECT id, name FROM test WHERE age > 18;");
    std::unique_ptr<Statement> select_stmt;
    status = select_parser.parse(select_stmt);
    assert(status.ok());
    
    std::string select_string = select_stmt->to_string();
    assert(select_string.find("Select") != std::string::npos);
    assert(select_string.find("FROM") != std::string::npos);
    assert(select_string.find("WHERE") != std::string::npos);
    
    std::cout << "AST node to_string test passed!" << std::endl;
}

void test_parser_whitespace_handling() {
    std::cout << "Testing Parser whitespace handling..." << std::endl;
    
    // 测试各种空白字符组合
    std::vector<std::string> whitespace_variations = {
        "SELECT * FROM test;",
        "  SELECT   *   FROM   test  ;  ",
        "\tSELECT\t*\tFROM\ttest\t;\t",
        "\nSELECT\n*\nFROM\ntest\n;\n",
        "SELECT\r\n*\r\nFROM\r\ntest\r\n;\r\n",
    };
    
    for (const std::string& sql : whitespace_variations) {
        Parser parser(sql);
        std::unique_ptr<Statement> stmt;
        Status status = parser.parse(stmt);
        assert(status.ok());
        assert(stmt->get_type() == ASTNodeType::SELECT_STMT);
        
        SelectStatement* select_stmt = static_cast<SelectStatement*>(stmt.get());
        assert(select_stmt->get_from_table() != nullptr);
        assert(select_stmt->get_from_table()->get_table_name() == "TEST");
    }
    
    std::cout << "Parser whitespace handling test passed!" << std::endl;
}

int main() {
    try {
        // 设置日志级别
        Logger::instance()->set_level(LogLevel::ERROR);
        
        test_tokenizer_comprehensive();
        test_tokenizer_edge_cases();
        test_parser_create_table_variations();
        test_parser_insert_variations();
        test_parser_select_variations();
        test_parser_expression_parsing();
        test_parser_delete_variations();
        test_parser_drop_table_variations();
        test_parser_error_handling();
        test_parser_operator_precedence();
        test_parser_function_calls();
        test_parser_case_sensitivity();
        test_ast_node_to_string();
        test_parser_whitespace_handling();
        
        std::cout << "\n🎉 All extended parser tests passed!" << std::endl;
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "❌ Extended parser test failed: " << e.what() << std::endl;
        return 1;
    }
}

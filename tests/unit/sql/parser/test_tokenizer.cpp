#include "sql/parser/tokenizer.h"
#include <iostream>
#include <cassert>
#include <vector>

using namespace minidb;

void test_tokenizer_comprehensive() {
    std::cout << "Testing Tokenizer comprehensive..." << std::endl;

    std::string complex_sql = "SELECT id, name, sin(age * 3.14 / 180) FROM student WHERE age >= 18 AND name != 'test' OR score <= 95.5;";

    Tokenizer tokenizer(complex_sql);
    auto tokens = tokenizer.tokenize_all();

    assert(tokens.size() > 20);
    assert(tokens[0].type == TokenType::SELECT);
    assert(tokens[1].type == TokenType::IDENTIFIER);
    assert(tokens[1].value == "ID");

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

    // Empty string
    Tokenizer empty_tokenizer("");
    auto empty_tokens = empty_tokenizer.tokenize_all();
    assert(empty_tokens.empty());

    // Only whitespace
    Tokenizer whitespace_tokenizer("   \t\n  ");
    auto whitespace_tokens = whitespace_tokenizer.tokenize_all();
    assert(whitespace_tokens.empty());

    // String literals
    Tokenizer string_tokenizer("'hello world' \"quoted string\" 'with\\nnewline'");
    auto string_tokens = string_tokenizer.tokenize_all();
    assert(string_tokens.size() == 3);
    assert(string_tokens[0].type == TokenType::STRING);
    assert(string_tokens[0].value == "hello world");

    // Numbers
    Tokenizer number_tokenizer("123 45.67 0 999.999");
    auto number_tokens = number_tokenizer.tokenize_all();
    assert(number_tokens.size() == 4);
    assert(number_tokens[0].type == TokenType::INTEGER);
    assert(number_tokens[1].type == TokenType::DECIMAL);

    std::cout << "Tokenizer edge cases test passed!" << std::endl;
}

void test_tokenizer_whitespace_handling() {
    std::cout << "Testing Tokenizer whitespace handling..." << std::endl;

    std::vector<std::string> whitespace_variations = {
        "SELECT * FROM test;",
        "  SELECT   *   FROM   test  ;  ",
        "\tSELECT\t*\tFROM\ttest\t;\t",
        "\nSELECT\n*\nFROM\ntest\n;\n",
    };

    for (const std::string& sql : whitespace_variations) {
        Tokenizer tokenizer(sql);
        auto tokens = tokenizer.tokenize_all();
        assert(tokens.size() >= 4);
        assert(tokens[0].type == TokenType::SELECT);
    }

    std::cout << "Tokenizer whitespace handling test passed!" << std::endl;
}

void test_tokenizer_join_keywords() {
    std::cout << "Testing Tokenizer JOIN keywords..." << std::endl;

    // Test simple INNER JOIN
    {
        std::string sql = "SELECT * FROM t1 JOIN t2 ON t1.id = t2.id;";
        Tokenizer tokenizer(sql);
        auto tokens = tokenizer.tokenize_all();

        bool found_join = false;
        bool found_on = false;

        for (const auto& token : tokens) {
            if (token.type == TokenType::JOIN) {
                found_join = true;
                assert(token.value == "JOIN");
            }
            if (token.type == TokenType::ON) {
                found_on = true;
                assert(token.value == "ON");
            }
        }

        assert(found_join && "JOIN keyword not found");
        assert(found_on && "ON keyword not found");
    }

    // Test INNER JOIN
    {
        std::string sql = "SELECT * FROM t1 INNER JOIN t2 ON t1.id = t2.id;";
        Tokenizer tokenizer(sql);
        auto tokens = tokenizer.tokenize_all();

        bool found_inner = false;
        bool found_join = false;

        for (const auto& token : tokens) {
            if (token.type == TokenType::INNER) {
                found_inner = true;
                assert(token.value == "INNER");
            }
            if (token.type == TokenType::JOIN) found_join = true;
        }

        assert(found_inner && "INNER keyword not found");
        assert(found_join && "JOIN keyword not found");
    }

    // Test LEFT/RIGHT/FULL OUTER JOIN keywords
    {
        std::string sql = "LEFT RIGHT FULL OUTER";
        Tokenizer tokenizer(sql);
        auto tokens = tokenizer.tokenize_all();

        assert(tokens.size() == 4);
        assert(tokens[0].type == TokenType::LEFT);
        assert(tokens[0].value == "LEFT");
        assert(tokens[1].type == TokenType::RIGHT);
        assert(tokens[1].value == "RIGHT");
        assert(tokens[2].type == TokenType::FULL);
        assert(tokens[2].value == "FULL");
        assert(tokens[3].type == TokenType::OUTER);
        assert(tokens[3].value == "OUTER");
    }

    // Test AS keyword for table aliases
    {
        std::string sql = "SELECT u.name FROM users AS u;";
        Tokenizer tokenizer(sql);
        auto tokens = tokenizer.tokenize_all();

        bool found_as = false;

        for (const auto& token : tokens) {
            if (token.type == TokenType::AS) {
                found_as = true;
                assert(token.value == "AS");
            }
        }

        assert(found_as && "AS keyword not found");
    }

    std::cout << "Tokenizer JOIN keywords test passed!" << std::endl;
}

int main() {
    try {
        test_tokenizer_comprehensive();
        test_tokenizer_edge_cases();
        test_tokenizer_whitespace_handling();
        test_tokenizer_join_keywords();

        std::cout << "\nAll tokenizer tests passed!" << std::endl;
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Tokenizer test failed: " << e.what() << std::endl;
        return 1;
    }
}

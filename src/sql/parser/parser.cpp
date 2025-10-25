#include "sql/parser/parser.h"
#include "common/utils.h"
#include <sstream>

namespace minidb {

Parser::Parser(const std::string& sql) 
    : tokenizer_(sql), current_token_(TokenType::UNKNOWN, "", 0, 0) {
    advance(); // 读取第一个token
}

Status Parser::parse(std::unique_ptr<Statement>& stmt) {
    try {
        stmt = parse_statement();
        if (!stmt) {
            return Status::ParseError(error_message_);
        }
        return Status::OK();
    } catch (const std::exception& e) {
        return Status::ParseError(e.what());
    }
}

void Parser::advance() {
    current_token_ = tokenizer_.next_token();
}

bool Parser::match(TokenType type) {
    return current_token_.type == type;
}

bool Parser::consume(TokenType type, const std::string& error_msg) {
    if (match(type)) {
        advance();
        return true;
    }
    
    if (!error_msg.empty()) {
        set_error(error_msg);
    }
    return false;
}

void Parser::set_error(const std::string& message) {
    std::ostringstream oss;
    oss << "Parse error at line " << current_token_.line 
        << ", column " << current_token_.column << ": " << message;
    error_message_ = oss.str();
}

std::unique_ptr<Statement> Parser::parse_statement() {
    switch (current_token_.type) {
        case TokenType::CREATE:
            return parse_create_table();
        case TokenType::DROP:
            return parse_drop_table();
        case TokenType::INSERT:
            return parse_insert();
        case TokenType::SELECT:
            return parse_select();
        case TokenType::DELETE:
            return parse_delete();
        default:
            set_error("Expected CREATE, DROP, INSERT, SELECT, or DELETE");
            return nullptr;
    }
}

std::unique_ptr<CreateTableStatement> Parser::parse_create_table() {
    if (!consume(TokenType::CREATE, "Expected CREATE")) {
        return nullptr;
    }
    
    if (!consume(TokenType::TABLE, "Expected TABLE")) {
        return nullptr;
    }
    
    bool if_not_exists = false;
    if (match(TokenType::IF)) {
        advance();
        if (!consume(TokenType::NOT, "Expected NOT after IF")) {
            return nullptr;
        }
        if (!consume(TokenType::EXISTS, "Expected EXISTS after NOT")) {
            return nullptr;
        }
        if_not_exists = true;
    }
    
    if (!match(TokenType::IDENTIFIER)) {
        set_error("Expected table name");
        return nullptr;
    }
    std::string table_name = current_token_.value;
    advance();
    
    if (!consume(TokenType::LEFT_PAREN, "Expected '(' after table name")) {
        return nullptr;
    }
    
    std::vector<std::unique_ptr<ColumnDefinition>> columns;
    
    // 解析列定义
    do {
        auto column = parse_column_definition();
        if (!column) {
            return nullptr;
        }
        columns.push_back(std::move(column));
        
        if (match(TokenType::COMMA)) {
            advance();
        } else {
            break;
        }
    } while (true);
    
    if (!consume(TokenType::RIGHT_PAREN, "Expected ')' after column definitions")) {
        return nullptr;
    }
    
    return make_unique<CreateTableStatement>(table_name, std::move(columns), if_not_exists);
}

std::unique_ptr<DropTableStatement> Parser::parse_drop_table() {
    if (!consume(TokenType::DROP, "Expected DROP")) {
        return nullptr;
    }
    
    if (!consume(TokenType::TABLE, "Expected TABLE")) {
        return nullptr;
    }
    
    bool if_exists = false;
    if (match(TokenType::IF)) {
        advance();
        if (!consume(TokenType::EXISTS, "Expected EXISTS after IF")) {
            return nullptr;
        }
        if_exists = true;
    }
    
    if (!match(TokenType::IDENTIFIER)) {
        set_error("Expected table name");
        return nullptr;
    }
    std::string table_name = current_token_.value;
    advance();
    
    return make_unique<DropTableStatement>(table_name, if_exists);
}

std::unique_ptr<InsertStatement> Parser::parse_insert() {
    if (!consume(TokenType::INSERT, "Expected INSERT")) {
        return nullptr;
    }
    
    if (!consume(TokenType::INTO, "Expected INTO")) {
        return nullptr;
    }
    
    if (!match(TokenType::IDENTIFIER)) {
        set_error("Expected table name");
        return nullptr;
    }
    std::string table_name = current_token_.value;
    advance();
    
    std::vector<std::string> columns;
    
    // 可选的列名列表
    if (match(TokenType::LEFT_PAREN)) {
        advance();
        columns = parse_identifier_list();
        if (!consume(TokenType::RIGHT_PAREN, "Expected ')' after column list")) {
            return nullptr;
        }
    }
    
    if (!consume(TokenType::VALUES, "Expected VALUES")) {
        return nullptr;
    }
    
    std::vector<std::vector<std::unique_ptr<Expression>>> values;
    
    // 解析值列表
    do {
        if (!consume(TokenType::LEFT_PAREN, "Expected '(' before value list")) {
            return nullptr;
        }
        
        auto value_list = parse_expression_list();
        values.push_back(std::move(value_list));
        
        if (!consume(TokenType::RIGHT_PAREN, "Expected ')' after value list")) {
            return nullptr;
        }
        
        if (match(TokenType::COMMA)) {
            advance();
        } else {
            break;
        }
    } while (true);
    
    return make_unique<InsertStatement>(table_name, std::move(columns), std::move(values));
}

std::unique_ptr<SelectStatement> Parser::parse_select() {
    if (!consume(TokenType::SELECT, "Expected SELECT")) {
        return nullptr;
    }
    
    // 解析选择列表
    std::vector<std::unique_ptr<Expression>> select_list;
    
    // 特殊处理 SELECT *
    if (match(TokenType::MULTIPLY)) {
        advance();
        // 创建一个特殊的列引用表达式表示 *
        select_list.push_back(make_unique<ColumnRefExpression>("*"));
    } else {
        select_list = parse_expression_list();
    }
    
    std::unique_ptr<TableReference> from_table;
    if (match(TokenType::FROM)) {
        advance();
        from_table = parse_table_reference();
        if (!from_table) {
            return nullptr;
        }
    }
    
    std::unique_ptr<Expression> where_clause;
    if (match(TokenType::WHERE)) {
        advance();
        where_clause = parse_expression();
        if (!where_clause) {
            return nullptr;
        }
    }
    
    return make_unique<SelectStatement>(std::move(select_list), std::move(from_table), std::move(where_clause));
}

std::unique_ptr<DeleteStatement> Parser::parse_delete() {
    if (!consume(TokenType::DELETE, "Expected DELETE")) {
        return nullptr;
    }
    
    if (!consume(TokenType::FROM, "Expected FROM")) {
        return nullptr;
    }
    
    auto from_table = parse_table_reference();
    if (!from_table) {
        return nullptr;
    }
    
    std::unique_ptr<Expression> where_clause;
    if (match(TokenType::WHERE)) {
        advance();
        where_clause = parse_expression();
        if (!where_clause) {
            return nullptr;
        }
    }
    
    return make_unique<DeleteStatement>(std::move(from_table), std::move(where_clause));
}

std::unique_ptr<Expression> Parser::parse_expression() {
    return parse_or_expression();
}

std::unique_ptr<Expression> Parser::parse_or_expression() {
    auto left = parse_and_expression();
    if (!left) return nullptr;
    
    while (match(TokenType::OR)) {
        BinaryOperatorType op = token_to_binary_operator(current_token_.type);
        advance();
        auto right = parse_and_expression();
        if (!right) return nullptr;
        left = make_unique<BinaryExpression>(op, std::move(left), std::move(right));
    }
    
    return left;
}

std::unique_ptr<Expression> Parser::parse_and_expression() {
    auto left = parse_equality_expression();
    if (!left) return nullptr;
    
    while (match(TokenType::AND)) {
        BinaryOperatorType op = token_to_binary_operator(current_token_.type);
        advance();
        auto right = parse_equality_expression();
        if (!right) return nullptr;
        left = make_unique<BinaryExpression>(op, std::move(left), std::move(right));
    }
    
    return left;
}

std::unique_ptr<Expression> Parser::parse_equality_expression() {
    auto left = parse_relational_expression();
    if (!left) return nullptr;
    
    while (match(TokenType::EQUAL) || match(TokenType::NOT_EQUAL)) {
        BinaryOperatorType op = token_to_binary_operator(current_token_.type);
        advance();
        auto right = parse_relational_expression();
        if (!right) return nullptr;
        left = make_unique<BinaryExpression>(op, std::move(left), std::move(right));
    }
    
    return left;
}

std::unique_ptr<Expression> Parser::parse_relational_expression() {
    auto left = parse_additive_expression();
    if (!left) return nullptr;
    
    while (match(TokenType::LESS_THAN) || match(TokenType::LESS_EQUAL) ||
           match(TokenType::GREATER_THAN) || match(TokenType::GREATER_EQUAL)) {
        BinaryOperatorType op = token_to_binary_operator(current_token_.type);
        advance();
        auto right = parse_additive_expression();
        if (!right) return nullptr;
        left = make_unique<BinaryExpression>(op, std::move(left), std::move(right));
    }
    
    return left;
}

std::unique_ptr<Expression> Parser::parse_additive_expression() {
    auto left = parse_multiplicative_expression();
    if (!left) return nullptr;
    
    while (match(TokenType::PLUS) || match(TokenType::MINUS)) {
        BinaryOperatorType op = token_to_binary_operator(current_token_.type);
        advance();
        auto right = parse_multiplicative_expression();
        if (!right) return nullptr;
        left = make_unique<BinaryExpression>(op, std::move(left), std::move(right));
    }
    
    return left;
}

std::unique_ptr<Expression> Parser::parse_multiplicative_expression() {
    auto left = parse_unary_expression();
    if (!left) return nullptr;
    
    while (match(TokenType::MULTIPLY) || match(TokenType::DIVIDE)) {
        BinaryOperatorType op = token_to_binary_operator(current_token_.type);
        advance();
        auto right = parse_unary_expression();
        if (!right) return nullptr;
        left = make_unique<BinaryExpression>(op, std::move(left), std::move(right));
    }
    
    return left;
}

std::unique_ptr<Expression> Parser::parse_unary_expression() {
    return parse_primary_expression();
}

std::unique_ptr<Expression> Parser::parse_primary_expression() {
    switch (current_token_.type) {
        case TokenType::INTEGER: {
            std::string value = current_token_.value;
            advance();
            return make_unique<LiteralExpression>(DataType::INT, value);
        }
        case TokenType::DECIMAL: {
            std::string value = current_token_.value;
            advance();
            return make_unique<LiteralExpression>(DataType::DECIMAL, value);
        }
        case TokenType::STRING: {
            std::string value = current_token_.value;
            advance();
            return make_unique<LiteralExpression>(DataType::STRING, value);
        }
        case TokenType::BOOLEAN: {
            std::string value = current_token_.value;
            advance();
            return make_unique<LiteralExpression>(DataType::BOOL, value);
        }
        case TokenType::IDENTIFIER: {
            std::string name = current_token_.value;
            advance();
            return make_unique<ColumnRefExpression>(name);
        }
        case TokenType::SIN:
        case TokenType::COS:
        case TokenType::SUBSTR:
            return parse_function_call();
        case TokenType::LEFT_PAREN: {
            advance();
            auto expr = parse_expression();
            if (!consume(TokenType::RIGHT_PAREN, "Expected ')' after expression")) {
                return nullptr;
            }
            return expr;
        }
        default:
            set_error("Expected expression");
            return nullptr;
    }
}

std::unique_ptr<Expression> Parser::parse_function_call() {
    FunctionType func_type = token_to_function_type(current_token_.type);
    advance();
    
    if (!consume(TokenType::LEFT_PAREN, "Expected '(' after function name")) {
        return nullptr;
    }
    
    std::vector<std::unique_ptr<Expression>> args;
    if (!match(TokenType::RIGHT_PAREN)) {
        args = parse_expression_list();
    }
    
    if (!consume(TokenType::RIGHT_PAREN, "Expected ')' after function arguments")) {
        return nullptr;
    }
    
    return make_unique<FunctionExpression>(func_type, std::move(args));
}

std::unique_ptr<ColumnDefinition> Parser::parse_column_definition() {
    if (!match(TokenType::IDENTIFIER)) {
        set_error("Expected column name");
        return nullptr;
    }
    
    std::string column_name = current_token_.value;
    advance();
    
    DataType data_type = parse_data_type();
    
    return make_unique<ColumnDefinition>(column_name, data_type);
}

std::unique_ptr<TableReference> Parser::parse_table_reference() {
    if (!match(TokenType::IDENTIFIER)) {
        set_error("Expected table name");
        return nullptr;
    }
    
    std::string table_name = current_token_.value;
    advance();
    
    return make_unique<TableReference>(table_name);
}

std::vector<std::unique_ptr<Expression>> Parser::parse_expression_list() {
    std::vector<std::unique_ptr<Expression>> expressions;
    
    do {
        auto expr = parse_expression();
        if (!expr) {
            break;
        }
        expressions.push_back(std::move(expr));
        
        if (match(TokenType::COMMA)) {
            advance();
        } else {
            break;
        }
    } while (true);
    
    return expressions;
}

std::vector<std::string> Parser::parse_identifier_list() {
    std::vector<std::string> identifiers;
    
    do {
        if (!match(TokenType::IDENTIFIER)) {
            set_error("Expected identifier");
            break;
        }
        
        identifiers.push_back(current_token_.value);
        advance();
        
        if (match(TokenType::COMMA)) {
            advance();
        } else {
            break;
        }
    } while (true);
    
    return identifiers;
}

DataType Parser::parse_data_type() {
    switch (current_token_.type) {
        case TokenType::INT:
            advance();
            return DataType::INT;
        case TokenType::STRING_TYPE:
            advance();
            return DataType::STRING;
        case TokenType::BOOL:
            advance();
            return DataType::BOOL;
        case TokenType::DECIMAL_TYPE:
            advance();
            return DataType::DECIMAL;
        default:
            set_error("Expected data type");
            return DataType::INT; // 默认值
    }
}

BinaryOperatorType Parser::token_to_binary_operator(TokenType type) {
    switch (type) {
        case TokenType::PLUS: return BinaryOperatorType::ADD;
        case TokenType::MINUS: return BinaryOperatorType::SUBTRACT;
        case TokenType::MULTIPLY: return BinaryOperatorType::MULTIPLY;
        case TokenType::DIVIDE: return BinaryOperatorType::DIVIDE;
        case TokenType::EQUAL: return BinaryOperatorType::EQUAL;
        case TokenType::NOT_EQUAL: return BinaryOperatorType::NOT_EQUAL;
        case TokenType::LESS_THAN: return BinaryOperatorType::LESS_THAN;
        case TokenType::LESS_EQUAL: return BinaryOperatorType::LESS_EQUAL;
        case TokenType::GREATER_THAN: return BinaryOperatorType::GREATER_THAN;
        case TokenType::GREATER_EQUAL: return BinaryOperatorType::GREATER_EQUAL;
        case TokenType::AND: return BinaryOperatorType::AND;
        case TokenType::OR: return BinaryOperatorType::OR;
        default: return BinaryOperatorType::EQUAL; // 默认值
    }
}

FunctionType Parser::token_to_function_type(TokenType type) {
    switch (type) {
        case TokenType::SIN: return FunctionType::SIN;
        case TokenType::COS: return FunctionType::COS;
        case TokenType::SUBSTR: return FunctionType::SUBSTR;
        default: return FunctionType::SIN; // 默认值
    }
}

} // namespace minidb

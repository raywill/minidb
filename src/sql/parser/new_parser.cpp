#include "sql/parser/new_parser.h"
#include "common/utils.h"
#include <sstream>

namespace minidb {

SQLParser::SQLParser(const std::string& sql)
    : tokenizer_(sql), current_token_(TokenType::UNKNOWN, "", 0, 0) {
    advance();
}

Status SQLParser::parse(std::unique_ptr<StmtAST>& ast) {
    try {
        ast = parse_statement();
        if (!ast) {
            return Status::ParseError(error_message_);
        }
        return Status::OK();
    } catch (const std::exception& e) {
        return Status::ParseError(e.what());
    }
}

void SQLParser::advance() {
    current_token_ = tokenizer_.next_token();
}

bool SQLParser::match(TokenType type) {
    return current_token_.type == type;
}

bool SQLParser::consume(TokenType type, const std::string& error_msg) {
    if (match(type)) {
        advance();
        return true;
    }
    if (!error_msg.empty()) {
        set_error(error_msg);
    }
    return false;
}

void SQLParser::set_error(const std::string& message) {
    std::ostringstream oss;
    oss << "Parse error at line " << current_token_.line
        << ", column " << current_token_.column << ": " << message;
    error_message_ = oss.str();
}

std::unique_ptr<StmtAST> SQLParser::parse_statement() {
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

std::unique_ptr<CreateTableAST> SQLParser::parse_create_table() {
    if (!consume(TokenType::CREATE)) return nullptr;
    if (!consume(TokenType::TABLE)) return nullptr;

    bool if_not_exists = false;
    if (match(TokenType::IF)) {
        advance();
        if (!consume(TokenType::NOT)) return nullptr;
        if (!consume(TokenType::EXISTS)) return nullptr;
        if_not_exists = true;
    }

    if (!match(TokenType::IDENTIFIER)) {
        set_error("Expected table name");
        return nullptr;
    }
    std::string table_name = current_token_.value;
    advance();

    if (!consume(TokenType::LEFT_PAREN)) return nullptr;

    std::vector<std::unique_ptr<ColumnDefAST>> columns;
    do {
        auto column = parse_column_definition();
        if (!column) return nullptr;
        columns.push_back(std::move(column));

        if (match(TokenType::COMMA)) {
            advance();
        } else {
            break;
        }
    } while (true);

    if (!consume(TokenType::RIGHT_PAREN)) return nullptr;

    return make_unique<CreateTableAST>(table_name, std::move(columns), if_not_exists);
}

std::unique_ptr<DropTableAST> SQLParser::parse_drop_table() {
    if (!consume(TokenType::DROP)) return nullptr;
    if (!consume(TokenType::TABLE)) return nullptr;

    bool if_exists = false;
    if (match(TokenType::IF)) {
        advance();
        if (!consume(TokenType::EXISTS)) return nullptr;
        if_exists = true;
    }

    if (!match(TokenType::IDENTIFIER)) {
        set_error("Expected table name");
        return nullptr;
    }
    std::string table_name = current_token_.value;
    advance();

    return make_unique<DropTableAST>(table_name, if_exists);
}

std::unique_ptr<InsertAST> SQLParser::parse_insert() {
    if (!consume(TokenType::INSERT)) return nullptr;
    if (!consume(TokenType::INTO)) return nullptr;

    if (!match(TokenType::IDENTIFIER)) {
        set_error("Expected table name");
        return nullptr;
    }
    std::string table_name = current_token_.value;
    advance();

    std::vector<std::string> columns;
    if (match(TokenType::LEFT_PAREN)) {
        advance();
        columns = parse_identifier_list();
        if (!consume(TokenType::RIGHT_PAREN)) return nullptr;
    }

    if (!consume(TokenType::VALUES)) return nullptr;

    std::vector<std::vector<std::unique_ptr<ExprAST>>> values;
    do {
        if (!consume(TokenType::LEFT_PAREN)) return nullptr;

        auto value_list = parse_expression_list();
        values.push_back(std::move(value_list));

        if (!consume(TokenType::RIGHT_PAREN)) return nullptr;

        if (match(TokenType::COMMA)) {
            advance();
        } else {
            break;
        }
    } while (true);

    return make_unique<InsertAST>(table_name, std::move(columns), std::move(values));
}

std::unique_ptr<SelectAST> SQLParser::parse_select() {
    if (!consume(TokenType::SELECT)) return nullptr;

    std::vector<std::unique_ptr<ExprAST>> select_list;

    if (match(TokenType::MULTIPLY)) {
        advance();
        select_list.push_back(make_unique<ColumnRefAST>("*"));
    } else {
        select_list = parse_expression_list();
    }

    std::unique_ptr<TableRefAST> from_table;
    if (match(TokenType::FROM)) {
        advance();
        from_table = parse_table_reference();
        if (!from_table) return nullptr;
    }

    // Parse JOIN clauses
    std::vector<std::unique_ptr<JoinClauseAST>> join_clauses;
    while (match(TokenType::JOIN) || match(TokenType::INNER) ||
           match(TokenType::LEFT) || match(TokenType::RIGHT) || match(TokenType::FULL)) {
        auto join_clause = parse_join_clause();
        if (!join_clause) return nullptr;
        join_clauses.push_back(std::move(join_clause));
    }

    std::unique_ptr<ExprAST> where_clause;
    if (match(TokenType::WHERE)) {
        advance();
        where_clause = parse_expression();
        if (!where_clause) return nullptr;
    }

    // If there are JOIN clauses, use the constructor that accepts them
    if (!join_clauses.empty()) {
        return make_unique<SelectAST>(std::move(select_list), std::move(from_table),
                                      std::move(join_clauses), std::move(where_clause));
    }

    return make_unique<SelectAST>(std::move(select_list), std::move(from_table), std::move(where_clause));
}

std::unique_ptr<DeleteAST> SQLParser::parse_delete() {
    if (!consume(TokenType::DELETE)) return nullptr;
    if (!consume(TokenType::FROM)) return nullptr;

    auto from_table = parse_table_reference();
    if (!from_table) return nullptr;

    std::unique_ptr<ExprAST> where_clause;
    if (match(TokenType::WHERE)) {
        advance();
        where_clause = parse_expression();
        if (!where_clause) return nullptr;
    }

    return make_unique<DeleteAST>(std::move(from_table), std::move(where_clause));
}

// ============= 表达式解析 =============
std::unique_ptr<ExprAST> SQLParser::parse_expression() {
    return parse_or_expression();
}

std::unique_ptr<ExprAST> SQLParser::parse_or_expression() {
    auto left = parse_and_expression();
    if (!left) return nullptr;

    while (match(TokenType::OR)) {
        BinaryOp op = token_to_binary_op(current_token_.type);
        advance();
        auto right = parse_and_expression();
        if (!right) return nullptr;
        left = make_unique<BinaryOpAST>(op, std::move(left), std::move(right));
    }

    return left;
}

std::unique_ptr<ExprAST> SQLParser::parse_and_expression() {
    auto left = parse_equality_expression();
    if (!left) return nullptr;

    while (match(TokenType::AND)) {
        BinaryOp op = token_to_binary_op(current_token_.type);
        advance();
        auto right = parse_equality_expression();
        if (!right) return nullptr;
        left = make_unique<BinaryOpAST>(op, std::move(left), std::move(right));
    }

    return left;
}

std::unique_ptr<ExprAST> SQLParser::parse_equality_expression() {
    auto left = parse_relational_expression();
    if (!left) return nullptr;

    while (match(TokenType::EQUAL) || match(TokenType::NOT_EQUAL)) {
        BinaryOp op = token_to_binary_op(current_token_.type);
        advance();
        auto right = parse_relational_expression();
        if (!right) return nullptr;
        left = make_unique<BinaryOpAST>(op, std::move(left), std::move(right));
    }

    return left;
}

std::unique_ptr<ExprAST> SQLParser::parse_relational_expression() {
    auto left = parse_additive_expression();
    if (!left) return nullptr;

    while (match(TokenType::LESS_THAN) || match(TokenType::LESS_EQUAL) ||
           match(TokenType::GREATER_THAN) || match(TokenType::GREATER_EQUAL)) {
        BinaryOp op = token_to_binary_op(current_token_.type);
        advance();
        auto right = parse_additive_expression();
        if (!right) return nullptr;
        left = make_unique<BinaryOpAST>(op, std::move(left), std::move(right));
    }

    return left;
}

std::unique_ptr<ExprAST> SQLParser::parse_additive_expression() {
    auto left = parse_multiplicative_expression();
    if (!left) return nullptr;

    while (match(TokenType::PLUS) || match(TokenType::MINUS)) {
        BinaryOp op = token_to_binary_op(current_token_.type);
        advance();
        auto right = parse_multiplicative_expression();
        if (!right) return nullptr;
        left = make_unique<BinaryOpAST>(op, std::move(left), std::move(right));
    }

    return left;
}

std::unique_ptr<ExprAST> SQLParser::parse_multiplicative_expression() {
    auto left = parse_unary_expression();
    if (!left) return nullptr;

    while (match(TokenType::MULTIPLY) || match(TokenType::DIVIDE)) {
        BinaryOp op = token_to_binary_op(current_token_.type);
        advance();
        auto right = parse_unary_expression();
        if (!right) return nullptr;
        left = make_unique<BinaryOpAST>(op, std::move(left), std::move(right));
    }

    return left;
}

std::unique_ptr<ExprAST> SQLParser::parse_unary_expression() {
    return parse_primary_expression();
}

std::unique_ptr<ExprAST> SQLParser::parse_primary_expression() {
    switch (current_token_.type) {
        case TokenType::INTEGER: {
            std::string value = current_token_.value;
            advance();
            return make_unique<LiteralAST>(DataType::INT, value);
        }
        case TokenType::DECIMAL: {
            std::string value = current_token_.value;
            advance();
            return make_unique<LiteralAST>(DataType::DECIMAL, value);
        }
        case TokenType::STRING: {
            std::string value = current_token_.value;
            advance();
            return make_unique<LiteralAST>(DataType::STRING, value);
        }
        case TokenType::BOOLEAN: {
            std::string value = current_token_.value;
            advance();
            return make_unique<LiteralAST>(DataType::BOOL, value);
        }
        case TokenType::IDENTIFIER: {
            std::string first_name = current_token_.value;
            advance();

            // Check for table.column reference
            if (match(TokenType::DOT)) {
                advance();
                if (!match(TokenType::IDENTIFIER)) {
                    set_error("Expected column name after '.'");
                    return nullptr;
                }
                std::string column_name = current_token_.value;
                advance();
                return make_unique<ColumnRefAST>(first_name, column_name);
            }

            // Simple column reference
            return make_unique<ColumnRefAST>(first_name);
        }
        case TokenType::SIN:
        case TokenType::COS:
        case TokenType::SUBSTR:
            return parse_function_call();
        case TokenType::LEFT_PAREN: {
            advance();
            auto expr = parse_expression();
            if (!consume(TokenType::RIGHT_PAREN)) return nullptr;
            return expr;
        }
        default:
            set_error("Expected expression");
            return nullptr;
    }
}

std::unique_ptr<ExprAST> SQLParser::parse_function_call() {
    FuncType func_type = token_to_func_type(current_token_.type);
    advance();

    if (!consume(TokenType::LEFT_PAREN)) return nullptr;

    std::vector<std::unique_ptr<ExprAST>> args;
    if (!match(TokenType::RIGHT_PAREN)) {
        args = parse_expression_list();
    }

    if (!consume(TokenType::RIGHT_PAREN)) return nullptr;

    return make_unique<FunctionCallAST>(func_type, std::move(args));
}

// ============= 其他解析方法 =============
std::unique_ptr<ColumnDefAST> SQLParser::parse_column_definition() {
    if (!match(TokenType::IDENTIFIER)) {
        set_error("Expected column name");
        return nullptr;
    }

    std::string column_name = current_token_.value;
    advance();

    DataType data_type = parse_data_type();

    return make_unique<ColumnDefAST>(column_name, data_type);
}

std::unique_ptr<TableRefAST> SQLParser::parse_table_reference() {
    if (!match(TokenType::IDENTIFIER)) {
        set_error("Expected table name");
        return nullptr;
    }

    std::string table_name = current_token_.value;
    advance();

    // Parse optional table alias
    std::string alias;
    if (match(TokenType::AS)) {
        advance();
        if (!match(TokenType::IDENTIFIER)) {
            set_error("Expected alias name after AS");
            return nullptr;
        }
        alias = current_token_.value;
        advance();
    } else if (match(TokenType::IDENTIFIER)) {
        // Also support implicit alias without AS keyword (e.g., "FROM users u")
        alias = current_token_.value;
        advance();
    }

    return make_unique<TableRefAST>(table_name, alias);
}

JoinType SQLParser::parse_join_type() {
    // Default to INNER JOIN
    JoinType join_type = JoinType::INNER;

    if (match(TokenType::INNER)) {
        advance();
        join_type = JoinType::INNER;
    } else if (match(TokenType::LEFT)) {
        advance();
        if (match(TokenType::OUTER)) {
            advance();
        }
        join_type = JoinType::LEFT_OUTER;
    } else if (match(TokenType::RIGHT)) {
        advance();
        if (match(TokenType::OUTER)) {
            advance();
        }
        join_type = JoinType::RIGHT_OUTER;
    } else if (match(TokenType::FULL)) {
        advance();
        if (match(TokenType::OUTER)) {
            advance();
        }
        join_type = JoinType::FULL_OUTER;
    }

    return join_type;
}

std::unique_ptr<JoinClauseAST> SQLParser::parse_join_clause() {
    // Parse JOIN type (INNER, LEFT, RIGHT, FULL)
    JoinType join_type = parse_join_type();

    // Consume JOIN keyword
    if (!consume(TokenType::JOIN, "Expected JOIN keyword")) {
        return nullptr;
    }

    // Parse right table
    auto right_table = parse_table_reference();
    if (!right_table) {
        return nullptr;
    }

    // Parse ON condition
    if (!consume(TokenType::ON, "Expected ON keyword after JOIN table")) {
        return nullptr;
    }

    auto condition = parse_expression();
    if (!condition) {
        return nullptr;
    }

    return make_unique<JoinClauseAST>(join_type, std::move(right_table), std::move(condition));
}

std::vector<std::unique_ptr<ExprAST>> SQLParser::parse_expression_list() {
    std::vector<std::unique_ptr<ExprAST>> expressions;

    do {
        auto expr = parse_expression();
        if (!expr) break;
        expressions.push_back(std::move(expr));

        if (match(TokenType::COMMA)) {
            advance();
        } else {
            break;
        }
    } while (true);

    return expressions;
}

std::vector<std::string> SQLParser::parse_identifier_list() {
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

DataType SQLParser::parse_data_type() {
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
            return DataType::INT;
    }
}

BinaryOp SQLParser::token_to_binary_op(TokenType type) {
    switch (type) {
        case TokenType::PLUS: return BinaryOp::ADD;
        case TokenType::MINUS: return BinaryOp::SUBTRACT;
        case TokenType::MULTIPLY: return BinaryOp::MULTIPLY;
        case TokenType::DIVIDE: return BinaryOp::DIVIDE;
        case TokenType::EQUAL: return BinaryOp::EQUAL;
        case TokenType::NOT_EQUAL: return BinaryOp::NOT_EQUAL;
        case TokenType::LESS_THAN: return BinaryOp::LESS_THAN;
        case TokenType::LESS_EQUAL: return BinaryOp::LESS_EQUAL;
        case TokenType::GREATER_THAN: return BinaryOp::GREATER_THAN;
        case TokenType::GREATER_EQUAL: return BinaryOp::GREATER_EQUAL;
        case TokenType::AND: return BinaryOp::AND;
        case TokenType::OR: return BinaryOp::OR;
        default: return BinaryOp::EQUAL;
    }
}

FuncType SQLParser::token_to_func_type(TokenType type) {
    switch (type) {
        case TokenType::SIN: return FuncType::SIN;
        case TokenType::COS: return FuncType::COS;
        case TokenType::SUBSTR: return FuncType::SUBSTR;
        default: return FuncType::SIN;
    }
}

} // namespace minidb

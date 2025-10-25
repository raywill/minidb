#include "sql/optimizer/optimizer.h"
#include "sql/compiler/compiler.h"
#include "sql/parser/new_parser.h"
#include "storage/catalog.h"
#include <iostream>
#include <cassert>

using namespace minidb;

void cleanup_test_data() {
    system("rm -rf ./test_optimizer_data");
}

Catalog* setup_catalog_with_table() {
    cleanup_test_data();
    Catalog* catalog = new Catalog("./test_optimizer_data");
    assert(catalog->initialize().ok());

    TableSchema schema("users");
    schema.add_column("id", DataType::INT);
    schema.add_column("name", DataType::STRING);
    schema.add_column("age", DataType::INT);

    assert(catalog->create_table("USERS", schema).ok());

    return catalog;
}

void test_optimizer_passthrough() {
    std::cout << "Testing optimizer pass-through (no optimization)..." << std::endl;

    Catalog* catalog = setup_catalog_with_table();

    // Parse and compile a SELECT statement
    SQLParser parser("SELECT * FROM users WHERE age > 18;");
    std::unique_ptr<StmtAST> ast;
    assert(parser.parse(ast).ok());

    Compiler compiler(catalog);
    std::unique_ptr<Statement> stmt;
    assert(compiler.compile(ast.get(), stmt).ok());

    // Optimize (currently pass-through)
    Optimizer optimizer;
    std::unique_ptr<Statement> optimized_stmt;
    Status status = optimizer.optimize(stmt.get(), optimized_stmt);
    assert(status.ok());

    // Currently, optimizer returns nullptr (no optimization applied)
    // The original statement should still be usable
    assert(stmt != nullptr);

    delete catalog;
    cleanup_test_data();
    std::cout << "Optimizer pass-through test passed!" << std::endl;
}

void test_optimizer_preserves_semantics() {
    std::cout << "Testing optimizer preserves query semantics..." << std::endl;

    cleanup_test_data();
    Catalog catalog("./test_optimizer_data");
    assert(catalog.initialize().ok());

    // Create table
    TableSchema schema("users");
    schema.add_column("id", DataType::INT);
    schema.add_column("name", DataType::STRING);
    schema.add_column("age", DataType::INT);
    assert(catalog.create_table("USERS", schema).ok());

    SQLParser parser("SELECT name FROM users WHERE age > 18;");
    std::unique_ptr<StmtAST> ast;
    assert(parser.parse(ast).ok());

    Compiler compiler(&catalog);
    std::unique_ptr<Statement> stmt;
    assert(compiler.compile(ast.get(), stmt).ok());

    // Optimize
    Optimizer optimizer;
    std::unique_ptr<Statement> optimized_stmt;
    Status status = optimizer.optimize(stmt.get(), optimized_stmt);
    assert(status.ok());

    // Since optimizer is pass-through, original statement should still be valid
    assert(stmt != nullptr);
    assert(stmt->get_type() == StatementType::SELECT);

    cleanup_test_data();
    std::cout << "Optimizer preserves semantics test passed!" << std::endl;
}

void test_optimizer_with_ddl() {
    std::cout << "Testing optimizer with DDL statements..." << std::endl;

    cleanup_test_data();
    Catalog catalog("./test_optimizer_data");
    assert(catalog.initialize().ok());

    // DDL statements don't need optimization
    SQLParser parser("CREATE TABLE test(id INT);");
    std::unique_ptr<StmtAST> ast;
    assert(parser.parse(ast).ok());

    Compiler compiler(&catalog);
    std::unique_ptr<Statement> stmt;
    assert(compiler.compile(ast.get(), stmt).ok());

    Optimizer optimizer;
    std::unique_ptr<Statement> optimized_stmt;
    Status status = optimizer.optimize(stmt.get(), optimized_stmt);
    assert(status.ok());

    // Original statement should remain valid
    assert(stmt->get_type() == StatementType::CREATE_TABLE);

    cleanup_test_data();
    std::cout << "Optimizer with DDL test passed!" << std::endl;
}

void test_optimizer_with_insert() {
    std::cout << "Testing optimizer with INSERT..." << std::endl;

    cleanup_test_data();
    Catalog catalog("./test_optimizer_data");
    assert(catalog.initialize().ok());

    TableSchema schema("users");
    schema.add_column("id", DataType::INT);
    schema.add_column("name", DataType::STRING);
    schema.add_column("age", DataType::INT);
    assert(catalog.create_table("USERS", schema).ok());

    SQLParser parser("INSERT INTO users VALUES (1, 'Alice', 20);");
    std::unique_ptr<StmtAST> ast;
    assert(parser.parse(ast).ok());

    Compiler compiler(&catalog);
    std::unique_ptr<Statement> stmt;
    assert(compiler.compile(ast.get(), stmt).ok());

    Optimizer optimizer;
    std::unique_ptr<Statement> optimized_stmt;
    assert(optimizer.optimize(stmt.get(), optimized_stmt).ok());

    // INSERT statements typically don't need optimization
    assert(stmt->get_type() == StatementType::INSERT);

    cleanup_test_data();
    std::cout << "Optimizer with INSERT test passed!" << std::endl;
}

void test_optimizer_with_delete() {
    std::cout << "Testing optimizer with DELETE..." << std::endl;

    cleanup_test_data();
    Catalog catalog("./test_optimizer_data");
    assert(catalog.initialize().ok());

    TableSchema schema("users");
    schema.add_column("id", DataType::INT);
    schema.add_column("name", DataType::STRING);
    schema.add_column("age", DataType::INT);
    assert(catalog.create_table("USERS", schema).ok());

    SQLParser parser("DELETE FROM users WHERE age < 18;");
    std::unique_ptr<StmtAST> ast;
    assert(parser.parse(ast).ok());

    Compiler compiler(&catalog);
    std::unique_ptr<Statement> stmt;
    assert(compiler.compile(ast.get(), stmt).ok());

    Optimizer optimizer;
    std::unique_ptr<Statement> optimized_stmt;
    assert(optimizer.optimize(stmt.get(), optimized_stmt).ok());

    assert(stmt->get_type() == StatementType::DELETE);

    cleanup_test_data();
    std::cout << "Optimizer with DELETE test passed!" << std::endl;
}

void test_optimizer_with_complex_query() {
    std::cout << "Testing optimizer with complex query..." << std::endl;

    cleanup_test_data();
    Catalog catalog("./test_optimizer_data");
    assert(catalog.initialize().ok());

    TableSchema schema("users");
    schema.add_column("id", DataType::INT);
    schema.add_column("name", DataType::STRING);
    schema.add_column("age", DataType::INT);
    assert(catalog.create_table("USERS", schema).ok());

    // Complex query with multiple conditions
    SQLParser parser("SELECT name, age FROM users WHERE age > 18 AND name != 'test';");
    std::unique_ptr<StmtAST> ast;
    assert(parser.parse(ast).ok());

    Compiler compiler(&catalog);
    std::unique_ptr<Statement> stmt;
    assert(compiler.compile(ast.get(), stmt).ok());

    Optimizer optimizer;
    std::unique_ptr<Statement> optimized_stmt;
    Status status = optimizer.optimize(stmt.get(), optimized_stmt);
    assert(status.ok());

    // Future: optimizer could apply optimizations like:
    // - Predicate pushdown
    // - Constant folding
    // - Column pruning
    // - Index selection
    // For now, it's pass-through

    cleanup_test_data();
    std::cout << "Optimizer with complex query test passed!" << std::endl;
}

// Future test cases to add when optimizer is implemented:
// - test_constant_folding()
// - test_predicate_pushdown()
// - test_column_pruning()
// - test_join_reordering()
// - test_index_selection()

int main() {
    try {
        test_optimizer_passthrough();
        test_optimizer_preserves_semantics();
        test_optimizer_with_ddl();
        test_optimizer_with_insert();
        test_optimizer_with_delete();
        test_optimizer_with_complex_query();

        std::cout << "\nAll Optimizer tests passed!" << std::endl;
        std::cout << "\nNote: Optimizer is currently pass-through." << std::endl;
        std::cout << "Future enhancements:" << std::endl;
        std::cout << "  - Constant folding" << std::endl;
        std::cout << "  - Predicate pushdown" << std::endl;
        std::cout << "  - Column pruning" << std::endl;
        std::cout << "  - Index selection" << std::endl;
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Optimizer test failed: " << e.what() << std::endl;
        cleanup_test_data();
        return 1;
    }
}

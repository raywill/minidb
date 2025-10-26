# Simple Makefile for MiniDB
CXX = g++
CXXFLAGS = -std=c++11 -Wall -Wextra -O2 -g
INCLUDES = -Iinclude -Isrc

# Source directories
SRC_DIRS = src/common src/mem src/log src/sql/parser src/sql/ast src/sql/compiler src/sql/optimizer src/storage src/exec src/exec/operators src/exec/executor src/exec/plan src/net src/server src/client
SOURCES = $(foreach dir,$(SRC_DIRS),$(wildcard $(dir)/*.cpp))
# Exclude main.cpp and old files from core objects
CORE_SOURCES = $(filter-out src/server/main.cpp src/client/main.cpp src/exec/executor/executor.cpp src/sql/parser/parser.cpp src/sql/ast/ast_node.cpp src/sql/ast/statements.cpp, $(SOURCES))
OBJECTS = $(CORE_SOURCES:.cpp=.o)

# Test files - organized by module
COMMON_TEST_SOURCES = tests/unit/common/test_types.cpp tests/unit/common/test_crash_handler.cpp tests/unit/common/test_crash_handler_extended.cpp
MEMORY_TEST_SOURCES = tests/unit/memory/test_allocator.cpp tests/unit/memory/test_arena.cpp
LOG_TEST_SOURCES = tests/unit/log/test_logger.cpp
PARSER_TEST_SOURCES = tests/unit/sql/parser/test_parser.cpp tests/unit/sql/parser/test_tokenizer.cpp \
                      tests/unit/sql/parser/test_parser_ddl.cpp tests/unit/sql/parser/test_parser_dml.cpp \
                      tests/unit/sql/parser/test_parser_expressions.cpp tests/unit/sql/parser/test_parser_join.cpp
COMPILER_TEST_SOURCES = tests/unit/sql/compiler/test_compiler_ddl.cpp tests/unit/sql/compiler/test_compiler_dml.cpp \
                        tests/unit/sql/compiler/test_compiler_semantic.cpp tests/unit/sql/compiler/test_expression_clone.cpp \
                        tests/unit/sql/compiler/test_compiler_join.cpp
OPTIMIZER_TEST_SOURCES = tests/unit/sql/optimizer/test_optimizer.cpp
STORAGE_TEST_SOURCES = tests/unit/storage/test_storage.cpp tests/unit/storage/test_storage_simple.cpp
EXECUTOR_TEST_SOURCES = tests/unit/exec/executor/test_executor.cpp tests/unit/exec/executor/test_expression_eval.cpp
OPERATOR_TEST_SOURCES = tests/unit/exec/operators/test_operators.cpp
PLANNER_TEST_SOURCES = tests/unit/exec/plan/test_planner_ddl.cpp tests/unit/exec/plan/test_planner_dml.cpp
NETWORK_TEST_SOURCES = tests/unit/net/test_network.cpp tests/unit/net/test_network_simple.cpp
CLIENT_TEST_SOURCES = tests/unit/client/test_command_history.cpp

UNIT_TEST_SOURCES = $(COMMON_TEST_SOURCES) $(MEMORY_TEST_SOURCES) $(LOG_TEST_SOURCES) $(PARSER_TEST_SOURCES) \
                    $(COMPILER_TEST_SOURCES) $(OPTIMIZER_TEST_SOURCES) $(STORAGE_TEST_SOURCES) \
                    $(EXECUTOR_TEST_SOURCES) $(OPERATOR_TEST_SOURCES) $(PLANNER_TEST_SOURCES) \
                    $(NETWORK_TEST_SOURCES) $(CLIENT_TEST_SOURCES)
INTEGRATION_TEST_SOURCES = tests/integration/test_full_system.cpp tests/integration/test_e2e_basic.cpp tests/integration/test_join_e2e.cpp
TEST_SOURCES = $(UNIT_TEST_SOURCES) $(INTEGRATION_TEST_SOURCES)
TEST_OBJECTS = $(TEST_SOURCES:.cpp=.o)

# Test target names
COMMON_TEST_TARGETS = test_types test_crash_handler test_crash_handler_extended
MEMORY_TEST_TARGETS = test_allocator test_arena
LOG_TEST_TARGETS = test_logger
PARSER_TEST_TARGETS = test_parser test_tokenizer test_parser_ddl test_parser_dml test_parser_expressions test_parser_join
COMPILER_TEST_TARGETS = test_compiler_ddl test_compiler_dml test_compiler_semantic test_expression_clone test_compiler_join
OPTIMIZER_TEST_TARGETS = test_optimizer
STORAGE_TEST_TARGETS = test_storage test_storage_simple
EXECUTOR_TEST_TARGETS = test_executor test_expression_eval
OPERATOR_TEST_TARGETS = test_operators
PLANNER_TEST_TARGETS = test_planner_ddl test_planner_dml
NETWORK_TEST_TARGETS = test_network test_network_simple
CLIENT_TEST_TARGETS = test_command_history

UNIT_TEST_TARGETS = $(COMMON_TEST_TARGETS) $(MEMORY_TEST_TARGETS) $(LOG_TEST_TARGETS) $(PARSER_TEST_TARGETS) \
                    $(COMPILER_TEST_TARGETS) $(OPTIMIZER_TEST_TARGETS) $(STORAGE_TEST_TARGETS) \
                    $(EXECUTOR_TEST_TARGETS) $(OPERATOR_TEST_TARGETS) $(PLANNER_TEST_TARGETS) \
                    $(NETWORK_TEST_TARGETS) $(CLIENT_TEST_TARGETS)
INTEGRATION_TEST_TARGETS = test_full_system test_e2e_basic test_join_e2e
ALL_TEST_TARGETS = $(UNIT_TEST_TARGETS) $(INTEGRATION_TEST_TARGETS)

# Default target
all: dbserver dbcli $(UNIT_TEST_TARGETS)

# Main targets
dbserver: $(OBJECTS) src/server/main.o
	@mkdir -p bin
	$(CXX) $(CXXFLAGS) -o bin/$@ $^ -lpthread

dbcli: $(OBJECTS) src/client/main.o
	@mkdir -p bin
	$(CXX) $(CXXFLAGS) -o bin/$@ $^ -lpthread

# Common module tests
test_types: $(OBJECTS) tests/unit/common/test_types.o
	@mkdir -p tests/bin
	$(CXX) $(CXXFLAGS) -o tests/bin/$@ $^ -lpthread

test_crash_handler: $(OBJECTS) tests/unit/common/test_crash_handler.o
	@mkdir -p tests/bin
	$(CXX) $(CXXFLAGS) -o tests/bin/$@ $^ -lpthread

test_crash_handler_extended: $(OBJECTS) tests/unit/common/test_crash_handler_extended.o
	@mkdir -p tests/bin
	$(CXX) $(CXXFLAGS) -o tests/bin/$@ $^ -lpthread

# Memory module tests
test_allocator: $(OBJECTS) tests/unit/memory/test_allocator.o
	@mkdir -p tests/bin
	$(CXX) $(CXXFLAGS) -o tests/bin/$@ $^ -lpthread

test_arena: $(OBJECTS) tests/unit/memory/test_arena.o
	@mkdir -p tests/bin
	$(CXX) $(CXXFLAGS) -o tests/bin/$@ $^ -lpthread

# Log module tests
test_logger: $(OBJECTS) tests/unit/log/test_logger.o
	@mkdir -p tests/bin
	$(CXX) $(CXXFLAGS) -o tests/bin/$@ $^ -lpthread

# Parser module tests
test_parser: $(OBJECTS) tests/unit/sql/parser/test_parser.o
	@mkdir -p tests/bin
	$(CXX) $(CXXFLAGS) -o tests/bin/$@ $^ -lpthread

test_tokenizer: $(OBJECTS) tests/unit/sql/parser/test_tokenizer.o
	@mkdir -p tests/bin
	$(CXX) $(CXXFLAGS) -o tests/bin/$@ $^ -lpthread

test_parser_ddl: $(OBJECTS) tests/unit/sql/parser/test_parser_ddl.o
	@mkdir -p tests/bin
	$(CXX) $(CXXFLAGS) -o tests/bin/$@ $^ -lpthread

test_parser_dml: $(OBJECTS) tests/unit/sql/parser/test_parser_dml.o
	@mkdir -p tests/bin
	$(CXX) $(CXXFLAGS) -o tests/bin/$@ $^ -lpthread

test_parser_expressions: $(OBJECTS) tests/unit/sql/parser/test_parser_expressions.o
	@mkdir -p tests/bin
	$(CXX) $(CXXFLAGS) -o tests/bin/$@ $^ -lpthread

test_parser_join: $(OBJECTS) tests/unit/sql/parser/test_parser_join.o
	@mkdir -p tests/bin
	$(CXX) $(CXXFLAGS) -o tests/bin/$@ $^ -lpthread

# Compiler module tests
test_compiler_ddl: $(OBJECTS) tests/unit/sql/compiler/test_compiler_ddl.o
	@mkdir -p tests/bin
	$(CXX) $(CXXFLAGS) -o tests/bin/$@ $^ -lpthread

test_compiler_dml: $(OBJECTS) tests/unit/sql/compiler/test_compiler_dml.o
	@mkdir -p tests/bin
	$(CXX) $(CXXFLAGS) -o tests/bin/$@ $^ -lpthread

test_compiler_semantic: $(OBJECTS) tests/unit/sql/compiler/test_compiler_semantic.o
	@mkdir -p tests/bin
	$(CXX) $(CXXFLAGS) -o tests/bin/$@ $^ -lpthread

test_expression_clone: $(OBJECTS) tests/unit/sql/compiler/test_expression_clone.o
	@mkdir -p tests/bin
	$(CXX) $(CXXFLAGS) -o tests/bin/$@ $^ -lpthread

test_compiler_join: $(OBJECTS) tests/unit/sql/compiler/test_compiler_join.o
	@mkdir -p tests/bin
	$(CXX) $(CXXFLAGS) -o tests/bin/$@ $^ -lpthread

# Optimizer module tests
test_optimizer: $(OBJECTS) tests/unit/sql/optimizer/test_optimizer.o
	@mkdir -p tests/bin
	$(CXX) $(CXXFLAGS) -o tests/bin/$@ $^ -lpthread

# Storage module tests
test_storage: $(OBJECTS) tests/unit/storage/test_storage.o
	@mkdir -p tests/bin
	$(CXX) $(CXXFLAGS) -o tests/bin/$@ $^ -lpthread

test_storage_simple: $(OBJECTS) tests/unit/storage/test_storage_simple.o
	@mkdir -p tests/bin
	$(CXX) $(CXXFLAGS) -o tests/bin/$@ $^ -lpthread

# Executor module tests
test_executor: $(OBJECTS) tests/unit/exec/executor/test_executor.o
	@mkdir -p tests/bin
	$(CXX) $(CXXFLAGS) -o tests/bin/$@ $^ -lpthread

test_expression_eval: $(OBJECTS) tests/unit/exec/executor/test_expression_eval.o
	@mkdir -p tests/bin
	$(CXX) $(CXXFLAGS) -o tests/bin/$@ $^ -lpthread

# Operator module tests
test_operators: $(OBJECTS) tests/unit/exec/operators/test_operators.o
	@mkdir -p tests/bin
	$(CXX) $(CXXFLAGS) -o tests/bin/$@ $^ -lpthread

# Planner module tests
test_planner_ddl: $(OBJECTS) tests/unit/exec/plan/test_planner_ddl.o
	@mkdir -p tests/bin
	$(CXX) $(CXXFLAGS) -o tests/bin/$@ $^ -lpthread

test_planner_dml: $(OBJECTS) tests/unit/exec/plan/test_planner_dml.o
	@mkdir -p tests/bin
	$(CXX) $(CXXFLAGS) -o tests/bin/$@ $^ -lpthread

# Network module tests
test_network: $(OBJECTS) tests/unit/net/test_network.o
	@mkdir -p tests/bin
	$(CXX) $(CXXFLAGS) -o tests/bin/$@ $^ -lpthread

test_network_simple: $(OBJECTS) tests/unit/net/test_network_simple.o
	@mkdir -p tests/bin
	$(CXX) $(CXXFLAGS) -o tests/bin/$@ $^ -lpthread

# Client module tests
test_command_history: $(OBJECTS) tests/unit/client/test_command_history.o
	@mkdir -p tests/bin
	$(CXX) $(CXXFLAGS) -o tests/bin/$@ $^ -lpthread

# Integration test targets
test_full_system: $(OBJECTS) tests/integration/test_full_system.o
	@mkdir -p tests/bin
	$(CXX) $(CXXFLAGS) -o tests/bin/$@ $^ -lpthread

# Object file compilation
%.o: %.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

# Clean
clean:
	rm -f $(OBJECTS) $(TEST_OBJECTS)
	rm -f src/server/main.o src/client/main.o
	rm -rf bin/ tests/bin/

# Run all unit tests
test_unit: $(UNIT_TEST_TARGETS)
	@echo "🧪 Running unit tests..."
	@for test in $(UNIT_TEST_TARGETS); do \
		echo "Running $$test..."; \
		./tests/bin/$$test || exit 1; \
		echo "✅ $$test passed"; \
		echo; \
	done
	@echo "🎉 All unit tests passed!"

# Run integration tests
test_integration: $(INTEGRATION_TEST_TARGETS)
	@echo "🔗 Running integration tests..."
	@for test in $(INTEGRATION_TEST_TARGETS); do \
		echo "Running $$test..."; \
		./tests/bin/$$test || exit 1; \
		echo "✅ $$test passed"; \
		echo; \
	done
	@echo "🎉 All integration tests passed!"

# Run all tests
test: test_unit
	@echo "🎯 All tests completed successfully!"

# Force rebuild everything
rebuild: clean all

.PHONY: all clean test test_unit test_integration rebuild
# E2E test
test_e2e_basic: $(OBJECTS) tests/integration/test_e2e_basic.o
	@mkdir -p tests/bin
	$(CXX) $(CXXFLAGS) -o tests/bin/$@ $^ -lpthread

# JOIN E2E test
test_join_e2e: $(OBJECTS) tests/integration/test_join_e2e.o
	@mkdir -p tests/bin
	$(CXX) $(CXXFLAGS) -o tests/bin/$@ $^ -lpthread

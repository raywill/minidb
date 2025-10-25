# Simple Makefile for MiniDB
CXX = g++
CXXFLAGS = -std=c++11 -Wall -Wextra -O2 -g
INCLUDES = -Iinclude -Isrc

# Source directories  
SRC_DIRS = src/common src/mem src/log src/sql/parser src/sql/ast src/storage src/exec src/exec/operators src/exec/executor src/net src/server src/client
SOURCES = $(foreach dir,$(SRC_DIRS),$(wildcard $(dir)/*.cpp))
# Exclude main.cpp files from core objects
CORE_SOURCES = $(filter-out src/server/main.cpp src/client/main.cpp, $(SOURCES))
OBJECTS = $(CORE_SOURCES:.cpp=.o)

# Test files
UNIT_TEST_SOURCES = tests/unit/test_types.cpp tests/unit/test_parser.cpp tests/unit/test_crash_handler.cpp \
                   tests/unit/test_allocator.cpp tests/unit/test_arena.cpp tests/unit/test_logger.cpp \
                   tests/unit/test_storage.cpp tests/unit/test_parser_extended.cpp tests/unit/test_operators.cpp \
                   tests/unit/test_executor.cpp tests/unit/test_network.cpp tests/unit/test_crash_handler_extended.cpp \
                   tests/unit/test_command_history.cpp tests/unit/test_storage_simple.cpp tests/unit/test_network_simple.cpp \
                   tests/unit/test_cli_client.cpp tests/unit/test_where_clause.cpp
INTEGRATION_TEST_SOURCES = tests/integration/test_full_system.cpp
TEST_SOURCES = $(UNIT_TEST_SOURCES) $(INTEGRATION_TEST_SOURCES)
TEST_OBJECTS = $(TEST_SOURCES:.cpp=.o)
TEST_TARGETS = $(TEST_SOURCES:.cpp=)

# Test target names
UNIT_TEST_TARGETS = test_types test_parser test_crash_handler test_allocator test_arena test_logger \
                   test_storage test_parser_extended test_operators test_executor test_network test_crash_handler_extended \
                   test_command_history test_storage_simple test_network_simple test_cli_client test_where_clause
INTEGRATION_TEST_TARGETS = test_full_system
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

# Unit test targets
test_types: $(OBJECTS) tests/unit/test_types.o
	@mkdir -p tests/bin
	$(CXX) $(CXXFLAGS) -o tests/bin/$@ $^ -lpthread

test_parser: $(OBJECTS) tests/unit/test_parser.o
	@mkdir -p tests/bin
	$(CXX) $(CXXFLAGS) -o tests/bin/$@ $^ -lpthread

test_crash_handler: $(OBJECTS) tests/unit/test_crash_handler.o
	@mkdir -p tests/bin
	$(CXX) $(CXXFLAGS) -o tests/bin/$@ $^ -lpthread

test_allocator: $(OBJECTS) tests/unit/test_allocator.o
	@mkdir -p tests/bin
	$(CXX) $(CXXFLAGS) -o tests/bin/$@ $^ -lpthread

test_arena: $(OBJECTS) tests/unit/test_arena.o
	@mkdir -p tests/bin
	$(CXX) $(CXXFLAGS) -o tests/bin/$@ $^ -lpthread

test_logger: $(OBJECTS) tests/unit/test_logger.o
	@mkdir -p tests/bin
	$(CXX) $(CXXFLAGS) -o tests/bin/$@ $^ -lpthread

test_storage: $(OBJECTS) tests/unit/test_storage.o
	@mkdir -p tests/bin
	$(CXX) $(CXXFLAGS) -o tests/bin/$@ $^ -lpthread

test_storage_simple: $(OBJECTS) tests/unit/test_storage_simple.o
	@mkdir -p tests/bin
	$(CXX) $(CXXFLAGS) -o tests/bin/$@ $^ -lpthread

test_parser_extended: $(OBJECTS) tests/unit/test_parser_extended.o
	@mkdir -p tests/bin
	$(CXX) $(CXXFLAGS) -o tests/bin/$@ $^ -lpthread

test_operators: $(OBJECTS) tests/unit/test_operators.o
	@mkdir -p tests/bin
	$(CXX) $(CXXFLAGS) -o tests/bin/$@ $^ -lpthread

test_executor: $(OBJECTS) tests/unit/test_executor.o
	@mkdir -p tests/bin
	$(CXX) $(CXXFLAGS) -o tests/bin/$@ $^ -lpthread

test_network: $(OBJECTS) tests/unit/test_network.o
	@mkdir -p tests/bin
	$(CXX) $(CXXFLAGS) -o tests/bin/$@ $^ -lpthread

test_network_simple: $(OBJECTS) tests/unit/test_network_simple.o
	@mkdir -p tests/bin
	$(CXX) $(CXXFLAGS) -o tests/bin/$@ $^ -lpthread

test_crash_handler_extended: $(OBJECTS) tests/unit/test_crash_handler_extended.o
	@mkdir -p tests/bin
	$(CXX) $(CXXFLAGS) -o tests/bin/$@ $^ -lpthread

test_command_history: $(OBJECTS) tests/unit/test_command_history.o
	@mkdir -p tests/bin
	$(CXX) $(CXXFLAGS) -o tests/bin/$@ $^ -lpthread

test_cli_client: $(OBJECTS) tests/unit/test_cli_client.o
	@mkdir -p tests/bin
	$(CXX) $(CXXFLAGS) -o tests/bin/$@ $^ -lpthread

test_cli_client_simple: $(OBJECTS) tests/unit/test_cli_client_simple.o
	@mkdir -p tests/bin
	$(CXX) $(CXXFLAGS) -o tests/bin/$@ $^ -lpthread

test_where_clause: $(OBJECTS) tests/unit/test_where_clause.o
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
cmake_minimum_required(VERSION 3.10)
project(MiniDB VERSION 1.0.0 LANGUAGES CXX)

# 设置C++标准
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 设置编译选项
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra")
set(CMAKE_CXX_FLAGS_DEBUG "-g -O0 -DDEBUG")
set(CMAKE_CXX_FLAGS_RELEASE "-O2 -DNDEBUG")

# 设置默认构建类型
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")

# 包含目录
include_directories(include)
include_directories(src)

# 查找依赖
find_package(Threads REQUIRED)

# 定义源文件
set(COMMON_SOURCES
    src/common/crash_handler.cpp
    src/common/status.cpp
    src/common/types.cpp
)

set(MEMORY_SOURCES
    src/mem/allocator.cpp
    src/mem/arena.cpp
)

set(LOG_SOURCES
    src/log/logger.cpp
)

set(PARSER_SOURCES
    # 旧文件（暂时注释掉）
    # src/sql/parser/parser.cpp
    src/sql/parser/tokenizer.cpp
    # src/sql/ast/ast_node.cpp
    # src/sql/ast/statements.cpp
    # 新架构文件
    src/sql/parser/new_parser.cpp
    src/sql/ast/ast.cpp
    src/sql/compiler/statement.cpp
    src/sql/compiler/compiler.cpp
    src/sql/optimizer/optimizer.cpp
)

set(STORAGE_SOURCES
    src/storage/catalog.cpp
    src/storage/table.cpp
)

set(EXECUTION_SOURCES
    src/exec/operator.cpp
    src/exec/operators/filter_operator.cpp
    src/exec/operators/final_result_operator.cpp
    src/exec/operators/projection_operator.cpp
    src/exec/operators/scan_operator.cpp
    # 旧文件（暂时注释掉）
    # src/exec/executor/executor.cpp
    # 新架构文件
    src/exec/plan/plan.cpp
    src/exec/plan/planner.cpp
    src/exec/executor/new_executor.cpp
)

set(NETWORK_SOURCES
    src/net/tcp_client.cpp
    src/net/tcp_server.cpp
)

set(SERVER_SOURCES
    src/server/database_server.cpp
)

set(CLIENT_SOURCES
    src/client/cli_client.cpp
    src/client/command_history.cpp
)

# 核心库
set(CORE_SOURCES
    ${COMMON_SOURCES}
    ${MEMORY_SOURCES}
    ${LOG_SOURCES}
    ${PARSER_SOURCES}
    ${STORAGE_SOURCES}
    ${EXECUTION_SOURCES}
    ${NETWORK_SOURCES}
    ${SERVER_SOURCES}
    ${CLIENT_SOURCES}
)

# 创建核心静态库
add_library(minidb_core STATIC ${CORE_SOURCES})
target_link_libraries(minidb_core Threads::Threads)

# 服务器可执行文件
add_executable(dbserver src/server/main.cpp)
target_link_libraries(dbserver minidb_core)

# 客户端可执行文件
add_executable(dbcli src/client/main.cpp)
target_link_libraries(dbcli minidb_core)

# 测试选项
option(BUILD_TESTS "Build test programs" ON)

if(BUILD_TESTS)
    # 单元测试 - 按模块组织

    # Common模块测试
    set(COMMON_TESTS
        test_types
        test_crash_handler
        test_crash_handler_extended
    )
    foreach(test_name ${COMMON_TESTS})
        add_executable(${test_name} tests/unit/common/${test_name}.cpp)
        target_link_libraries(${test_name} minidb_core)
    endforeach()

    # Memory模块测试
    set(MEMORY_TESTS
        test_allocator
        test_arena
    )
    foreach(test_name ${MEMORY_TESTS})
        add_executable(${test_name} tests/unit/memory/${test_name}.cpp)
        target_link_libraries(${test_name} minidb_core)
    endforeach()

    # Log模块测试
    set(LOG_TESTS
        test_logger
    )
    foreach(test_name ${LOG_TESTS})
        add_executable(${test_name} tests/unit/log/${test_name}.cpp)
        target_link_libraries(${test_name} minidb_core)
    endforeach()

    # SQL Parser模块测试
    set(PARSER_TESTS
        test_parser
        test_tokenizer
        test_parser_ddl
        test_parser_dml
        test_parser_expressions
    )
    foreach(test_name ${PARSER_TESTS})
        add_executable(${test_name} tests/unit/sql/parser/${test_name}.cpp)
        target_link_libraries(${test_name} minidb_core)
    endforeach()

    # SQL Compiler模块测试
    set(COMPILER_TESTS
        test_compiler_ddl
        test_compiler_dml
        test_compiler_semantic
    )
    foreach(test_name ${COMPILER_TESTS})
        add_executable(${test_name} tests/unit/sql/compiler/${test_name}.cpp)
        target_link_libraries(${test_name} minidb_core)
    endforeach()

    # SQL Optimizer模块测试
    set(OPTIMIZER_TESTS
        test_optimizer
    )
    foreach(test_name ${OPTIMIZER_TESTS})
        add_executable(${test_name} tests/unit/sql/optimizer/${test_name}.cpp)
        target_link_libraries(${test_name} minidb_core)
    endforeach()

    # Storage模块测试
    set(STORAGE_TESTS
        test_storage
        test_storage_simple
    )
    foreach(test_name ${STORAGE_TESTS})
        add_executable(${test_name} tests/unit/storage/${test_name}.cpp)
        target_link_libraries(${test_name} minidb_core)
    endforeach()

    # Execution模块测试
    set(EXECUTOR_TESTS
        test_executor
    )
    foreach(test_name ${EXECUTOR_TESTS})
        add_executable(${test_name} tests/unit/exec/executor/${test_name}.cpp)
        target_link_libraries(${test_name} minidb_core)
    endforeach()

    set(OPERATOR_TESTS
        test_operators
    )
    foreach(test_name ${OPERATOR_TESTS})
        add_executable(${test_name} tests/unit/exec/operators/${test_name}.cpp)
        target_link_libraries(${test_name} minidb_core)
    endforeach()

    # Planner模块测试
    set(PLANNER_TESTS
        test_planner_ddl
        test_planner_dml
    )
    foreach(test_name ${PLANNER_TESTS})
        add_executable(${test_name} tests/unit/exec/plan/${test_name}.cpp)
        target_link_libraries(${test_name} minidb_core)
    endforeach()

    # Network模块测试
    set(NETWORK_TESTS
        test_network
        test_network_simple
    )
    foreach(test_name ${NETWORK_TESTS})
        add_executable(${test_name} tests/unit/net/${test_name}.cpp)
        target_link_libraries(${test_name} minidb_core)
    endforeach()

    # Client模块测试
    set(CLIENT_TESTS
        test_command_history
    )
    foreach(test_name ${CLIENT_TESTS})
        add_executable(${test_name} tests/unit/client/${test_name}.cpp)
        target_link_libraries(${test_name} minidb_core)
    endforeach()

    # 所有单元测试列表
    set(UNIT_TESTS
        ${COMMON_TESTS}
        ${MEMORY_TESTS}
        ${LOG_TESTS}
        ${PARSER_TESTS}
        ${COMPILER_TESTS}
        ${OPTIMIZER_TESTS}
        ${STORAGE_TESTS}
        ${EXECUTOR_TESTS}
        ${OPERATOR_TESTS}
        ${PLANNER_TESTS}
        ${NETWORK_TESTS}
        ${CLIENT_TESTS}
    )

    # 集成测试
    add_executable(test_full_system tests/integration/test_full_system.cpp)
    target_link_libraries(test_full_system minidb_core)

    # 测试目标
    enable_testing()
    
    foreach(test_name ${UNIT_TESTS})
        add_test(NAME ${test_name} COMMAND ${test_name})
    endforeach()
    
    add_test(NAME test_full_system COMMAND test_full_system)

    # 自定义测试目标
    add_custom_target(test_unit
        COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure -R "test_"
        DEPENDS ${UNIT_TESTS}
        COMMENT "Running unit tests"
    )

    add_custom_target(test_integration
        COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure -R "test_full_system"
        DEPENDS test_full_system
        COMMENT "Running integration tests"
    )
endif()

# 安装规则
install(TARGETS dbserver dbcli
    RUNTIME DESTINATION bin
)

install(DIRECTORY include/
    DESTINATION include/minidb
    FILES_MATCHING PATTERN "*.h"
)

# 打包配置
set(CPACK_PACKAGE_NAME "MiniDB")
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "A columnar analytical database system")
set(CPACK_PACKAGE_VENDOR "MiniDB Team")
set(CPACK_PACKAGE_CONTACT "minidb@example.com")

# 根据平台选择打包器
if(WIN32)
    set(CPACK_GENERATOR "ZIP;NSIS")
elseif(APPLE)
    set(CPACK_GENERATOR "ZIP;DragNDrop")
else()
    set(CPACK_GENERATOR "TGZ;DEB;RPM")
endif()

include(CPack)

# 显示配置信息
message(STATUS "=== MiniDB Configuration ===")
message(STATUS "Version: ${PROJECT_VERSION}")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "C++ standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "Build tests: ${BUILD_TESTS}")
message(STATUS "Install prefix: ${CMAKE_INSTALL_PREFIX}")
message(STATUS "=============================")

# 添加自定义目标
add_custom_target(clean-all
    COMMAND ${CMAKE_BUILD_TOOL} clean
    COMMAND ${CMAKE_COMMAND} -E remove_directory ${CMAKE_BINARY_DIR}/CMakeFiles
    COMMAND ${CMAKE_COMMAND} -E remove ${CMAKE_BINARY_DIR}/CMakeCache.txt
    COMMENT "Cleaning all build files"
)

# 代码格式化目标（如果有clang-format）
find_program(CLANG_FORMAT clang-format)
if(CLANG_FORMAT)
    file(GLOB_RECURSE ALL_SOURCE_FILES
        ${CMAKE_SOURCE_DIR}/src/*.cpp
        ${CMAKE_SOURCE_DIR}/src/*.h
        ${CMAKE_SOURCE_DIR}/include/*.h
        ${CMAKE_SOURCE_DIR}/tests/*.cpp
    )
    
    add_custom_target(format
        COMMAND ${CLANG_FORMAT} -i ${ALL_SOURCE_FILES}
        COMMENT "Formatting source code"
    )
endif()

# 文档生成目标（如果有Doxygen）
find_package(Doxygen)
if(DOXYGEN_FOUND)
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/docs/Doxyfile.in 
                   ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile @ONLY)
    
    add_custom_target(docs
        ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        COMMENT "Generating API documentation with Doxygen"
        VERBATIM
    )
endif()
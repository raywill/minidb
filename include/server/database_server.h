#pragma once

#include "net/tcp_server.h"
#include "exec/executor/new_executor.h"
#include "exec/plan/planner.h"
#include "sql/parser/new_parser.h"
#include "sql/compiler/compiler.h"
#include "sql/optimizer/optimizer.h"
#include "storage/catalog.h"
#include "storage/table.h"
#include "common/status.h"
#include <memory>

namespace minidb {

// 数据库服务器 - 实现ConnectionHandler接口
class DatabaseServer : public ConnectionHandler {
public:
    explicit DatabaseServer(const std::string& data_dir, int port = 9876);
    ~DatabaseServer();
    
    // 启动服务器
    Status start();
    
    // 停止服务器
    void stop();
    
    // 检查服务器是否在运行
    bool is_running() const;
    
    // 实现ConnectionHandler接口
    std::string handle_request(const std::string& request) override;
    
private:
    std::string data_directory_;
    int port_;
    
    std::unique_ptr<Catalog> catalog_;
    std::unique_ptr<TableManager> table_manager_;
    std::unique_ptr<Compiler> compiler_;
    std::unique_ptr<Optimizer> optimizer_;
    std::unique_ptr<Planner> planner_;
    std::unique_ptr<QueryExecutor> executor_;
    std::unique_ptr<TCPServer> tcp_server_;
    
    // 初始化数据库组件
    Status initialize_database();
    
    // 处理SQL请求
    std::string process_sql(const std::string& sql);
    
    // 格式化错误响应
    std::string format_error_response(const std::string& error);
    
    // 格式化成功响应
    std::string format_success_response(const std::string& result);
};

} // namespace minidb

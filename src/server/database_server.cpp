#include "server/database_server.h"
#include "log/logger.h"
#include "common/utils.h"
#include "common/crash_handler.h"

namespace minidb {

DatabaseServer::DatabaseServer(const std::string& data_dir, int port)
    : data_directory_(data_dir), port_(port) {
}

DatabaseServer::~DatabaseServer() {
    stop();
}

Status DatabaseServer::start() {
    LOG_INFO("DatabaseServer", "Startup", "Starting MiniDB server");
    
    // 初始化数据库组件
    Status status = initialize_database();
    if (!status.ok()) {
        LOG_ERROR("DatabaseServer", "Startup", "Failed to initialize database: " + status.ToString());
        return status;
    }
    
    // 创建TCP服务器
    tcp_server_ = make_unique<TCPServer>(port_);
    tcp_server_->set_handler(this);
    
    // 启动TCP服务器
    status = tcp_server_->start();
    if (!status.ok()) {
        LOG_ERROR("DatabaseServer", "Startup", "Failed to start TCP server: " + status.ToString());
        return status;
    }
    
    LOG_INFO("DatabaseServer", "Startup", "MiniDB server started successfully on port " + std::to_string(port_));
    return Status::OK();
}

void DatabaseServer::stop() {
    if (tcp_server_) {
        LOG_INFO("DatabaseServer", "Shutdown", "Stopping MiniDB server");
        tcp_server_->stop();
        tcp_server_.reset();
    }
}

bool DatabaseServer::is_running() const {
    return tcp_server_ && tcp_server_->is_running();
}

std::string DatabaseServer::handle_request(const std::string& request) {
    LOG_DEBUG("DatabaseServer", "HandleRequest", "Processing SQL: " + request);
    
    try {
        return process_sql(request);
    } catch (const DatabaseException& e) {
        LOG_ERROR("DatabaseServer", "HandleRequest", "Database exception: " + std::string(e.what()));
        return format_error_response("Database error: " + std::string(e.what()));
    } catch (const std::exception& e) {
        LOG_ERROR("DatabaseServer", "HandleRequest", "Exception processing SQL: " + std::string(e.what()));
        
        // 生成崩溃转储文件
        std::string dump_file = CrashHandler::generate_dump_file();
        LOG_ERROR("DatabaseServer", "HandleRequest", "Generated crash dump: " + dump_file);
        
        return format_error_response("Internal server error: " + std::string(e.what()) + 
                                   " (crash dump: " + dump_file + ")");
    }
}

Status DatabaseServer::initialize_database() {
    // 初始化崩溃处理器
    CrashHandler::initialize();
    
    // 初始化日志系统
    Logger::instance()->add_sink(std::shared_ptr<LogSink>(new FileSink("minidb.log")));
    Logger::instance()->set_level(LogLevel::DEBUG);
    
    // 初始化目录
    catalog_ = make_unique<Catalog>(data_directory_);
    Status status = catalog_->initialize();
    if (!status.ok()) {
        return status;
    }
    
    // 初始化表管理器
    table_manager_ = make_unique<TableManager>(catalog_.get());
    
    // 初始化执行器
    executor_ = make_unique<Executor>(catalog_.get(), table_manager_.get());
    
    LOG_INFO("DatabaseServer", "Initialize", "Database components initialized successfully");
    return Status::OK();
}

std::string DatabaseServer::process_sql(const std::string& sql) {
    if (sql.empty()) {
        return format_error_response("Empty SQL statement");
    }
    
    // 解析SQL
    Parser parser(sql);
    std::unique_ptr<Statement> stmt;
    Status status = parser.parse(stmt);
    
    if (!status.ok()) {
        return format_error_response("Parse error: " + status.ToString());
    }
    
    if (!stmt) {
        return format_error_response("Failed to parse SQL statement");
    }
    
    // 执行SQL
    QueryResult result = executor_->execute_statement(stmt.get());
    
    if (result.success) {
        return format_success_response(result.result_text);
    } else {
        return format_error_response(result.error_message);
    }
}

std::string DatabaseServer::format_error_response(const std::string& error) {
    return "ERROR: " + error;
}

std::string DatabaseServer::format_success_response(const std::string& result) {
    if (result.empty()) {
        return "OK";
    }
    return result;
}

} // namespace minidb

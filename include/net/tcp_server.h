#pragma once

#include "common/status.h"
#include <string>
#include <thread>
#include <atomic>
#include <memory>
#include <functional>

namespace minidb {

// TCP连接处理器接口
class ConnectionHandler {
public:
    virtual ~ConnectionHandler() = default;
    
    // 处理客户端请求
    virtual std::string handle_request(const std::string& request) = 0;
};

// TCP连接会话
class Session {
public:
    Session(int socket_fd, ConnectionHandler* handler);
    ~Session();
    
    // 运行会话循环
    void run();
    
private:
    int socket_fd_;
    ConnectionHandler* handler_;
    
    // 读取请求
    Status read_request(std::string& request);
    
    // 发送响应
    Status send_response(const std::string& response);
};

// TCP服务器
class TCPServer {
public:
    explicit TCPServer(int port);
    ~TCPServer();
    
    // 设置连接处理器
    void set_handler(ConnectionHandler* handler);
    
    // 启动服务器
    Status start();
    
    // 停止服务器
    void stop();
    
    // 检查服务器是否在运行
    bool is_running() const;
    
private:
    int port_;
    int server_socket_;
    ConnectionHandler* handler_;
    std::atomic<bool> running_;
    std::unique_ptr<std::thread> accept_thread_;
    
    // 接受连接循环
    void accept_loop();
    
    // 处理客户端连接
    void handle_client(int client_socket);
    
    // 创建服务器socket
    Status create_server_socket();
    
    // 关闭socket
    void close_socket(int socket_fd);
};

} // namespace minidb

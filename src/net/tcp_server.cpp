#include "net/tcp_server.h"
#include "log/logger.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <cstring>

namespace minidb {

// Session 实现
Session::Session(int socket_fd, ConnectionHandler* handler)
    : socket_fd_(socket_fd), handler_(handler) {
}

Session::~Session() {
    if (socket_fd_ >= 0) {
        close(socket_fd_);
    }
}

void Session::run() {
    LOG_INFO("Session", "Connection", "Starting session for socket " + std::to_string(socket_fd_));
    
    while (true) {
        std::string request;
        Status status = read_request(request);
        if (!status.ok()) {
            LOG_INFO("Session", "Connection", "Client disconnected or error reading request");
            break;
        }
        
        if (request.empty()) {
            continue;
        }
        
        LOG_DEBUG("Session", "Connection", "Received request: " + request);
        
        // 处理请求
        std::string response = handler_->handle_request(request);
        
        // 发送响应
        status = send_response(response);
        if (!status.ok()) {
            LOG_ERROR("Session", "Connection", "Failed to send response: " + status.ToString());
            break;
        }
        
        LOG_DEBUG("Session", "Connection", "Sent response");
    }
    
    LOG_INFO("Session", "Connection", "Session ended for socket " + std::to_string(socket_fd_));
}

Status Session::read_request(std::string& request) {
    request.clear();
    char buffer[4096];
    
    while (true) {
        ssize_t bytes_read = recv(socket_fd_, buffer, sizeof(buffer) - 1, 0);
        
        if (bytes_read <= 0) {
            if (bytes_read == 0) {
                return Status::NetworkError("Client disconnected");
            } else {
                return Status::NetworkError("Failed to read from socket");
            }
        }
        
        buffer[bytes_read] = '\0';
        request += buffer;
        
        // 检查是否收到完整请求（以\n\n结尾）
        if (request.length() >= 2 && request.substr(request.length() - 2) == "\n\n") {
            // 移除结尾的\n\n
            request = request.substr(0, request.length() - 2);
            break;
        }
    }
    
    return Status::OK();
}

Status Session::send_response(const std::string& response) {
    std::string full_response = response + "\n\n";
    
    size_t total_sent = 0;
    while (total_sent < full_response.length()) {
        ssize_t bytes_sent = send(socket_fd_, 
                                 full_response.c_str() + total_sent,
                                 full_response.length() - total_sent, 0);
        
        if (bytes_sent <= 0) {
            return Status::NetworkError("Failed to send response");
        }
        
        total_sent += bytes_sent;
    }
    
    return Status::OK();
}

// TCPServer 实现
TCPServer::TCPServer(int port) 
    : port_(port), server_socket_(-1), handler_(nullptr), running_(false) {
}

TCPServer::~TCPServer() {
    stop();
}

void TCPServer::set_handler(ConnectionHandler* handler) {
    handler_ = handler;
}

Status TCPServer::start() {
    if (running_.load()) {
        return Status::InvalidArgument("Server is already running");
    }
    
    if (!handler_) {
        return Status::InvalidArgument("Connection handler not set");
    }
    
    LOG_INFO("TCPServer", "Startup", "Starting TCP server on port " + std::to_string(port_));
    
    // 创建服务器socket
    Status status = create_server_socket();
    if (!status.ok()) {
        return status;
    }
    
    running_.store(true);
    
    // 启动接受连接的线程
    accept_thread_ = std::unique_ptr<std::thread>(new std::thread(&TCPServer::accept_loop, this));
    
    LOG_INFO("TCPServer", "Startup", "TCP server started successfully");
    return Status::OK();
}

void TCPServer::stop() {
    if (!running_.load()) {
        return;
    }

    LOG_INFO("TCPServer", "Shutdown", "Stopping TCP server");

    running_.store(false);

    // 先shutdown socket，让阻塞的accept()立即返回
    if (server_socket_ >= 0) {
        shutdown(server_socket_, SHUT_RDWR);
    }

    // 等待接受线程结束
    if (accept_thread_ && accept_thread_->joinable()) {
        accept_thread_->join();
    }

    // 关闭服务器socket
    if (server_socket_ >= 0) {
        close_socket(server_socket_);
        server_socket_ = -1;
    }

    LOG_INFO("TCPServer", "Shutdown", "TCP server stopped");
}

bool TCPServer::is_running() const {
    return running_.load();
}

void TCPServer::accept_loop() {
    LOG_INFO("TCPServer", "AcceptLoop", "Accept loop started");
    
    while (running_.load()) {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        
        int client_socket = accept(server_socket_, 
                                  (struct sockaddr*)&client_addr, 
                                  &client_len);
        
        if (client_socket < 0) {
            if (running_.load()) {
                LOG_ERROR("TCPServer", "AcceptLoop", "Failed to accept client connection");
            }
            continue;
        }
        
        std::string client_ip = inet_ntoa(client_addr.sin_addr);
        int client_port = ntohs(client_addr.sin_port);
        
        LOG_INFO("TCPServer", "AcceptLoop", 
                "Accepted connection from " + client_ip + ":" + std::to_string(client_port));
        
        // 在新线程中处理客户端
        std::thread client_thread(&TCPServer::handle_client, this, client_socket);
        client_thread.detach();
    }
    
    LOG_INFO("TCPServer", "AcceptLoop", "Accept loop ended");
}

void TCPServer::handle_client(int client_socket) {
    Session session(client_socket, handler_);
    session.run();
}

Status TCPServer::create_server_socket() {
    // 创建socket
    server_socket_ = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket_ < 0) {
        return Status::NetworkError("Failed to create socket");
    }
    
    // 设置socket选项
    int opt = 1;
    if (setsockopt(server_socket_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        close_socket(server_socket_);
        return Status::NetworkError("Failed to set socket options");
    }
    
    // 绑定地址
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port_);
    
    if (bind(server_socket_, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        close_socket(server_socket_);
        return Status::NetworkError("Failed to bind socket to port " + std::to_string(port_));
    }
    
    // 开始监听
    if (listen(server_socket_, 10) < 0) {
        close_socket(server_socket_);
        return Status::NetworkError("Failed to listen on socket");
    }
    
    return Status::OK();
}

void TCPServer::close_socket(int socket_fd) {
    if (socket_fd >= 0) {
        close(socket_fd);
    }
}

} // namespace minidb

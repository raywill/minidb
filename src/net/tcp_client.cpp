#include "net/tcp_client.h"
#include "log/logger.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>
#include <cstring>

namespace minidb {

TCPClient::TCPClient() : socket_fd_(-1), connected_(false) {
}

TCPClient::~TCPClient() {
    disconnect();
}

Status TCPClient::connect(const std::string& host, int port) {
    if (connected_) {
        return Status::InvalidArgument("Client is already connected");
    }
    
    LOG_INFO("TCPClient", "Connect", "Connecting to " + host + ":" + std::to_string(port));
    
    // 创建socket
    socket_fd_ = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_fd_ < 0) {
        return Status::NetworkError("Failed to create socket");
    }
    
    // 解析主机名
    struct hostent* server = gethostbyname(host.c_str());
    if (server == nullptr) {
        close(socket_fd_);
        socket_fd_ = -1;
        return Status::NetworkError("Failed to resolve hostname: " + host);
    }
    
    // 设置服务器地址
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    memcpy(&server_addr.sin_addr.s_addr, server->h_addr, server->h_length);
    server_addr.sin_port = htons(port);
    
    // 连接到服务器
    if (::connect(socket_fd_, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        close(socket_fd_);
        socket_fd_ = -1;
        return Status::NetworkError("Failed to connect to server");
    }
    
    connected_ = true;
    LOG_INFO("TCPClient", "Connect", "Connected successfully");
    return Status::OK();
}

void TCPClient::disconnect() {
    if (connected_) {
        LOG_INFO("TCPClient", "Disconnect", "Disconnecting from server");
        close(socket_fd_);
        socket_fd_ = -1;
        connected_ = false;
    }
}

Status TCPClient::send_request(const std::string& request, std::string& response) {
    if (!connected_) {
        return Status::InvalidArgument("Client is not connected");
    }
    
    LOG_DEBUG("TCPClient", "SendRequest", "Sending request: " + request);
    
    // 发送请求
    Status status = send_data(request + "\n\n");
    if (!status.ok()) {
        return status;
    }
    
    // 接收响应
    status = receive_data(response);
    if (!status.ok()) {
        return status;
    }
    
    LOG_DEBUG("TCPClient", "SendRequest", "Received response");
    return Status::OK();
}

bool TCPClient::is_connected() const {
    return connected_;
}

Status TCPClient::send_data(const std::string& data) {
    size_t total_sent = 0;
    while (total_sent < data.length()) {
        ssize_t bytes_sent = send(socket_fd_, 
                                 data.c_str() + total_sent,
                                 data.length() - total_sent, 0);
        
        if (bytes_sent <= 0) {
            connected_ = false;
            return Status::NetworkError("Failed to send data");
        }
        
        total_sent += bytes_sent;
    }
    
    return Status::OK();
}

Status TCPClient::receive_data(std::string& data) {
    data.clear();
    char buffer[4096];
    
    while (true) {
        ssize_t bytes_read = recv(socket_fd_, buffer, sizeof(buffer) - 1, 0);
        
        if (bytes_read <= 0) {
            connected_ = false;
            if (bytes_read == 0) {
                return Status::NetworkError("Server disconnected");
            } else {
                return Status::NetworkError("Failed to receive data");
            }
        }
        
        buffer[bytes_read] = '\0';
        data += buffer;
        
        // 检查是否收到完整响应（以\n\n结尾）
        if (data.length() >= 2 && data.substr(data.length() - 2) == "\n\n") {
            // 移除结尾的\n\n
            data = data.substr(0, data.length() - 2);
            break;
        }
    }
    
    return Status::OK();
}

} // namespace minidb

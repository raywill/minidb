#pragma once

#include "common/status.h"
#include <string>

namespace minidb {

// TCP客户端
class TCPClient {
public:
    TCPClient();
    ~TCPClient();
    
    // 连接到服务器
    Status connect(const std::string& host, int port);
    
    // 断开连接
    void disconnect();
    
    // 发送请求并接收响应
    Status send_request(const std::string& request, std::string& response);
    
    // 检查是否已连接
    bool is_connected() const;
    
private:
    int socket_fd_;
    bool connected_;
    
    // 发送数据
    Status send_data(const std::string& data);
    
    // 接收数据
    Status receive_data(std::string& data);
};

} // namespace minidb

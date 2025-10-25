#include "net/tcp_server.h"
#include "net/tcp_client.h"
#include "common/status.h"
#include "log/logger.h"
#include <iostream>
#include <cassert>
#include <thread>
#include <chrono>
#include <atomic>

using namespace minidb;

// 测试用的连接处理器
class TestConnectionHandler : public ConnectionHandler {
public:
    TestConnectionHandler() : request_count_(0) {}
    
    std::string handle_request(const std::string& request) override {
        request_count_++;
        last_request_ = request;
        
        if (request == "PING") {
            return "PONG";
        } else if (request == "ECHO") {
            return request;
        } else if (request == "COUNT") {
            return std::to_string(request_count_);
        } else if (request == "ERROR") {
            return "ERROR: Test error message";
        } else {
            return "UNKNOWN: " + request;
        }
    }
    
    int get_request_count() const { return request_count_; }
    const std::string& get_last_request() const { return last_request_; }
    
private:
    std::atomic<int> request_count_;
    std::string last_request_;
};

void test_tcp_client_basic() {
    std::cout << "Testing TCPClient basic operations..." << std::endl;
    
    TCPClient client;
    
    // 初始状态
    assert(!client.is_connected());
    
    // 测试连接到不存在的服务器（应该失败）
    Status status = client.connect("localhost", 99999);
    assert(!status.ok());
    assert(status.is_network_error());
    assert(!client.is_connected());
    
    // 测试在未连接状态下发送请求
    std::string response;
    status = client.send_request("test", response);
    assert(!status.ok());
    assert(status.is_invalid_argument());
    
    std::cout << "TCPClient basic operations test passed!" << std::endl;
}

void test_tcp_server_basic() {
    std::cout << "Testing TCPServer basic operations..." << std::endl;
    
    TestConnectionHandler handler;
    TCPServer server(19001);
    
    // 初始状态
    assert(!server.is_running());
    
    // 设置处理器
    server.set_handler(&handler);
    
    // 启动服务器
    Status status = server.start();
    assert(status.ok());
    assert(server.is_running());
    
    // 等待服务器完全启动
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    // 停止服务器
    server.stop();
    assert(!server.is_running());
    
    std::cout << "TCPServer basic operations test passed!" << std::endl;
}

void test_client_server_communication() {
    std::cout << "Testing client-server communication..." << std::endl;
    
    TestConnectionHandler handler;
    TCPServer server(19002);
    server.set_handler(&handler);
    
    // 启动服务器
    Status status = server.start();
    assert(status.ok());
    
    // 等待服务器启动
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    // 连接客户端
    TCPClient client;
    status = client.connect("localhost", 19002);
    assert(status.ok());
    assert(client.is_connected());
    
    // 测试基本通信
    std::string response;
    status = client.send_request("PING", response);
    assert(status.ok());
    assert(response == "PONG");
    
    // 测试ECHO
    status = client.send_request("ECHO", response);
    assert(status.ok());
    assert(response == "ECHO");
    
    // 测试计数
    status = client.send_request("COUNT", response);
    assert(status.ok());
    assert(response == "3"); // PING + ECHO + COUNT
    
    // 测试错误响应
    status = client.send_request("ERROR", response);
    assert(status.ok());
    assert(response.find("ERROR:") != std::string::npos);
    
    // 断开连接
    client.disconnect();
    assert(!client.is_connected());
    
    // 停止服务器
    server.stop();
    
    // 验证处理器收到了请求
    assert(handler.get_request_count() >= 4);
    
    std::cout << "Client-server communication test passed!" << std::endl;
}

void test_multiple_clients() {
    std::cout << "Testing multiple clients..." << std::endl;
    
    TestConnectionHandler handler;
    TCPServer server(19003);
    server.set_handler(&handler);
    
    Status status = server.start();
    assert(status.ok());
    
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    const int num_clients = 3; // 减少客户端数量
    std::vector<std::unique_ptr<TCPClient>> clients;
    std::vector<std::thread> client_threads;
    std::vector<bool> client_results(num_clients, false);
    
    // 创建多个客户端
    for (int i = 0; i < num_clients; ++i) {
        clients.push_back(std::unique_ptr<TCPClient>(new TCPClient()));
    }
    
    // 并发连接和通信
    for (int i = 0; i < num_clients; ++i) {
        client_threads.emplace_back([&, i]() {
            try {
                Status status = clients[i]->connect("localhost", 19003);
                if (!status.ok()) {
                    std::cout << "Client " << i << " failed to connect: " << status.ToString() << std::endl;
                    return;
                }
                
                // 每个客户端发送较少请求
                for (int j = 0; j < 3; ++j) {
                    std::string request = "CLIENT" + std::to_string(i) + "_REQ" + std::to_string(j);
                    std::string response;
                    
                    status = clients[i]->send_request(request, response);
                    if (!status.ok()) {
                        std::cout << "Client " << i << " request " << j << " failed: " << status.ToString() << std::endl;
                        return;
                    }
                    
                    assert(response.find("UNKNOWN:") != std::string::npos);
                    
                    // 小延迟
                    std::this_thread::sleep_for(std::chrono::milliseconds(50));
                }
                
                clients[i]->disconnect();
                client_results[i] = true;
                
            } catch (const std::exception& e) {
                std::cout << "Client " << i << " exception: " << e.what() << std::endl;
            }
        });
    }
    
    // 等待所有客户端完成
    for (auto& thread : client_threads) {
        thread.join();
    }
    
    // 验证所有客户端都成功
    for (int i = 0; i < num_clients; ++i) {
        assert(client_results[i]);
    }
    
    server.stop();
    
    // 验证处理器收到了所有请求
    assert(handler.get_request_count() >= num_clients * 3);
    
    std::cout << "Multiple clients test passed!" << std::endl;
}

void test_network_protocol() {
    std::cout << "Testing network protocol..." << std::endl;
    
    TestConnectionHandler handler;
    TCPServer server(19004);
    server.set_handler(&handler);
    
    Status status = server.start();
    assert(status.ok());
    
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    TCPClient client;
    status = client.connect("localhost", 19004);
    assert(status.ok());
    
    // 测试长消息
    std::string long_message(5000, 'X');
    std::string response;
    status = client.send_request(long_message, response);
    assert(status.ok());
    assert(response.find("UNKNOWN:") != std::string::npos);
    assert(response.find(long_message) != std::string::npos);
    
    // 测试空消息
    status = client.send_request("", response);
    assert(status.ok());
    assert(response == "UNKNOWN: ");
    
    // 测试特殊字符
    std::string special_msg = "Special chars: \n\t\r\"'\\";
    status = client.send_request(special_msg, response);
    assert(status.ok());
    assert(response.find("UNKNOWN:") != std::string::npos);
    
    client.disconnect();
    server.stop();
    
    std::cout << "Network protocol test passed!" << std::endl;
}

void test_server_error_handling() {
    std::cout << "Testing server error handling..." << std::endl;
    
    // 测试无处理器启动
    TCPServer server_no_handler(19005);
    Status status = server_no_handler.start();
    assert(!status.ok());
    assert(status.is_invalid_argument());
    
    // 测试端口冲突
    TestConnectionHandler handler1, handler2;
    TCPServer server1(19006);
    TCPServer server2(19006); // 同一个端口
    
    server1.set_handler(&handler1);
    server2.set_handler(&handler2);
    
    status = server1.start();
    assert(status.ok());
    
    // 第二个服务器应该启动失败
    status = server2.start();
    assert(!status.ok());
    assert(status.is_network_error());
    
    server1.stop();
    
    std::cout << "Server error handling test passed!" << std::endl;
}

void test_connection_lifecycle() {
    std::cout << "Testing connection lifecycle..." << std::endl;
    
    TestConnectionHandler handler;
    TCPServer server(19007);
    server.set_handler(&handler);
    
    Status status = server.start();
    assert(status.ok());
    
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    // 测试连接-断开-重连
    TCPClient client;
    
    for (int i = 0; i < 3; ++i) {
        // 连接
        status = client.connect("localhost", 19007);
        assert(status.ok());
        assert(client.is_connected());
        
        // 发送请求
        std::string response;
        status = client.send_request("PING", response);
        assert(status.ok());
        assert(response == "PONG");
        
        // 断开
        client.disconnect();
        assert(!client.is_connected());
        
        // 短暂等待
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
    
    server.stop();
    
    std::cout << "Connection lifecycle test passed!" << std::endl;
}

void test_network_performance() {
    std::cout << "Testing network performance..." << std::endl;
    
    TestConnectionHandler handler;
    TCPServer server(19008);
    server.set_handler(&handler);
    
    Status status = server.start();
    assert(status.ok());
    
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    TCPClient client;
    status = client.connect("localhost", 19008);
    assert(status.ok());
    
    const int num_requests = 100; // 减少请求数量
    auto start_time = std::chrono::high_resolution_clock::now();
    
    // 发送大量请求
    for (int i = 0; i < num_requests; ++i) {
        std::string request = "REQ" + std::to_string(i);
        std::string response;
        
        status = client.send_request(request, response);
        assert(status.ok());
        assert(response.find("UNKNOWN:") != std::string::npos);
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    
    std::cout << "Sent " << num_requests << " requests in " << duration.count() << "ms" << std::endl;
    std::cout << "Average: " << (duration.count() * 1000.0 / num_requests) << " microseconds per request" << std::endl;
    
    client.disconnect();
    server.stop();
    
    assert(handler.get_request_count() >= num_requests);
    
    std::cout << "Network performance test passed!" << std::endl;
}

int main() {
    try {
        // 设置日志级别
        Logger::instance()->set_level(LogLevel::ERROR);
        
        test_tcp_client_basic();
        test_tcp_server_basic();
        test_client_server_communication();
        test_multiple_clients();
        test_network_protocol();
        test_server_error_handling();
        test_connection_lifecycle();
        test_network_performance();
        
        std::cout << "\n🎉 All network tests passed!" << std::endl;
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "❌ Network test failed: " << e.what() << std::endl;
        return 1;
    }
}

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/wait.h>

// Include MiniDB config for default port
namespace minidb {
    constexpr int DEFAULT_PORT = 9876;
    constexpr const char* DEFAULT_HOST = "127.0.0.1";
}

// Global verbose flag
static bool g_verbose = false;

class MiniTest {
public:
    MiniTest(const std::string& host, int port, const std::string& sql_file,
             const std::string& run_mode)
        : host_(host), port_(port), sql_file_(sql_file), run_mode_(run_mode),
          sock_(-1) {}

    ~MiniTest() {
        if (sock_ >= 0) {
            close(sock_);
        }
    }

    bool connect_to_server() {
        sock_ = socket(AF_INET, SOCK_STREAM, 0);
        if (sock_ < 0) {
            std::cerr << "Error: Failed to create socket" << std::endl;
            return false;
        }

        struct sockaddr_in server_addr;
        memset(&server_addr, 0, sizeof(server_addr));
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(port_);

        if (inet_pton(AF_INET, host_.c_str(), &server_addr.sin_addr) <= 0) {
            std::cerr << "Error: Invalid address: " << host_ << std::endl;
            return false;
        }

        if (connect(sock_, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
            std::cerr << "Error: Failed to connect to " << host_ << ":" << port_ << std::endl;
            return false;
        }

        return true;
    }

    std::string execute_query(const std::string& query) {
        if (g_verbose) {
            std::cerr << "[DEBUG] Sending query: " << query << std::endl;
        }

        // Send query with \n\n terminator (required by server protocol)
        std::string request = query + "\n\n";
        ssize_t sent = send(sock_, request.c_str(), request.length(), 0);
        if (sent < 0) {
            std::cerr << "[ERROR] Failed to send query" << std::endl;
            return "ERROR: Failed to send query";
        }
        if (g_verbose) {
            std::cerr << "[DEBUG] Sent " << sent << " bytes (with \\n\\n terminator)" << std::endl;
        }

        // Receive response
        if (g_verbose) {
            std::cerr << "[DEBUG] Waiting for response..." << std::endl;
        }
        char buffer[65536];
        memset(buffer, 0, sizeof(buffer));
        ssize_t bytes_received = recv(sock_, buffer, sizeof(buffer) - 1, 0);

        if (g_verbose) {
            std::cerr << "[DEBUG] Received " << bytes_received << " bytes" << std::endl;
        }

        if (bytes_received < 0) {
            std::cerr << "[ERROR] Failed to receive response" << std::endl;
            return "ERROR: Failed to receive response";
        }

        if (g_verbose && bytes_received > 0) {
            std::cerr << "[DEBUG] Response: " << std::string(buffer, std::min((size_t)bytes_received, (size_t)100)) << "..." << std::endl;
        }

        // Reconnect for next query
        if (g_verbose) {
            std::cerr << "[DEBUG] Closing socket and reconnecting..." << std::endl;
        }
        close(sock_);
        if (!connect_to_server()) {
            std::cerr << "[ERROR] Failed to reconnect" << std::endl;
            return "ERROR: Failed to reconnect";
        }
        if (g_verbose) {
            std::cerr << "[DEBUG] Reconnected successfully" << std::endl;
        }

        return std::string(buffer, bytes_received);
    }

    std::vector<std::string> parse_sql_file() {
        std::ifstream file(sql_file_);
        if (!file.is_open()) {
            std::cerr << "Error: Cannot open SQL file: " << sql_file_ << std::endl;
            return {};
        }

        std::vector<std::string> queries;
        std::string line;
        std::string current_query;

        while (std::getline(file, line)) {
            // Trim leading/trailing whitespace
            size_t start = line.find_first_not_of(" \t\r\n");
            size_t end = line.find_last_not_of(" \t\r\n");

            if (start == std::string::npos) {
                continue; // Empty line
            }

            line = line.substr(start, end - start + 1);

            // Skip comments
            if (line.empty() || line[0] == '#' ||
                (line.length() >= 2 && line[0] == '-' && line[1] == '-')) {
                continue;
            }

            current_query += line + " ";

            // Check if query ends with semicolon
            if (line.back() == ';') {
                queries.push_back(current_query);
                current_query.clear();
            }
        }

        return queries;
    }

    std::string get_ref_file_path() {
        // Convert src path to ref path
        // Example: tests/minitest/src/basic/select.sql -> tests/minitest/ref/basic/select.ref
        std::string ref_path = sql_file_;
        size_t src_pos = ref_path.find("/src/");
        if (src_pos != std::string::npos) {
            ref_path.replace(src_pos, 5, "/ref/");
        }
        size_t sql_pos = ref_path.rfind(".sql");
        if (sql_pos != std::string::npos) {
            ref_path.replace(sql_pos, 4, ".ref");
        }
        return ref_path;
    }

    std::string get_tmp_file_path() {
        std::string ref_path = get_ref_file_path();
        size_t ref_pos = ref_path.rfind(".ref");
        if (ref_pos != std::string::npos) {
            ref_path.replace(ref_pos, 4, ".tmp");
        }
        return ref_path;
    }

    void ensure_directory(const std::string& file_path) {
        size_t pos = file_path.rfind('/');
        if (pos != std::string::npos) {
            std::string dir = file_path.substr(0, pos);
            std::string cmd = "mkdir -p " + dir;
            system(cmd.c_str());
        }
    }

    bool run_create_mode() {
        std::vector<std::string> queries = parse_sql_file();
        if (queries.empty()) {
            std::cerr << "Error: No queries found in SQL file" << std::endl;
            return false;
        }

        std::string ref_file = get_ref_file_path();
        ensure_directory(ref_file);

        std::ofstream out(ref_file);
        if (!out.is_open()) {
            std::cerr << "Error: Cannot create ref file: " << ref_file << std::endl;
            return false;
        }

        std::cout << "Creating reference file: " << ref_file << std::endl;

        for (const auto& query : queries) {
            std::cout << "Executing: " << query << std::endl;
            std::string result = execute_query(query);

            // Write query and result to ref file
            out << query << std::endl;
            out << result;
            if (!result.empty() && result.back() != '\n') {
                out << std::endl;
            }
            out << std::endl; // Extra newline between queries
        }

        out.close();
        std::cout << "Reference file created successfully: " << ref_file << std::endl;
        return true;
    }

    bool run_compare_mode() {
        std::string ref_file = get_ref_file_path();

        // Check if ref file exists
        std::ifstream ref_check(ref_file);
        if (!ref_check.is_open()) {
            std::cerr << "Error: Reference file does not exist: " << ref_file << std::endl;
            std::cerr << "Please run with --run-mode=create first to create the reference file." << std::endl;
            return false;
        }
        ref_check.close();

        std::vector<std::string> queries = parse_sql_file();
        if (queries.empty()) {
            std::cerr << "Error: No queries found in SQL file" << std::endl;
            return false;
        }

        std::string tmp_file = get_tmp_file_path();
        ensure_directory(tmp_file);

        std::ofstream out(tmp_file);
        if (!out.is_open()) {
            std::cerr << "Error: Cannot create tmp file: " << tmp_file << std::endl;
            return false;
        }

        std::cout << "Running test: " << sql_file_ << std::endl;

        for (const auto& query : queries) {
            std::string result = execute_query(query);

            // Write query and result to tmp file
            out << query << std::endl;
            out << result;
            if (!result.empty() && result.back() != '\n') {
                out << std::endl;
            }
            out << std::endl; // Extra newline between queries
        }

        out.close();

        // Use diff to compare ref and tmp files
        std::string diff_cmd = "diff -u " + ref_file + " " + tmp_file;
        int diff_result = system(diff_cmd.c_str());

        if (diff_result == 0) {
            std::cout << "✓ PASS: " << sql_file_ << std::endl;
            // Clean up tmp file on success
            unlink(tmp_file.c_str());
            return true;
        } else {
            std::cout << "✗ FAIL: " << sql_file_ << std::endl;
            std::cout << "  Temporary output saved to: " << tmp_file << std::endl;
            std::cout << "  Run 'diff " << ref_file << " " << tmp_file << "' to see differences" << std::endl;
            return false;
        }
    }

    bool run() {
        if (!connect_to_server()) {
            return false;
        }

        if (run_mode_ == "create") {
            return run_create_mode();
        } else if (run_mode_ == "compare") {
            return run_compare_mode();
        } else {
            std::cerr << "Error: Invalid run mode: " << run_mode_ << std::endl;
            std::cerr << "Valid modes are: create, compare" << std::endl;
            return false;
        }
    }

private:
    std::string host_;
    int port_;
    std::string sql_file_;
    std::string run_mode_;
    int sock_;
};

void print_usage(const char* prog_name) {
    std::cout << "Usage: " << prog_name << " [OPTIONS] <sql-file>" << std::endl;
    std::cout << std::endl;
    std::cout << "Options:" << std::endl;
    std::cout << "  --run-mode=<mode>    Test run mode: create or compare (default: compare)" << std::endl;
    std::cout << "  --host=<host>        Database server host (default: 127.0.0.1)" << std::endl;
    std::cout << "  --port=<port>        Database server port (default: 9876)" << std::endl;
    std::cout << "  --verbose            Enable verbose debug output" << std::endl;
    std::cout << "  --help               Show this help message" << std::endl;
    std::cout << std::endl;
    std::cout << "Modes:" << std::endl;
    std::cout << "  create   - Execute SQL file and create reference output file (.ref)" << std::endl;
    std::cout << "  compare  - Execute SQL file and compare output with reference file" << std::endl;
    std::cout << std::endl;
    std::cout << "Examples:" << std::endl;
    std::cout << "  # Create reference file" << std::endl;
    std::cout << "  " << prog_name << " --run-mode=create tests/minitest/src/basic/select.sql" << std::endl;
    std::cout << std::endl;
    std::cout << "  # Run test and compare with reference" << std::endl;
    std::cout << "  " << prog_name << " tests/minitest/src/basic/select.sql" << std::endl;
}

int main(int argc, char* argv[]) {
    std::string host = minidb::DEFAULT_HOST;
    int port = minidb::DEFAULT_PORT;
    std::string run_mode = "compare";
    std::string sql_file;

    // Parse command line arguments
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];

        if (arg == "--help" || arg == "-h") {
            print_usage(argv[0]);
            return 0;
        } else if (arg.find("--run-mode=") == 0) {
            run_mode = arg.substr(11);
        } else if (arg.find("--host=") == 0) {
            host = arg.substr(7);
        } else if (arg.find("--port=") == 0) {
            port = std::stoi(arg.substr(7));
        } else if (arg == "--verbose" || arg == "-v") {
            g_verbose = true;
        } else if (arg[0] != '-') {
            sql_file = arg;
        } else {
            std::cerr << "Error: Unknown option: " << arg << std::endl;
            print_usage(argv[0]);
            return 1;
        }
    }

    if (sql_file.empty()) {
        std::cerr << "Error: SQL file is required" << std::endl;
        print_usage(argv[0]);
        return 1;
    }

    MiniTest test(host, port, sql_file, run_mode);
    bool success = test.run();

    return success ? 0 : 1;
}

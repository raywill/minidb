#pragma once

// MiniDB Configuration Constants
// This file contains configuration constants shared across all MiniDB binaries

namespace minidb {

// Default network port for MiniDB server and clients
constexpr int DEFAULT_PORT = 9876;

// Default server host for clients
constexpr const char* DEFAULT_HOST = "127.0.0.1";

// Default data directory for server
constexpr const char* DEFAULT_DATA_DIR = "./data";

}  // namespace minidb

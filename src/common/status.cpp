#include "common/status.h"

namespace minidb {

std::string Status::ToString() const {
    if (ok()) {
        return "OK";
    }
    
    std::string code_str;
    switch (code_) {
        case StatusCode::INVALID_ARGUMENT:
            code_str = "INVALID_ARGUMENT";
            break;
        case StatusCode::NOT_FOUND:
            code_str = "NOT_FOUND";
            break;
        case StatusCode::ALREADY_EXISTS:
            code_str = "ALREADY_EXISTS";
            break;
        case StatusCode::IO_ERROR:
            code_str = "IO_ERROR";
            break;
        case StatusCode::MEMORY_ERROR:
            code_str = "MEMORY_ERROR";
            break;
        case StatusCode::PARSE_ERROR:
            code_str = "PARSE_ERROR";
            break;
        case StatusCode::EXECUTION_ERROR:
            code_str = "EXECUTION_ERROR";
            break;
        case StatusCode::NETWORK_ERROR:
            code_str = "NETWORK_ERROR";
            break;
        case StatusCode::INTERNAL_ERROR:
            code_str = "INTERNAL_ERROR";
            break;
        default:
            code_str = "UNKNOWN";
            break;
    }
    
    return code_str + ": " + message_;
}

} // namespace minidb

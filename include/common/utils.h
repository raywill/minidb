#pragma once

#include <memory>

namespace minidb {

// C++11兼容的make_unique实现
template<typename T, typename... Args>
std::unique_ptr<T> make_unique(Args&&... args) {
    return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
}

} // namespace minidb

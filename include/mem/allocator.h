#pragma once

#include <cstddef>
#include <memory>
#include <atomic>
#include <mutex>

namespace minidb {

// 内存分配器接口
class Allocator {
public:
    virtual ~Allocator() = default;
    
    // 分配内存
    virtual void* allocate(size_t size) = 0;
    
    // 释放内存
    virtual void free(void* ptr) = 0;
    
    // 获取已分配的内存大小
    virtual size_t allocated_bytes() const = 0;
    
    // 获取分配次数
    virtual size_t allocation_count() const = 0;
};

// 默认分配器实现
class DefaultAllocator : public Allocator {
public:
    DefaultAllocator();
    ~DefaultAllocator() override;
    
    void* allocate(size_t size) override;
    void free(void* ptr) override;
    size_t allocated_bytes() const override;
    size_t allocation_count() const override;
    
    // 获取全局默认分配器实例
    static DefaultAllocator* instance();
    
private:
    mutable std::mutex mutex_;
    std::atomic<size_t> allocated_bytes_;
    std::atomic<size_t> allocation_count_;
    
    static DefaultAllocator* instance_;
    static std::once_flag init_flag_;
};

// 内存池分配器
class PoolAllocator : public Allocator {
public:
    explicit PoolAllocator(size_t pool_size = 1024 * 1024); // 默认1MB
    ~PoolAllocator() override;
    
    void* allocate(size_t size) override;
    void free(void* ptr) override;
    size_t allocated_bytes() const override;
    size_t allocation_count() const override;
    
    // 重置内存池
    void reset();
    
private:
    struct Block {
        void* ptr;
        size_t size;
        bool in_use;
        Block* next;
        
        Block(void* p, size_t s) : ptr(p), size(s), in_use(false), next(nullptr) {}
    };
    
    std::mutex mutex_;
    uint8_t* pool_;
    size_t pool_size_;
    size_t current_offset_;
    Block* free_blocks_;
    std::atomic<size_t> allocated_bytes_;
    std::atomic<size_t> allocation_count_;
    
    Block* find_free_block(size_t size);
    void split_block(Block* block, size_t size);
    void merge_free_blocks();
};

} // namespace minidb

#pragma once

#include "mem/allocator.h"
#include "common/status.h"
#include <vector>
#include <memory>

namespace minidb {

// Arena内存管理器 - 用于短期内存生命周期管理
class Arena {
public:
    explicit Arena(Allocator* allocator = nullptr);
    ~Arena();
    
    // 禁止拷贝和赋值
    Arena(const Arena&) = delete;
    Arena& operator=(const Arena&) = delete;
    
    // 分配内存
    void* allocate(size_t size);
    
    // 分配对齐内存
    void* allocate_aligned(size_t size, size_t alignment = 8);
    
    // 重置Arena，释放所有内存
    void reset();
    
    // 获取已分配的内存大小
    size_t allocated_bytes() const { return allocated_bytes_; }
    
    // 获取分配次数
    size_t allocation_count() const { return allocation_count_; }
    
    // 创建对象（使用placement new）
    template<typename T, typename... Args>
    T* create(Args&&... args) {
        void* ptr = allocate_aligned(sizeof(T), alignof(T));
        return new(ptr) T(std::forward<Args>(args)...);
    }
    
    // 创建数组
    template<typename T>
    T* create_array(size_t count) {
        void* ptr = allocate_aligned(sizeof(T) * count, alignof(T));
        T* array = static_cast<T*>(ptr);
        for (size_t i = 0; i < count; ++i) {
            new(array + i) T();
        }
        return array;
    }

private:
    struct Block {
        uint8_t* data;
        size_t size;
        size_t offset;
        
        Block(size_t s) : size(s), offset(0) {
            data = static_cast<uint8_t*>(std::malloc(s));
            if (!data) {
                throw DatabaseException(Status::MemoryError("Failed to allocate arena block"));
            }
        }
        
        ~Block() {
            if (data) {
                std::free(data);
            }
        }
        
        void* allocate(size_t bytes) {
            if (offset + bytes > size) {
                return nullptr; // 空间不足
            }
            void* ptr = data + offset;
            offset += bytes;
            return ptr;
        }
        
        void* allocate_aligned(size_t bytes, size_t alignment) {
            size_t aligned_offset = (offset + alignment - 1) & ~(alignment - 1);
            if (aligned_offset + bytes > size) {
                return nullptr; // 空间不足
            }
            void* ptr = data + aligned_offset;
            offset = aligned_offset + bytes;
            return ptr;
        }
        
        void reset() {
            offset = 0;
        }
    };
    
    Allocator* allocator_;
    std::vector<std::unique_ptr<Block>> blocks_;
    size_t current_block_size_;
    size_t allocated_bytes_;
    size_t allocation_count_;
    
    static const size_t DEFAULT_BLOCK_SIZE = 4096; // 4KB
    static const size_t MAX_BLOCK_SIZE = 1024 * 1024; // 1MB
    
    Block* get_or_create_block(size_t required_size);
};

// RAII Arena管理器
class ScopedArena {
public:
    explicit ScopedArena(Allocator* allocator = nullptr) 
        : arena_(allocator) {}
    
    ~ScopedArena() {
        arena_.reset();
    }
    
    Arena* get() { return &arena_; }
    Arena* operator->() { return &arena_; }
    Arena& operator*() { return arena_; }
    
private:
    Arena arena_;
};

} // namespace minidb

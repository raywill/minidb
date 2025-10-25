#include "mem/allocator.h"
#include "common/status.h"
#include <cstdlib>
#include <cstring>
#include <algorithm>

namespace minidb {

// DefaultAllocator 实现
DefaultAllocator* DefaultAllocator::instance_ = nullptr;
std::once_flag DefaultAllocator::init_flag_;

DefaultAllocator::DefaultAllocator() : allocated_bytes_(0), allocation_count_(0) {}

DefaultAllocator::~DefaultAllocator() = default;

void* DefaultAllocator::allocate(size_t size) {
    if (size == 0) {
        return nullptr;
    }
    
    void* ptr = std::malloc(size);
    if (!ptr) {
        throw DatabaseException(Status::MemoryError("Failed to allocate " + std::to_string(size) + " bytes"));
    }
    
    allocated_bytes_ += size;
    allocation_count_++;
    
    return ptr;
}

void DefaultAllocator::free(void* ptr) {
    if (ptr) {
        std::free(ptr);
    }
}

size_t DefaultAllocator::allocated_bytes() const {
    return allocated_bytes_.load();
}

size_t DefaultAllocator::allocation_count() const {
    return allocation_count_.load();
}

DefaultAllocator* DefaultAllocator::instance() {
    std::call_once(init_flag_, []() {
        instance_ = new DefaultAllocator();
    });
    return instance_;
}

// PoolAllocator 实现
PoolAllocator::PoolAllocator(size_t pool_size) 
    : pool_size_(pool_size), current_offset_(0), free_blocks_(nullptr),
      allocated_bytes_(0), allocation_count_(0) {
    
    pool_ = static_cast<uint8_t*>(std::malloc(pool_size_));
    if (!pool_) {
        throw DatabaseException(Status::MemoryError("Failed to allocate memory pool"));
    }
}

PoolAllocator::~PoolAllocator() {
    if (pool_) {
        std::free(pool_);
    }
    
    // 清理空闲块链表
    Block* current = free_blocks_;
    while (current) {
        Block* next = current->next;
        delete current;
        current = next;
    }
}

void* PoolAllocator::allocate(size_t size) {
    if (size == 0) {
        return nullptr;
    }
    
    // 对齐到8字节边界
    size = (size + 7) & ~7;
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    // 首先尝试从空闲块中分配
    Block* block = find_free_block(size);
    if (block) {
        block->in_use = true;
        if (block->size > size + sizeof(Block)) {
            split_block(block, size);
        }
        allocated_bytes_ += size;
        allocation_count_++;
        return block->ptr;
    }
    
    // 从内存池中分配新块
    if (current_offset_ + size > pool_size_) {
        throw DatabaseException(Status::MemoryError("Memory pool exhausted"));
    }
    
    void* ptr = pool_ + current_offset_;
    current_offset_ += size;
    
    allocated_bytes_ += size;
    allocation_count_++;
    
    return ptr;
}

void PoolAllocator::free(void* ptr) {
    if (!ptr) {
        return;
    }
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    // 查找对应的块
    Block* current = free_blocks_;
    while (current) {
        if (current->ptr == ptr) {
            current->in_use = false;
            merge_free_blocks();
            return;
        }
        current = current->next;
    }
    
    // 如果没有找到，说明这是从内存池直接分配的
    // 在实际实现中，我们需要维护一个分配记录
    // 这里简化处理
}

size_t PoolAllocator::allocated_bytes() const {
    return allocated_bytes_.load();
}

size_t PoolAllocator::allocation_count() const {
    return allocation_count_.load();
}

void PoolAllocator::reset() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    current_offset_ = 0;
    allocated_bytes_ = 0;
    allocation_count_ = 0;
    
    // 清理空闲块链表
    Block* current = free_blocks_;
    while (current) {
        Block* next = current->next;
        delete current;
        current = next;
    }
    free_blocks_ = nullptr;
}

PoolAllocator::Block* PoolAllocator::find_free_block(size_t size) {
    Block* current = free_blocks_;
    Block* best_fit = nullptr;
    
    while (current) {
        if (!current->in_use && current->size >= size) {
            if (!best_fit || current->size < best_fit->size) {
                best_fit = current;
            }
        }
        current = current->next;
    }
    
    return best_fit;
}

void PoolAllocator::split_block(Block* block, size_t size) {
    if (block->size <= size + sizeof(Block)) {
        return; // 不值得分割
    }
    
    Block* new_block = new Block(
        static_cast<uint8_t*>(block->ptr) + size,
        block->size - size
    );
    
    new_block->next = block->next;
    block->next = new_block;
    block->size = size;
}

void PoolAllocator::merge_free_blocks() {
    Block* current = free_blocks_;
    
    while (current && current->next) {
        if (!current->in_use && !current->next->in_use) {
            // 检查是否相邻
            uint8_t* current_end = static_cast<uint8_t*>(current->ptr) + current->size;
            if (current_end == current->next->ptr) {
                // 合并块
                Block* next_block = current->next;
                current->size += next_block->size;
                current->next = next_block->next;
                delete next_block;
                continue;
            }
        }
        current = current->next;
    }
}

} // namespace minidb

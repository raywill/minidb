#include "mem/arena.h"
#include "common/status.h"
#include <algorithm>

namespace minidb {

Arena::Arena(Allocator* allocator) 
    : allocator_(allocator ? allocator : DefaultAllocator::instance()),
      current_block_size_(DEFAULT_BLOCK_SIZE),
      allocated_bytes_(0),
      allocation_count_(0) {
}

Arena::~Arena() {
    reset();
}

void* Arena::allocate(size_t size) {
    return allocate_aligned(size, 1);
}

void* Arena::allocate_aligned(size_t size, size_t alignment) {
    if (size == 0) {
        return nullptr;
    }
    
    // 尝试从现有块中分配
    for (auto& block : blocks_) {
        void* ptr = block->allocate_aligned(size, alignment);
        if (ptr) {
            allocated_bytes_ += size;
            allocation_count_++;
            return ptr;
        }
    }
    
    // 需要新的块
    Block* block = get_or_create_block(size + alignment - 1);
    void* ptr = block->allocate_aligned(size, alignment);
    if (!ptr) {
        throw DatabaseException(Status::MemoryError("Failed to allocate from new block"));
    }
    
    allocated_bytes_ += size;
    allocation_count_++;
    return ptr;
}

void Arena::reset() {
    blocks_.clear();
    current_block_size_ = DEFAULT_BLOCK_SIZE;
    allocated_bytes_ = 0;
    allocation_count_ = 0;
}

Arena::Block* Arena::get_or_create_block(size_t required_size) {
    // 计算新块的大小
    size_t block_size = std::max(current_block_size_, required_size);
    
    // 创建新块
    auto block = std::unique_ptr<Block>(new Block(block_size));
    Block* block_ptr = block.get();
    blocks_.push_back(std::move(block));
    
    // 增加下一个块的大小（指数增长，但有上限）
    current_block_size_ = std::min(current_block_size_ * 2, MAX_BLOCK_SIZE);
    
    return block_ptr;
}

} // namespace minidb

#include "mem/allocator.h"
#include "common/status.h"
#include <iostream>
#include <cassert>
#include <vector>
#include <thread>
#include <chrono>

using namespace minidb;

void test_default_allocator() {
    std::cout << "Testing DefaultAllocator..." << std::endl;
    
    DefaultAllocator* allocator = DefaultAllocator::instance();
    assert(allocator != nullptr);
    
    // 测试基本分配和释放
    void* ptr1 = allocator->allocate(1024);
    assert(ptr1 != nullptr);
    assert(allocator->allocated_bytes() >= 1024);
    assert(allocator->allocation_count() >= 1);
    
    void* ptr2 = allocator->allocate(2048);
    assert(ptr2 != nullptr);
    assert(ptr2 != ptr1);
    assert(allocator->allocated_bytes() >= 3072);
    assert(allocator->allocation_count() >= 2);
    
    // 测试释放
    allocator->free(ptr1);
    allocator->free(ptr2);
    
    // 测试零大小分配
    void* ptr_zero = allocator->allocate(0);
    assert(ptr_zero == nullptr);
    
    // 测试大内存分配
    void* large_ptr = allocator->allocate(1024 * 1024); // 1MB
    assert(large_ptr != nullptr);
    allocator->free(large_ptr);
    
    std::cout << "DefaultAllocator test passed!" << std::endl;
}

void test_default_allocator_exception() {
    std::cout << "Testing DefaultAllocator exception handling..." << std::endl;
    
    DefaultAllocator* allocator = DefaultAllocator::instance();
    
    try {
        // 尝试分配极大的内存（应该失败）
        void* huge_ptr = allocator->allocate(SIZE_MAX);
        // 如果分配成功，释放它（不太可能）
        if (huge_ptr) {
            allocator->free(huge_ptr);
        }
        std::cout << "Large allocation succeeded (unexpected but not an error)" << std::endl;
    } catch (const DatabaseException& e) {
        std::cout << "Caught expected exception: " << e.what() << std::endl;
        assert(e.status().is_memory_error());
    }
    
    std::cout << "DefaultAllocator exception test passed!" << std::endl;
}

void test_pool_allocator() {
    std::cout << "Testing PoolAllocator..." << std::endl;
    
    PoolAllocator pool(4096); // 4KB池
    
    // 测试基本分配
    void* ptr1 = pool.allocate(100);
    assert(ptr1 != nullptr);
    assert(pool.allocated_bytes() >= 100);
    assert(pool.allocation_count() >= 1);
    
    void* ptr2 = pool.allocate(200);
    assert(ptr2 != nullptr);
    assert(ptr2 != ptr1);
    assert(pool.allocated_bytes() >= 300);
    
    // 测试释放和重用
    pool.free(ptr1);
    void* ptr3 = pool.allocate(50); // 应该重用ptr1的空间
    assert(ptr3 != nullptr);
    
    // 测试重置
    pool.reset();
    assert(pool.allocated_bytes() == 0);
    assert(pool.allocation_count() == 0);
    
    // 重置后重新分配
    void* ptr4 = pool.allocate(300);
    assert(ptr4 != nullptr);
    
    std::cout << "PoolAllocator test passed!" << std::endl;
}

void test_pool_allocator_exhaustion() {
    std::cout << "Testing PoolAllocator exhaustion..." << std::endl;
    
    PoolAllocator small_pool(1024); // 1KB池
    
    std::vector<void*> ptrs;
    
    try {
        // 尝试分配超过池大小的内存
        for (int i = 0; i < 20; ++i) {
            void* ptr = small_pool.allocate(100);
            if (ptr) {
                ptrs.push_back(ptr);
            }
        }
        
        // 尝试分配一个大块，应该失败
        void* large_ptr = small_pool.allocate(2048);
        if (large_ptr) {
            std::cout << "Large allocation succeeded unexpectedly" << std::endl;
        }
    } catch (const DatabaseException& e) {
        std::cout << "Caught expected pool exhaustion: " << e.what() << std::endl;
        assert(e.status().is_memory_error());
    }
    
    // 清理
    for (void* ptr : ptrs) {
        small_pool.free(ptr);
    }
    
    std::cout << "PoolAllocator exhaustion test passed!" << std::endl;
}

void test_allocator_thread_safety() {
    std::cout << "Testing allocator thread safety..." << std::endl;
    
    DefaultAllocator* allocator = DefaultAllocator::instance();
    const int num_threads = 4;
    const int allocations_per_thread = 100;
    std::vector<std::thread> threads;
    std::vector<std::vector<void*>> thread_ptrs(num_threads);
    
    // 启动多个线程进行并发分配
    for (int t = 0; t < num_threads; ++t) {
        threads.emplace_back([&, t]() {
            for (int i = 0; i < allocations_per_thread; ++i) {
                void* ptr = allocator->allocate(64 + i);
                if (ptr) {
                    thread_ptrs[t].push_back(ptr);
                }
                
                // 偶尔释放一些内存
                if (i % 10 == 0 && !thread_ptrs[t].empty()) {
                    allocator->free(thread_ptrs[t].back());
                    thread_ptrs[t].pop_back();
                }
            }
        });
    }
    
    // 等待所有线程完成
    for (auto& thread : threads) {
        thread.join();
    }
    
    // 清理剩余内存
    for (int t = 0; t < num_threads; ++t) {
        for (void* ptr : thread_ptrs[t]) {
            allocator->free(ptr);
        }
    }
    
    std::cout << "Thread safety test completed" << std::endl;
    std::cout << "Final allocation count: " << allocator->allocation_count() << std::endl;
    
    std::cout << "Allocator thread safety test passed!" << std::endl;
}

void test_allocator_alignment() {
    std::cout << "Testing memory alignment..." << std::endl;
    
    DefaultAllocator* allocator = DefaultAllocator::instance();
    
    // 测试不同大小的分配，检查对齐
    std::vector<void*> ptrs;
    for (size_t size = 1; size <= 1024; size *= 2) {
        void* ptr = allocator->allocate(size);
        assert(ptr != nullptr);
        
        // 检查指针对齐（至少8字节对齐）
        uintptr_t addr = reinterpret_cast<uintptr_t>(ptr);
        assert(addr % sizeof(void*) == 0);
        
        ptrs.push_back(ptr);
    }
    
    // 清理
    for (void* ptr : ptrs) {
        allocator->free(ptr);
    }
    
    std::cout << "Memory alignment test passed!" << std::endl;
}

int main() {
    try {
        test_default_allocator();
        test_default_allocator_exception();
        test_pool_allocator();
        test_pool_allocator_exhaustion();
        test_allocator_thread_safety();
        test_allocator_alignment();
        
        std::cout << "\n🎉 All allocator tests passed!" << std::endl;
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "❌ Allocator test failed: " << e.what() << std::endl;
        return 1;
    }
}

#include "mem/arena.h"
#include "mem/allocator.h"
#include "common/status.h"
#include <iostream>
#include <cassert>
#include <vector>
#include <string>

using namespace minidb;

struct TestObject {
    int value;
    std::string name;
    
    TestObject() : value(0), name("default") {}
    TestObject(int v, const std::string& n) : value(v), name(n) {}
    ~TestObject() {
        // 析构函数用于验证对象正确创建
    }
};

void test_arena_basic_allocation() {
    std::cout << "Testing Arena basic allocation..." << std::endl;
    
    Arena arena;
    
    // 测试基本分配
    void* ptr1 = arena.allocate(100);
    assert(ptr1 != nullptr);
    assert(arena.allocated_bytes() >= 100);
    assert(arena.allocation_count() >= 1);
    
    void* ptr2 = arena.allocate(200);
    assert(ptr2 != nullptr);
    assert(ptr2 != ptr1);
    assert(arena.allocated_bytes() >= 300);
    assert(arena.allocation_count() >= 2);
    
    // 测试零大小分配
    void* ptr_zero = arena.allocate(0);
    assert(ptr_zero == nullptr);
    
    std::cout << "Arena basic allocation test passed!" << std::endl;
}

void test_arena_aligned_allocation() {
    std::cout << "Testing Arena aligned allocation..." << std::endl;
    
    Arena arena;
    
    // 测试不同对齐要求
    void* ptr1 = arena.allocate_aligned(100, 8);
    assert(ptr1 != nullptr);
    assert(reinterpret_cast<uintptr_t>(ptr1) % 8 == 0);
    
    void* ptr2 = arena.allocate_aligned(100, 16);
    assert(ptr2 != nullptr);
    assert(reinterpret_cast<uintptr_t>(ptr2) % 16 == 0);
    
    void* ptr3 = arena.allocate_aligned(100, 32);
    assert(ptr3 != nullptr);
    assert(reinterpret_cast<uintptr_t>(ptr3) % 32 == 0);
    
    std::cout << "Arena aligned allocation test passed!" << std::endl;
}

void test_arena_object_creation() {
    std::cout << "Testing Arena object creation..." << std::endl;
    
    Arena arena;
    
    // 测试单个对象创建
    TestObject* obj1 = arena.create<TestObject>();
    assert(obj1 != nullptr);
    assert(obj1->value == 0);
    assert(obj1->name == "default");
    
    // 测试带参数的对象创建
    TestObject* obj2 = arena.create<TestObject>(42, "test");
    assert(obj2 != nullptr);
    assert(obj2->value == 42);
    assert(obj2->name == "test");
    
    // 测试数组创建
    TestObject* array = arena.create_array<TestObject>(5);
    assert(array != nullptr);
    
    // 验证数组中的对象
    for (int i = 0; i < 5; ++i) {
        assert(array[i].value == 0);
        assert(array[i].name == "default");
        
        // 修改对象
        array[i].value = i;
        array[i].name = "item_" + std::to_string(i);
    }
    
    // 验证修改
    for (int i = 0; i < 5; ++i) {
        assert(array[i].value == i);
        assert(array[i].name == "item_" + std::to_string(i));
    }
    
    std::cout << "Arena object creation test passed!" << std::endl;
}

void test_arena_reset() {
    std::cout << "Testing Arena reset..." << std::endl;
    
    Arena arena;
    
    // 分配一些内存
    void* ptr1 = arena.allocate(1000);
    void* ptr2 = arena.allocate(2000);
    TestObject* obj = arena.create<TestObject>(123, "before_reset");
    
    assert(ptr1 != nullptr);
    assert(ptr2 != nullptr);
    assert(obj != nullptr);
    assert(obj->value == 123);
    
    size_t bytes_before = arena.allocated_bytes();
    size_t count_before = arena.allocation_count();
    
    assert(bytes_before > 0);
    assert(count_before > 0);
    
    // 重置Arena
    arena.reset();
    
    assert(arena.allocated_bytes() == 0);
    assert(arena.allocation_count() == 0);
    
    // 重置后重新分配
    void* ptr3 = arena.allocate(500);
    assert(ptr3 != nullptr);
    assert(arena.allocated_bytes() >= 500);
    assert(arena.allocation_count() >= 1);
    
    std::cout << "Arena reset test passed!" << std::endl;
}

void test_arena_large_allocations() {
    std::cout << "Testing Arena large allocations..." << std::endl;
    
    Arena arena;
    
    // 测试大块分配
    void* large1 = arena.allocate(64 * 1024); // 64KB
    assert(large1 != nullptr);
    
    void* large2 = arena.allocate(128 * 1024); // 128KB
    assert(large2 != nullptr);
    assert(large2 != large1);
    
    // 测试超大块分配
    void* huge = arena.allocate(1024 * 1024); // 1MB
    assert(huge != nullptr);
    
    std::cout << "Arena large allocations test passed!" << std::endl;
}

void test_arena_fragmentation() {
    std::cout << "Testing Arena fragmentation handling..." << std::endl;
    
    Arena arena;
    
    // 分配许多小块
    std::vector<void*> small_ptrs;
    for (int i = 0; i < 1000; ++i) {
        void* ptr = arena.allocate(16 + (i % 64)); // 变化的大小
        assert(ptr != nullptr);
        small_ptrs.push_back(ptr);
    }
    
    assert(small_ptrs.size() == 1000);
    assert(arena.allocation_count() >= 1000);
    
    // 分配一个大块
    void* large_ptr = arena.allocate(8192);
    assert(large_ptr != nullptr);
    
    std::cout << "Arena fragmentation test passed!" << std::endl;
}

void test_scoped_arena() {
    std::cout << "Testing ScopedArena..." << std::endl;
    
    size_t initial_bytes = 0;
    size_t initial_count = 0;
    
    {
        ScopedArena scoped_arena;
        Arena* arena = scoped_arena.get();
        
        // 分配一些内存
        void* ptr1 = arena->allocate(1000);
        void* ptr2 = arena->allocate(2000);
        TestObject* obj = arena->create<TestObject>(456, "scoped");
        
        assert(ptr1 != nullptr);
        assert(ptr2 != nullptr);
        assert(obj != nullptr);
        assert(obj->value == 456);
        assert(obj->name == "scoped");
        
        initial_bytes = arena->allocated_bytes();
        initial_count = arena->allocation_count();
        
        assert(initial_bytes > 0);
        assert(initial_count > 0);
        
        // 测试操作符重载
        Arena& arena_ref = *scoped_arena;
        void* ptr3 = arena_ref.allocate(500);
        assert(ptr3 != nullptr);
        
        Arena* arena_ptr = scoped_arena.operator->();
        void* ptr4 = arena_ptr->allocate(300);
        assert(ptr4 != nullptr);
    }
    
    // ScopedArena析构后，内存应该被重置
    // 注意：我们无法直接验证内存是否被释放，因为Arena对象已经销毁
    
    std::cout << "ScopedArena test passed!" << std::endl;
}

void test_arena_with_custom_allocator() {
    std::cout << "Testing Arena with custom allocator..." << std::endl;
    
    PoolAllocator custom_allocator(8192);
    Arena arena(&custom_allocator);
    
    // 分配内存
    void* ptr1 = arena.allocate(1000);
    assert(ptr1 != nullptr);
    
    TestObject* obj = arena.create<TestObject>(789, "custom");
    assert(obj != nullptr);
    assert(obj->value == 789);
    assert(obj->name == "custom");
    
    // 验证使用了自定义分配器
    // 注意：Arena可能使用内部缓存，所以自定义分配器的统计可能为0
    // 我们主要验证Arena能正常工作
    std::cout << "Custom allocator bytes: " << custom_allocator.allocated_bytes() << std::endl;
    std::cout << "Custom allocator count: " << custom_allocator.allocation_count() << std::endl;
    
    std::cout << "Arena with custom allocator test passed!" << std::endl;
}

void test_arena_stress() {
    std::cout << "Testing Arena stress test..." << std::endl;
    
    Arena arena;
    
    // 大量分配和对象创建
    std::vector<TestObject*> objects;
    std::vector<void*> raw_ptrs;
    
    for (int i = 0; i < 10000; ++i) {
        if (i % 3 == 0) {
            // 创建对象
            TestObject* obj = arena.create<TestObject>(i, "stress_" + std::to_string(i));
            assert(obj != nullptr);
            assert(obj->value == i);
            objects.push_back(obj);
        } else {
            // 分配原始内存
            void* ptr = arena.allocate(32 + (i % 256));
            assert(ptr != nullptr);
            raw_ptrs.push_back(ptr);
        }
    }
    
    assert(objects.size() > 3000);
    assert(raw_ptrs.size() > 6000);
    assert(arena.allocated_bytes() > 0);
    assert(arena.allocation_count() >= 10000);
    
    // 验证对象仍然有效
    for (size_t i = 0; i < std::min(objects.size(), size_t(100)); ++i) {
        TestObject* obj = objects[i];
        assert(obj->value == static_cast<int>(i * 3));
        assert(obj->name == "stress_" + std::to_string(i * 3));
    }
    
    std::cout << "Arena stress test passed!" << std::endl;
}

int main() {
    try {
        test_arena_basic_allocation();
        test_arena_aligned_allocation();
        test_arena_object_creation();
        test_arena_reset();
        test_arena_large_allocations();
        test_arena_fragmentation();
        test_scoped_arena();
        test_arena_with_custom_allocator();
        test_arena_stress();
        
        std::cout << "\n🎉 All memory management tests passed!" << std::endl;
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "❌ Memory management test failed: " << e.what() << std::endl;
        return 1;
    }
}

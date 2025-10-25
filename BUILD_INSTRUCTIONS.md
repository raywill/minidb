# MiniDB 构建说明

## 🎯 完整命令链条

MiniDB支持两种构建系统，您可以根据需要选择：

### 方案1: 传统Makefile（推荐，无需额外依赖）

```bash
# 完整构建流程
make clean              # 1. 清理构建产物
make all               # 2. 编译所有目标（服务器+客户端+测试）
make test_unit         # 3. 运行单元测试（可选）

# 一键构建命令
make clean && make all
```

### 方案2: 现代CMake（需要安装CMake）

```bash
# 安装CMake（macOS）
brew install cmake

# 安装CMake（Ubuntu/Debian）
sudo apt-get install cmake

# 完整构建流程
mkdir -p build && cd build    # 1. 创建构建目录
cmake -DCMAKE_BUILD_TYPE=Release ..  # 2. CMake配置
cmake --build . --parallel   # 3. 编译
ctest                        # 4. 运行测试
```

## 🚀 快速开始（推荐使用Makefile）

由于您的系统当前没有CMake，我们使用现有的Makefile系统：

### 1. 完整构建

```bash
cd /Users/rayu/code/raywill/minidb

# 清理并重新构建
make clean
make all

# 验证构建结果
ls -la dbserver dbcli test_*
```

### 2. 运行系统

```bash
# 启动服务器（终端1）
./dbserver --port 9876

# 启动客户端（终端2）
./dbcli --host localhost --port 9876
```

### 3. 测试历史记录功能

```bash
# 在客户端中：
# 1. 输入SQL命令：CREATE TABLE test(id INT);
# 2. 按↑箭头键，应该能看到之前的命令
# 3. 输入 'history' 查看历史记录
# 4. 输入 'help' 查看所有功能
```

## 📋 详细构建流程

### Makefile构建系统

#### 构建目标

```bash
# 核心可执行文件
make dbserver          # 数据库服务器
make dbcli            # 数据库客户端（包含历史记录功能）

# 单元测试
make test_types       # 基础类型测试
make test_parser      # SQL解析器测试
make test_allocator   # 内存分配器测试
make test_arena       # Arena内存管理测试
make test_logger      # 日志系统测试
make test_storage_simple  # 存储引擎测试（简化版）
make test_network_simple  # 网络通信测试（简化版）
make test_command_history # 命令历史记录测试
make test_parser_extended # 扩展解析器测试
make test_crash_handler_extended # 崩溃处理器测试

# 批量目标
make all              # 构建所有目标
make test_unit        # 运行所有单元测试
make clean            # 清理构建产物
```

#### 构建过程详解

```bash
# 1. 源码编译阶段
# 编译所有.cpp文件为.o目标文件
g++ -std=c++11 -Wall -Wextra -O2 -g -Iinclude -Isrc -c src/common/crash_handler.cpp -o src/common/crash_handler.o
g++ -std=c++11 -Wall -Wextra -O2 -g -Iinclude -Isrc -c src/client/command_history.cpp -o src/client/command_history.o
# ... 更多源文件编译

# 2. 链接阶段
# 将目标文件链接为可执行文件
g++ -std=c++11 -Wall -Wextra -O2 -g -o dbserver [所有.o文件] src/server/main.o -lpthread
g++ -std=c++11 -Wall -Wextra -O2 -g -o dbcli [所有.o文件] src/client/main.o -lpthread

# 3. 测试程序链接
g++ -std=c++11 -Wall -Wextra -O2 -g -o test_command_history [所有.o文件] tests/unit/test_command_history.o -lpthread
```

### CMake构建系统（可选）

如果您想使用CMake，需要先安装：

#### 安装CMake

```bash
# macOS (使用Homebrew)
brew install cmake

# Ubuntu/Debian
sudo apt-get update
sudo apt-get install cmake

# CentOS/RHEL
sudo yum install cmake

# 或者从源码安装
wget https://cmake.org/files/v3.20/cmake-3.20.0.tar.gz
tar -xzf cmake-3.20.0.tar.gz
cd cmake-3.20.0
./configure && make && sudo make install
```

#### CMake构建流程

```bash
# 1. 配置阶段（生成构建文件）
mkdir -p build
cd build
cmake -DCMAKE_BUILD_TYPE=Release ..

# 2. 编译阶段
cmake --build . --parallel $(nproc)

# 3. 测试阶段
ctest --output-on-failure

# 4. 安装阶段（可选）
sudo cmake --install . --prefix /usr/local

# 5. 打包阶段（可选）
cpack -G ZIP
```

## 🔧 开发工作流

### 日常开发

```bash
# 1. 修改代码后重新编译
make clean && make all

# 2. 运行特定测试
./test_command_history
./test_parser
./test_storage_simple

# 3. 运行所有测试
make test_unit

# 4. 调试模式编译
make CXXFLAGS="-std=c++11 -Wall -Wextra -g -O0 -DDEBUG" all
```

### 性能优化构建

```bash
# Release模式（默认）
make CXXFLAGS="-std=c++11 -Wall -Wextra -O2 -DNDEBUG" all

# 最大优化
make CXXFLAGS="-std=c++11 -Wall -Wextra -O3 -DNDEBUG -march=native" all
```

### 调试构建

```bash
# Debug模式
make CXXFLAGS="-std=c++11 -Wall -Wextra -g -O0 -DDEBUG" all

# 使用GDB调试
gdb ./dbserver
(gdb) run --port 9876
(gdb) bt  # 查看调用栈
```

## 📊 构建验证

### 验证脚本

```bash
#!/bin/bash
echo "🧪 验证MiniDB构建..."

# 检查可执行文件
if [ -x "./dbserver" ] && [ -x "./dbcli" ]; then
    echo "✅ 核心可执行文件构建成功"
    echo "   dbserver: $(ls -lh dbserver | awk '{print $5}')"
    echo "   dbcli: $(ls -lh dbcli | awk '{print $5}')"
else
    echo "❌ 核心可执行文件构建失败"
    exit 1
fi

# 检查测试程序
test_count=$(ls test_* 2>/dev/null | wc -l | tr -d ' ')
echo "✅ 测试程序: $test_count 个"

# 运行基本测试
echo "🧪 运行基本测试..."
./test_command_history && echo "✅ 命令历史测试通过"
./test_types && echo "✅ 基础类型测试通过"
./test_parser && echo "✅ SQL解析器测试通过"

echo "🎉 MiniDB构建验证完成！"
```

## 🎯 故障排除

### 常见编译错误

1. **找不到头文件**
   ```bash
   # 确保包含路径正确
   make INCLUDES="-Iinclude -Isrc" all
   ```

2. **链接错误**
   ```bash
   # 确保pthread库链接
   make LDFLAGS="-lpthread" all
   ```

3. **C++标准问题**
   ```bash
   # 确保使用C++11
   make CXXFLAGS="-std=c++11" all
   ```

### 清理和重建

```bash
# 完全清理
make clean
rm -f *.o */*.o */*/*.o

# 强制重建
make clean && make all -j$(nproc)
```

## 🎉 总结

### Makefile方式（当前推荐）

**优点:**
- ✅ 无需额外依赖
- ✅ 构建速度快
- ✅ 配置简单
- ✅ 适合小型项目

**完整命令链条:**
```bash
make clean → make all → ./dbserver & ./dbcli
```

### CMake方式（未来推荐）

**优点:**
- ✅ 跨平台支持
- ✅ 现代构建系统
- ✅ IDE集成好
- ✅ 扩展性强

**完整命令链条:**
```bash
mkdir build → cd build → cmake .. → cmake --build . → ctest
```

**当前您可以使用Makefile系统快速开始，未来如果需要跨平台支持或更复杂的构建需求，可以迁移到CMake！** 🚀

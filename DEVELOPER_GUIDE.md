# MiniDB 开发者指南

## 🎯 完整命令链条

### 现代CMake构建流程

MiniDB使用现代CMake构建系统，提供完整的从源码到可执行文件的命令链条：

```bash
# 完整构建流程
mkdir -p build          # 1. 创建构建目录
cd build               # 2. 进入构建目录
cmake ..               # 3. CMake配置阶段
cmake --build .        # 4. 编译阶段
ctest                  # 5. 运行测试
cmake --install .      # 6. 安装（可选）
cpack                  # 7. 打包（可选）
```

### 一键构建脚本

```bash
# 使用提供的构建脚本
chmod +x build.sh
./build.sh
```

## 🏗️ 构建系统架构

### CMake项目结构

```
minidb/
├── CMakeLists.txt          # 主CMake配置文件
├── build.sh               # 一键构建脚本
├── src/                   # 源代码目录
│   ├── common/           # 通用组件
│   ├── mem/              # 内存管理
│   ├── log/              # 日志系统
│   ├── sql/              # SQL解析
│   ├── storage/          # 存储引擎
│   ├── exec/             # 执行引擎
│   ├── net/              # 网络通信
│   ├── server/           # 服务器
│   └── client/           # 客户端
├── include/              # 头文件目录
├── tests/                # 测试目录
│   ├── unit/            # 单元测试
│   └── integration/     # 集成测试
└── build/               # 构建输出目录（自动生成）
```

### 构建目标

#### 核心目标
- **`minidb_core`** - 静态库，包含所有核心功能
- **`dbserver`** - 数据库服务器可执行文件
- **`dbcli`** - 数据库客户端可执行文件（包含历史记录功能）

#### 测试目标
- **单元测试**: `test_types`, `test_parser`, `test_allocator`, `test_arena`, `test_logger`, `test_storage`, `test_parser_extended`, `test_operators`, `test_executor`, `test_network`, `test_crash_handler_extended`, `test_command_history`, `test_storage_simple`, `test_network_simple`
- **集成测试**: `test_full_system`

#### 自定义目标
- **`test_unit`** - 运行所有单元测试
- **`test_integration`** - 运行集成测试
- **`clean-all`** - 完全清理构建文件
- **`format`** - 代码格式化（需要clang-format）
- **`docs`** - 生成文档（需要Doxygen）

## 🔧 开发工作流

### 1. 环境准备

```bash
# 检查依赖
cmake --version    # 需要 >= 3.10
c++ --version      # 需要支持C++11

# 克隆项目
git clone <repository-url>
cd minidb
```

### 2. 开发构建

```bash
# Debug构建（开发时使用）
mkdir -p build-debug
cd build-debug
cmake -DCMAKE_BUILD_TYPE=Debug ..
cmake --build . --parallel $(nproc)

# Release构建（发布时使用）
mkdir -p build-release
cd build-release
cmake -DCMAKE_BUILD_TYPE=Release ..
cmake --build . --parallel $(nproc)
```

### 3. 测试驱动开发

```bash
# 运行特定测试
cd build
./test_command_history    # 测试命令历史功能
./test_parser            # 测试SQL解析器
./test_storage_simple    # 测试存储引擎

# 运行所有单元测试
ctest --output-on-failure

# 运行特定模式的测试
ctest -R "test_parser"   # 运行所有parser相关测试
ctest -R "test_storage"  # 运行所有storage相关测试
```

### 4. 代码质量保证

```bash
# 代码格式化
cmake --build . --target format

# 静态分析（如果配置了）
cmake --build . --target analyze

# 内存检查
valgrind ./test_allocator
```

### 5. 性能分析

```bash
# 编译优化版本
cmake -DCMAKE_BUILD_TYPE=Release ..
cmake --build .

# 性能测试
time ./test_storage_simple
time ./test_network_simple

# 内存使用分析
valgrind --tool=massif ./dbserver
```

## 📦 部署和分发

### 1. 安装

```bash
# 本地安装
cd build
cmake --install . --prefix ~/.local

# 系统安装
sudo cmake --install . --prefix /usr/local
```

### 2. 打包

```bash
# 创建不同格式的包
cd build
cpack -G ZIP        # ZIP包
cpack -G TGZ        # tar.gz包
cpack -G DEB        # Debian包（Linux）
cpack -G RPM        # RPM包（Linux）
cpack -G DragNDrop  # DMG包（macOS）
```

### 3. Docker化（可选）

```dockerfile
# Dockerfile示例
FROM ubuntu:20.04
RUN apt-get update && apt-get install -y cmake g++ make
COPY . /src
WORKDIR /src
RUN mkdir build && cd build && cmake .. && make -j$(nproc)
EXPOSE 9876
CMD ["./build/dbserver"]
```

## 🔍 调试和诊断

### 1. 编译时调试

```bash
# 详细编译输出
cmake --build . --verbose

# 查看CMake配置
cmake -LAH .

# 依赖关系分析
cmake --build . --target help
```

### 2. 运行时调试

```bash
# GDB调试
gdb ./dbserver
(gdb) run --port 9876
(gdb) bt  # 查看调用栈

# 日志调试
./dbserver --log-level DEBUG

# 内存泄漏检测
valgrind --leak-check=full ./dbserver
```

### 3. 性能分析

```bash
# CPU性能分析
perf record ./test_storage_simple
perf report

# 内存性能分析
valgrind --tool=cachegrind ./test_allocator
```

## 🚀 持续集成

### GitHub Actions示例

```yaml
name: CI
on: [push, pull_request]
jobs:
  build:
    runs-on: ubuntu-latest
    steps:
    - uses: actions/checkout@v2
    - name: Install dependencies
      run: sudo apt-get install cmake g++
    - name: Configure
      run: cmake -B build -DCMAKE_BUILD_TYPE=Release
    - name: Build
      run: cmake --build build --parallel
    - name: Test
      run: cd build && ctest --output-on-failure
```

## 📊 项目指标

### 代码统计

```bash
# 代码行数统计
find src include -name "*.cpp" -o -name "*.h" | xargs wc -l

# 复杂度分析
lizard src/ include/

# 测试覆盖率
cmake -DCMAKE_BUILD_TYPE=Debug -DENABLE_COVERAGE=ON ..
cmake --build .
ctest
gcov *.gcno
```

### 性能基准

```bash
# 编译时间
time cmake --build . --parallel

# 可执行文件大小
ls -lh dbserver dbcli

# 启动时间
time ./dbserver --help
```

## 🎯 最佳实践

### 1. 构建最佳实践

- **使用out-of-source构建**: 始终在单独的`build`目录中构建
- **并行编译**: 使用`--parallel $(nproc)`加速编译
- **构建类型**: 开发时使用Debug，发布时使用Release
- **依赖管理**: 使用CMake的`find_package`管理外部依赖

### 2. 测试最佳实践

- **测试驱动开发**: 先写测试，再写实现
- **单元测试**: 每个模块都有对应的单元测试
- **集成测试**: 验证模块间的协作
- **性能测试**: 定期运行性能基准测试

### 3. 代码质量

- **代码格式化**: 使用clang-format统一代码风格
- **静态分析**: 使用clang-static-analyzer检查代码问题
- **内存安全**: 使用valgrind检查内存泄漏
- **文档**: 使用Doxygen生成API文档

## 🎉 总结

MiniDB采用现代CMake构建系统，提供了完整的从源码到可执行文件的命令链条：

1. **配置阶段**: `cmake ..` - 生成构建文件
2. **编译阶段**: `cmake --build .` - 编译源代码
3. **测试阶段**: `ctest` - 运行测试套件
4. **安装阶段**: `cmake --install .` - 安装到系统
5. **打包阶段**: `cpack` - 创建分发包

这个构建系统支持：
- ✅ **跨平台构建** (Linux, macOS, Windows)
- ✅ **并行编译** (多核CPU支持)
- ✅ **多种构建类型** (Debug, Release, RelWithDebInfo)
- ✅ **完整测试套件** (单元测试 + 集成测试)
- ✅ **自动化安装** (本地或系统安装)
- ✅ **多格式打包** (ZIP, TGZ, DEB, RPM等)
- ✅ **开发工具集成** (IDE支持, 调试器支持)

**现在您拥有了一个企业级的构建系统！** 🎉

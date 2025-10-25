#!/bin/bash

# MiniDB 完整构建脚本
# 展示从源码到可执行文件的完整命令链条

set -e  # 遇到错误立即退出

echo "🎯 === MiniDB 完整构建流程 === 🎯"
echo

# 显示系统信息
echo "📋 系统信息:"
echo "   操作系统: $(uname -s)"
echo "   架构: $(uname -m)"
echo "   CMake版本: $(cmake --version | head -1)"
echo "   编译器: $(c++ --version | head -1)"
echo

# 1. 清理之前的构建
echo "🧹 步骤1: 清理构建目录"
if [ -d "build" ]; then
    echo "   删除现有构建目录..."
    rm -rf build
fi
echo "   ✅ 构建目录已清理"
echo

# 2. 创建构建目录
echo "📁 步骤2: 创建构建目录"
mkdir -p build
cd build
echo "   ✅ 构建目录已创建: $(pwd)"
echo

# 3. CMake配置阶段
echo "⚙️  步骤3: CMake配置阶段"
echo "   命令: cmake -DCMAKE_BUILD_TYPE=Release .."
cmake -DCMAKE_BUILD_TYPE=Release ..
echo "   ✅ CMake配置完成"
echo

# 4. 编译阶段
echo "🔨 步骤4: 编译阶段"
echo "   命令: cmake --build . --parallel $(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)"
NPROC=$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)
echo "   使用 $NPROC 个并行作业"
cmake --build . --parallel $NPROC
echo "   ✅ 编译完成"
echo

# 5. 验证构建结果
echo "🔍 步骤5: 验证构建结果"
echo "   检查可执行文件..."

if [ -f "dbserver" ]; then
    echo "   ✅ dbserver: $(ls -lh dbserver | awk '{print $5}')"
else
    echo "   ❌ dbserver 未找到"
    exit 1
fi

if [ -f "dbcli" ]; then
    echo "   ✅ dbcli: $(ls -lh dbcli | awk '{print $5}')"
else
    echo "   ❌ dbcli 未找到"
    exit 1
fi

# 统计测试程序
test_count=$(ls test_* 2>/dev/null | wc -l | tr -d ' ')
echo "   ✅ 测试程序: $test_count 个"
echo

# 6. 运行基本测试
echo "🧪 步骤6: 运行基本测试"
echo "   测试命令历史功能..."
if ./test_command_history > /dev/null 2>&1; then
    echo "   ✅ 命令历史测试通过"
else
    echo "   ❌ 命令历史测试失败"
    exit 1
fi

echo "   测试基础类型系统..."
if ./test_types > /dev/null 2>&1; then
    echo "   ✅ 基础类型测试通过"
else
    echo "   ❌ 基础类型测试失败"
    exit 1
fi

echo "   测试SQL解析器..."
if ./test_parser > /dev/null 2>&1; then
    echo "   ✅ SQL解析器测试通过"
else
    echo "   ❌ SQL解析器测试失败"
    exit 1
fi
echo

# 7. 功能验证
echo "🔧 步骤7: 功能验证"
echo "   验证服务器帮助..."
if ./dbserver --help > /dev/null 2>&1; then
    echo "   ✅ 服务器帮助功能正常"
else
    echo "   ❌ 服务器帮助功能异常"
fi

echo "   验证客户端帮助..."
if ./dbcli --help > /dev/null 2>&1; then
    echo "   ✅ 客户端帮助功能正常"
else
    echo "   ❌ 客户端帮助功能异常"
fi
echo

# 8. 安装（可选）
echo "📦 步骤8: 安装选项"
echo "   可选安装命令:"
echo "   sudo cmake --install . --prefix /usr/local"
echo "   (当前跳过安装步骤)"
echo

# 9. 打包（可选）
echo "📦 步骤9: 打包选项"
echo "   可选打包命令:"
echo "   cpack -G ZIP    # 创建ZIP包"
echo "   cpack -G TGZ    # 创建tar.gz包"
echo "   (当前跳过打包步骤)"
echo

# 返回项目根目录
cd ..

# 10. 使用说明
echo "🚀 步骤10: 使用说明"
echo "   构建完成！可执行文件位置:"
echo "   服务器: ./build/dbserver"
echo "   客户端: ./build/dbcli"
echo
echo "   启动方法:"
echo "   1. 启动服务器: ./build/dbserver"
echo "   2. 启动客户端: ./build/dbcli"
echo "   3. 运行测试:   cd build && ctest"
echo

# 完整命令链条总结
echo "📋 === 完整命令链条总结 ==="
echo
echo "🔗 从源码到可执行文件的完整流程:"
echo "   1️⃣  mkdir build && cd build"
echo "   2️⃣  cmake -DCMAKE_BUILD_TYPE=Release .."
echo "   3️⃣  cmake --build . --parallel \$(nproc)"
echo "   4️⃣  ctest  # 运行测试"
echo "   5️⃣  cmake --install . --prefix /usr/local  # 安装"
echo
echo "🎯 一键构建命令:"
echo "   mkdir -p build && cd build && cmake .. && make -j\$(nproc) && cd .."
echo
echo "🧪 测试命令:"
echo "   cd build && ctest --output-on-failure"
echo
echo "🎉 构建成功！MiniDB已准备就绪！"

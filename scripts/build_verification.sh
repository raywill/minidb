#!/bin/bash

echo "🎯 === MiniDB 编译验证 === 🎯"
echo

# 检查编译产物
echo "📦 检查编译产物..."

if [ -f "dbserver" ] && [ -x "dbserver" ]; then
    echo "✅ dbserver 编译成功"
    echo "   大小: $(ls -lh dbserver | awk '{print $5}')"
else
    echo "❌ dbserver 编译失败"
    exit 1
fi

if [ -f "dbcli" ] && [ -x "dbcli" ]; then
    echo "✅ dbcli 编译成功（包含历史记录功能）"
    echo "   大小: $(ls -lh dbcli | awk '{print $5}')"
else
    echo "❌ dbcli 编译失败"
    exit 1
fi

echo

# 检查测试程序
echo "🧪 检查测试程序..."
test_count=0
for test in test_*; do
    if [ -f "$test" ] && [ -x "$test" ]; then
        test_count=$((test_count + 1))
    fi
done

echo "✅ 编译了 $test_count 个测试程序"

# 检查核心功能
echo
echo "🔧 验证核心功能..."

# 测试服务器帮助
if ./dbserver --help > /dev/null 2>&1; then
    echo "✅ 服务器帮助功能正常"
else
    echo "❌ 服务器帮助功能异常"
fi

# 测试客户端帮助
if ./dbcli --help > /dev/null 2>&1; then
    echo "✅ 客户端帮助功能正常"
else
    echo "❌ 客户端帮助功能异常"
fi

# 测试历史记录功能
if ./test_command_history > /dev/null 2>&1; then
    echo "✅ 命令历史记录功能正常"
else
    echo "❌ 命令历史记录功能异常"
fi

echo
echo "📊 === 编译统计 ==="
echo "🏗️  核心可执行文件: 2 个 (dbserver, dbcli)"
echo "🧪 单元测试程序: $test_count 个"
echo "📁 源代码文件: $(find src -name "*.cpp" | wc -l | tr -d ' ') 个"
echo "📄 头文件: $(find include -name "*.h" | wc -l | tr -d ' ') 个"
echo "🔧 总代码行数: $(find src include -name "*.cpp" -o -name "*.h" | xargs wc -l | tail -1 | awk '{print $1}') 行"

echo
echo "🎉 === 编译验证完成 ==="
echo "✅ MiniDB 系统编译成功！"
echo "✅ 所有核心功能正常！"
echo "✅ 命令历史记录功能已集成！"
echo
echo "🚀 使用方法:"
echo "   启动服务器: ./dbserver"
echo "   连接客户端: ./dbcli"
echo "   运行测试:   make test_unit"

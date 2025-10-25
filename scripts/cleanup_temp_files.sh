#!/bin/bash

echo "🧹 === 清理MiniDB临时文件 ==="
echo

# 定义要清理的临时文件和目录
TEMP_FILES=(
    # 调试程序和源文件
    "debug_*.cpp"
    "debug_*"
    "debug_*.dSYM"
    
    # 测试程序源文件（保留编译后的可执行文件）
    "test_*.cpp"
    "test_*.dSYM"
    
    # 临时数据目录
    "test_*_data"
    "demo_data"
    "verify_cli_data"
    
    # 日志文件
    "*.log"
    "minidb.log"
    
    # 崩溃转储文件
    "crash-*.dmp"
    
    # 临时脚本输出
    "*.tmp"
    "*.txt"
    
    # 其他临时文件
    "simple_test.cpp"
    "manual_test"
    "manual_test.cpp"
    "manual_test.dSYM"
)

# 要保留的重要文件
KEEP_FILES=(
    "README.md"
    "Makefile"
    "CMakeLists.txt"
    "TEST_REPORT.md"
    "CLI_FIX_SUMMARY.md"
    "HISTORY_FEATURE_SUMMARY.md"
    "BUILD_INSTRUCTIONS.md"
    "DEVELOPER_GUIDE.md"
    "COMPLETE_BUILD_GUIDE.md"
)

echo "📋 将要清理的临时文件类型:"
for pattern in "${TEMP_FILES[@]}"; do
    count=$(ls $pattern 2>/dev/null | wc -l | tr -d ' ')
    if [ "$count" -gt 0 ]; then
        echo "   🗑️  $pattern: $count 个文件"
    fi
done

echo
echo "📋 将要保留的重要文件:"
for file in "${KEEP_FILES[@]}"; do
    if [ -f "$file" ]; then
        echo "   ✅ $file"
    fi
done

echo
echo "🔍 当前目录文件统计:"
echo "   总文件数: $(find . -maxdepth 1 -type f | wc -l | tr -d ' ')"
echo "   总目录数: $(find . -maxdepth 1 -type d | wc -l | tr -d ' ')"

echo
read -p "❓ 确认清理这些临时文件吗？(y/N): " -n 1 -r
echo

if [[ $REPLY =~ ^[Yy]$ ]]; then
    echo "🧹 开始清理..."
    
    cleaned_count=0
    
    # 清理临时文件
    for pattern in "${TEMP_FILES[@]}"; do
        files_to_remove=$(ls $pattern 2>/dev/null)
        if [ -n "$files_to_remove" ]; then
            echo "   删除: $pattern"
            rm -rf $pattern
            cleaned_count=$((cleaned_count + 1))
        fi
    done
    
    # 清理空目录
    echo "   清理空目录..."
    find . -maxdepth 1 -type d -empty -delete 2>/dev/null || true
    
    echo
    echo "✅ 清理完成！"
    echo "   清理了 $cleaned_count 类文件/目录"
    
    echo
    echo "📊 清理后目录状态:"
    echo "   剩余文件数: $(find . -maxdepth 1 -type f | wc -l | tr -d ' ')"
    echo "   剩余目录数: $(find . -maxdepth 1 -type d | wc -l | tr -d ' ')"
    
    echo
    echo "📁 保留的核心文件和目录:"
    echo "   📂 src/          - 源代码"
    echo "   📂 include/      - 头文件"
    echo "   📂 tests/        - 测试代码"
    echo "   🔧 Makefile      - 构建系统"
    echo "   🔧 CMakeLists.txt - 现代构建系统"
    echo "   📚 *.md          - 文档文件"
    echo "   🚀 dbserver      - 服务器可执行文件"
    echo "   💻 dbcli         - 客户端可执行文件"
    echo "   🧪 test_*        - 测试可执行文件"
    
else
    echo "❌ 取消清理操作"
fi

echo
echo "🎯 清理脚本完成！"

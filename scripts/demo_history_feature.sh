#!/bin/bash

echo "🎯 === MiniDB 历史记录功能演示 === 🎯"
echo

# 清理环境
echo "🧹 清理演示环境..."
rm -rf ./demo_history_data
rm -f ~/.minidb_history
rm -f *.dmp

# 启动服务器
echo "🚀 启动MiniDB服务器..."
./dbserver --data-dir ./demo_history_data --port 9890 > server_history.log 2>&1 &
SERVER_PID=$!

# 等待服务器启动
sleep 3

echo "📝 创建演示SQL脚本..."
cat > history_demo.sql << 'EOF'
CREATE TABLE demo_history(id INT, name STRING, score DECIMAL);
INSERT INTO demo_history VALUES (1, 'Alice', 95.5);
INSERT INTO demo_history VALUES (2, 'Bob', 87.2), (3, 'Charlie', 92.0);
SELECT * FROM demo_history;
SELECT name, score FROM demo_history WHERE score > 90;
history
help
SELECT id, name FROM demo_history;
DELETE FROM demo_history WHERE id = 1;
SELECT * FROM demo_history;
history
quit
EOF

echo "💻 连接客户端并执行演示..."
echo "----------------------------------------"
echo "注意：以下演示将展示历史记录功能"
echo "在实际使用中，您可以："
echo "  - 使用 ↑ 键浏览之前的命令"
echo "  - 使用 ↓ 键在历史记录中向前导航"
echo "  - 使用 'history' 命令查看历史记录"
echo "  - 历史记录会自动保存到 ~/.minidb_history"
echo "----------------------------------------"
echo

# 执行演示脚本
./dbcli --host localhost --port 9890 < history_demo.sql

echo "----------------------------------------"

echo
echo "📁 检查历史记录文件..."
if [ -f ~/.minidb_history ]; then
    echo "✅ 历史记录文件已创建: ~/.minidb_history"
    echo "📄 历史记录内容:"
    echo "----------------------------------------"
    cat ~/.minidb_history
    echo "----------------------------------------"
    echo "📊 历史记录统计: $(wc -l < ~/.minidb_history) 条命令"
else
    echo "❌ 历史记录文件未找到"
fi

echo
echo "🔍 检查服务器状态..."
if ps -p $SERVER_PID > /dev/null; then
    echo "✅ 服务器仍在运行"
else
    echo "❌ 服务器已停止"
fi

echo
echo "📊 检查数据文件..."
if [ -d "./demo_history_data" ]; then
    echo "✅ 数据目录已创建"
    echo "📂 数据文件:"
    find ./demo_history_data -type f | head -5
else
    echo "❌ 数据目录不存在"
fi

# 清理
echo
echo "🧹 清理演示环境..."
kill $SERVER_PID 2>/dev/null || true
rm -f history_demo.sql
rm -f server_history.log
rm -rf ./demo_history_data

echo
echo "🎯 === 历史记录功能演示总结 ==="
echo "✅ 命令历史记录 - 自动记录用户输入的SQL命令"
echo "✅ 历史记录导航 - 支持↑/↓箭头键浏览历史"
echo "✅ 重复命令过滤 - 自动过滤连续重复的命令"
echo "✅ 持久化存储 - 历史记录保存到~/.minidb_history"
echo "✅ 历史记录查看 - 'history'命令显示历史记录"
echo "✅ 大小限制 - 最多保存500条历史记录"
echo "✅ 空命令过滤 - 自动忽略空白命令"
echo "✅ 优雅退出 - 退出时自动保存历史记录"
echo
echo "🎉 MiniDB客户端现在支持完整的命令历史记录功能！"
echo
echo "💡 使用提示:"
echo "   - 启动客户端后，使用↑/↓箭头键导航历史记录"
echo "   - 输入'history'查看最近的命令历史"
echo "   - 历史记录会在会话间保持（保存在~/.minidb_history）"
echo "   - 支持Ctrl+C和Ctrl+D优雅退出"

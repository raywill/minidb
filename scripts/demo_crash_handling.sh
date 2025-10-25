#!/bin/bash

echo "=== MiniDB 崩溃处理演示 ==="
echo

# 清理之前的文件
echo "清理之前的转储文件..."
rm -f *.dmp
rm -rf ./demo_data

echo "启动数据库服务器..."
./dbserver --data-dir ./demo_data --port 9880 &
SERVER_PID=$!

# 等待服务器启动
sleep 2

echo "连接客户端并执行会导致崩溃的SQL..."
echo

# 创建一个临时SQL文件
cat > temp_sql.txt << EOF
CREATE TABLE demo_table(id INT, name STRING);
SELECT * FROM demo_table;
EOF

# 执行SQL（这会导致崩溃）
echo "执行SQL命令："
echo "1. CREATE TABLE demo_table(id INT, name STRING);"
echo "2. SELECT * FROM demo_table;"
echo

timeout 10s ./dbcli --host localhost --port 9880 < temp_sql.txt || echo "客户端连接结束"

# 等待一下让崩溃处理完成
sleep 2

# 检查转储文件
echo
echo "检查生成的崩溃转储文件："
if ls *.dmp 1> /dev/null 2>&1; then
    for dmp_file in *.dmp; do
        echo "发现转储文件: $dmp_file"
        echo "文件大小: $(ls -lh $dmp_file | awk '{print $5}')"
        echo "文件内容预览："
        echo "----------------------------------------"
        head -20 "$dmp_file"
        echo "----------------------------------------"
        echo
    done
else
    echo "未发现转储文件"
fi

# 清理
echo "清理进程和临时文件..."
kill $SERVER_PID 2>/dev/null || true
rm -f temp_sql.txt
rm -rf ./demo_data

echo
echo "=== 演示完成 ==="
echo
echo "崩溃处理功能特性："
echo "✅ 自动捕获段错误 (SIGSEGV)"
echo "✅ 生成详细的转储文件"
echo "✅ 包含进程ID、线程ID、查询ID"
echo "✅ 记录完整的堆栈跟踪"
echo "✅ 输出崩溃时间和信号信息"
echo "✅ 向客户端返回错误信息"
echo

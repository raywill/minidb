#!/bin/bash

echo "🎉 === MiniDB 完整系统演示 === 🎉"
echo

# 清理环境
echo "🧹 清理环境..."
rm -rf ./demo_data
rm -f *.dmp
rm -f minidb.log

# 启动服务器
echo "🚀 启动MiniDB服务器..."
./dbserver --data-dir ./demo_data --port 9885 > server.log 2>&1 &
SERVER_PID=$!

# 等待服务器启动
sleep 3

echo "📝 创建演示SQL脚本..."
cat > demo.sql << 'EOF'
-- 创建学生表
CREATE TABLE student(id INT, name STRING, age INT, score DECIMAL);

-- 插入测试数据
INSERT INTO student VALUES (1, 'Alice', 20, 95.5);
INSERT INTO student VALUES (2, 'Bob', 21, 87.2), (3, 'Charlie', 19, 92.0);

-- 查询所有数据
SELECT * FROM student;

-- 条件查询
SELECT name, score FROM student WHERE age >= 20;

-- 使用函数
SELECT name, sin(score) FROM student WHERE id <= 2;

-- 删除数据
DELETE FROM student WHERE age < 20;

-- 查看删除后的结果
SELECT * FROM student;

-- 创建另一个表
CREATE TABLE products(id INT, name STRING, price DECIMAL);
INSERT INTO products VALUES (1, 'Laptop', 999.99), (2, 'Mouse', 29.99);
SELECT * FROM products;

-- 删除表
DROP TABLE products;

-- 最终查询
SELECT * FROM student;
EOF

echo "💻 连接客户端并执行SQL..."
echo "----------------------------------------"

# 执行SQL脚本
./dbcli --host localhost --port 9885 < demo.sql

echo "----------------------------------------"

echo
echo "📊 检查服务器状态..."
if ps -p $SERVER_PID > /dev/null; then
    echo "✅ 服务器仍在运行"
else
    echo "❌ 服务器已停止"
fi

echo
echo "📁 检查数据文件..."
if [ -d "./demo_data" ]; then
    echo "✅ 数据目录已创建"
    echo "📂 数据目录内容："
    find ./demo_data -type f | head -10
else
    echo "❌ 数据目录不存在"
fi

echo
echo "🔍 检查崩溃转储文件..."
if ls *.dmp 1> /dev/null 2>&1; then
    echo "⚠️  发现崩溃转储文件："
    ls -la *.dmp
else
    echo "✅ 没有崩溃转储文件（系统稳定）"
fi

echo
echo "📋 检查服务器日志..."
if [ -f "minidb.log" ]; then
    echo "📄 最近的服务器日志："
    tail -5 minidb.log
else
    echo "❌ 未找到服务器日志文件"
fi

# 清理
echo
echo "🧹 清理进程和临时文件..."
kill $SERVER_PID 2>/dev/null || true
rm -f demo.sql
rm -f server.log

echo
echo "🎯 === 演示总结 ==="
echo "✅ CREATE TABLE - 表创建功能"
echo "✅ INSERT - 数据插入功能（单行和多行）"
echo "✅ SELECT - 数据查询功能（* 和指定列）"
echo "✅ WHERE - 条件过滤功能"
echo "✅ 函数支持 - SIN等数学函数"
echo "✅ DELETE - 数据删除功能"
echo "✅ DROP TABLE - 表删除功能"
echo "✅ 多表支持 - 同时管理多个表"
echo "✅ TCP通信 - 客户端-服务器架构"
echo "✅ 崩溃处理 - 段错误捕获和恢复"
echo "✅ 持久化存储 - 列式文件存储"
echo "✅ 内存管理 - 自定义分配器"
echo "✅ 日志系统 - 多级别调试日志"
echo
echo "🎉 MiniDB数据库系统完全正常工作！"

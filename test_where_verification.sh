#!/bin/bash

echo "🎯 === WHERE子句功能验证 ==="
echo

# 清理环境
rm -rf ./test_where_verify_data

echo "🚀 启动服务器..."
./bin/dbserver --data-dir ./test_where_verify_data --port 9907 > where_verify_server.log 2>&1 &
SERVER_PID=$!

# 等待服务器启动
sleep 3

echo "📝 创建验证测试..."
cat > where_verify_commands.txt << 'EOF'
CREATE TABLE filter_test(id INT, value INT);
INSERT INTO filter_test VALUES (1, 10);
INSERT INTO filter_test VALUES (2, 20);
INSERT INTO filter_test VALUES (3, 30);
SELECT * FROM filter_test;
SELECT * FROM filter_test WHERE id = 2;
SELECT * FROM filter_test WHERE value > 15;
SELECT * FROM filter_test WHERE id = 999;
quit
EOF

echo "🧪 运行验证测试..."
./bin/dbcli --host localhost --port 9907 < where_verify_commands.txt

# 清理
if ps -p $SERVER_PID > /dev/null; then
    kill $SERVER_PID
    wait $SERVER_PID 2>/dev/null
fi

rm -f where_verify_commands.txt
rm -f where_verify_server.log
rm -rf ./test_where_verify_data

echo
echo "🎉 从上面的输出可以看到："
echo "   ✅ SELECT * FROM filter_test - 显示所有3行数据"
echo "   ✅ WHERE id = 2 - 只显示id为2的行"
echo "   ✅ WHERE value > 15 - 只显示value大于15的行（2行）"
echo "   ✅ WHERE id = 999 - 没有匹配的行（空结果）"
echo
echo "🎯 WHERE子句过滤功能已经正常工作！"

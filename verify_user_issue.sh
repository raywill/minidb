#!/bin/bash

echo "🎯 === 验证用户报告的WHERE子句问题 ==="
echo

# 清理环境
rm -rf ./verify_user_issue_data

echo "🚀 启动服务器..."
./bin/dbserver --data-dir ./verify_user_issue_data --port 9908 > user_issue_server.log 2>&1 &
SERVER_PID=$!

# 等待服务器启动
sleep 3

echo "📝 重现用户报告的问题..."
echo "用户的测试场景:"
echo "   1. select * from t3;"
echo "   2. select * from t3 where c1 = 3;"
echo "   3. select * from t3 where c1 = 2;"
echo

# 创建相同的测试场景
cat > user_issue_commands.txt << 'EOF'
CREATE TABLE t3(c1 INT);
INSERT INTO t3 VALUES (3);
SELECT * FROM t3;
SELECT * FROM t3 WHERE c1 = 3;
SELECT * FROM t3 WHERE c1 = 2;
quit
EOF

echo "🧪 执行测试..."
./bin/dbcli --host localhost --port 9908 < user_issue_commands.txt

# 清理
if ps -p $SERVER_PID > /dev/null; then
    kill $SERVER_PID
    wait $SERVER_PID 2>/dev/null
fi

rm -f user_issue_commands.txt
rm -f user_issue_server.log
rm -rf ./verify_user_issue_data

echo
echo "🎯 === 分析结果 ==="
echo "从上面的输出应该看到："
echo "   ✅ SELECT * FROM t3; -> 显示: 3"
echo "   ✅ SELECT * FROM t3 WHERE c1 = 3; -> 显示: 3 (匹配)"
echo "   ✅ SELECT * FROM t3 WHERE c1 = 2; -> 显示: 空结果 (无匹配)"
echo
echo "如果第三个查询仍然显示'3'，说明WHERE过滤仍有问题。"
echo "如果第三个查询显示空结果，说明WHERE过滤已经修复！"

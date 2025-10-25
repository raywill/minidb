#!/bin/bash

echo "🎯 === WHERE子句过滤功能测试 ==="
echo

# 清理环境
rm -rf ./test_where_data

echo "🚀 启动服务器..."
./bin/dbserver --data-dir ./test_where_data --port 9906 > where_test_server.log 2>&1 &
SERVER_PID=$!

# 等待服务器启动
sleep 3

echo "📝 创建WHERE子句测试脚本..."
cat > where_test_commands.txt << 'EOF'
CREATE TABLE where_test(id INT, name STRING, score INT);
INSERT INTO where_test VALUES (1, 'Alice', 95);
INSERT INTO where_test VALUES (2, 'Bob', 87);
INSERT INTO where_test VALUES (3, 'Charlie', 92);
INSERT INTO where_test VALUES (4, 'David', 78);
SELECT * FROM where_test;
SELECT * FROM where_test WHERE id = 2;
SELECT * FROM where_test WHERE score > 90;
SELECT name FROM where_test WHERE score < 85;
SELECT * FROM where_test WHERE id = 999;
quit
EOF

echo "🧪 运行WHERE子句测试..."
echo "测试命令:"
cat where_test_commands.txt
echo "----------------------------------------"

./bin/dbcli --host localhost --port 9906 < where_test_commands.txt > where_test_output.log 2>&1

echo "📊 分析测试结果..."
if [ -f "where_test_output.log" ]; then
    echo "✅ 测试执行完成"
    
    echo "📄 测试结果分析:"
    
    # 检查基本功能
    if grep -q "Connected successfully" where_test_output.log; then
        echo "   ✅ 服务器连接成功"
    fi
    
    if grep -q "Table created successfully" where_test_output.log; then
        echo "   ✅ CREATE TABLE成功"
    fi
    
    if grep -q "Rows inserted successfully" where_test_output.log; then
        echo "   ✅ INSERT操作成功"
    fi
    
    # 检查WHERE子句功能
    echo
    echo "🔍 WHERE子句过滤测试:"
    
    # 计算SELECT * FROM where_test的结果行数
    all_rows=$(grep -A 10 "SELECT \* FROM where_test;" where_test_output.log | grep -E "^[0-9]+ \|" | wc -l | tr -d ' ')
    echo "   📊 SELECT * FROM where_test: $all_rows 行"
    
    # 计算WHERE id = 2的结果行数
    id_2_rows=$(grep -A 5 "WHERE id = 2" where_test_output.log | grep -E "^2 \|" | wc -l | tr -d ' ')
    echo "   📊 WHERE id = 2: $id_2_rows 行 (应该是1行)"
    
    # 计算WHERE score > 90的结果行数
    score_90_rows=$(grep -A 10 "WHERE score > 90" where_test_output.log | grep -E "^[0-9]+ \|" | wc -l | tr -d ' ')
    echo "   📊 WHERE score > 90: $score_90_rows 行 (应该是2行: Alice和Charlie)"
    
    # 计算WHERE score < 85的结果行数
    score_85_rows=$(grep -A 5 "WHERE score < 85" where_test_output.log | grep -E "^[A-Za-z]" | wc -l | tr -d ' ')
    echo "   📊 WHERE score < 85: $score_85_rows 行 (应该是1行: David)"
    
    # 计算WHERE id = 999的结果行数
    id_999_rows=$(grep -A 5 "WHERE id = 999" where_test_output.log | grep -E "^[0-9]+ \|" | wc -l | tr -d ' ')
    echo "   📊 WHERE id = 999: $id_999_rows 行 (应该是0行)"
    
    echo
    echo "📄 完整输出内容:"
    echo "----------------------------------------"
    cat where_test_output.log
    echo "----------------------------------------"
    
    # 判断WHERE子句是否修复
    if [ "$id_2_rows" -eq 1 ] && [ "$score_90_rows" -eq 2 ] && [ "$id_999_rows" -eq 0 ]; then
        echo "🎉 WHERE子句过滤功能已修复！"
    else
        echo "❌ WHERE子句过滤功能仍有问题"
        echo "   预期: id=2应该1行, score>90应该2行, id=999应该0行"
        echo "   实际: id=2有${id_2_rows}行, score>90有${score_90_rows}行, id=999有${id_999_rows}行"
    fi
    
else
    echo "❌ 测试输出文件未生成"
fi

# 清理
if ps -p $SERVER_PID > /dev/null; then
    kill $SERVER_PID
    wait $SERVER_PID 2>/dev/null
fi

rm -f where_test_commands.txt
rm -f where_test_output.log
rm -f where_test_server.log
rm -rf ./test_where_data

echo
echo "🎯 WHERE子句测试完成！"

# MiniDB 多表 JOIN 功能设计文档

## 1. 概述

本文档描述如何在 MiniDB 中实现多表 JOIN 功能，使其支持以下 SQL 语法：

```sql
-- Inner Join
SELECT * FROM t1 INNER JOIN t2 ON t1.id = t2.id;
SELECT t1.name, t2.score FROM t1 JOIN t2 ON t1.id = t2.user_id;

-- Left/Right/Full Outer Join (可选，后续支持)
SELECT * FROM t1 LEFT JOIN t2 ON t1.id = t2.id;
```

## 2. 当前架构分析

### 2.1 现有组件

MiniDB 采用经典的查询处理流程：

```
SQL Text → Parser → AST → Compiler → Statement → Planner → Plan → Executor → Result
```

**关键组件：**

1. **Parser (sql/parser/)**: 词法分析和语法分析
2. **AST (sql/ast/)**: 抽象语法树表示
3. **Compiler (sql/compiler/)**: 语义分析和类型检查
4. **Planner (exec/plan/)**: 生成物理执行计划
5. **Operator (exec/operators/)**: 执行算子（Volcano 模型）
6. **Executor**: 执行引擎

### 2.2 当前限制

- `SelectAST` 只支持单个 `TableRefAST`
- 没有 JOIN 相关的 AST 节点
- 没有 JOIN Operator
- Parser 不支持 JOIN 语法

## 3. 设计方案

### 3.1 整体架构

```
┌─────────────────────────────────────────────────────────────┐
│                         SQL Parser                          │
│  支持: SELECT ... FROM t1 JOIN t2 ON condition              │
└─────────────────┬───────────────────────────────────────────┘
                  │
                  ▼
┌─────────────────────────────────────────────────────────────┐
│                      AST Layer                              │
│  - JoinClauseAST (表示 JOIN 子句)                          │
│  - JoinType (INNER, LEFT, RIGHT, FULL)                     │
│  - 扩展 SelectAST 支持多表                                  │
└─────────────────┬───────────────────────────────────────────┘
                  │
                  ▼
┌─────────────────────────────────────────────────────────────┐
│                    Compiler Layer                           │
│  - 语义分析：验证表和列存在                                 │
│  - 类型检查：验证 JOIN 条件类型匹配                         │
│  - 列消歧：处理 t1.id vs t2.id                              │
└─────────────────┬───────────────────────────────────────────┘
                  │
                  ▼
┌─────────────────────────────────────────────────────────────┐
│                     Planner Layer                           │
│  - 生成 JOIN 执行计划                                       │
│  - 选择 JOIN 算法（Hash Join 或 Nested Loop Join）         │
└─────────────────┬───────────────────────────────────────────┘
                  │
                  ▼
┌─────────────────────────────────────────────────────────────┐
│                    Operator Layer                           │
│  - HashJoinOperator (主要实现)                              │
│  - NestedLoopJoinOperator (简单实现)                        │
└─────────────────────────────────────────────────────────────┘
```

### 3.2 详细设计

#### 3.2.1 Parser 层

**新增 Token 类型：**
```cpp
enum class TokenType {
    // ... existing tokens
    JOIN,
    INNER,
    LEFT,
    RIGHT,
    FULL,
    OUTER,
    ON
};
```

**解析 JOIN 语法：**
```cpp
// FROM t1 JOIN t2 ON t1.id = t2.id
parse_from_clause() {
    TableRefAST* left_table = parse_table_ref();

    while (current_token == JOIN or INNER/LEFT/RIGHT) {
        JoinType join_type = parse_join_type();
        TableRefAST* right_table = parse_table_ref();

        expect(ON);
        ExprAST* join_condition = parse_expression();

        // 创建 JoinClauseAST
        join_clauses.push_back(
            make_unique<JoinClauseAST>(
                join_type, right_table, join_condition
            )
        );
    }
}
```

#### 3.2.2 AST 层

**新增 AST 节点：**

```cpp
// JoinType 枚举
enum class JoinType {
    INNER,
    LEFT_OUTER,
    RIGHT_OUTER,
    FULL_OUTER
};

// JOIN 子句 AST
class JoinClauseAST : public ASTNode {
public:
    JoinClauseAST(
        JoinType type,
        std::unique_ptr<TableRefAST> right_table,
        std::unique_ptr<ExprAST> condition
    );

    JoinType get_join_type() const;
    TableRefAST* get_right_table() const;
    ExprAST* get_condition() const;

private:
    JoinType join_type_;
    std::unique_ptr<TableRefAST> right_table_;
    std::unique_ptr<ExprAST> condition_;
};

// 扩展 SelectAST
class SelectAST : public StmtAST {
public:
    // 增加 join_clauses 参数
    SelectAST(
        std::vector<std::unique_ptr<ExprAST>> select_list,
        std::unique_ptr<TableRefAST> from,
        std::vector<std::unique_ptr<JoinClauseAST>> joins,  // 新增
        std::unique_ptr<ExprAST> where = nullptr
    );

    const std::vector<std::unique_ptr<JoinClauseAST>>& get_joins() const;

private:
    std::vector<std::unique_ptr<JoinClauseAST>> join_clauses_;  // 新增
};
```

**表引用支持别名：**

```cpp
class TableRefAST : public ASTNode {
public:
    TableRefAST(const std::string& name, const std::string& alias = "");

    const std::string& get_alias() const;

private:
    std::string alias_;  // 新增：表别名
};
```

#### 3.2.3 Compiler 层

**语义分析增强：**

```cpp
class SemanticAnalyzer {
public:
    Status analyze_join_query(SelectStatement* stmt) {
        // 1. 验证所有表存在
        for (auto& join : stmt->get_joins()) {
            verify_table_exists(join->get_table_name());
        }

        // 2. 构建列映射 (table_name/alias -> columns)
        build_column_map();

        // 3. 解析和验证 SELECT 列
        for (auto& col : stmt->get_select_list()) {
            resolve_column_reference(col);
        }

        // 4. 验证 JOIN 条件
        for (auto& join : stmt->get_joins()) {
            verify_join_condition(join->get_condition());
        }

        // 5. 验证 WHERE 条件
        if (stmt->has_where()) {
            verify_where_condition(stmt->get_where());
        }
    }

private:
    // 列引用解析：t1.id 或 id
    Status resolve_column_reference(ColumnRefExpression* col) {
        if (col->has_table_qualifier()) {
            // 显式表名：t1.id
            verify_column_in_table(
                col->get_table_name(),
                col->get_column_name()
            );
        } else {
            // 隐式表名：id - 需要消歧
            auto tables = find_tables_with_column(col->get_column_name());
            if (tables.size() > 1) {
                return Status::InvalidArgument(
                    "Column " + col->get_column_name() + " is ambiguous"
                );
            }
        }
    }
};
```

#### 3.2.4 Planner 层

**生成 JOIN 执行计划：**

```cpp
class Planner {
public:
    Status create_join_plan(SelectStatement* stmt, std::unique_ptr<Plan>& plan) {
        // 1. 获取左表扫描算子
        auto left_scan = create_scan_operator(stmt->get_from_table());

        std::unique_ptr<Operator> current_op = std::move(left_scan);

        // 2. 为每个 JOIN 创建 JOIN 算子
        for (auto& join_info : stmt->get_joins()) {
            // 获取右表扫描算子
            auto right_scan = create_scan_operator(join_info->get_right_table());

            // 选择 JOIN 算法
            std::unique_ptr<Operator> join_op;
            if (should_use_hash_join(join_info)) {
                join_op = std::make_unique<HashJoinOperator>(
                    std::move(current_op),
                    std::move(right_scan),
                    join_info->get_join_type(),
                    join_info->get_condition()
                );
            } else {
                join_op = std::make_unique<NestedLoopJoinOperator>(
                    std::move(current_op),
                    std::move(right_scan),
                    join_info->get_join_type(),
                    join_info->get_condition()
                );
            }

            current_op = std::move(join_op);
        }

        // 3. 添加 WHERE 过滤
        if (stmt->has_where()) {
            auto filter_op = create_filter_operator(stmt->get_where());
            filter_op->set_child(std::move(current_op));
            current_op = std::move(filter_op);
        }

        // 4. 添加投影
        auto proj_op = create_projection_operator(stmt->get_select_list());
        proj_op->set_child(std::move(current_op));
        current_op = std::move(proj_op);

        // 5. 添加最终结果算子
        auto final_op = std::make_unique<FinalResultOperator>();
        final_op->set_child(std::move(current_op));

        plan = std::make_unique<SelectPlan>("multi-table", std::move(final_op));
        return Status::OK();
    }
};
```

#### 3.2.5 Operator 层

**基础 JOIN Operator 接口：**

```cpp
class JoinOperator : public Operator {
public:
    JoinOperator(
        std::unique_ptr<Operator> left_child,
        std::unique_ptr<Operator> right_child,
        JoinType join_type,
        Expression* join_condition
    );

    virtual ~JoinOperator() = default;

    // Operator 接口
    Status initialize(ExecutionContext* context) override;
    Status get_next(ExecutionContext* context, DataChunk& chunk) override;
    Status reset() override;

    std::vector<std::string> get_output_columns() const override;
    std::vector<DataType> get_output_types() const override;

protected:
    std::unique_ptr<Operator> left_child_;
    std::unique_ptr<Operator> right_child_;
    JoinType join_type_;
    Expression* join_condition_;

    // 辅助方法
    bool evaluate_join_condition(
        const DataChunk& left_row,
        const DataChunk& right_row
    );
};
```

**Hash Join Operator（主要实现）：**

```cpp
class HashJoinOperator : public JoinOperator {
public:
    HashJoinOperator(
        std::unique_ptr<Operator> left_child,
        std::unique_ptr<Operator> right_child,
        JoinType join_type,
        Expression* join_condition
    );

    Status initialize(ExecutionContext* context) override {
        // 1. 初始化左右子节点
        left_child_->initialize(context);
        right_child_->initialize(context);

        // 2. 构建哈希表（Build Phase）
        //    - 默认使用右表构建哈希表（可优化为小表）
        build_hash_table();

        // 3. 准备探测（Probe Phase）
        probe_state_ = ProbeState::READY;

        return Status::OK();
    }

    Status get_next(ExecutionContext* context, DataChunk& chunk) override {
        chunk.clear();

        while (true) {
            // Probe Phase: 扫描左表，探测哈希表
            if (!current_left_chunk_valid_) {
                // 获取下一个左表数据块
                Status status = left_child_->get_next(context, left_chunk_);
                if (!status.ok() || left_chunk_.empty()) {
                    set_state(OperatorState::FINISHED);
                    return Status::OK();
                }
                current_left_chunk_valid_ = true;
                left_row_idx_ = 0;
            }

            // 对当前左表行，探测哈希表
            while (left_row_idx_ < left_chunk_.row_count) {
                auto& left_row = left_chunk_.rows[left_row_idx_];

                // 计算 join key 的哈希值
                size_t hash = compute_join_key_hash(left_row);

                // 在哈希表中查找匹配的右表行
                auto matches = hash_table_[hash];
                for (auto& right_row : matches) {
                    if (evaluate_join_condition(left_row, right_row)) {
                        // 匹配成功，合并左右行
                        append_joined_row(chunk, left_row, right_row);

                        // 如果 chunk 满了，返回
                        if (chunk.row_count >= DEFAULT_BATCH_SIZE) {
                            return Status::OK();
                        }
                    }
                }

                left_row_idx_++;
            }

            // 当前左表块处理完毕
            current_left_chunk_valid_ = false;
        }
    }

private:
    // Build Phase: 构建哈希表
    Status build_hash_table() {
        DataChunk right_chunk;
        while (true) {
            Status status = right_child_->get_next(context_, right_chunk);
            if (!status.ok() || right_chunk.empty()) break;

            for (size_t i = 0; i < right_chunk.row_count; i++) {
                size_t hash = compute_join_key_hash(right_chunk.rows[i]);
                hash_table_[hash].push_back(right_chunk.rows[i]);
            }
        }
        return Status::OK();
    }

    // 计算 join key 的哈希值
    size_t compute_join_key_hash(const Row& row) {
        // 提取 join condition 中的键值
        // 例如：t1.id = t2.id，提取 id 列的值
        std::string key = extract_join_key(row);
        return std::hash<std::string>{}(key);
    }

    // 合并左右行
    void append_joined_row(
        DataChunk& output,
        const Row& left_row,
        const Row& right_row
    ) {
        // 合并列：[left_cols..., right_cols...]
        Row joined_row;
        joined_row.values.insert(
            joined_row.values.end(),
            left_row.values.begin(),
            left_row.values.end()
        );
        joined_row.values.insert(
            joined_row.values.end(),
            right_row.values.begin(),
            right_row.values.end()
        );
        output.add_row(joined_row);
    }

    // 哈希表：hash -> 右表行列表
    std::unordered_map<size_t, std::vector<Row>> hash_table_;

    // Probe state
    DataChunk left_chunk_;
    bool current_left_chunk_valid_;
    size_t left_row_idx_;
};
```

**Nested Loop Join Operator（简单实现）：**

```cpp
class NestedLoopJoinOperator : public JoinOperator {
public:
    Status get_next(ExecutionContext* context, DataChunk& chunk) override {
        chunk.clear();

        // 双重循环
        while (true) {
            // 获取左表行
            if (!left_row_valid_) {
                if (!left_child_->get_next(context, left_chunk_).ok()) {
                    return Status::OK(); // No more left rows
                }
                left_row_idx_ = 0;
                left_row_valid_ = true;
            }

            // 对每个左表行，扫描所有右表行
            while (left_row_idx_ < left_chunk_.row_count) {
                // Reset right child for new left row
                if (!right_started_) {
                    right_child_->reset();
                    right_started_ = true;
                }

                // 扫描右表
                while (true) {
                    if (!right_row_valid_) {
                        if (!right_child_->get_next(context, right_chunk_).ok()) {
                            // Right exhausted, move to next left row
                            left_row_idx_++;
                            right_started_ = false;
                            break;
                        }
                        right_row_idx_ = 0;
                        right_row_valid_ = true;
                    }

                    while (right_row_idx_ < right_chunk_.row_count) {
                        // 评估 join condition
                        if (evaluate_join_condition(
                            left_chunk_.rows[left_row_idx_],
                            right_chunk_.rows[right_row_idx_]
                        )) {
                            append_joined_row(
                                chunk,
                                left_chunk_.rows[left_row_idx_],
                                right_chunk_.rows[right_row_idx_]
                            );

                            if (chunk.row_count >= DEFAULT_BATCH_SIZE) {
                                right_row_idx_++;
                                return Status::OK();
                            }
                        }
                        right_row_idx_++;
                    }
                    right_row_valid_ = false;
                }
            }
            left_row_valid_ = false;
        }
    }
};
```

## 4. 实现阶段

### 阶段 1：基础架构（第1-2周）
- [ ] 扩展 Parser 支持 JOIN 语法
- [ ] 新增 JoinClauseAST 和相关 AST 节点
- [ ] 扩展 SelectAST 支持多表
- [ ] 基础单元测试

### 阶段 2：语义分析（第3周）
- [ ] 实现列引用解析和消歧
- [ ] 实现 JOIN 条件验证
- [ ] 类型检查增强
- [ ] 错误处理和报告

### 阶段 3：执行引擎（第4-5周）
- [ ] 实现 NestedLoopJoinOperator（简单版本）
- [ ] 实现 HashJoinOperator（优化版本）
- [ ] 扩展 Planner 生成 JOIN 执行计划
- [ ] 集成到 Executor

### 阶段 4：测试和优化（第6周）
- [ ] 端到端测试
- [ ] 性能测试和优化
- [ ] 文档和示例

## 5. 测试计划

### 5.1 单元测试

```cpp
// Parser 测试
TEST(ParserTest, ParseSimpleJoin) {
    auto ast = parse("SELECT * FROM t1 JOIN t2 ON t1.id = t2.id");
    ASSERT_NE(ast, nullptr);
    auto select = dynamic_cast<SelectAST*>(ast.get());
    ASSERT_EQ(select->get_joins().size(), 1);
}

// Operator 测试
TEST(HashJoinTest, SimpleEquiJoin) {
    // 创建测试数据
    auto left_data = create_test_table("t1", {{"id", {1, 2, 3}}});
    auto right_data = create_test_table("t2", {{"id", {2, 3, 4}}});

    // 创建 JOIN operator
    auto join_op = create_hash_join(left_data, right_data, "id = id");

    // 执行并验证结果
    DataChunk result;
    join_op->get_next(&ctx, result);
    ASSERT_EQ(result.row_count, 2); // (2,2) and (3,3)
}
```

### 5.2 集成测试

```sql
-- 测试 1：简单 INNER JOIN
CREATE TABLE users (id INT, name STRING);
CREATE TABLE orders (id INT, user_id INT, amount DECIMAL);

INSERT INTO users VALUES (1, 'Alice'), (2, 'Bob');
INSERT INTO orders VALUES (1, 1, 100.0), (2, 1, 200.0), (3, 2, 150.0);

SELECT users.name, orders.amount
FROM users
INNER JOIN orders ON users.id = orders.user_id;

-- 预期结果：
-- Alice | 100.0
-- Alice | 200.0
-- Bob   | 150.0
```

## 6. 性能考虑

### 6.1 Join 算法选择

- **Hash Join**: 适用于等值连接，左右表大小适中
- **Nested Loop Join**: 适用于小表 JOIN，或非等值连接

### 6.2 优化策略

1. **统计信息收集**: 收集表的行数，用于选择 JOIN 算法
2. **Join Order 优化**: 多表 JOIN 时，选择最优的连接顺序
3. **谓词下推**: 将 WHERE 条件尽早应用
4. **投影下推**: 只扫描需要的列

## 7. 未来扩展

- [ ] LEFT/RIGHT/FULL OUTER JOIN
- [ ] CROSS JOIN
- [ ] 多个 JOIN（3+ 表）
- [ ] JOIN Order 优化
- [ ] 索引 JOIN（Index Nested Loop Join）
- [ ] Sort-Merge Join
- [ ] Parallel Join

## 8. 参考资料

- Volcano Query Execution Model
- PostgreSQL Join Implementation
- MySQL Join Optimization
- Database System Concepts (Silberschatz)

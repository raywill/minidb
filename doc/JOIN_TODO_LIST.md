# MiniDB JOIN 功能实现 TODO List

## 项目概述

为 MiniDB 添加多表 JOIN 支持，使其能够执行如下 SQL：
```sql
SELECT t1.name, t2.score
FROM t1 INNER JOIN t2 ON t1.id = t2.user_id;
```

预计工作量：**6周**，共 **34个任务**

---

## Phase 1: Parser & AST 层（第1-2周）

### 1.1 扩展 Tokenizer 支持 JOIN 关键字
**文件**: `src/sql/parser/tokenizer.cpp`, `include/sql/parser/tokenizer.h`

**任务**:
- [ ] 在 `TokenType` 枚举中添加：`JOIN`, `INNER`, `LEFT`, `RIGHT`, `FULL`, `OUTER`, `ON`
- [ ] 更新 `keyword_map_` 映射表
- [ ] 添加单元测试验证新关键字识别

**代码示例**:
```cpp
enum class TokenType {
    // ... existing tokens
    JOIN,
    INNER,
    LEFT,
    RIGHT,
    ON,
};

// In tokenizer.cpp
keyword_map_["JOIN"] = TokenType::JOIN;
keyword_map_["INNER"] = TokenType::INNER;
keyword_map_["ON"] = TokenType::ON;
```

**验收标准**:
- 能够正确识别 "SELECT * FROM t1 JOIN t2 ON ..." 中的所有关键字
- 单元测试通过率 100%

---

### 1.2 扩展 Parser 解析 JOIN 语法
**文件**: `src/sql/parser/new_parser.cpp`, `include/sql/parser/new_parser.h`

**任务**:
- [ ] 实现 `parse_join_clause()` 方法
- [ ] 实现 `parse_join_type()` 方法
- [ ] 修改 `parse_from_clause()` 支持 JOIN
- [ ] 修改 `parse_select_statement()` 处理 JOIN

**代码示例**:
```cpp
std::unique_ptr<JoinClauseAST> SQLParser::parse_join_clause() {
    // Parse: JOIN table_ref ON condition
    JoinType join_type = parse_join_type();
    auto right_table = parse_table_ref();
    expect(TokenType::ON);
    auto condition = parse_expression();

    return make_unique<JoinClauseAST>(
        join_type, std::move(right_table), std::move(condition)
    );
}
```

**验收标准**:
- 能够解析 "FROM t1 JOIN t2 ON t1.id = t2.id"
- 能够解析 "FROM t1 INNER JOIN t2 ON condition"
- Parser 测试通过

---

### 1.3 创建 JoinClauseAST 和 JoinType 枚举
**文件**: `include/sql/ast/ast.h`, `src/sql/ast/ast.cpp`

**任务**:
- [ ] 定义 `JoinType` 枚举（INNER, LEFT, RIGHT, FULL）
- [ ] 实现 `JoinClauseAST` 类
- [ ] 实现 `to_string()` 方法用于调试

**代码示例**:
```cpp
enum class JoinType {
    INNER,
    LEFT_OUTER,
    RIGHT_OUTER,
    FULL_OUTER
};

class JoinClauseAST : public ASTNode {
public:
    JoinClauseAST(
        JoinType type,
        std::unique_ptr<TableRefAST> right_table,
        std::unique_ptr<ExprAST> condition
    );

    JoinType get_join_type() const { return join_type_; }
    TableRefAST* get_right_table() const { return right_table_.get(); }
    ExprAST* get_condition() const { return condition_.get(); }

    std::string to_string() const override;

private:
    JoinType join_type_;
    std::unique_ptr<TableRefAST> right_table_;
    std::unique_ptr<ExprAST> condition_;
};
```

**验收标准**:
- AST 节点能够正确存储 JOIN 信息
- `to_string()` 能够输出可读的 JOIN 表示

---

### 1.4 扩展 SelectAST 支持 join_clauses_ 列表
**文件**: `include/sql/ast/ast.h`

**任务**:
- [ ] 在 `SelectAST` 中添加 `join_clauses_` 成员
- [ ] 更新构造函数接受 JOIN 子句
- [ ] 添加 `get_joins()` getter 方法
- [ ] 更新 `to_string()` 方法

**代码示例**:
```cpp
class SelectAST : public StmtAST {
public:
    SelectAST(
        std::vector<std::unique_ptr<ExprAST>> select_list,
        std::unique_ptr<TableRefAST> from,
        std::vector<std::unique_ptr<JoinClauseAST>> joins,  // 新增
        std::unique_ptr<ExprAST> where = nullptr
    );

    const std::vector<std::unique_ptr<JoinClauseAST>>& get_joins() const {
        return join_clauses_;
    }

private:
    std::vector<std::unique_ptr<JoinClauseAST>> join_clauses_;  // 新增
};
```

**验收标准**:
- SelectAST 能够存储多个 JOIN 子句
- 向后兼容：无 JOIN 的查询仍然正常工作

---

### 1.5 扩展 TableRefAST 支持表别名
**文件**: `include/sql/ast/ast.h`

**任务**:
- [ ] 在 `TableRefAST` 中添加 `alias_` 成员
- [ ] 更新构造函数支持别名
- [ ] 添加 `get_alias()` 和 `has_alias()` 方法
- [ ] 更新 Parser 解析表别名（如 "t1 AS users"）

**代码示例**:
```cpp
class TableRefAST : public ASTNode {
public:
    TableRefAST(const std::string& name, const std::string& alias = "")
        : ASTNode(ASTType::TABLE_REF),
          table_name_(name),
          alias_(alias) {}

    const std::string& get_alias() const { return alias_; }
    bool has_alias() const { return !alias_.empty(); }

    // 返回实际引用名（别名优先）
    const std::string& get_reference_name() const {
        return alias_.empty() ? table_name_ : alias_;
    }

private:
    std::string table_name_;
    std::string alias_;  // 新增
};
```

**验收标准**:
- 能够解析 "FROM users u" 或 "FROM users AS u"
- 能够在 JOIN 中使用别名

---

### 1.6 编写 Parser 层单元测试
**文件**: `tests/unit/sql/parser/test_parser_join.cpp`

**任务**:
- [ ] 测试简单 JOIN 解析
- [ ] 测试带别名的 JOIN
- [ ] 测试多个 JOIN
- [ ] 测试错误情况（缺少 ON 等）

**测试用例**:
```cpp
TEST(ParserJoinTest, SimpleInnerJoin) {
    SQLParser parser("SELECT * FROM t1 JOIN t2 ON t1.id = t2.id");
    auto ast = parser.parse();
    ASSERT_NE(ast, nullptr);

    auto select = dynamic_cast<SelectAST*>(ast.get());
    ASSERT_EQ(select->get_joins().size(), 1);
    ASSERT_EQ(select->get_joins()[0]->get_join_type(), JoinType::INNER);
}

TEST(ParserJoinTest, JoinWithAlias) {
    SQLParser parser("SELECT u.name FROM users u JOIN orders o ON u.id = o.user_id");
    auto ast = parser.parse();
    // ... 验证解析结果
}
```

**验收标准**:
- 所有 Parser 测试通过
- 代码覆盖率 > 80%

---

## Phase 2: Compiler & 语义分析层（第3周）

### 2.1 扩展 Compiler 创建 JoinStatement
**文件**: `include/sql/compiler/statement.h`, `src/sql/compiler/statement.cpp`

**任务**:
- [ ] 定义 `JoinInfo` 结构体存储 JOIN 信息
- [ ] 扩展 `SelectStatement` 支持 JOIN
- [ ] 更新 `Compiler::compile_select()` 处理 JOIN

**代码示例**:
```cpp
struct JoinInfo {
    std::string table_name;
    std::string alias;
    JoinType join_type;
    std::unique_ptr<Expression> condition;
    TableSchema schema;  // 表的 schema 信息
};

class SelectStatement : public Statement {
public:
    // 添加 JOIN 相关方法
    void add_join(JoinInfo join_info);
    const std::vector<JoinInfo>& get_joins() const;

    // 获取所有涉及的表
    std::vector<std::string> get_all_tables() const;

private:
    std::vector<JoinInfo> joins_;  // 新增
};
```

**验收标准**:
- Compiler 能够将 AST 转换为包含 JOIN 信息的 Statement
- JOIN 信息完整（表名、别名、条件、类型）

---

### 2.2 实现列引用解析器
**文件**: `src/sql/compiler/compiler.cpp`

**任务**:
- [ ] 实现 `resolve_column_reference()` 方法
- [ ] 支持 `table.column` 和 `alias.column` 格式
- [ ] 支持无限定符的列引用（需要查找所属表）

**代码示例**:
```cpp
Status Compiler::resolve_column_reference(
    ColumnRefExpression* col_ref,
    const std::vector<JoinInfo>& joins
) {
    if (col_ref->has_table_qualifier()) {
        // 显式表名/别名：t1.id
        std::string table_ref = col_ref->get_table_name();

        // 在 FROM 和 JOINs 中查找表
        for (auto& join : joins) {
            if (join.alias == table_ref || join.table_name == table_ref) {
                // 验证列存在
                if (!has_column(join.schema, col_ref->get_column_name())) {
                    return Status::InvalidArgument(
                        "Column " + col_ref->get_column_name() +
                        " not found in table " + table_ref
                    );
                }
                return Status::OK();
            }
        }

        return Status::NotFound("Table " + table_ref + " not found");
    } else {
        // 隐式表名：id - 需要消歧
        return resolve_unqualified_column(col_ref, joins);
    }
}
```

**验收标准**:
- 能够正确解析 "t1.id", "users.name" 等带限定符的列
- 能够处理表别名

---

### 2.3 实现列消歧逻辑
**文件**: `src/sql/compiler/compiler.cpp`

**任务**:
- [ ] 实现 `resolve_unqualified_column()` 方法
- [ ] 检测列名冲突（多个表有同名列）
- [ ] 生成详细的错误信息

**代码示例**:
```cpp
Status Compiler::resolve_unqualified_column(
    ColumnRefExpression* col_ref,
    const std::vector<JoinInfo>& joins
) {
    std::string col_name = col_ref->get_column_name();
    std::vector<std::string> matching_tables;

    // 在所有表中查找该列
    for (auto& join : joins) {
        if (has_column(join.schema, col_name)) {
            matching_tables.push_back(
                join.alias.empty() ? join.table_name : join.alias
            );
        }
    }

    if (matching_tables.empty()) {
        return Status::NotFound("Column " + col_name + " not found in any table");
    } else if (matching_tables.size() > 1) {
        // 列名冲突
        std::string error_msg = "Column " + col_name + " is ambiguous. Found in: ";
        for (size_t i = 0; i < matching_tables.size(); i++) {
            if (i > 0) error_msg += ", ";
            error_msg += matching_tables[i];
        }
        return Status::InvalidArgument(error_msg);
    }

    // 唯一匹配，自动添加表限定符
    col_ref->set_table_name(matching_tables[0]);
    return Status::OK();
}
```

**验收标准**:
- 能够检测并报告冲突的列名
- 能够自动解析无歧义的列引用

---

### 2.4 实现 JOIN 条件语义分析和类型检查
**文件**: `src/sql/compiler/compiler.cpp`

**任务**:
- [ ] 验证 JOIN 条件中的列存在
- [ ] 验证 JOIN 条件的类型兼容性
- [ ] 支持复杂的 JOIN 条件（AND, OR）

**代码示例**:
```cpp
Status Compiler::verify_join_condition(
    Expression* condition,
    const JoinInfo& left_join,
    const JoinInfo& right_join
) {
    // 解析条件中的列引用
    auto column_refs = extract_column_references(condition);

    for (auto* col_ref : column_refs) {
        // 验证列存在于左表或右表
        bool found = false;

        if (has_column(left_join.schema, col_ref->get_column_name()) ||
            has_column(right_join.schema, col_ref->get_column_name())) {
            found = true;
        }

        if (!found) {
            return Status::NotFound(
                "Column " + col_ref->get_column_name() +
                " not found in join tables"
            );
        }
    }

    // 类型检查：确保比较的两侧类型兼容
    if (auto* binary = dynamic_cast<BinaryExpression*>(condition)) {
        if (binary->get_op() == BinaryOperatorType::EQUAL) {
            DataType left_type = get_expression_type(binary->get_left());
            DataType right_type = get_expression_type(binary->get_right());

            if (!are_types_compatible(left_type, right_type)) {
                return Status::InvalidArgument(
                    "Type mismatch in JOIN condition: " +
                    DataTypeToString(left_type) + " vs " +
                    DataTypeToString(right_type)
                );
            }
        }
    }

    return Status::OK();
}
```

**验收标准**:
- 能够验证 JOIN 条件的正确性
- 能够检测类型不匹配

---

### 2.5 实现错误报告
**文件**: `src/sql/compiler/compiler.cpp`

**任务**:
- [ ] 实现友好的错误消息
- [ ] 提供错误位置信息（行号、列号）
- [ ] 提供修复建议

**错误示例**:
```
ERROR: Column 'id' is ambiguous in JOIN query
  Found in tables: users, orders
  Suggestion: Use qualified column name like 'users.id' or 'orders.id'

ERROR: Table 'products' not found in database
  Available tables: users, orders, categories

ERROR: Type mismatch in JOIN condition
  Left side: users.id (INT)
  Right side: orders.amount (DECIMAL)
```

**验收标准**:
- 错误信息清晰易懂
- 包含修复建议

---

### 2.6 编写 Compiler 层单元测试
**文件**: `tests/unit/sql/compiler/test_compiler_join.cpp`

**任务**:
- [ ] 测试列引用解析
- [ ] 测试列消歧
- [ ] 测试 JOIN 条件验证
- [ ] 测试错误情况

**测试用例**:
```cpp
TEST(CompilerJoinTest, ResolveQualifiedColumn) {
    // 测试 t1.id 的解析
}

TEST(CompilerJoinTest, DetectAmbiguousColumn) {
    // 测试冲突列名检测
    // CREATE TABLE t1 (id INT, name STRING);
    // CREATE TABLE t2 (id INT, score INT);
    // SELECT id FROM t1 JOIN t2 ...  -- 应该报错
}

TEST(CompilerJoinTest, ValidateJoinCondition) {
    // 测试 JOIN 条件验证
}
```

**验收标准**:
- 所有 Compiler 测试通过
- 覆盖主要的错误场景

---

## Phase 3: Operator & Executor 层（第4-5周）

### 3.1 定义 JoinOperator 基类接口
**文件**: `include/exec/operators/join_operator.h`

**任务**:
- [ ] 定义 `JoinOperator` 基类
- [ ] 定义通用的 JOIN 接口
- [ ] 添加辅助方法（条件求值、行合并等）

**代码示例**:
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
    Status initialize(ExecutionContext* context) override = 0;
    Status get_next(ExecutionContext* context, DataChunk& chunk) override = 0;
    Status reset() override;

    std::vector<std::string> get_output_columns() const override;
    std::vector<DataType> get_output_types() const override;

protected:
    // 子 Operator
    std::unique_ptr<Operator> left_child_;
    std::unique_ptr<Operator> right_child_;

    // JOIN 配置
    JoinType join_type_;
    Expression* join_condition_;

    // 辅助方法
    bool evaluate_join_condition(
        const DataChunk& left_chunk,
        size_t left_row_idx,
        const DataChunk& right_chunk,
        size_t right_row_idx
    );

    void merge_rows(
        DataChunk& output,
        const DataChunk& left_chunk,
        size_t left_row_idx,
        const DataChunk& right_chunk,
        size_t right_row_idx
    );
};
```

**验收标准**:
- JoinOperator 接口清晰
- 提供通用的辅助方法

---

### 3.2-3.7 实现 NestedLoopJoinOperator
**文件**:
- `include/exec/operators/nested_loop_join_operator.h`
- `src/exec/operators/nested_loop_join_operator.cpp`

**任务**:
- [ ] 实现 NestedLoopJoinOperator 类
- [ ] 实现 initialize() 方法
- [ ] 实现 get_next() 方法（双重循环）
- [ ] 实现 JOIN 条件求值
- [ ] 实现行合并逻辑
- [ ] 支持 INNER JOIN
- [ ] 编写单元测试

**核心算法**:
```cpp
Status NestedLoopJoinOperator::get_next(ExecutionContext* context, DataChunk& chunk) {
    chunk.clear();

    while (true) {
        // 外层循环：遍历左表
        if (!left_chunk_valid_) {
            Status s = left_child_->get_next(context, left_chunk_);
            if (!s.ok() || left_chunk_.empty()) {
                set_state(OperatorState::FINISHED);
                return Status::OK();
            }
            left_chunk_valid_ = true;
            left_row_idx_ = 0;
        }

        // 对每个左表行，扫描所有右表行
        while (left_row_idx_ < left_chunk_.row_count) {
            // 重置右表扫描
            if (!right_scan_started_) {
                right_child_->reset();
                right_scan_started_ = true;
                right_chunk_valid_ = false;
            }

            // 内层循环：遍历右表
            while (true) {
                if (!right_chunk_valid_) {
                    Status s = right_child_->get_next(context, right_chunk_);
                    if (!s.ok() || right_chunk_.empty()) {
                        // 右表扫描完毕，移到下一个左表行
                        left_row_idx_++;
                        right_scan_started_ = false;
                        break;
                    }
                    right_chunk_valid_ = true;
                    right_row_idx_ = 0;
                }

                // 匹配左右表行
                while (right_row_idx_ < right_chunk_.row_count) {
                    if (evaluate_join_condition(
                        left_chunk_, left_row_idx_,
                        right_chunk_, right_row_idx_
                    )) {
                        // 匹配成功，合并行
                        merge_rows(chunk,
                            left_chunk_, left_row_idx_,
                            right_chunk_, right_row_idx_
                        );

                        // 如果输出 chunk 满了，返回
                        if (chunk.row_count >= DEFAULT_BATCH_SIZE) {
                            right_row_idx_++;
                            return Status::OK();
                        }
                    }
                    right_row_idx_++;
                }

                right_chunk_valid_ = false;
            }
        }

        left_chunk_valid_ = false;
    }
}
```

**验收标准**:
- 能够正确执行 INNER JOIN
- 单元测试覆盖各种场景
- 性能测试基准建立

---

### 3.8-3.12 实现 HashJoinOperator
**文件**:
- `include/exec/operators/hash_join_operator.h`
- `src/exec/operators/hash_join_operator.cpp`

**任务**:
- [ ] 实现 HashJoinOperator 类
- [ ] 实现 Build Phase（构建哈希表）
- [ ] 实现 Probe Phase（探测哈希表）
- [ ] 实现哈希函数
- [ ] 处理哈希冲突
- [ ] 编写单元测试

**核心算法**:
```cpp
Status HashJoinOperator::initialize(ExecutionContext* context) {
    // 1. 初始化子节点
    left_child_->initialize(context);
    right_child_->initialize(context);

    // 2. Build Phase: 构建哈希表（使用右表）
    return build_hash_table(context);
}

Status HashJoinOperator::build_hash_table(ExecutionContext* context) {
    DataChunk right_chunk;

    while (true) {
        Status s = right_child_->get_next(context, right_chunk);
        if (!s.ok() || right_chunk.empty()) break;

        // 为每一行构建哈希表条目
        for (size_t i = 0; i < right_chunk.row_count; i++) {
            // 提取 JOIN key
            std::string join_key = extract_join_key(right_chunk, i);

            // 计算哈希值
            size_t hash = std::hash<std::string>{}(join_key);

            // 存储到哈希表
            hash_table_[hash].push_back(right_chunk.rows[i]);
        }
    }

    return Status::OK();
}

Status HashJoinOperator::get_next(ExecutionContext* context, DataChunk& chunk) {
    chunk.clear();

    // Probe Phase: 探测左表
    while (true) {
        if (!left_chunk_valid_) {
            Status s = left_child_->get_next(context, left_chunk_);
            if (!s.ok() || left_chunk_.empty()) {
                set_state(OperatorState::FINISHED);
                return Status::OK();
            }
            left_chunk_valid_ = true;
            left_row_idx_ = 0;
        }

        // 对每个左表行，在哈希表中查找匹配
        while (left_row_idx_ < left_chunk_.row_count) {
            // 提取 JOIN key
            std::string join_key = extract_join_key(left_chunk_, left_row_idx_);

            // 计算哈希值
            size_t hash = std::hash<std::string>{}(join_key);

            // 在哈希表中查找
            if (hash_table_.count(hash) > 0) {
                auto& candidates = hash_table_[hash];

                // 遍历候选行（处理哈希冲突）
                for (auto& right_row : candidates) {
                    if (evaluate_join_condition_detailed(
                        left_chunk_, left_row_idx_, right_row
                    )) {
                        // 匹配成功
                        merge_rows(chunk,
                            left_chunk_, left_row_idx_, right_row
                        );

                        if (chunk.row_count >= DEFAULT_BATCH_SIZE) {
                            return Status::OK();
                        }
                    }
                }
            }

            left_row_idx_++;
        }

        left_chunk_valid_ = false;
    }
}

std::string HashJoinOperator::extract_join_key(
    const DataChunk& chunk,
    size_t row_idx
) {
    // 从 JOIN 条件中提取键列
    // 例如：t1.id = t2.id，提取 id 列的值

    // 这里简化处理，实际需要解析 join_condition_
    // 假设是简单的等值条件
    size_t key_col_idx = /* 从条件中获取列索引 */ 0;
    return chunk.columns[key_col_idx].get_string(row_idx);
}
```

**验收标准**:
- Hash Join 正确性验证
- 性能优于 Nested Loop Join（中大型表）
- 单元测试通过

---

### 3.13-3.16 扩展 Planner 和 Executor
**文件**:
- `src/exec/plan/planner.cpp`
- `src/exec/executor/new_executor.cpp`

**任务**:
- [ ] 扩展 Planner 生成 JOIN 执行计划
- [ ] 实现 JOIN 算法选择逻辑
- [ ] 更新 Executor 执行 JOIN Plan
- [ ] 编写 Planner 单元测试

**代码示例**:
```cpp
Status Planner::create_join_plan(
    SelectStatement* stmt,
    std::unique_ptr<Plan>& plan
) {
    // 1. 获取左表（FROM 表）
    auto left_scan = create_scan_operator(
        stmt->get_from_table(),
        stmt->get_required_columns()
    );

    std::unique_ptr<Operator> current_op = std::move(left_scan);

    // 2. 为每个 JOIN 创建 JOIN Operator
    for (auto& join_info : stmt->get_joins()) {
        // 获取右表
        auto right_scan = create_scan_operator(
            join_info.table_name,
            join_info.required_columns
        );

        // 选择 JOIN 算法
        std::unique_ptr<Operator> join_op;

        if (should_use_hash_join(stmt, join_info)) {
            join_op = std::make_unique<HashJoinOperator>(
                std::move(current_op),
                std::move(right_scan),
                join_info.join_type,
                join_info.condition.get()
            );
        } else {
            join_op = std::make_unique<NestedLoopJoinOperator>(
                std::move(current_op),
                std::move(right_scan),
                join_info.join_type,
                join_info.condition.get()
            );
        }

        current_op = std::move(join_op);
    }

    // 3. 添加 WHERE 过滤
    if (stmt->has_where()) {
        auto filter = create_filter_operator(stmt->get_where());
        filter->set_child(std::move(current_op));
        current_op = std::move(filter);
    }

    // 4. 添加投影
    auto proj = create_projection_operator(stmt->get_select_list());
    proj->set_child(std::move(current_op));
    current_op = std::move(proj);

    // 5. 最终结果
    auto final = std::make_unique<FinalResultOperator>();
    final->set_child(std::move(current_op));

    plan = std::make_unique<SelectPlan>("join-query", std::move(final));
    return Status::OK();
}

bool Planner::should_use_hash_join(
    SelectStatement* stmt,
    const JoinInfo& join_info
) {
    // 简单策略：
    // 1. 如果是等值连接（=），使用 Hash Join
    // 2. 如果表较小（< 1000 行），使用 Nested Loop
    // 3. 其他情况根据统计信息决定

    if (!is_equi_join(join_info.condition)) {
        return false;  // 非等值连接，使用 Nested Loop
    }

    // TODO: 获取表统计信息
    size_t left_rows = estimate_table_rows(stmt->get_from_table());
    size_t right_rows = estimate_table_rows(join_info.table_name);

    if (left_rows < 1000 && right_rows < 1000) {
        return false;  // 小表，使用 Nested Loop
    }

    return true;  // 默认使用 Hash Join
}
```

**验收标准**:
- Planner 能够生成正确的 JOIN 执行计划
- 算法选择合理
- Executor 能够执行 JOIN Plan

---

## Phase 4: 测试 & 优化（第6周）

### 4.1-4.4 端到端测试
**文件**: `tests/integration/test_join_e2e.cpp`

**任务**:
- [ ] 简单两表 INNER JOIN
- [ ] 带 WHERE 条件的 JOIN
- [ ] SELECT 特定列的 JOIN
- [ ] 带表别名的 JOIN
- [ ] 多个 JOIN（3+ 表）

**测试场景**:
```cpp
TEST(JoinE2ETest, SimpleTwoTableJoin) {
    // CREATE TABLE users (id INT, name STRING);
    // CREATE TABLE orders (id INT, user_id INT, amount DECIMAL);
    // INSERT INTO users VALUES (1, 'Alice'), (2, 'Bob');
    // INSERT INTO orders VALUES (1, 1, 100.0), (2, 1, 200.0);
    // SELECT users.name, orders.amount
    // FROM users JOIN orders ON users.id = orders.user_id;

    // 预期结果：
    // Alice | 100.0
    // Alice | 200.0
}

TEST(JoinE2ETest, JoinWithWhereClause) {
    // SELECT users.name
    // FROM users JOIN orders ON users.id = orders.user_id
    // WHERE orders.amount > 150;

    // 预期结果：
    // Alice (order 200.0)
}

TEST(JoinE2ETest, JoinWithAlias) {
    // SELECT u.name, o.amount
    // FROM users u JOIN orders o ON u.id = o.user_id;
}

TEST(JoinE2ETest, MultiTableJoin) {
    // CREATE TABLE categories (id INT, name STRING);
    // SELECT u.name, o.amount, c.name
    // FROM users u
    // JOIN orders o ON u.id = o.user_id
    // JOIN categories c ON o.category_id = c.id;
}
```

**验收标准**:
- 所有 E2E 测试通过
- 结果正确性验证

---

### 4.5 性能测试
**文件**: `tests/benchmark/benchmark_join.cpp`

**任务**:
- [ ] Nested Loop vs Hash Join 性能对比
- [ ] 不同表大小的性能测试
- [ ] 性能回归测试

**性能指标**:
```
表大小  | Nested Loop | Hash Join | 性能提升
-------|------------|-----------|----------
100    | 10ms       | 15ms      | -50%
1000   | 500ms      | 80ms      | 6.25x
10000  | 50s        | 1.2s      | 41.7x
100000 | timeout    | 15s       | -
```

**验收标准**:
- Hash Join 在大表上性能显著优于 Nested Loop
- 小表上 Nested Loop 可能更快（避免哈希表开销）

---

### 4.6 性能优化
**任务**:
- [ ] Profile 热点代码
- [ ] 优化哈希函数
- [ ] 优化内存分配
- [ ] 优化批处理大小

**优化方向**:
1. **哈希表优化**: 使用更高效的哈希表实现
2. **内存池**: 减少内存分配开销
3. **SIMD**: 向量化比较操作
4. **缓存友好**: 改进数据布局

**验收标准**:
- 性能提升 > 20%
- 内存使用合理

---

### 4.7 文档更新
**文件**: `README.md`, `doc/JOIN_USER_GUIDE.md`

**任务**:
- [ ] 更新功能列表
- [ ] 编写 JOIN 使用指南
- [ ] 添加示例和最佳实践
- [ ] 更新 API 文档

**文档内容**:
```markdown
# MiniDB JOIN 使用指南

## 基本语法

### INNER JOIN
SELECT columns
FROM table1
INNER JOIN table2 ON table1.key = table2.key;

### 使用别名
SELECT u.name, o.amount
FROM users u
JOIN orders o ON u.id = o.user_id;

### 多表 JOIN
SELECT *
FROM t1
JOIN t2 ON t1.id = t2.t1_id
JOIN t3 ON t2.id = t3.t2_id;

## 性能建议
1. 对于大表 JOIN，确保 JOIN key 列有索引
2. 使用表别名简化查询
3. 尽早应用 WHERE 过滤
```

**验收标准**:
- 文档完整准确
- 包含清晰的示例

---

### 4.8 代码审查和重构
**任务**:
- [ ] 代码风格检查
- [ ] 重复代码消除
- [ ] 性能瓶颈识别
- [ ] 错误处理完善

**审查清单**:
- [ ] 代码符合项目风格指南
- [ ] 没有内存泄漏
- [ ] 错误处理完善
- [ ] 日志信息充分
- [ ] 单元测试覆盖率 > 80%
- [ ] 文档完整

**验收标准**:
- Code Review 通过
- 所有测试通过
- 准备合并到主分支

---

## 总结

### 工作量估算
- **Phase 1 (Parser & AST)**: 2周，6个任务
- **Phase 2 (Compiler)**: 1周，6个任务
- **Phase 3 (Operator & Executor)**: 2周，16个任务
- **Phase 4 (测试 & 优化)**: 1周，6个任务

**总计**: 6周，34个任务

### 里程碑
1. **Week 2 结束**: 完成 Parser 和 AST，能够解析 JOIN 语句
2. **Week 3 结束**: 完成语义分析，能够验证 JOIN 查询
3. **Week 5 结束**: 完成执行引擎，能够执行简单 JOIN
4. **Week 6 结束**: 完成测试优化，准备发布

### 风险和挑战
1. **性能优化**: Hash Join 性能调优可能需要额外时间
2. **复杂度**: 多表 JOIN 和 Outer Join 增加实现复杂度
3. **兼容性**: 确保 JOIN 功能不破坏现有功能

### 下一步
等待审批后，可以开始 Phase 1 的实现。建议采用增量开发方式，每完成一个 Phase 就进行测试和验证。

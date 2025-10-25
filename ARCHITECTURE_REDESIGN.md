# MiniDB 架构重新设计

## 🎯 当前架构问题

### ❌ 现有架构（不合理）
```
SQL文本 → Parser → AST + Statement → Executor → Plan + 执行
```

**问题:**
- AST和Statement混合在一起
- 缺少独立的编译器层
- 没有优化器层
- Executor承担了太多职责

### ✅ 正确的架构（应该实现）
```
SQL文本 → Parser → AST → Compiler → Statement → Optimizer → Statement → Executor → Plan → 执行
```

## 🏗️ 重新设计的架构

### 1. 词法分析器 (Tokenizer)
```cpp
SQL文本 → Tokens
"SELECT * FROM users WHERE id = 1;" → [SELECT, *, FROM, users, WHERE, id, =, 1, ;]
```

### 2. 语法分析器 (Parser) 
```cpp
Tokens → AST (抽象语法树)
[SELECT, *, FROM, users, WHERE, id, =, 1] → SelectNode{
    select_list: [ColumnRef("*")],
    from_table: TableRef("users"),
    where_clause: BinaryExpr(ColumnRef("id"), EQUAL, Literal(1))
}
```

### 3. 编译器 (Compiler)
```cpp
AST → Statement (执行语句)
SelectNode → SelectStatement{
    target_table: "users",
    projection: ["*"],
    filter: FilterCondition{column: "id", op: EQUAL, value: 1}
}
```

### 4. 优化器 (Optimizer) - 可选
```cpp
Statement → OptimizedStatement
SelectStatement → OptimizedSelectStatement{
    // 可能的优化：
    // - 谓词下推
    // - 列裁剪
    // - 索引选择
    // - 连接重排序
}
```

### 5. 执行器 (Executor)
```cpp
Statement → ExecutionPlan → 执行结果
SelectStatement → {
    ScanOperator("users") → 
    FilterOperator(id = 1) → 
    ProjectionOperator(*) → 
    FinalResultOperator
}
```

## 📁 新的目录结构

```
src/
├── sql/
│   ├── lexer/           # 词法分析器
│   │   └── tokenizer.cpp
│   ├── parser/          # 语法分析器
│   │   └── parser.cpp
│   ├── ast/             # 抽象语法树
│   │   ├── ast_node.cpp
│   │   └── ast_visitor.cpp
│   ├── compiler/        # 编译器 (NEW)
│   │   ├── compiler.cpp
│   │   └── statement_builder.cpp
│   └── optimizer/       # 优化器 (NEW)
│       ├── optimizer.cpp
│       └── rule_based_optimizer.cpp
├── exec/
│   ├── statement/       # 执行语句 (NEW)
│   │   ├── statement.cpp
│   │   ├── select_statement.cpp
│   │   ├── insert_statement.cpp
│   │   └── create_statement.cpp
│   ├── planner/         # 执行计划器 (NEW)
│   │   └── planner.cpp
│   ├── operators/       # 执行算子
│   └── executor/        # 执行器
└── ...
```

## 🔧 新的类设计

### AST层 (纯语法树)
```cpp
// 纯粹的语法树节点，不包含执行逻辑
class ASTNode {
    virtual void accept(ASTVisitor* visitor) = 0;
    virtual std::string to_string() const = 0;
};

class SelectNode : public ASTNode {
    std::vector<std::unique_ptr<ExpressionNode>> select_list_;
    std::unique_ptr<TableRefNode> from_table_;
    std::unique_ptr<ExpressionNode> where_clause_;
};
```

### Statement层 (执行语句)
```cpp
// 编译后的执行语句，包含执行所需的信息
class Statement {
    virtual StatementType get_type() const = 0;
    virtual std::string to_string() const = 0;
};

class SelectStatement : public Statement {
    std::string target_table_;
    std::vector<std::string> projection_columns_;
    std::unique_ptr<FilterCondition> filter_;
    std::unique_ptr<OrderByClause> order_by_;
};
```

### Compiler层 (编译器)
```cpp
class Compiler {
public:
    Status compile(ASTNode* ast, std::unique_ptr<Statement>& stmt);
    
private:
    Status compile_select(SelectNode* node, std::unique_ptr<SelectStatement>& stmt);
    Status compile_insert(InsertNode* node, std::unique_ptr<InsertStatement>& stmt);
    Status compile_create_table(CreateTableNode* node, std::unique_ptr<CreateTableStatement>& stmt);
};
```

### Optimizer层 (优化器)
```cpp
class Optimizer {
public:
    Status optimize(std::unique_ptr<Statement> input, std::unique_ptr<Statement>& output);
    
private:
    Status apply_predicate_pushdown(SelectStatement* stmt);
    Status apply_column_pruning(SelectStatement* stmt);
    Status apply_index_selection(SelectStatement* stmt);
};
```

### Planner层 (计划器)
```cpp
class Planner {
public:
    Status create_plan(Statement* stmt, std::unique_ptr<Operator>& plan);
    
private:
    Status create_select_plan(SelectStatement* stmt, std::unique_ptr<Operator>& plan);
    Status create_insert_plan(InsertStatement* stmt, std::unique_ptr<Operator>& plan);
};
```

## 🔄 新的执行流程

### 完整的SQL执行流程
```cpp
class SQLEngine {
public:
    QueryResult execute_sql(const std::string& sql) {
        // 1. 词法分析
        Tokenizer tokenizer(sql);
        std::vector<Token> tokens;
        Status status = tokenizer.tokenize(tokens);
        if (!status.ok()) return QueryResult::error_result(status.ToString());
        
        // 2. 语法分析
        Parser parser(tokens);
        std::unique_ptr<ASTNode> ast;
        status = parser.parse(ast);
        if (!status.ok()) return QueryResult::error_result(status.ToString());
        
        // 3. 编译
        Compiler compiler;
        std::unique_ptr<Statement> stmt;
        status = compiler.compile(ast.get(), stmt);
        if (!status.ok()) return QueryResult::error_result(status.ToString());
        
        // 4. 优化 (可选)
        if (enable_optimization_) {
            Optimizer optimizer;
            std::unique_ptr<Statement> optimized_stmt;
            status = optimizer.optimize(std::move(stmt), optimized_stmt);
            if (status.ok()) {
                stmt = std::move(optimized_stmt);
            }
        }
        
        // 5. 计划生成
        Planner planner;
        std::unique_ptr<Operator> plan;
        status = planner.create_plan(stmt.get(), plan);
        if (!status.ok()) return QueryResult::error_result(status.ToString());
        
        // 6. 执行
        Executor executor;
        std::string result;
        status = executor.execute_plan(std::move(plan), result);
        if (!status.ok()) return QueryResult::error_result(status.ToString());
        
        return QueryResult::success_result(result);
    }
};
```

## 🎯 重构计划

### 阶段1: 分离AST和Statement
1. 创建纯粹的AST节点类
2. 创建独立的Statement类
3. 实现AST到Statement的转换

### 阶段2: 实现Compiler层
1. 创建Compiler类
2. 实现各种AST节点到Statement的编译
3. 处理语义分析和类型检查

### 阶段3: 实现Optimizer层
1. 创建Optimizer基础框架
2. 实现基本的优化规则
3. 集成到执行流程中

### 阶段4: 重构Executor
1. 简化Executor职责
2. 创建独立的Planner
3. 清理执行逻辑

## 🔍 当前代码问题分析

### 问题1: AST和Statement混合
```cpp
// 当前代码 (错误)
class SelectStatement : public Statement, public ASTNode {
    // 既是AST节点又是执行语句，职责不清
};

// 应该是 (正确)
class SelectNode : public ASTNode {
    // 纯粹的语法树节点
};

class SelectStatement : public Statement {
    // 纯粹的执行语句
};
```

### 问题2: Parser直接生成Statement
```cpp
// 当前代码 (错误)
Parser::parse() → std::unique_ptr<Statement>

// 应该是 (正确)
Parser::parse() → std::unique_ptr<ASTNode>
Compiler::compile(ASTNode*) → std::unique_ptr<Statement>
```

### 问题3: Executor承担编译职责
```cpp
// 当前代码 (错误)
Executor::execute_statement(Statement*) {
    // 直接将Statement转换为执行计划
    build_select_plan(stmt, plan);
}

// 应该是 (正确)
Planner::create_plan(Statement*) → ExecutionPlan
Executor::execute_plan(ExecutionPlan*) → Result
```

## 🚀 重构的好处

### 1. 职责分离清晰
- **Parser**: 只负责语法分析
- **Compiler**: 只负责语义分析和Statement生成
- **Optimizer**: 只负责优化
- **Planner**: 只负责计划生成
- **Executor**: 只负责计划执行

### 2. 扩展性更好
- 容易添加新的SQL语法
- 容易添加新的优化规则
- 容易添加新的执行算子

### 3. 测试性更好
- 每个层次可以独立测试
- 更容易定位问题
- 更好的单元测试覆盖

### 4. 维护性更好
- 代码结构清晰
- 修改影响范围小
- 符合编译器设计原理

## 💡 建议

这是一个重大的架构重构，建议：

1. **保持当前版本可用** - 现有功能正常工作
2. **逐步重构** - 分阶段实现新架构
3. **保持向后兼容** - 重构期间保持API稳定
4. **充分测试** - 每个阶段都要有完整测试

**您的架构观察非常准确！这确实是一个需要重构的重要问题。** 🎯

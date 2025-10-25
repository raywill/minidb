# MiniDB 架构重构文档

## 重构目标

将原来混乱的架构重构为清晰的分层架构：
- **Parser** 只负责生成 Token Tree (AST)
- **Compiler** 负责生成 Statement（逻辑查询）
- **Optimizer** 负责优化 Statement
- **Planner** 负责将 Statement 转换成 Plan（物理执行计划）
- **Executor** 负责执行 Plan

## 新架构流程

```
SQL文本
  ↓
[Parser] - 词法分析 + 语法分析
  ↓
AST (Abstract Syntax Tree) - 纯语法树
  ↓
[Compiler] - 语义分析 + 类型检查 + 符号解析
  ↓
Statement (Logical Query) - 逻辑查询
  ↓
[Optimizer] - 查询优化
  ↓
Optimized Statement - 优化后的逻辑查询
  ↓
[Planner] - 生成物理计划
  ↓
Plan (Physical Execution Plan) - 物理执行计划
  ↓
[Executor] - 执行计划
  ↓
QueryResult - 查询结果
```

## 新模块说明

### 1. AST (Abstract Syntax Tree)
**文件**: `include/sql/ast/ast.h`, `src/sql/ast/ast.cpp`

**职责**: 纯语法树节点，只表示SQL的语法结构，不包含任何语义信息

**主要类**:
- `StmtAST` - 语句AST基类
- `ExprAST` - 表达式AST基类
- `CreateTableAST`, `SelectAST`, `InsertAST`, etc.

### 2. Parser
**文件**: `include/sql/parser/new_parser.h`, `src/sql/parser/new_parser.cpp`

**职责**: 只负责词法分析和语法分析，生成AST

**输入**: SQL文本字符串
**输出**: AST (语法树)

### 3. Compiler
**文件**: `include/sql/compiler/compiler.h`, `src/sql/compiler/compiler.cpp`

**职责**: 将AST编译为Statement，过程中进行：
- 语义分析（表是否存在、列是否存在）
- 类型检查
- 符号解析（将列名解析为列索引）

**输入**: AST + Catalog
**输出**: Statement (逻辑查询)

### 4. Statement (Logical Query)
**文件**: `include/sql/compiler/statement.h`, `src/sql/compiler/statement.cpp`

**职责**: 表示逻辑查询，已经完成语义分析，包含：
- 已解析的表名
- 已解析的列名和列索引
- 类型检查后的表达式

**主要类**:
- `Statement` - 语句基类
- `Expression` - 表达式（已编译，包含列索引）
- `CreateTableStatement`, `SelectStatement`, etc.

### 5. Optimizer
**文件**: `include/sql/optimizer/optimizer.h`, `src/sql/optimizer/optimizer.cpp`

**职责**: 优化逻辑查询（当前为简单pass-through实现）

**未来可添加的优化**:
- 谓词下推 (Predicate Pushdown)
- 常量折叠 (Constant Folding)
- 列裁剪 (Column Pruning)
- 连接重排序 (Join Reordering)

**输入**: Statement
**输出**: Optimized Statement

### 6. Planner
**文件**: `include/exec/plan/planner.h`, `src/exec/plan/planner.cpp`

**职责**: 将逻辑查询转换为物理执行计划

**输入**: Statement + TableManager
**输出**: Plan (物理执行计划)

### 7. Plan (Physical Execution Plan)
**文件**: `include/exec/plan/plan.h`, `src/exec/plan/plan.cpp`

**职责**: 表示物理执行计划，包含：
- 打开的Table对象
- Operator树（对于SELECT）
- 所有执行所需的资源

**主要类**:
- `Plan` - 计划基类
- `CreateTablePlan`, `InsertPlan`, `SelectPlan`, etc.
- `SelectPlan` 包含完整的 Operator 树

### 8. QueryExecutor
**文件**: `include/exec/executor/new_executor.h`, `src/exec/executor/new_executor.cpp`

**职责**: 只负责执行物理计划

**输入**: Plan
**输出**: QueryResult

## 架构对比

### 旧架构（问题）
```
Parser → Statement (混合AST和语义信息)
     ↓
Executor (内部生成Plan并执行，职责混乱)
```

**问题**:
1. Parser直接生成Statement，混合了语法和语义
2. Executor既生成Plan又执行Plan
3. 没有Compiler层做语义分析
4. 没有Optimizer
5. Statement既是AST又是逻辑查询，职责不清

### 新架构（清晰）
```
Parser → AST
     ↓
Compiler → Statement
     ↓
Optimizer → Optimized Statement
     ↓
Planner → Plan
     ↓
Executor → Result
```

**优势**:
1. 每个模块职责单一明确
2. AST、Statement、Plan 三层清晰分离
3. 易于扩展和维护
4. 符合数据库系统标准架构

## 文件清单

### 新增文件
1. `include/sql/ast/ast.h`
2. `src/sql/ast/ast.cpp`
3. `include/sql/compiler/statement.h`
4. `src/sql/compiler/statement.cpp`
5. `include/sql/compiler/compiler.h`
6. `src/sql/compiler/compiler.cpp`
7. `include/exec/plan/plan.h`
8. `src/exec/plan/plan.cpp`
9. `include/exec/plan/planner.h`
10. `src/exec/plan/planner.cpp`
11. `include/sql/optimizer/optimizer.h`
12. `src/sql/optimizer/optimizer.cpp`
13. `include/sql/parser/new_parser.h`
14. `src/sql/parser/new_parser.cpp`
15. `include/exec/executor/new_executor.h`
16. `src/exec/executor/new_executor.cpp`

### 待废弃文件
1. `include/sql/ast/ast_node.h` (被 ast.h 替代)
2. `include/sql/ast/statements.h` (被 statement.h 替代)
3. `include/sql/parser/parser.h` (被 new_parser.h 替代)
4. `src/sql/parser/parser.cpp` (被 new_parser.cpp 替代)
5. `src/sql/ast/ast_node.cpp` (被 ast.cpp 替代)
6. `src/sql/ast/statements.cpp` (被 statement.cpp 替代)
7. `include/exec/executor/executor.h` (被 new_executor.h 替代)
8. `src/exec/executor/executor.cpp` (被 new_executor.cpp 替代)

## 下一步工作

1. ✅ 创建所有新模块
2. ⏳ 更新 CMakeLists.txt
3. ⏳ 更新 database_server.cpp 使用新架构
4. ⏳ 编译验证
5. ⏳ 测试验证
6. ⏳ 删除旧文件

## 使用示例

```cpp
// 新架构使用流程
SQLParser parser(sql_text);
std::unique_ptr<StmtAST> ast;
parser.parse(ast);

Compiler compiler(catalog);
std::unique_ptr<Statement> stmt;
compiler.compile(ast.get(), stmt);

Optimizer optimizer;
std::unique_ptr<Statement> optimized_stmt;
optimizer.optimize(stmt.get(), optimized_stmt);

Planner planner(catalog, table_manager);
std::unique_ptr<Plan> plan;
planner.create_plan(optimized_stmt ? optimized_stmt.get() : stmt.get(), plan);

QueryExecutor executor(catalog, table_manager);
QueryResult result = executor.execute_plan(plan.get());
```

# MiniDB 类型化表达式求值系统

## 概述

本文档描述了 MiniDB 中类型化表达式求值系统的设计和实现。该系统消除了基于字符串的表达式计算开销，使用原生类型（int64_t, double, bool, string）进行高效计算。

## 实现日期

2025-10-26

## 动机

### 旧系统的问题
原有的表达式求值系统使用字符串作为统一的中间表示：
- 所有值都转换为字符串（`std::to_string`）
- 运算时再解析回数值类型（`std::stod`, `std::stoi`）
- 大量字符串操作和内存分配开销
- 性能低下，尤其是在大量数据处理时

### 新系统的优势
1. **性能提升**：直接使用原生类型运算，避免字符串转换
2. **类型安全**：编译时确定表达式结果类型
3. **SQL 标准**：实现 SQL 标准的类型转换规则
4. **代码简洁**：统一的 Value 接口，移除 400+ 行字符串操作代码

## 架构设计

### 三层架构

```
┌─────────────────────────────────────────┐
│   Compiler (编译时类型推导)              │
│   - infer_binary_result_type()          │
│   - infer_function_result_type()        │
│   - Expression::set_result_type()       │
└─────────────────────────────────────────┘
                    ↓
┌─────────────────────────────────────────┐
│   TypedExpressionEvaluator (运行时求值)  │
│   - evaluate()                          │
│   - evaluate_row()                      │
│   - compute_arithmetic/comparison/...   │
└─────────────────────────────────────────┘
                    ↓
┌─────────────────────────────────────────┐
│   Value (类型化值容器)                   │
│   - Union 存储: int64_t/double/bool     │
│   - 独立存储: std::string               │
│   - as_int/as_double/as_string/as_bool  │
└─────────────────────────────────────────┘
```

## 核心组件

### 1. Value 类（src/common/value.h/cpp）

**功能**：高效的类型化值容器

**设计要点**：
- 使用 `union` 存储基本类型（零拷贝）
- 独立存储 `std::string`（需要堆分配）
- SQL 标准类型转换：`"12hello" + 3 = 15`（解析到第一个非数字字符）

**关键方法**：
```cpp
class Value {
    // 工厂方法
    static Value make_int(int64_t val);
    static Value make_double(double val);
    static Value make_bool(bool val);
    static Value make_string(const std::string& val);

    // 类型转换（SQL 标准）
    int64_t as_int() const;
    double as_double() const;
    bool as_bool() const;
    std::string as_string() const;

    // 比较运算符重载
    bool operator==(const Value& other) const;
    bool operator<(const Value& other) const;
    // ...
};
```

### 2. TypedExpressionEvaluator（src/exec/operators/typed_expression_evaluator.h/cpp）

**功能**：类型化表达式求值引擎

**关键特性**：
- 批量求值：`evaluate(DataChunk, vector<Value>&)`
- 单行求值：`evaluate_row(DataChunk, size_t, Value&)`
- 类型特化运算：整数用 `int64_t`，浮点用 `double`
- 递归求值：支持复杂嵌套表达式

**求值流程**：
```cpp
Expression (AST)
    ↓
evaluate_row() // 根据表达式类型分发
    ↓
├─ LiteralExpression    → evaluate_literal()
├─ ColumnRefExpression  → evaluate_column_ref()
├─ BinaryExpression     → evaluate_binary_op()
│                           ↓
│                       compute_arithmetic()
│                       compute_comparison()
│                       compute_logical()
└─ FunctionExpression   → evaluate_function()
    ↓
Value (结果)
```

### 3. Compiler 类型推导（src/sql/compiler/compiler.cpp）

**功能**：编译时确定表达式结果类型

**类型推导规则**：
```cpp
// 比较运算符 → BOOL
age > 30           → BOOL

// 算术运算 → 类型提升
INT + INT          → INT
INT + DECIMAL      → DECIMAL
DECIMAL + DECIMAL  → DECIMAL

// 字符串参与运算 → 转换为数值
STRING + INT       → 根据字符串内容决定
                     "123" → INT
                     "12.5" → DECIMAL

// 逻辑运算 → BOOL
condition AND condition → BOOL
```

**关键方法**：
```cpp
DataType infer_binary_result_type(
    DataType left_type,
    DataType right_type,
    BinaryOperatorType op
);

DataType infer_function_result_type(
    FunctionType func,
    const vector<DataType>& arg_types
);
```

## 集成点

### 1. FilterOperator（src/exec/operators/filter_operator.cpp）

**变更**：
- 移除旧的 `ExpressionEvaluator`（字符串版本）
- 使用 `TypedExpressionEvaluator`
- `evaluate_predicate()` 返回 `vector<Value>`，转换为 `vector<bool>`

**代码示例**：
```cpp
Status FilterOperator::evaluate_predicate(
    const DataChunk& input_chunk,
    std::vector<bool>& selection) {

    // 使用类型化求值器
    std::vector<Value> results;
    Status status = evaluator_->evaluate(input_chunk, results);
    if (!status.ok()) return status;

    // 将 Value 转换为布尔选择向量
    selection.clear();
    selection.reserve(results.size());
    for (const Value& val : results) {
        selection.push_back(val.as_bool());
    }

    return Status::OK();
}
```

### 2. NestedLoopJoinOperator（src/exec/operators/nested_loop_join_operator.cpp）

**变更**：
- JOIN 条件求值使用 `TypedExpressionEvaluator`
- 直接使用 `Value::as_bool()` 判断条件

**代码示例**：
```cpp
bool NestedLoopJoinOperator::evaluate_join_condition(...) {
    if (!join_condition_) return true;

    // 创建合并的 DataChunk
    DataChunk merged_chunk;
    // ... 添加左右表的列 ...

    // 使用类型化表达式求值器
    TypedExpressionEvaluator evaluator(join_condition_.get());
    Value result;
    Status status = evaluator.evaluate_row(merged_chunk, 0, result);

    if (!status.ok()) return false;

    // 直接转换为 bool
    return result.as_bool();
}
```

### 3. Executor（src/exec/executor/executor.cpp, new_executor.cpp）

**变更**：
- DELETE 语句的 WHERE 条件求值使用 `TypedExpressionEvaluator`
- `vector<Value>` → `vector<bool>` 用于行选择

## 性能对比

### 理论性能提升

| 操作类型 | 旧系统 | 新系统 | 预期提升 |
|---------|--------|--------|----------|
| 整数算术 | string → int64_t → string | 直接 int64_t 运算 | **3-5x** |
| 浮点算术 | string → double → string | 直接 double 运算 | **2-3x** |
| 比较操作 | string 解析 + 比较 | 直接类型比较 | **5-10x** |
| 内存分配 | 每次运算都分配 string | Union 零拷贝 | **显著减少** |

### 实际测试结果

**测试环境**：
- 数据集：4 行用户数据，4 行订单数据
- 查询类型：SELECT with WHERE, JOIN with WHERE

**测试用例**：

1. **WHERE 整数比较**
   ```sql
   SELECT * FROM users WHERE age > 28;
   ```
   - 输入：4 行
   - 输出：2 行
   - 状态：✅ 通过

2. **JOIN + WHERE**
   ```sql
   SELECT users.name, orders.amount
   FROM users JOIN orders ON users.id = orders.user_id
   WHERE orders.amount > 200;
   ```
   - JOIN 评估：16 次（4x4 笛卡尔积）
   - 过滤后：2 行
   - 状态：✅ 通过

## SQL 标准类型转换

### 字符串转数值规则

遵循 SQL 标准的隐式类型转换：

```cpp
"123"      → 123       // 纯数字
"12hello"  → 12        // 解析到第一个非数字
"hello"    → 0         // 无法解析，返回 0
"12.5"     → 12.5      // 浮点数
"  -456"   → -456      // 跳过前导空格，支持负数
```

**实现**（src/common/value.cpp）：
```cpp
int64_t Value::parse_int(const std::string& str) {
    // 跳过前导空格
    size_t i = 0;
    while (i < str.length() && std::isspace(str[i])) i++;

    // 处理符号
    bool negative = false;
    if (i < str.length() && str[i] == '-') {
        negative = true;
        i++;
    }

    // 解析数字，遇到非数字停止
    int64_t result = 0;
    while (i < str.length() && std::isdigit(str[i])) {
        result = result * 10 + (str[i] - '0');
        i++;
    }

    return negative ? -result : result;
}
```

### 类型提升规则

算术运算的类型提升遵循以下优先级：
```
STRING → INT → DECIMAL → BOOL
        (低)         (高)
```

示例：
```cpp
INT    + INT     → INT
INT    + DECIMAL → DECIMAL
STRING + INT     → INT (STRING 先转换为 INT)
BOOL   + INT     → INT (BOOL 转换为 0/1)
```

## 代码统计

### 新增代码
- `src/common/value.h`：130 行
- `src/common/value.cpp`：280 行
- `src/exec/operators/typed_expression_evaluator.h`：42 行
- `src/exec/operators/typed_expression_evaluator.cpp`：335 行
- **总计**：~790 行

### 移除代码
- `src/exec/operators/filter_operator.cpp`：移除旧 ExpressionEvaluator（~300 行）
- 字符串转换逻辑：~100 行
- **总计**：~400 行

### 净增加
- **~390 行**（提供了更好的性能和类型安全）

## 文件清单

### 核心文件

**Value 类型系统**：
- `include/common/value.h` - Value 类定义
- `src/common/value.cpp` - Value 类实现

**类型化求值器**：
- `src/exec/operators/typed_expression_evaluator.h` - TypedExpressionEvaluator 定义
- `src/exec/operators/typed_expression_evaluator.cpp` - TypedExpressionEvaluator 实现

**编译器类型推导**：
- `include/sql/compiler/compiler.h` - 添加类型推导方法声明
- `src/sql/compiler/compiler.cpp` - 类型推导实现

**表达式类型增强**：
- `include/sql/compiler/statement.h` - Expression 添加 result_type_

### 集成文件

**算子集成**：
- `include/exec/operators/filter_operator.h` - 更新为使用 TypedExpressionEvaluator
- `src/exec/operators/filter_operator.cpp` - FilterOperator 实现更新
- `src/exec/operators/nested_loop_join_operator.cpp` - JOIN 条件求值更新

**执行器集成**：
- `src/exec/executor/executor.cpp` - DELETE WHERE 条件求值更新
- `src/exec/executor/new_executor.cpp` - DELETE WHERE 条件求值更新

## 测试验证

### 单元测试
- ✅ Compiler 语义分析测试（test_compiler_semantic）
- ✅ Expression clone 测试（test_expression_clone）

### 集成测试
- ✅ SELECT with WHERE 条件
- ✅ JOIN with WHERE 条件
- ✅ 多表 JOIN 查询
- ✅ INSERT 操作

### 端到端测试
所有测试通过，验证：
- 类型化求值正确性
- 算子集成正确性
- 端到端查询功能

## 未来改进

### 可能的优化
1. **SIMD 向量化**：批量运算可使用 SIMD 指令加速
2. **JIT 编译**：将表达式编译为机器码（LLVM）
3. **表达式缓存**：缓存常用表达式的求值结果
4. **谓词下推**：更早地过滤数据

### 扩展功能
1. **更多函数**：添加更多 SQL 标准函数（UPPER, LOWER, CONCAT 等）
2. **类型检查**：编译时更严格的类型检查
3. **NULL 处理**：完善 NULL 值的三值逻辑
4. **CAST 支持**：显式类型转换

## 维护说明

### 添加新的表达式类型
1. 在 `Expression` 中添加新的派生类
2. 在 `TypedExpressionEvaluator::evaluate_row()` 中添加分发逻辑
3. 实现对应的 `evaluate_xxx()` 方法
4. 在 `Compiler` 中添加类型推导逻辑

### 添加新的数据类型
1. 在 `DataType` 枚举中添加新类型
2. 在 `Value` 中添加对应的存储和访问方法
3. 更新类型转换方法（`as_xxx()`）
4. 更新 `Compiler` 的类型推导规则

## 参考资料

- SQL-92 标准：类型转换规则
- DuckDB：类型化表达式系统设计
- PostgreSQL：表达式求值优化

## 版本历史

- **v1.0** (2025-10-26): 初始实现，完成核心功能和集成

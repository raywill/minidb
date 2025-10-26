# MiniDB - 单节点分析型SQL数据库

MiniDB是一个从零开始实现的单节点、列存储、向量化执行的关系型数据库系统。它支持基本的SQL操作，并提供TCP协议的命令行客户端。

## 🚀 特性

- **列式存储**: 采用列式存储格式，优化分析型查询性能
- **向量化执行**: 使用向量化执行引擎，批量处理数据
- **算子支持**: 实现SCAN、FILTER、PROJECTION、JOIN、FINAL_RESULT等算子
- **SQL支持**: 支持CREATE TABLE、DROP TABLE、INSERT、SELECT、DELETE、JOIN语句
- **JOIN操作**: 支持INNER JOIN，包括多表连接和表达式连接条件
- **数据类型**: 支持INT、STRING、BOOL、DECIMAL数据类型
- **内置函数**: 支持SIN、COS、SUBSTR等数学和字符串函数
- **TCP协议**: 基于TCP的客户端-服务器架构
- **命令历史**: 支持↑/↓箭头键的命令历史记录
- **内存管理**: 自定义内存分配器和Arena内存管理
- **日志系统**: 多级别、线程安全的日志系统
- **崩溃处理**: 段错误捕获、堆栈转储、错误恢复
- **持久化**: 数据持久化到磁盘，支持服务器重启
- **回归测试**: minitest工具，类似MySQL的mysqltest，支持自动化测试

## 📁 项目结构

```
minidb/
├── src/
│   ├── common/          # 通用类型和状态码
│   ├── mem/             # 内存管理 (Allocator, Arena)
│   ├── log/             # 日志系统
│   ├── sql/             # SQL解析和AST
│   │   ├── parser/      # 词法和语法分析器
│   │   └── ast/         # 抽象语法树
│   ├── storage/         # 存储引擎
│   │   ├── catalog/     # 数据库目录管理
│   │   └── table/       # 表和列文件管理
│   ├── exec/            # 执行引擎
│   │   ├── operators/   # 算子实现
│   │   └── executor/    # 查询执行器
│   ├── net/             # 网络通信
│   ├── server/          # 数据库服务器
│   └── client/          # 命令行客户端
├── include/             # 头文件
├── tests/               # 单元测试
│   └── minitest/        # 回归测试用例
│       ├── src/         # SQL测试文件
│       └── ref/         # 参考输出文件
├── tools/               # 工具程序
│   └── minitest/        # 回归测试工具
└── data/                # 数据目录
```

## 🛠️ 构建说明

### 系统要求

- C++11兼容的编译器 (GCC 4.8+ 或 Clang 3.3+)
- Make
- POSIX兼容系统 (Linux, macOS)

### 编译步骤

```bash
# 克隆项目
git clone <repository-url>
cd minidb

# 编译所有组件
make all

# 或者分别编译
make dbserver    # 编译服务器
make dbcli       # 编译客户端
make minitest    # 编译回归测试工具
make test        # 编译并运行单元测试
```

### 编译产物

- `bin/dbserver`: 数据库服务器程序
- `bin/dbcli`: 命令行客户端程序
- `bin/minitest`: 回归测试工具
- `tests/bin/test_*`: 各类单元测试程序

## 🚀 使用说明

### 启动服务器

```bash
# 使用默认配置启动服务器
./bin/dbserver

# 指定数据目录和端口
./bin/dbserver --data-dir ./mydata --port 8080

# 查看帮助
./bin/dbserver --help
```

服务器默认配置：
- 端口: 9876
- 数据目录: ./data

### 连接客户端

```bash
# 连接到本地服务器
./bin/dbcli

# 连接到指定服务器
./bin/dbcli --host 192.168.1.100 --port 8080

# 查看帮助
./bin/dbcli --help
```

### 客户端功能

- **交互式命令行**: 支持实时SQL输入和结果显示
- **命令历史记录**: 使用↑/↓箭头键浏览历史命令
- **历史记录持久化**: 命令历史保存在`~/.minidb_history`
- **智能补全**: 自动过滤重复和空白命令
- **优雅退出**: Ctrl+C或Ctrl+D安全退出
- **帮助系统**: `help`命令显示使用说明
- **历史查看**: `history`命令显示最近的命令

### SQL示例

```sql
-- 创建表
CREATE TABLE student(id INT, name STRING, age INT);

-- 插入数据
INSERT INTO student VALUES (1, 'Alice', 20), (2, 'Bob', 21), (3, 'Charlie', 19);

-- 查询数据
SELECT * FROM student;
SELECT id, name FROM student WHERE age > 19;

-- 使用函数
SELECT name, sin(age * 3.14 / 180) FROM student;
SELECT substr(name, 0, 3) FROM student WHERE age >= 20;

-- JOIN 操作
CREATE TABLE departments(id INT, name STRING);
INSERT INTO departments VALUES (1, 'CS'), (2, 'Math');

CREATE TABLE employees(id INT, name STRING, dept_id INT);
INSERT INTO employees VALUES (1, 'Alice', 1), (2, 'Bob', 1), (3, 'Charlie', 2);

-- 简单 JOIN
SELECT * FROM employees JOIN departments ON (employees.dept_id = departments.id);

-- 带表达式的 JOIN
SELECT * FROM employees e JOIN departments d ON (e.dept_id = d.id);

-- 删除数据
DELETE FROM student WHERE age < 20;

-- 删除表
DROP TABLE student;
DROP TABLE IF EXISTS employees;
```

## 📊 支持的SQL语法

### DDL (数据定义语言)

```sql
-- 创建表
CREATE TABLE table_name(column1 TYPE, column2 TYPE, ...);
CREATE TABLE IF NOT EXISTS table_name(column1 TYPE, column2 TYPE, ...);

-- 删除表
DROP TABLE table_name;
DROP TABLE IF EXISTS table_name;
```

### DML (数据操作语言)

```sql
-- 插入数据
INSERT INTO table_name VALUES (value1, value2, ...);
INSERT INTO table_name VALUES (value1, value2, ...), (value3, value4, ...);

-- 查询数据
SELECT column1, column2 FROM table_name;
SELECT * FROM table_name WHERE condition;

-- JOIN 查询
SELECT * FROM table1 JOIN table2 ON (join_condition);
SELECT * FROM t1 JOIN t2 ON (t1.id = t2.id);
SELECT * FROM t1 JOIN t2 ON (t1.id = t2.id + 1);

-- 多表 JOIN
SELECT * FROM t1 JOIN t2 ON (t1.id = t2.id) JOIN t3 ON (t2.id = t3.id);

-- 删除数据
DELETE FROM table_name WHERE condition;
```

### 数据类型

- `INT`: 32位整数
- `STRING`: 变长字符串
- `BOOL`: 布尔值 (true/false)
- `DECIMAL`: 双精度浮点数

### 支持的函数

- `SIN(x)`: 正弦函数
- `COS(x)`: 余弦函数  
- `SUBSTR(str, start, length)`: 字符串截取

### 支持的操作符

- 算术操作符: `+`, `-`, `*`, `/`
- 比较操作符: `=`, `!=`, `<`, `<=`, `>`, `>=`
- 逻辑操作符: `AND`, `OR`

## 🏗️ 架构设计

### 执行流程

1. **SQL解析**: 词法分析 → 语法分析 → AST生成
2. **查询编译**: AST → 逻辑计划 → 物理计划
3. **查询执行**: 算子流水线 → 向量化处理 → 结果输出

### 存储格式

- **目录结构**: 每个表对应一个目录
- **列文件**: 每列存储为独立的二进制文件
- **文件格式**: 文件头 + 数据块
- **持久化**: 写操作后立即同步到磁盘

### 内存管理

- **Allocator**: 统一的内存分配接口
- **Arena**: 短期内存生命周期管理
- **无内存泄漏**: RAII原则，自动资源管理

### 崩溃处理

- **信号捕获**: 自动捕获SIGSEGV、SIGBUS、SIGFPE等崩溃信号
- **堆栈转储**: 生成详细的调用堆栈信息
- **转储文件**: 格式为`crash-进程ID-线程ID-查询ID.dmp`
- **错误恢复**: 向客户端返回错误信息而非直接崩溃
- **调试信息**: 包含崩溃时间、信号类型、内存地址等

## 🧪 测试

### 单元测试

```bash
# 运行所有单元测试
make test

# 运行特定测试
./tests/bin/test_parser_dml          # DML解析器测试
./tests/bin/test_compiler_dml        # DML编译器测试
./tests/bin/test_expression_eval     # 表达式求值测试
./tests/bin/test_e2e_basic          # 端到端基础测试
```

### 回归测试 (minitest)

minitest 是类似 MySQL 的 mysqltest 的回归测试工具，用于自动化SQL测试。

#### minitest 使用方法

```bash
# 启动数据库服务器
./bin/dbserver --port 9876

# 创建参考输出文件（首次运行）
./bin/minitest --run-mode=create tests/minitest/src/basic/select.sql

# 运行测试并与参考输出对比
./bin/minitest tests/minitest/src/basic/select.sql

# 使用 verbose 模式查看详细调试信息
./bin/minitest --verbose tests/minitest/src/basic/select.sql

# 指定服务器地址和端口
./bin/minitest --host 127.0.0.1 --port 9876 tests/minitest/src/basic/select.sql
```

#### minitest 命令行选项

- `--run-mode=<mode>`: 测试运行模式（create 或 compare，默认：compare）
  - `create`: 执行SQL并创建参考输出文件（.ref）
  - `compare`: 执行SQL并与参考输出对比
- `--host=<host>`: 数据库服务器地址（默认：127.0.0.1）
- `--port=<port>`: 数据库服务器端口（默认：9876）
- `--verbose` 或 `-v`: 启用详细调试输出
- `--help` 或 `-h`: 显示帮助信息

#### 测试用例结构

```
tests/minitest/
├── src/              # SQL测试文件
│   ├── basic/        # 基础功能测试
│   ├── join/         # JOIN操作测试
│   └── ...
└── ref/              # 参考输出文件
    ├── basic/
    ├── join/
    └── ...
```

#### 编写测试用例

测试用例是包含SQL语句的文本文件（.sql），每条语句以分号结尾：

```sql
-- 创建表
CREATE TABLE test(id INT, name STRING);

-- 插入数据
INSERT INTO test VALUES (1, 'Alice'), (2, 'Bob');

-- 查询数据
SELECT * FROM test;

-- 清理
DROP TABLE test;
```

#### 测试输出

- **成功**: `✓ PASS: tests/minitest/src/basic/select.sql`
- **失败**: `✗ FAIL: tests/minitest/src/basic/select.sql`
  - 临时输出保存在 `.tmp` 文件
  - 使用 `diff ref_file tmp_file` 查看差异

## 📈 性能特性

- **批处理**: 默认批大小1024行，减少函数调用开销
- **向量化**: SIMD友好的数据布局和处理
- **列式存储**: 减少I/O，提高缓存命中率
- **内存池**: 减少内存分配/释放开销

## 🔧 配置选项

### 服务器配置

- `--data-dir`: 数据存储目录
- `--port`: TCP监听端口

### 客户端配置

- `--host`: 服务器主机名或IP
- `--port`: 服务器端口

## 🚨 限制和注意事项

1. **单线程执行**: 当前版本不支持并发查询
2. **无事务**: 不支持ACID事务，使用表级锁
3. **内存限制**: 查询结果需要完全加载到内存
4. **无索引**: 所有查询都是全表扫描
5. **简单优化**: 仅支持基本的谓词下推和列裁剪

## 🛣️ 未来计划

- [x] JOIN操作 (已完成)
- [x] 回归测试工具 (已完成)
- [ ] 查询优化器增强
- [ ] 索引支持
- [ ] 并发控制
- [ ] 事务支持
- [ ] 聚合函数 (SUM, COUNT, AVG, MAX, MIN)
- [ ] 排序和分组 (ORDER BY, GROUP BY)
- [ ] LEFT/RIGHT JOIN 支持
- [ ] 子查询支持
- [ ] 插件系统

## 📝 许可证

本项目采用MIT许可证。详见LICENSE文件。

## 🤝 贡献

欢迎提交Issue和Pull Request！

## 📞 联系方式

如有问题或建议，请通过以下方式联系：
- 提交GitHub Issue
- 发送邮件至项目维护者

---

**MiniDB** - 学习数据库系统的最佳实践项目！

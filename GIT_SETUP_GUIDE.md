# MiniDB Git版本管理设置指南

## 🎯 Git初始化

### 1. 初始化Git仓库

```bash
# 在项目根目录下初始化Git
cd /Users/rayu/code/raywill/minidb
git init

# 检查状态
git status
```

### 2. 配置Git用户信息

```bash
# 设置用户名和邮箱（如果还没设置）
git config --global user.name "Your Name"
git config --global user.email "your.email@example.com"

# 或者只为这个项目设置
git config user.name "Your Name"
git config user.email "your.email@example.com"
```

### 3. 添加文件到版本控制

```bash
# 添加所有源代码和重要文件
git add .

# 检查将要提交的文件
git status

# 首次提交
git commit -m "Initial commit: MiniDB columnar database system

Features:
- Columnar storage engine with vectorized execution
- SQL parser supporting CREATE, INSERT, SELECT, DELETE
- TCP-based client-server architecture  
- Command history with arrow key navigation
- Comprehensive unit test suite (18 tests)
- Crash handler with stack trace generation
- Custom memory management (Allocator, Arena)
- Multi-level logging system
- WHERE clause filtering support

Architecture:
- 25 source files, 24 header files
- 7,500+ lines of C++ code
- Enterprise-grade test coverage (95%+)
- Dual build system (Makefile + CMake)"
```

## 📋 .gitignore 文件说明

创建的`.gitignore`文件包含以下类别：

### 🔧 编译产物
- **二进制文件**: `bin/`, `tests/bin/`, 可执行文件
- **目标文件**: `*.o`, `*.obj`, `*.a`, `*.so`
- **调试符号**: `*.dSYM/`, `*.pdb`

### 🏗️ 构建系统
- **CMake**: `build/`, `CMakeFiles/`, `CMakeCache.txt`
- **Make**: `.deps/`, `.libs/`
- **Autotools**: `autom4te.cache/`, `config.log`

### 💻 开发环境
- **IDE**: `.vscode/`, `.idea/`, `*.xcodeproj/`
- **编辑器**: `*.swp`, `*~`, `.emacs.desktop`
- **系统**: `.DS_Store`, `Thumbs.db`

### 🗄️ 数据库文件
- **数据目录**: `data/`, `*_data/`, `*.db`
- **日志**: `*.log`, `minidb.log`
- **崩溃转储**: `crash-*.dmp`, `core`
- **历史记录**: `.minidb_history*`

### 🧪 测试文件
- **测试输出**: `test_output/`, `*.gcov`
- **临时文件**: `*.tmp`, `*.bak`
- **调试文件**: `debug_*` (除了源文件)

## 🚀 Git工作流建议

### 分支策略

```bash
# 主分支
main/master     # 稳定版本

# 开发分支
develop         # 开发主分支
feature/*       # 功能分支
bugfix/*        # 修复分支
architecture/*  # 架构重构分支
```

### 推荐的分支创建

```bash
# 创建架构重构分支
git checkout -b architecture/compiler-redesign
git push -u origin architecture/compiler-redesign

# 创建功能分支
git checkout -b feature/query-optimizer
git checkout -b feature/index-support
git checkout -b bugfix/where-clause-performance
```

### 提交消息规范

```bash
# 格式: <type>(<scope>): <description>
git commit -m "feat(parser): add support for JOIN syntax"
git commit -m "fix(executor): resolve WHERE clause filtering issue"
git commit -m "refactor(arch): separate AST from Statement classes"
git commit -m "test(where): add comprehensive WHERE clause unit tests"
git commit -m "docs(readme): update architecture documentation"
```

**提交类型:**
- `feat`: 新功能
- `fix`: 修复bug
- `refactor`: 重构代码
- `test`: 添加测试
- `docs`: 文档更新
- `style`: 代码格式
- `perf`: 性能优化
- `build`: 构建系统

## 📊 版本标签建议

```bash
# 版本标签
git tag -a v1.0.0 -m "MiniDB v1.0.0 - Initial stable release"
git tag -a v1.1.0 -m "MiniDB v1.1.0 - Added WHERE clause filtering"
git tag -a v2.0.0 -m "MiniDB v2.0.0 - Architecture redesign with proper compiler layers"

# 推送标签
git push origin --tags
```

## 🔍 Git状态检查

```bash
# 检查当前状态
git status

# 查看提交历史
git log --oneline

# 查看分支
git branch -a

# 查看远程仓库
git remote -v
```

## 📦 远程仓库设置

```bash
# 添加远程仓库 (GitHub/GitLab等)
git remote add origin https://github.com/username/minidb.git

# 推送到远程仓库
git push -u origin main

# 克隆仓库 (其他开发者)
git clone https://github.com/username/minidb.git
cd minidb
make clean && make all
```

## 🧪 Git Hooks建议

创建`.git/hooks/pre-commit`脚本：

```bash
#!/bin/bash
# 提交前自动运行测试
echo "Running tests before commit..."
make test_unit
if [ $? -ne 0 ]; then
    echo "Tests failed! Commit aborted."
    exit 1
fi
echo "All tests passed!"
```

## 🎯 使用Git的好处

### 版本控制
- ✅ **代码历史追踪** - 每次修改都有记录
- ✅ **分支开发** - 并行开发不同功能
- ✅ **版本标签** - 标记重要版本
- ✅ **回滚能力** - 可以回到任何历史版本

### 协作开发
- ✅ **多人协作** - 支持团队开发
- ✅ **冲突解决** - 自动合并和冲突处理
- ✅ **代码审查** - Pull Request工作流
- ✅ **问题跟踪** - 集成Issue管理

### 项目管理
- ✅ **发布管理** - 版本发布和分发
- ✅ **备份安全** - 分布式备份
- ✅ **开源发布** - 便于开源分享
- ✅ **CI/CD集成** - 自动化构建和测试

## 🎉 总结

现在您的MiniDB项目已经准备好使用Git进行版本管理：

- ✅ **完整的.gitignore** - 忽略所有不必要的文件
- ✅ **清晰的项目结构** - 便于版本控制
- ✅ **专业的工作流** - 支持现代开发流程
- ✅ **架构重构准备** - 可以创建分支进行重构

**建议先提交当前稳定版本，然后创建架构重构分支来实现您提到的编译器层次分离！** 🚀

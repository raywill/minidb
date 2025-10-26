# MiniDB Regression Test Suite

This is a regression test suite for MiniDB, similar to MySQL's mysqltest.

## Directory Structure

```
tests/minitest/
├── src/          # SQL test files organized by feature
│   ├── basic/    # Basic functionality tests
│   └── join/     # JOIN tests
├── ref/          # Reference output files
│   ├── basic/
│   └── join/
└── run_all.sh    # Script to run all tests
```

## Usage

### Creating Reference Files

When you create a new test or update existing functionality:

```bash
# Start the database server
./bin/dbserver --port 5432 --data-dir ./data_minitest_ref

# Create reference file for a single test
./bin/minitest --run-mode=create --port 5432 tests/minitest/src/basic/select.sql

# Or create all reference files
cd tests/minitest
./run_all.sh create localhost 5432
```

### Running Tests

```bash
# Start the database server
./bin/dbserver --port 5432 --data-dir ./data_minitest_test

# Run a single test
./bin/minitest --run-mode=compare --port 5432 tests/minitest/src/basic/select.sql

# Or run all tests
cd tests/minitest
./run_all.sh compare localhost 5432
```

## Test File Format

Test files are simple SQL files with queries separated by semicolons:

```sql
-- Test: Basic SELECT
CREATE TABLE users (id INT, name STRING, age INT);
INSERT INTO users VALUES (1, 'Alice', 25), (2, 'Bob', 30);
SELECT * FROM users;
```

Comments starting with `#` or `--` are ignored.

## Adding New Tests

1. Create a new `.sql` file in the appropriate subdirectory under `src/`
2. Start a clean database server
3. Generate the reference file using `--run-mode=create`
4. Run the test using `--run-mode=compare` to verify it passes

## Examples

```bash
# Create and run a new test
echo "SELECT 1 + 1;" > tests/minitest/src/basic/arithmetic.sql
./bin/minitest --run-mode=create --port 5432 tests/minitest/src/basic/arithmetic.sql
./bin/minitest --run-mode=compare --port 5432 tests/minitest/src/basic/arithmetic.sql
```

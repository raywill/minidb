-- Clean up existing tables
DROP TABLE IF EXISTS employees;
DROP TABLE IF EXISTS departments;
DROP TABLE IF EXISTS projects;
DROP TABLE IF EXISTS assignments;
DROP TABLE IF EXISTS salaries;

-- Create tables with multiple columns
CREATE TABLE employees (id INT, name STRING, dept_id INT);
CREATE TABLE departments (id INT, name STRING, budget INT);
CREATE TABLE projects (id INT, name STRING, dept_id INT);
CREATE TABLE assignments (emp_id INT, proj_id INT, hours INT);
CREATE TABLE salaries (emp_id INT, amount INT, level INT);

-- Insert more data into employees (10 rows)
INSERT INTO employees VALUES (1, 'Alice', 1), (2, 'Bob', 1), (3, 'Charlie', 2);
INSERT INTO employees VALUES (4, 'David', 2), (5, 'Eve', 3), (6, 'Frank', 3);
INSERT INTO employees VALUES (7, 'Grace', 1), (8, 'Henry', 2), (9, 'Ivy', 3);
INSERT INTO employees VALUES (10, 'Jack', 1);

-- Insert data into departments
INSERT INTO departments VALUES (1, 'Engineering', 100000), (2, 'Sales', 80000);
INSERT INTO departments VALUES (3, 'Marketing', 60000), (4, 'HR', 50000);

-- Insert data into projects
INSERT INTO projects VALUES (1, 'ProjectA', 1), (2, 'ProjectB', 1);
INSERT INTO projects VALUES (3, 'ProjectC', 2), (4, 'ProjectD', 3);
INSERT INTO projects VALUES (5, 'ProjectE', 2);

-- Insert data into assignments (employee-project relationships)
INSERT INTO assignments VALUES (1, 1, 40), (1, 2, 20), (2, 1, 30);
INSERT INTO assignments VALUES (3, 3, 50), (4, 3, 40), (5, 4, 35);
INSERT INTO assignments VALUES (6, 4, 45), (7, 1, 25), (8, 5, 30);
INSERT INTO assignments VALUES (9, 4, 40), (10, 2, 35);

-- Insert data into salaries
INSERT INTO salaries VALUES (1, 90000, 3), (2, 85000, 2), (3, 95000, 3);
INSERT INTO salaries VALUES (4, 88000, 2), (5, 92000, 3), (6, 87000, 2);
INSERT INTO salaries VALUES (7, 82000, 2), (8, 91000, 3), (9, 86000, 2);
INSERT INTO salaries VALUES (10, 84000, 2);

-- Three-table join: employees working on projects in their departments
SELECT * FROM employees JOIN departments ON (employees.dept_id = departments.id) JOIN projects ON (departments.id = projects.dept_id);

-- Four-table join: employees, their assignments, projects, and departments
SELECT * FROM employees JOIN assignments ON (employees.id = assignments.emp_id) JOIN projects ON (assignments.proj_id = projects.id) JOIN departments ON (projects.dept_id = departments.id);

-- Five-table join: employees, departments, projects, assignments, and salaries with complex conditions
SELECT * FROM employees JOIN departments ON (employees.dept_id = departments.id) JOIN projects ON (departments.id = projects.dept_id) JOIN assignments ON (employees.id = assignments.emp_id) JOIN salaries ON (employees.id = salaries.emp_id);

-- Test: Select data
CREATE TABLE users (id INT, name STRING, age INT);
INSERT INTO users VALUES (1, 'Alice', 25), (2, 'Bob', 30);
SELECT * FROM users;
SELECT name FROM users;

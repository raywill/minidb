-- Test: JOIN with arithmetic expression
CREATE TABLE t1 (c1 INT);
CREATE TABLE t2 (c1 INT);
INSERT INTO t1 VALUES (1), (2), (3);
INSERT INTO t2 VALUES (1), (2), (3), (4);
SELECT * FROM t1 JOIN t2 ON (t1.c1 = t2.c1 + 2);

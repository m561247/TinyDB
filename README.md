# TinyDB — 一个用于教学的数据库

## 编译及运行

```
make image
make build-dev
make run
```

## 系统功能

### 数据类型

数据库支持的基本类型有：

 * 整型（INT）
 * 浮点型（FLOAT）
 * 字符串型（VARCHAR）
 * 日期型（DATE），日期格式`YYYY-MM-dd`

日期类型的字面值和字符串相同，在实现中如果必要可以转换为字符串。

### SQL语句

我们支持的SQL语句一共有如下几种，您可以使用 testsql 目录下的 sql 测试

 * 插入语句：`INSERT INTO ... VALUES ...`
 * 删除语句：`DELETE FROM ... WHERE ...`
 * 查询语句：`SELECT ... FROM ... WHERE ...`
 * 更新语句：`UPDATE ... SET ... WHERE ...`
 * 创建数据库：`CREATE DATABASE ...`
 * 删除数据库：`DROP DATABASE ...`
 * 切换数据库：`USE ...`
 * 创建表：`CREATE TABLE ...`
 * 删除表：`DROP TABLE ...`
 * 创建索引：`CREATE INDEX ...`
 * 删除索引：`DROP INDEX ...`

### 复杂表达式处理

表达式大致可以分为两种：算术表达式和条件表达式。由于采用Bison进行解析，可以支持任意深度嵌套的复杂表达式。我们所支持的基本运算主要如下

 * 四则运算，针对整数和浮点数进行。
 * 比较运算符，即<=, <, =, >, >=, <>。
 * 模糊匹配运算符，即LIKE，其实现采用C++11的正则表达式库。
 * 范围匹配运算符，即IN，可以在表的CHECK约束中以及WHERE子句中使用。
 * 空值判定运算符，即IS NULL和IS NOT NULL两种。
 * 逻辑运算，包含NOT、AND和OR三种。

以下是一些复杂表达式运算的例子

```sql
UPDATE customer SET age = age + 1 WHERE age < 18 AND gender = 'F';
SELECT * FROM customer WHERE name LIKE 'John %son';
SELECT * FROM students WHERE grades IN ('A', 'B', 'C');
SELECT * FROM students WHERE name IS NOT NULL;
```

### 聚集查询
我们实现了五种聚集查询函数COUNT、SUM、AVG、MIN和MAX。其中COUNT不支持DISTINCT关键字。例如

```sql
SELECT COUNT(*) FROM customer WHERE age > 18;
SELECT AVG(age) FROM customer WHERE age <= 18;
```
### 属性完整性约束
我们支持多种属性完整性约束，分别是

 * 主键约束。一个表可以有多个列联合起来作为主键，只有在所有主键都相同时才认为两条记录有冲突，即这种情况下主键是一个元组。
 * 外键约束，每个域都可以有外键约束，引用另外一个表的主键。
 * UNIQUE约束，该约束限制某一列的值不能重复。
 * NOT NULL约束，该约束限制某一列不能有空值。
 * DEFAULT约束，该约束可以在INSERT语句不指定值是给某列赋予一个默认值。
 * CHECK约束，该约束可以对表中元素的值添加条件表达式的检查。

下面是一个简单的例子，注意如果在多个列都指定了PRIMARY KEY，那么就认为主键是一个元组，而不是有多个主键。例如Infos表的主键为(PersonID, InfoID)。
```sql
CREATE TABLE Persons (
    PersonID int PRIMARY KEY NOT NULL,
    Name varchar(20),
    Age int DEFAULT 1,
    Gender varchar(1),
    CHECK (Age >= 1 AND Age <= 100),
    CHECK (Gender IN ('F', 'M'))
);

CREATE TABLE Infos (
    PersonID int PRIMARY KEY,
    InfoID int PRIMARY KEY,
    FOREIGN KEY (PersonID) REFERENCES Persons(PersonID)
);

```
### 多表连接查询
在SELECT语句中，我们支持任意多表的连接操作，例如
```sql
SELECT * FROM A, B, C WHERE A.ID = B.ID AND C.Name = A.Name
```
并且，对于多个表的连接中形如A.Col1 = B.Col2的条件，那么如果这两个列的某一个拥有索引，会利用索引进行查询优化。例如如下查询就可以优化

```sql
SELECT * FROM Persons, Infos WHERE Persons.PersonID = Infos.PersonID;
SELECT * FROM Persons, Infos, Datas WHERE Persons.PersonID = Infos.PersonID AND Datas.N IS NOT NULL;
SELECT * FROM Persons, Infos, Datas WHERE Persons.PersonID = Infos.PersonID AND Datas.ID = Infos.PersonID;
```
具体的优化方法以及何种查询可以优化见文档中"查询优化"部分。
### 表别名
我们在多表连接查询时支持通过别名（alias）的方式对一个表进行连接，例如
```sql
SELECT * FROM Persons AS P1, Persons AS P2 WHERE P1.PersonID = P2.PersonID;
```

## Demo

![1](https://learnbycoding.oss-cn-beijing.aliyuncs.com/001.png)

![2](https://learnbycoding.oss-cn-beijing.aliyuncs.com/002.png)

![3](https://learnbycoding.oss-cn-beijing.aliyuncs.com/003.png)

![4](https://learnbycoding.oss-cn-beijing.aliyuncs.com/004.png)

```
CREATE DATABASE db;

USE db;

CREATE TABLE Persons (
   PersonID int PRIMARY KEY, 
   LastName varchar(20), 
   FirstName varchar(20), 
   Address varchar(20), 
   City varchar(10)
);

CREATE INDEX Persons(PersonID);
CREATE INDEX Persons(FirstName);
INSERT INTO Persons VALUES 
(23, 'Yi', '测试', 'Tsinghua Univ.', 'Beijing'), 
(-238, 'Zhong', 'Lei', 'Beijing Univ.', 'Neijing'),
(1+999, 'Wasserstein', 'Zhang', 'Hunan Univ.', 'Hunan');

INSERT INTO Persons VALUES (1, 'Yi', '测试', 'Tsinghua Univ.', 'Beijing'), 
(3, 'Zhong', 'Lei', 'Beijing Univ.', 'Neijing'),
(4, 'Wasserstein', 'Zhang', 'Hunan Univ.', 'Hunan');

SELECT PersonID, LastName, FirstName, Address, City FROM Persons;

SELECT PersonID, LastName, FirstName FROM Persons WHERE FirstName = '测试'; 

UPDATE Persons SET FirstName = 'CxSpace' WHERE PersonID = 3;

SELECT * FROM Persons;

INSERT INTO Persons VALUES 
(100001, 'Zarisk', 'C', 'Unknown', 'US'), 
(100002, 'Wasserstein', 'D', 'Unknwon.', 'EU');

DELETE FROM Persons WHERE PersonID < 0;
SELECT * FROM Persons;
DELETE FROM Persons;
SELECT * FROM Persons;
INSERT INTO Persons 
(LastName, PersonID) 
VALUES ('Zarisk', 10), 
       ('1999-10-10', 30), 
       ('Wasserstein', 20);
       
SELECT * FROM Persons;
SELECT COUNT(*) FROM Persons;
SELECT SUM(PersonID) FROM Persons;
SELECT AVG(PersonID) FROM Persons;
SELECT PersonID, LastName FROM Persons;
```

## Acknowledgments

*  基于项目 TrivialDB 开发


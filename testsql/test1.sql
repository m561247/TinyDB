CREATE DATABASE db;
SET OUTPUT = 'test1.out';
USE db;
SHOW DATABASE db;
CREATE TABLE Persons (PersonID int PRIMARY KEY, LastName varchar(20), FirstName varchar(20), Address varchar(20), City varchar(10));

67, 82, 69, 65, 84, 69, 32, 84, 

65, 66, 76, 69, 32, 80, 101, 114, 

115, 111, 110, 115, 32, 40, 

10, 32, 32, 32, 32, 80, 101, 114, 115, 111, 110, 73, 68, 32, 105, 110, 116, 32, 80, 82, 73, 77, 65, 82, 89, 32, 75, 69, 89, 44, 32, 
10, 32, 32, 32, 32, 76, 97, 115, 116, 78, 97, 109, 101, 32, 118, 97, 114, 99, 104, 97, 114, 40, 50, 48, 41, 44, 32,
 10, 32, 32, 32, 32, 70, 105, 114, 115, 116, 78, 97, 109, 101, 32, 118, 97, 114, 99, 104, 97, 114, 40, 50, 48, 41, 44, 32,
  10, 32, 32, 32, 32, 65, 100, 100, 114, 101, 115, 115, 32, 118, 97, 114, 99, 104, 97, 114, 40, 50, 48, 41, 44, 32,
   10, 32, 32, 32, 32, 67, 105, 116, 121, 32, 118, 97, 114, 99, 104, 97, 114, 40, 49, 48, 41, 41



CREATE INDEX Persons(PersonID);
CREATE INDEX Persons(FirstName);
INSERT INTO Persons VALUES (23, 'Yi', '测试', 'Tsinghua Univ.', 'Beijing'), (-238, 'Zhong', 'Lei', 'Beijing Univ.', 'Neijing'),(1+999, 'Wasserstein', 'Zhang', 'Hunan Univ.', 'Hunan');
SELECT PersonID, LastName, FirstName FROM Persons;
UPDATE Persons SET LastName = 'CxSpace' WHERE PersonID = -238;
SELECT * FROM Persons;
INSERT INTO Persons VALUES (100001, 'Zarisk', 'C', 'Unknown', 'US'), (100002, 'Wasserstein', 'D', 'Unknwon.', 'EU');
DELETE FROM Persons WHERE PersonID < 0;
SELECT * FROM Persons;
DELETE FROM Persons;
SELECT * FROM Persons;
INSERT INTO Persons (LastName, PersonID) VALUES ('Zarisk', 10), ('1999-10-10', 30), ('Wasserstein', 20);
SELECT * FROM Persons;
SELECT COUNT(*) FROM Persons;
SELECT SUM(PersonID) FROM Persons;
SELECT AVG(PersonID) FROM Persons;
SELECT PersonID, LastName FROM Persons;


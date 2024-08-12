-- MySQL dump 10.13  Distrib 8.0.36, for Linux (x86_64)
--
-- Host: localhost    Database: 58th
-- ------------------------------------------------------
-- Server version	8.0.36-0ubuntu0.22.04.1

/*!40101 SET @OLD_CHARACTER_SET_CLIENT=@@CHARACTER_SET_CLIENT */;
/*!40101 SET @OLD_CHARACTER_SET_RESULTS=@@CHARACTER_SET_RESULTS */;
/*!40101 SET @OLD_COLLATION_CONNECTION=@@COLLATION_CONNECTION */;
/*!50503 SET NAMES utf8mb4 */;
/*!40103 SET @OLD_TIME_ZONE=@@TIME_ZONE */;
/*!40103 SET TIME_ZONE='+00:00' */;
/*!40014 SET @OLD_UNIQUE_CHECKS=@@UNIQUE_CHECKS, UNIQUE_CHECKS=0 */;
/*!40014 SET @OLD_FOREIGN_KEY_CHECKS=@@FOREIGN_KEY_CHECKS, FOREIGN_KEY_CHECKS=0 */;
/*!40101 SET @OLD_SQL_MODE=@@SQL_MODE, SQL_MODE='NO_AUTO_VALUE_ON_ZERO' */;
/*!40111 SET @OLD_SQL_NOTES=@@SQL_NOTES, SQL_NOTES=0 */;

--
-- Table structure for table `person`
--

DROP TABLE IF EXISTS `person`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!50503 SET character_set_client = utf8mb4 */;
CREATE TABLE `person` (
  `id` int NOT NULL,
  `name` varchar(20) DEFAULT NULL,
  `birth` date DEFAULT NULL,
  `balance` int DEFAULT NULL,
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_0900_ai_ci;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `person`
--

LOCK TABLES `person` WRITE;
/*!40000 ALTER TABLE `person` DISABLE KEYS */;
/*!40000 ALTER TABLE `person` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `student`
--

DROP TABLE IF EXISTS `student`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!50503 SET character_set_client = utf8mb4 */;
CREATE TABLE `student` (
  `id` int NOT NULL,
  `name` varchar(20) DEFAULT NULL,
  `birth` date DEFAULT NULL,
  `chinese` int DEFAULT '60',
  `math` int DEFAULT '60',
  `english` int DEFAULT '60',
  PRIMARY KEY (`id`),
  UNIQUE KEY `name` (`name`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_0900_ai_ci;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `student`
--

LOCK TABLES `student` WRITE;
/*!40000 ALTER TABLE `student` DISABLE KEYS */;
INSERT INTO `student` VALUES (1,'dijia','1998-08-09',85,70,60),(2,'taijia','1998-07-06',60,70,60),(3,'sailuo','2000-01-01',60,70,80),(4,'oubu','2002-02-02',50,60,60),(5,'leiou','2003-03-03',90,88,80),(6,'daina','2004-04-04',66,65,90),(7,'迪迦','2001-02-02',78,67,90),(9,'赛罗','1995-02-02',78,67,90),(10,'gaiya','2004-05-06',40,80,50),(11,'泰迦','2005-06-07',60,95,95),(12,NULL,'1997-08-09',60,30,20);
/*!40000 ALTER TABLE `student` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `student2`
--

DROP TABLE IF EXISTS `student2`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!50503 SET character_set_client = utf8mb4 */;
CREATE TABLE `student2` (
  `id` int DEFAULT NULL,
  `name` varchar(20) DEFAULT NULL,
  `birth` date DEFAULT NULL,
  `chinese` int DEFAULT NULL,
  `math` int DEFAULT NULL,
  `english` int DEFAULT NULL
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_0900_ai_ci;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `student2`
--

LOCK TABLES `student2` WRITE;
/*!40000 ALTER TABLE `student2` DISABLE KEYS */;
/*!40000 ALTER TABLE `student2` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `student3`
--

DROP TABLE IF EXISTS `student3`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!50503 SET character_set_client = utf8mb4 */;
CREATE TABLE `student3` (
  `id` int DEFAULT NULL,
  `name` varchar(20) DEFAULT NULL,
  `birth` date DEFAULT NULL,
  `chinese` int DEFAULT NULL,
  `math` int DEFAULT NULL,
  `english` int DEFAULT NULL
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_0900_ai_ci;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `student3`
--

LOCK TABLES `student3` WRITE;
/*!40000 ALTER TABLE `student3` DISABLE KEYS */;
/*!40000 ALTER TABLE `student3` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `torder`
--

DROP TABLE IF EXISTS `torder`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!50503 SET character_set_client = utf8mb4 */;
CREATE TABLE `torder` (
  `id` int DEFAULT NULL,
  `name` varchar(20) DEFAULT NULL,
  `s_id` int DEFAULT NULL,
  KEY `fk_1` (`s_id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_0900_ai_ci;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `torder`
--

LOCK TABLES `torder` WRITE;
/*!40000 ALTER TABLE `torder` DISABLE KEYS */;
INSERT INTO `torder` VALUES (1,'变身器',12),(2,'光剑',3),(3,'头镖',6),(4,'双截棍',5),(5,'手枪',13),(6,'AK47',14);
/*!40000 ALTER TABLE `torder` ENABLE KEYS */;
UNLOCK TABLES;
/*!40103 SET TIME_ZONE=@OLD_TIME_ZONE */;

/*!40101 SET SQL_MODE=@OLD_SQL_MODE */;
/*!40014 SET FOREIGN_KEY_CHECKS=@OLD_FOREIGN_KEY_CHECKS */;
/*!40014 SET UNIQUE_CHECKS=@OLD_UNIQUE_CHECKS */;
/*!40101 SET CHARACTER_SET_CLIENT=@OLD_CHARACTER_SET_CLIENT */;
/*!40101 SET CHARACTER_SET_RESULTS=@OLD_CHARACTER_SET_RESULTS */;
/*!40101 SET COLLATION_CONNECTION=@OLD_COLLATION_CONNECTION */;
/*!40111 SET SQL_NOTES=@OLD_SQL_NOTES */;

-- Dump completed on 2024-06-11 14:35:06

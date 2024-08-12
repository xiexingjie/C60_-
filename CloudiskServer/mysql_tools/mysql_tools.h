#ifndef __MYSQL_TOOLS_H_
#define __MYSQL_TOOLS_H_
#include <mysql/mysql.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define mysql_tools_ip "127.0.0.1"
#define mysql_tools_port 3306
#define mysql_tools_username "root"
#define mysql_tools_password "root"
#define mysql_tools_database "C60"


// 确保声明 strdup
#ifdef _WIN32
#define strdup _strdup
#endif

// 获取mysql连接
MYSQL * mysql_tools_connect(const char * ip, const int port, const char * username, const char * password, const char * db);
// 执行sql语句
int execute_sql(MYSQL * conn, char * sql);
// 执行查询语句
MYSQL_RES * execute_store_query(MYSQL * conn, char * sql);
// 获取查询结果字段数量
int get_mysql_result_fields(MYSQL_RES * result);
// 获取结果多少行
int get_mysql_result_row(MYSQL_RES * result);
// 获取mysql结果
int get_mysql_result(MYSQL_RES * result, int fields, int rows);

//================ DEMO ===========================
//      MYSQL_ROW row;
//      while ((row = mysql_fetch_row(result))) {
//          for (int i = 0; i < num_fields; i++) {
//              printf("%s ", row[i] ? row[i] : "NULL");
//          }
//          printf("\n");
//      }
//===========================================

// 断开连接， 释放资源
int close_mysql_tools(MYSQL * conn, MYSQL_RES * result);
#endif
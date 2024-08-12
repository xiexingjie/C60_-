#include "mysql_tools.h"

MYSQL * mysql_tools_connect(const char * ip, const int port, const char * username, const char * password, const char * db)
{
    // 注意，如果成功返回MYSQL指针，后续需要关闭连接
    MYSQL * conn = mysql_init(NULL);
    if (conn == NULL) {
        fprintf(stderr, "mysql_init() failed\n");
        return NULL;
    }
    
    if (mysql_real_connect(conn, ip, username, password, db, port, NULL, 0) == NULL) {
        fprintf(stderr, "mysql_real_connect() failed\n");
        mysql_close(conn);
        return NULL;
    }
    return conn;
}

int execute_sql(MYSQL * conn, char * sql)
{
    if (mysql_query(conn, sql)) {
        fprintf(stderr, "execute failed. Error: %s\n", mysql_error(conn));
        return -1;
    }
    printf("Execute SQL successful.\n");
    return 0;
}

MYSQL_RES * execute_store_query(MYSQL * conn, char * sql)
{
    // 注意，该result用完需要释放
    if (mysql_query(conn, sql)) {
        fprintf(stderr, "select failed. Error: %s\n", mysql_error(conn));
        return NULL;
    }

    MYSQL_RES *result = mysql_store_result(conn);
    if (result == NULL) {
        fprintf(stderr, "mysql_store_result() failed. Error: %s\n", mysql_error(conn));
        return NULL;
    }
    return result;
}

int get_mysql_result_fields(MYSQL_RES * result)
{
    // 获取mysql的字段数
    return mysql_num_fields(result);
}

int get_mysql_result_row(MYSQL_RES * result)
{
    // 获取mysq结果的行数
    return mysql_num_rows(result);
}

int get_mysql_result(MYSQL_RES * result, int fields, int rows) {
    MYSQL_ROW row;
    
    while ((row = mysql_fetch_row(result))) {

        for (int i = 0; i < fields; i++) {
            printf("%s\t", row[i] ? row[i] : "NULL");
        }
        printf("\n");
    }
    return 0;
}


int close_mysql_tools(MYSQL * conn, MYSQL_RES * result)
{
    // 释放查询资源
    if(result != NULL)
        mysql_free_result(result);

    // 断开mysql连接
    if(conn != NULL)
        mysql_close(conn);

    return 0;
}


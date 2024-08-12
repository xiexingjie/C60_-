#include "user.h"
#include "hashtable.h"
#include "thread_pool.h"
#include "time_tools.h"
#include "config.h"
#include "../mysql_tools/mysql_tools.h"
#include <stdio.h>
#include <string.h>
#include <shadow.h>

void loginCheck1_version2(user_info_t * user, const char * username)
{   
    printf("loginCheck1_version2.\n");
    train_t t;
    int ret;
    memset(&t, 0, sizeof(t));

    MYSQL * conn = mysql_tools_connect(mysql_tools_ip, mysql_tools_port, mysql_tools_username, mysql_tools_password, mysql_tools_database);

    char sql[1024] = {0};
    sprintf(sql, "select salt, password, id from user where name = '%s';", username);
    // printf("sql: %s\n", sql);
    // 执行查询语句
    mysql_real_query(conn, sql, strlen(sql));

    MYSQL_RES *res;
    MYSQL_ROW row;
    // 获取数据集
    res = mysql_store_result(conn);

    if (res == NULL || mysql_num_rows(res) == 0) {
        t.len = 0;
        t.type = TASK_LOGIN_SECTION1_RESP_ERROR;
        ret = sendn(user->sockfd, &t, 8);
        printf("loginCheck1_version2 failed.\n");
    } else {
        row = mysql_fetch_row(res); // 用户名唯一，不存在相同数据

        strcpy(user->name, username);
        // 发送的是salt，但是保存的是密文也就是加密密码
        strcpy(user->encrypted, row[1]);
        user->user_id = atoi(row[2]);

        t.len = strlen(row[0]);
        t.type = TASK_LOGIN_SECTION1_RESP_OK;
        strncpy(t.buff, row[0], t.len);
        // 发送setting
        ret = sendn(user->sockfd, &t, 8 + t.len);

        printf("loginCheck1_version2 successful.\n");
    }

    // 释放资源
    close_mysql_tools(conn, res);
    return;
}


void loginCheck2_version2(user_info_t * user, const char * password)
{
    printf("loginCheck2_version2.\n");
    int ret;
    train_t t;
    memset(&t, 0, sizeof(t));

    if(strcmp(user->encrypted, password) == 0) {
        //登录成功
        user->status = STATUS_LOGIN;//更新用户登录成功的状态
        t.type = TASK_LOGIN_SECTION2_RESP_OK;
        
        char * success = "The client successfully connects to the server.";
        t.len = strlen(success);
        strcpy(t.buff, success);
        ret = sendn(user->sockfd, &t, 8 + t.len);
        // 设置用户的初始化路径
        // 用户路径和id
        MYSQL * conn = mysql_tools_connect(mysql_tools_ip, mysql_tools_port, mysql_tools_username, mysql_tools_password, mysql_tools_database);
        char sql[1024] = {0};
        // 注册默认登录, token暂时不做处理
        sprintf(sql, "select id from user_file where filename='%s' and owner_id = %d and parent_id = 0;", user->name, user->user_id);
        // printf("sql: %s\n", sql);
        // 执行查询语句
        mysql_real_query(conn, sql, strlen(sql));

        MYSQL_RES *res;
        MYSQL_ROW row;
        // 获取数据集
        res = mysql_store_result(conn);

        row = mysql_fetch_row(res);

        user->user_file_id = atoi(row[0]);

        close_mysql_tools(conn, res);

        // printf("Login success.\n");
    } else {
        //登录失败, 密码错误
        t.type = TASK_LOGIN_SECTION2_RESP_ERROR;
        printf("Login failed.\n");
        ret = sendn(user->sockfd, &t, 8);
    }

    printf("client login success.\n", ret);
    return;
}

void rigister_user_info(user_info_t * user, const char * rigister_info)
{
    /* printf("loginCheck2.\n"); */
    int ret;
    train_t t;
    memset(&t, 0, sizeof(t));

    char* name = strtok(rigister_info, ";");
    char* password = strtok(NULL, ";");
    char* salt = strtok(NULL, ";");

    char * curtime = get_nowtime();
    // printf("curtime: %s\n", curtime);
    // 创建mysql连接
    MYSQL * conn = mysql_tools_connect(mysql_tools_ip, mysql_tools_port, mysql_tools_username, mysql_tools_password, mysql_tools_database);
    char sql[1024] = {0};
    // 注册默认登录, token暂时不做处理
    sprintf(sql, "insert into user(name, password, salt, pwd, time, status, token) values('%s', '%s', '%s', '%s', '%s', '%d', '%s');",
                                    name, password, salt, USER_FILEPATH, curtime, 1, "token");
    int result = execute_sql(conn, sql);
    // printf("sql: %s\n", sql);
    printf("insert user result: %d\n", result);

    if(result == -1)
    {
        t.type = TAST_REGISTER_RESP_ERROR;
        t.len = strlen(t.buff);
        sendn(user->sockfd, &t, 4 + 4);
        // 关闭连接
        close_mysql_tools(conn, NULL);
        free(curtime);
        return;
    }
    // 获取插入的自增 ID
    unsigned long long insert_user_id = mysql_insert_id(conn);
    // if (insert_id == 0 && mysql_errno(conn) != 0) {
    //     fprintf(stderr, "mysql_insert_id() failed. Error: %s\n", mysql_error(conn));
    // } else {
    //     printf("Inserted ID: %llu\n", insert_id);
    // }
    // 生成随机盐值
    t.type = TAST_REGISTER_RESP_OK;
    char output[65] = { 0 };
    char sha_str[128] = { 0 };
    sprintf(sha_str, "%s%s", name, CHARSET_FILE);
    sha256(sha_str, output);

    // 重置sql
    memset(sql, 0, sizeof(sql));
    // 向数据库插入一条起始数据；
    sprintf(sql, "insert into user_file(parent_id, filename, owner_id, pwd, sha256, filesize, type, time) values(%d, '%s', %d, '%s', '%s', %d, '%s', '%s');",
                                   0, name, insert_user_id, "", output, 0, "d", curtime);
    result = execute_sql(conn, sql);
    // printf("sql: %s\n", sql);
    printf("insert user_file result: %d\n", result);

    t.len = strlen(t.buff);
    sendn(user->sockfd, &t, 4 + 4);
    printf("register successful.\n");
    
    // 获取user_file的id
    unsigned long long insert_user_file_id = mysql_insert_id(conn);

    strcpy(user->encrypted, password);
    strcpy(user->name, name);
    user->user_id = insert_user_id;
    user->user_file_id = insert_user_file_id;
    // 关闭连接
    close_mysql_tools(conn, NULL);
    free(curtime);
    return;
}
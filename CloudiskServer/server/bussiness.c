#include "linked_list.h"
#include "thread_pool.h"
#include "../mysql_tools/mysql_tools.h"
#include "user.h"
#include "time_tools.h"
# define SPLICE_F_MOVE 1

#define MAX_PATH 1024
#define CHUNK_SIZE 4096

// 外部变量(userList是在main.c文件中定义的)
extern ListNode *userList;

// 主线程调用:处理客户端发过来的消息
void handleMessage(int sockfd, int epfd, task_queue_t *que)
{
    // 消息格式：cmd content
    // 1.1 获取消息长度
    int length = -1;
    int ret = recvn(sockfd, &length, sizeof(length));
    if (ret == 0)
    {
        goto end;
    }
    printf("\n\nrecv length: %d\n", length);

    // 1.2 获取消息类型
    int cmdType = -1;
    ret = recvn(sockfd, &cmdType, sizeof(cmdType));
    if (ret == 0)
    {
        goto end;
    }
    printf("recv cmd type: %d\n", cmdType);

    task_t *ptask = (task_t *)calloc(1, sizeof(task_t));
    ptask->peerfd = sockfd;
    ptask->epfd = epfd;
    ptask->type = cmdType;
    if (length > 0)
    {
        // 1.3 获取消息内容
        ret = recvn(sockfd, ptask->data, length);
        if (ret > 0)
        {
            // 往线程池中添加任务
            if (ptask->type == CMD_TYPE_PUTS)
            {
                // 是上传文件任务，就暂时先从epoll中删除监听
                delEpollReadfd(epfd, sockfd);
            }
            taskEnque(que, ptask);
        }
    }
    else if (length == 0)
    {
        taskEnque(que, ptask);
    }
end:
    if (ret == 0)
    { // 连接断开的情况

        printf("\nconn %d is closed.\n", sockfd);
        // 设置用户状态为0表示已断开连接
        // 暂时注释，需要等待登录更新
        // set_user_status_zero(sockfd);

        delEpollReadfd(epfd, sockfd);
        close(sockfd);
        deleteNode2(&userList, sockfd); // 删除用户信息
    }
}

// 注意：此函数可以根据实际的业务逻辑，进行相应的扩展
// 子线程调用
void doTask(task_t *task)
{
    assert(task);
    switch (task->type)
    {
    case CMD_TYPE_PWD:
        pwdCommand(task);
        break;
    case CMD_TYPE_CD:
        cdCommand(task);
        break;
    case CMD_TYPE_LS:
        lsCommand(task);
        break;
    case CMD_TYPE_MKDIR:
        mkdirCommand(task);
        break;
    case CMD_TYPE_RMDIR:
        rmdirCommand(task);
        break;
    case CMD_TYPE_NOTCMD:
        notCommand(task);
        break;
    case CMD_TYPE_PUTS:
        putsCommand(task);
        break;
    case CMD_TYPE_GETS:
        getsCommand(task);
        break;
    case TASK_LOGIN_SECTION1:
        userLoginCheck1(task);
        break;
    case TASK_LOGIN_SECTION2:
        userLoginCheck2(task);
        break;
    case TAST_REGISTER:
        userRegister(task);
        break;
    }
}
// 统计路径的 /
int count_slashes(const char *str)
{
    int count = 0;
    while (*str != '\0')
    {
        if (*str == '/')
        {
            count++;
        }
        str++;
    }
    return count;
}
// 获取绝对路径
void get_abspath_buff(char **buff, const char *data)
{
    char *temp_data = strdup(data); // 使用 strdup 复制字符串，确保可以修改
    char *temp = strtok(temp_data, "/");
    int index = 0;
    while (temp != NULL)
    {
        buff[index] = strdup(temp); // 复制每个子字符串
        index++;
        temp = strtok(NULL, "/");
    }
    // Add null terminator at the end of the array
    buff[index] = NULL;
    free(temp_data); // 释放临时数据
}
// 获取相对路径
void get_relpath_buff(char **buff, const char *data)
{
    char *temp_data = strdup(data); // 使用 strdup 复制字符串，确保可以修改
    char *temp = strtok(temp_data, "/");
    int index = 0;
    while (temp != NULL)
    {
        buff[index] = strdup(temp); // 复制每个子字符串
        index++;
        temp = strtok(NULL, "/");
    }
    // Add null terminator at the end of the array
    temp = strtok(NULL, "");
    buff[index] = strdup(temp); // 复制最后一个子字符串
    buff[index + 1] = NULL;
    free(temp_data); // 释放临时数据
}
// 格式化路径
void normalize_path(const char *path, char *result) {
    char stack[MAX_PATH][MAX_PATH];
    int top = -1;
    const char *p = path;
    char buffer[MAX_PATH];
    int buf_len = 0;

    while (*p) {
        if (*p == '/' || *p == '\0') {
            if (buf_len > 0) {
                buffer[buf_len] = '\0';
                if (strcmp(buffer, "..") == 0) {
                    // Handle ".."
                    if (top >= 0) {
                        top--;
                    }
                } else if (strcmp(buffer, ".") != 0 && strlen(buffer) > 0) {
                    // Handle normal directory names
                    strcpy(stack[++top], buffer);
                }
                buf_len = 0;
            }
            if (*p == '\0') break;
        } else {
            buffer[buf_len++] = *p;
        }
        p++;
    }

    // Handle the last segment
    if (buf_len > 0) {
        buffer[buf_len] = '\0';
        if (strcmp(buffer, "..") == 0) {
            if (top >= 0) {
                top--;
            }
        } else if (strcmp(buffer, ".") != 0 && strlen(buffer) > 0) {
            strcpy(stack[++top], buffer);
        }
    }

    // Construct the result path
    result[0] = '\0';
    for (int i = 0; i <= top; i++) {
        strcat(result, "/");
        strcat(result, stack[i]);
    }
    if (strlen(result) == 0) {
        strcpy(result, "/");
    }
}

void send_cd_error(int sockfd, const char *data, train_t *t) {
    char error_info[256] = {0};
    snprintf(error_info, sizeof(error_info), "bash: cd: %s: No such file or directory", data);

    t->type = CMD_TYPE_CD_ERROR;
    strcpy(t->buff, error_info);
    t->len = strlen(t->buff);
    sendn(sockfd, t, 8 + t->len);
}

void handle_root_directory(user_info_t *user, const char * client_info, train_t *t)
{
    printf("execute handle_root_directory\n");
    // 查询结果
    MYSQL *conn;
    MYSQL_RES *res;
    MYSQL_ROW row;
    
    char sql[1024] = { 0 };

    // 创建mysql连接
    conn = mysql_tools_connect(mysql_tools_ip, mysql_tools_port, mysql_tools_username, mysql_tools_password, mysql_tools_database);
    // 查找当前位置的父节点
    snprintf(sql, sizeof(sql), "select parent_id from user_file where id=%d;", user->user_file_id);
    mysql_real_query(conn, sql, strlen(sql));
    res = mysql_store_result(conn);
    if(res == NULL || mysql_num_rows(res) == 0)// 查询异常直接退出
    {   
        send_cd_error(user->sockfd, client_info, t);
        close_mysql_tools(conn, res);
        return;
    }    
    // 判断当前的parent_id是否为0
    row = mysql_fetch_row(res);
    printf("row[0]: %d\n", atoi(row[0]) == 0);

    if(atoi(row[0]) == 0) // 为0说明已经在根目录，无法后退
    {   
        t->type = CMD_TYPE_CD_SUCCESS;
        strcpy(t->buff, "/");
        t->len = strlen(t->buff);
        sendn(user->sockfd, t, 8 + t->len);
        close_mysql_tools(conn, res);
        return;

    }else{ // 否则，parent作为根节点，并查找pwd返回给客户端
        snprintf(sql, sizeof(sql), "select pwd, id from user_file where id=%d;", atoi(row[0]));
        mysql_real_query(conn, sql, strlen(sql));
        res = mysql_store_result(conn);
        if(res == NULL || mysql_num_rows(res) == 0)// 查询异常直接退出
        {   
            send_cd_error(user->sockfd, client_info, t);
            close_mysql_tools(conn, res);
            return;
        }
        row = mysql_fetch_row(res);
        // 切换当前目录id
        user->user_file_id = atoi(row[1]);
        // 发送数据
        t->type = CMD_TYPE_CD_SUCCESS;
        strcpy(t->buff, row[0]);
        t->len = strlen(t->buff);
        sendn(user->sockfd, t, 8 + t->len);
        // 关闭mysql连接资源
        close_mysql_tools(conn, res);
        return;
    }
}

void handle_target_directory(user_info_t *user, const char * target_path, const char * client_info, train_t * t)
{
    printf("execute handle_target_directory\n");
    // 查询结果
    MYSQL *conn;
    MYSQL_RES *res;
    MYSQL_ROW row;
    
    char sql[1024] = { 0 };
    char userpath[256] = {0};
    // 创建mysql连接
    conn = mysql_tools_connect(mysql_tools_ip, mysql_tools_port, mysql_tools_username, mysql_tools_password, mysql_tools_database);
    snprintf(sql, sizeof(sql), "select pwd from user_file where id = %d;", user->user_file_id);
    mysql_real_query(conn, sql, strlen(sql));
    res = mysql_store_result(conn);

    if(res == NULL || mysql_num_rows(res) == 0)// 查询异常直接退出
    {   
        send_cd_error(user->sockfd, client_info, t);
        close_mysql_tools(conn, res);
        return;
    }
    row = mysql_fetch_row(res);
    snprintf(userpath, sizeof(sql), "%s%s", row[0], target_path);

    // 根据结合的地址在数据库中查找
    snprintf(sql, sizeof(sql), "select id from user_file where owner_id = %d and pwd = '%s'", user->user_id, userpath);
    printf("%s\n", sql);
    mysql_real_query(conn, sql, strlen(sql));
    res = mysql_store_result(conn);
    if(res == NULL || mysql_num_rows(res) == 0)// 查询异常直接退出
    {   
        send_cd_error(user->sockfd, client_info, t);
        close_mysql_tools(conn, res);
        return;
    }

    row = mysql_fetch_row(res);
    printf("handle_target_directory query successful\n");
    // 查询到说明存在该地址，则切换user_file_id
    user->user_file_id = atoi(row[0]);
    printf("update user_file_id: %d\n", user->user_file_id);
    // 向客户端发送当前地址
    t->type = CMD_TYPE_CD_SUCCESS;
    strcpy(t->buff, userpath);
    t->len = strlen(t->buff);

    sendn(user->sockfd, t, 8 + t->len);
    printf("发送成功，t.type: %d, t.buff: %s, t.len: %d\n", t->type, t->buff, t->len);
    close_mysql_tools(conn, res);
    return;
}

void handle_cd_command(user_info_t * user, task_t *task)
{
    train_t t;
    char data[MAX_PATH] = { 0 };

    normalize_path(task->data, data); // 如果是.., 会转换成/
    // 格式化结构体t
    memset(&t, 0, sizeof(t));
    printf("data: %s\n", data);
    if(strcmp(data, "/") == 0)// 如果格式化后只有”/“，说明是回退
    {
        handle_root_directory(user, task->data, &t);
    }else{ // 执行进入目录功能
        handle_target_directory(user, data, task->data, &t);
    }
    return;
}
// 只允许相对路径和.. 
void cdCommand(task_t *task)
{
    printf("execute cdCommand\n");
    ListNode *pNode = userList;
    while (pNode != NULL)
    {
        user_info_t *user = (user_info_t *)pNode->val;
        if (user->sockfd == task->peerfd)
        {
            printf("==========================================================\n");
            handle_cd_command(user, task);
            return;
        }
        pNode = pNode->next;
    }
        
}

void mkdirCommand(task_t *task)
{
    printf("execute mkdirCommand\n");
    ListNode *pNode = userList;
    while (pNode != NULL)
    {
        user_info_t *user = (user_info_t *)pNode->val;
        if (user->sockfd == task->peerfd)
        {
            char *curtime = get_nowtime();
            if (curtime == NULL) {
                printf("get error\n");
                return;
            }

            // 绝对路径
            if (strncmp(task->data, "/", 1) == 0)
            {
                // 统计一共多少个路径
                int slash_nums = count_slashes(task->data);
                char **buff = (char **)calloc(slash_nums + 1, sizeof(char *));
                if(buff == NULL)
                {
                    char error_info[128] = "server mkdir fail.";
                    sendn(user->sockfd, error_info, strlen(error_info));
                    free(curtime);
                    return;
                }
                get_abspath_buff(buff, task->data);
                // 创建mysql连接
                MYSQL *conn = mysql_tools_connect(mysql_tools_ip, mysql_tools_port, mysql_tools_username, mysql_tools_password, mysql_tools_database);
                // 查询结果
                MYSQL_RES *res;
                MYSQL_ROW row;
                int ret; 

                // 先查找根目录id
                char sql[1024] = { 0 };
                sprintf(sql, "select id, pwd from user_file where filename='%s' and parent_id = 0", user->name);
                mysql_real_query(conn, sql, strlen(sql));
                res = mysql_store_result(conn);
                row = mysql_fetch_row(res);

                int id = atoi(row[0]);
                char pwd[256] = { 0 };
                strcpy(pwd, row[1]);

                for (int i = 0; i < slash_nums && buff[i] != NULL; i++)
                {
                    // 生成路径
                    strcat(pwd, "/");
                    strcat(pwd, buff[i]);

                    memset(sql, 0, sizeof(sql));
                    snprintf(sql, sizeof(sql), "select id, pwd from user_file where owner_id = %d and parent_id = %d and filename = '%s';",
                                                user->user_id, id, buff[i]);
                    mysql_real_query(conn, sql, strlen(sql));
                    res = mysql_store_result(conn);
                    if (res == NULL || mysql_num_rows(res) == 0)
                    {
                        char output[65] = { 0 };
                        char sha_str[128] = { 0 };
                        sprintf(sha_str, "%s%s", buff[i], CHARSET_FILE);
                        sha256(sha_str, output);

                        memset(sql, 0, sizeof(sql));
                        sprintf(sql, "insert into user_file(parent_id, filename, owner_id, pwd, sha256, filesize, type, time)"
                                    "values(%d, '%s', %d, '%s', '%s', %d, '%s', '%s');",
                                    id, buff[i], user->user_id, pwd, output, 0, "d", curtime);
                        execute_sql(conn, sql);
                        
                        // 获取插入的自增 ID
                        unsigned long long insert_id = mysql_insert_id(conn);
                        id = insert_id; 
                    }
                    else{
                        row = mysql_fetch_row(res);
                        id = atoi(row[0]);
                    }
                    
                }
                // 释放资源
                for (int i = 0; i < slash_nums && buff[i] != NULL; i++)
                {
                    free(buff[i]); // 释放每个复制的子字符串
                }
                free(curtime);
                free(buff);
                close_mysql_tools(conn, res);
                return;
            }
            // 相对路径
            else
            {   
                // 获取最后一层的filename
                int slash_nums = count_slashes(task->data);
                char **buff = (char **)calloc(slash_nums + 2, sizeof(char *));
                if(buff == NULL)
                {
                    char error_info[128] = "server mkdir fail.";
                    sendn(user->sockfd, error_info, strlen(error_info));
                    free(curtime);
                    return;
                }
                get_abspath_buff(buff, task->data);
                // 创建mysql连接
                MYSQL *conn = mysql_tools_connect(mysql_tools_ip, mysql_tools_port, mysql_tools_username, mysql_tools_password, mysql_tools_database);
                MYSQL_RES *res;
                MYSQL_ROW row;
                int ret;

                char sql[1024] = { 0 };
                int id = user->user_file_id;
                char pwd[256] = { 0 };
                
                snprintf(sql, sizeof(sql), "select pwd from user_file where id=%d", user->user_file_id);
                mysql_real_query(conn, sql, strlen(sql));
                res = mysql_store_result(conn);
                row = mysql_fetch_row(res);
                strcpy(pwd, row[0]);

                for (int i = 0; i < slash_nums + 1 && buff[i] != NULL; i++)
                {
                    // 生成路径
                    strcat(pwd, "/");
                    strcat(pwd, buff[i]);
                    
                    memset(sql, 0, sizeof(sql));
                    snprintf(sql, sizeof(sql), "select id from user_file where owner_id = %d and parent_id = %d and filename = '%s';",
                                                user->user_id, id, buff[i]);
                    mysql_real_query(conn, sql, strlen(sql));
                    res = mysql_store_result(conn);
                    if (res == NULL || mysql_num_rows(res) == 0)
                    {

                        char output[65] = { 0 };
                        char sha_str[128] = { 0 };
                        sprintf(sha_str, "%s%s", buff[i], CHARSET_FILE);
                        sha256(sha_str, output);
                        
                        memset(sql, 0, sizeof(sql));
                        snprintf(sql, sizeof(sql), "insert into user_file(parent_id, filename, owner_id, pwd, sha256, filesize, type, time)"
                                    " values(%d, '%s', %d, '%s', '%s', %d, '%s', '%s');",
                                        id, buff[i], user->user_id, pwd, output, 0, "d", curtime);
                        printf("sql: %s\n", sql);
                        ret = execute_sql(conn, sql);
                        if(ret == -1)
                        {

                            memset(sql, 0, sizeof(sql));
                            snprintf(sql, sizeof(sql), "select id from user_file where owner_id = %d and parent_id = %d and filename = '%s';",
                                                        user->user_id, id, buff[i]);
                            printf("sql: %s\n", sql);
                            res = mysql_store_result(conn);
                            row = mysql_fetch_row(res);
                            id = atoi(row[0]);
                            continue;
                        }
                        // 获取插入的自增 ID
                        unsigned long long insert_id = mysql_insert_id(conn);
                        id = insert_id;
                    }
                    else{
                        
                        row = mysql_fetch_row(res);
                        id = atoi(row[0]);
                    }
                }
                // 释放资源
                for (int i = 0; i < slash_nums + 1 && buff[i] != NULL; i++)
                {
                    free(buff[i]); // 释放每个复制的子字符串
                }
                free(curtime);
                free(buff);
                close_mysql_tools(conn, res);

                return;
            }
        }
        pNode = pNode->next;
    }
}

void rmdirCommand(task_t *task)
{
    ListNode *pNode = userList;
    while (pNode != NULL)
    {
        user_info_t *user = (user_info_t *)pNode->val;
        if (user->sockfd == task->peerfd)
        {
            char data[MAX_PATH] = { 0 };
            normalize_path(task->data, data);

            char sql[1024] = { 0 };
            // 创建mysql连接
            MYSQL *conn = mysql_tools_connect(mysql_tools_ip, mysql_tools_port, mysql_tools_username, mysql_tools_password, mysql_tools_database);
            snprintf(sql, sizeof(sql),  "delete from user_file where owner_id = %d and pwd like '%%%s%%'", user->user_id, data);
            execute_sql(conn, sql);

            close_mysql_tools(conn, NULL);
        }
        pNode = pNode->next;
    }
}

void lsCommand(task_t *task)
{
    printf("execute lsCommand\n");
    ListNode *pNode = userList;
    while (pNode != NULL)
    {
        user_info_t *user = (user_info_t *)pNode->val;
        if (user->sockfd == task->peerfd)
        {
            // 创建mysql连接
            MYSQL *conn = mysql_tools_connect(mysql_tools_ip, mysql_tools_port, mysql_tools_username, mysql_tools_password, mysql_tools_database);
            // 查询结果
            MYSQL_RES *res;
            MYSQL_ROW row;

            char sql[1024] = {0};
            sprintf(sql, "select filename, type from user_file where parent_id = %d;",
                    user->user_file_id);
            // printf("sql: %s\n", sql);
            // 执行查询语句
            mysql_real_query(conn, sql, strlen(sql));
            res = mysql_store_result(conn);
            if (res == NULL || mysql_num_rows(res) == 0)
            {
                printf("res is NULL.\n");
                // 释放资源
                close_mysql_tools(conn, res);
                return;
            }
            char buff[1024] = {0};

            while ((row = mysql_fetch_row(res)) != NULL)
            {   
                char temp[256] = {0};
                if (row[1] == NULL) {
                    continue;  // 或者处理空指针的情况
                }

                if (strcmp(row[1], "d") == 0)
                {   
                    snprintf(temp, sizeof(temp), "\033[1;34m%s\033[0m\t", row[0]);
                }
                else
                {
                    snprintf(temp, sizeof(temp), "\033[1;32m%s\033[0m\t", row[0]);
                }
                strcat(buff, temp);
            }

            sendn(user->sockfd, buff, strlen(buff));
            // 释放资源
            close_mysql_tools(conn, res);
            return;
        }
        pNode = pNode->next;
    }
}

void pwdCommand(task_t *task)
{
    ListNode *pNode = userList;
    while (pNode != NULL)
    {
        user_info_t *user = (user_info_t *)pNode->val;
        if (user->sockfd == task->peerfd)
        {
            // 创建mysql连接
            MYSQL *conn = mysql_tools_connect(mysql_tools_ip, mysql_tools_port, mysql_tools_username, mysql_tools_password, mysql_tools_database);
            // 查询结果
            MYSQL_RES *res;
            MYSQL_ROW row;

            char sql[1024] = {0};

            sprintf(sql, "select pwd from user_file where id=%d", user->user_file_id);
            
            // printf("sql: %s\n", sql);
            // 执行查询语句
            mysql_real_query(conn, sql, strlen(sql));
            res = mysql_store_result(conn);
            row = mysql_fetch_row(res);

            if(strcmp(row[0], "") != 0)
            {
                sendn(user->sockfd, row[0], strlen(row[0]));
            }
            else
            {
                sendn(user->sockfd, "/", strlen("/"));
            }
            // 释放资源
            close_mysql_tools(conn, res);
            return;
        }
        pNode = pNode->next;
    }
}

void notCommand(task_t *task)
{

    printf("execute not command.\n");
    // printf("userLoginCheck1.\n");
    ListNode *pNode = userList;
    while (pNode != NULL)
    {
        user_info_t *user = (user_info_t *)pNode->val;
        if (user->sockfd == task->peerfd)
        {
            send(user->sockfd, "command not found", strlen("command not found"), 0);
            return;
        }
        pNode = pNode->next;
    }
}

void sha256_file(const char *filePath, char outputBuffer[65]) {
    EVP_MD_CTX *ctx = EVP_MD_CTX_new();
    if (ctx == NULL) {
        fprintf(stderr, "EVP_MD_CTX_new failed\n");
        return;
    }

    const EVP_MD *md = EVP_sha256();
    unsigned char hash[EVP_MAX_MD_SIZE];
    unsigned int lengthOfHash = 0;

    if (EVP_DigestInit_ex(ctx, md, NULL) != 1) {
        fprintf(stderr, "EVP_DigestInit_ex failed\n");
        EVP_MD_CTX_free(ctx);
        return;
    }

    FILE *file = fopen(filePath, "rb");
    if (file == NULL) {
        perror("fopen failed");
        EVP_MD_CTX_free(ctx);
        return;
    }

    // Read the file in chunks
    unsigned char buffer[1024];
    size_t bytesRead;
    while ((bytesRead = fread(buffer, 1, sizeof(buffer), file)) > 0) {
        if (EVP_DigestUpdate(ctx, buffer, bytesRead) != 1) {
            fprintf(stderr, "EVP_DigestUpdate failed\n");
            fclose(file);
            EVP_MD_CTX_free(ctx);
            return;
        }
    }

    fclose(file);

    if (EVP_DigestFinal_ex(ctx, hash, &lengthOfHash) != 1) {
        fprintf(stderr, "EVP_DigestFinal_ex failed\n");
        EVP_MD_CTX_free(ctx);
        return;
    }

    EVP_MD_CTX_free(ctx);

    // Convert the hash to a hexadecimal string
    for (unsigned int i = 0; i < lengthOfHash; i++) {
        sprintf(outputBuffer + (i * 2), "%02x", hash[i]);
    }
    outputBuffer[64] = '\0'; // Null-terminate the string
}

int affirm_puts_sha256(int sockfd, train_t * t)
{
    int result = 1;
    recvn(sockfd, &t->len, sizeof(int));
    recvn(sockfd, &t->type, sizeof(int));
    recvn(sockfd, t->buff, t->len); // t.buff就是文件的sha256
    // 创建mysql连接
    MYSQL *conn = mysql_tools_connect(mysql_tools_ip, mysql_tools_port, mysql_tools_username, mysql_tools_password, mysql_tools_database);
    // 查询结果
    MYSQL_RES *res;
    MYSQL_ROW row;  
    char sql[256] = { 0 };
    snprintf(sql, sizeof(sql), "select filesize from filepath where sha256='%s' and status = 1", t->buff);
    mysql_real_query(conn, sql, strlen(sql));
    res = mysql_store_result(conn);
    if(res == NULL || mysql_num_rows(res) == 0)
        result = 0;
    else{
        row = mysql_fetch_row(res);
        result = atoi(row[0]);
    }
    // 释放资源
    close_mysql_tools(conn, res);
    return result;
}

void upload_userfile(user_info_t *user, const char *filename, int filesize, train_t * t)
{
    printf("execute upload_userfile\n");
    // 创建mysql连接
    MYSQL *conn = mysql_tools_connect(mysql_tools_ip, mysql_tools_port, mysql_tools_username, mysql_tools_password, mysql_tools_database);
    // 查询结果
    MYSQL_RES *res;
    MYSQL_ROW row;  
    char * curtime = get_nowtime();
    char sql[1024] = { 0 };
    snprintf(sql, sizeof(sql), "select pwd from user_file where id=%d", user->user_file_id);
    mysql_real_query(conn, sql, strlen(sql));
    res = mysql_store_result(conn);
    row = mysql_fetch_row(res);

    // 查找当前路径
    char pwd[128] = { 0 };
    snprintf(pwd, sizeof(pwd), "%s/%s", row[0], filename);    
    char filetype[2] = "f";
    snprintf(sql, sizeof(sql), "insert into user_file (parent_id, filename, owner_id, pwd, sha256, filesize, type, time)"
                                "values(%d, '%s', %d, '%s', '%s', %d, '%s', '%s')",
                                user->user_file_id, filename, user->user_id, pwd, t->buff, filesize, filetype, curtime);

    int result = execute_sql(conn, sql);
    printf("insert user_file result: %d\n", result);
    // 释放资源
    close_mysql_tools(conn, res);
    free(curtime);
}

void send_puts_error(int sockfd, train_t * t)
{
    t->type = CMD_TYPE_PUTS_ERROR;
    sendn(sockfd, &t->type, 4);
}

// 上传文件
void handle_upload_file(user_info_t *user, const char *filename, const char * file_sha256, train_t * t)
{   
    // 计算当前文件应该存放的位置
    char filepath[256] = { 0 };
    snprintf(filepath, sizeof(filepath), "%s/%s", USER_FILEPATH, file_sha256);
    int fd = open(filepath, O_RDWR | O_CREAT, 0644);
    memset(t, 0, sizeof(*t));
    if(fd == -1)
    {
        perror("open");
        printf("puts error.\n");
        send_puts_error(user->sockfd, t);
        return;
    }

    int pipe_fd[2];
    // 创建管道
    if (pipe(pipe_fd) == -1) {
        printf("puts error.\n");
        send_puts_error(user->sockfd, t);
        return;
    }

    // 确认准备ok，向客户端发送，等待客户端发送数据
    t->type = CMD_TYPE_PUTS_SUCCESS;
    sendn(user->sockfd, &t->type, 4);
    // ======================================================准备工作完成===========================================================
    // ======================================================准备接收文件===========================================================
    // 接收文件大小
    off_t filesize = 0;
    recvn(user->sockfd, &filesize, sizeof(off_t));

    // 查找服务端中是否存在该文件
    //获取文件大小
    struct stat st;
    memset(&st, 0, sizeof(st));
    // 获取文件的stat信息
    fstat(fd, &st);
    off_t system_filesize = st.st_size;
    printf("%s: size: %ld\n", filepath, system_filesize);

    sendn(user->sockfd, &system_filesize, sizeof(system_filesize));
    printf("system_filesize: %ld\n", system_filesize);
    printf("filesize: %ld\n", filesize);

    if(system_filesize == filesize)
    {
        printf("file already upload\n");
        return;
    }

    off_t bytes_received = 0;
    off_t total_bytes = 0;
    // Use splice to transfer data from socket to pipe
    // while ((bytes_received = splice(user->sockfd, NULL, pipe_fd[1], NULL, CHUNK_SIZE, SPLICE_F_MOVE)) > 0) {
    //     // Use splice to write data from pipe to file
    //     ssize_t bytes_written = splice(pipe_fd[0], NULL, fd, NULL, bytes_received, SPLICE_F_MOVE);
    //     if (bytes_written == -1) {
    //         perror("splice write");
    //         break;
    //     }
    //     total_bytes += bytes_written;
    //     // Check if all data has been received
    //     if (bytes_received < CHUNK_SIZE) {
    //         printf("Received final chunk of %zd bytes\n", bytes_received);
    //         break;
    //     }
    // }

    while (total_bytes < filesize) {
        // 计算剩余字节数
        ssize_t remaining_bytes = filesize - total_bytes;
        ssize_t to_read = remaining_bytes < CHUNK_SIZE ? remaining_bytes : CHUNK_SIZE;

        // 从网络接收数据并存入管道
        ssize_t bytes_received = splice(user->sockfd, NULL, pipe_fd[1], NULL, to_read, SPLICE_F_MOVE);
        if (bytes_received == -1) {
            perror("splice read");
            break;
        }

        // 将数据从管道写入文件
        ssize_t bytes_written = splice(pipe_fd[0], NULL, fd, NULL, bytes_received, SPLICE_F_MOVE);
        if (bytes_written == -1) {
            perror("splice write");
            break;
        }

        // 累积总字节数
        total_bytes += bytes_written;
    }

    // 所有数据接收完成
    printf("total_bytes: %ld\n", total_bytes);
    printf("filesize: %ld\n", filesize);

    int status;
    if(total_bytes == filesize)
        status = 1;
    else
        status = 0;
    // 先向filepath中添加数据
    // 创建mysql连接
    MYSQL * conn = mysql_tools_connect(mysql_tools_ip, mysql_tools_port, mysql_tools_username, mysql_tools_password, mysql_tools_database);
    MYSQL_RES *res;
    MYSQL_ROW row;
    
    char sql[1024] = {0};
    char * curtime = get_nowtime();
   
    // 插入filepath表
    snprintf(sql, sizeof(sql), "insert into filepath (sha256, pwd, filesize, actual_filesize, status, time)"
                                " values('%s', '%s', %d, %d, %d, '%s');",
                                file_sha256, USER_FILEPATH, total_bytes, filesize, status, curtime);
    printf("sql:%s\n", sql);
    int result = execute_sql(conn, sql);
    printf("result: %d\n", result);

    if(status == 1)
    {
        snprintf(sql, sizeof(sql), "select pwd from user_file where id = %d;", user->user_file_id);
        printf("sql: %s\n", sql);
        mysql_real_query(conn, sql, strlen(sql));
        res = mysql_store_result(conn);
        row = mysql_fetch_row(res);
        char pwd[128] = {0};
        snprintf(pwd, sizeof(pwd), "%s/%s", row[0], filename);

        char type[2] = "f";
        snprintf(sql, sizeof(sql), "insert into user_file (parent_id, filename, owner_id, pwd, sha256, filesize, type, time)"
                                " values(%d, '%s', %d, '%s', '%s', %d, '%s', '%s');",
                                user->user_file_id, filename, user->user_id,  pwd, file_sha256, filesize, type, curtime);
        printf("sql: %s\n", sql);

        int result = execute_sql(conn, sql);
        printf("result: %d\n", result);
    }
    // 关闭连接
    close_mysql_tools(conn, res);
    free(curtime);
}

void putsCommand(task_t * task) {
    printf("execute putsCommand\n");
    ListNode *pNode = userList;
    while (pNode != NULL)
    {
        user_info_t *user = (user_info_t *)pNode->val;
        if (user->sockfd == task->peerfd)
        {
            printf("==========================\n");
            train_t t;
            memset(&t, 0, sizeof(t));
            int filesize = affirm_puts_sha256(task->peerfd, &t); 
            if(filesize != 0) // filesize不为零说明文件已上传
            {
                upload_userfile(user, task->data, filesize, &t);

                memset(&t, 0, sizeof(t));
                t.type = CMD_TYPE_PUTS_SHA256_EXISTS;
                sendn(task->peerfd, &t.type, 4);
                // 重新添加监听
                addEpollReadfd(task->epfd, user->sockfd);
                return;
            }

            // 获取当前文件的sha256码
            char file_sha256[65] = {0};
            strcpy(file_sha256, t.buff); // 拷贝之前recv的sha256码
            // 向客户端发送信息，服务端不存在该文件，需要发送
            memset(&t, 0, sizeof(t));
            t.type = CMD_TYPE_PUTS_SHA256_NOEXISTS;
            sendn(user->sockfd, &t.type, 4);
            handle_upload_file(user, task->data, file_sha256, &t);
            printf("==========================\n");
            addEpollReadfd(task->epfd, user->sockfd);
            return;
        }
        pNode = pNode->next;
    }
}

void handle_gets_file(user_info_t * user, const char * filename)
{
    train_t t;
    memset(&t, 0, sizeof(t));
    
    // 创建mysql连接
    MYSQL * conn = mysql_tools_connect(mysql_tools_ip, mysql_tools_port, mysql_tools_username, mysql_tools_password, mysql_tools_database);
    MYSQL_RES *res;
    MYSQL_ROW row;
    
    char sql[1024] = {0};
    // 查询文件名下的sha256码
    snprintf(sql, sizeof(sql), "select sha256, type from user_file where parent_id = %d and filename = '%s';", user->user_file_id, filename);
    mysql_real_query(conn, sql, strlen(sql));
    res = mysql_store_result(conn);
    if(res == NULL || mysql_num_rows(res) == 0)
    {
        // 不存在需要向客户端发送状态
        t.type = CMD_TYPE_GETS_SHA256_NOEXISTS;
        sendn(user->sockfd, &t.type, sizeof(int));
        close_mysql_tools(conn, res);
        return;
    }
    row = mysql_fetch_row(res);
    // 需要判断是否是文件夹，不允许传文件夹
    if(strcmp(row[1], "d") == 0)
    {
        // 不存在需要向客户端发送状态
        t.type = CMD_TYPE_GETS_TYPE_ERROR;
        sendn(user->sockfd, &t.type, sizeof(int));
        close_mysql_tools(conn, res);
        return;
    }
    t.type = CMD_TYPE_GETS_SHA256_EXISTS;
    sendn(user->sockfd, &t.type, sizeof(int));
    // ================================================准备工作就绪，准备发送文件============================================================
    // 找到该文件后，查找路径
    // 先创建文件路径
    char filepath[256] = {0};
    snprintf(filepath, sizeof(filepath), "%s/%s", USER_FILEPATH, row[0]);
    printf("filepath: %s\n", filepath);
    int fd = open(filepath, O_RDONLY);
    struct stat st;
    fstat(fd, &st);
    off_t filesize = st.st_size;
    //发送文件长度
    sendn(user->sockfd, &filesize, sizeof(off_t));
    off_t offset = 0;
    // 发送文件
    off_t bytes_sent;
    off_t total_sent = 0;

    while (total_sent < filesize) {
        size_t bytes_to_send = CHUNK_SIZE;
        if (total_sent + bytes_to_send > st.st_size) {
            bytes_to_send = st.st_size - total_sent;
        }

        bytes_sent = sendfile(user->sockfd, fd, &offset, bytes_to_send);
        if (bytes_sent == -1) {
            perror("sendfile");
            break;
        }

        total_sent += bytes_sent;
        printf("Sent %zd bytes, total %zd/%zd\n", bytes_sent, total_sent, st.st_size);
    }
    
    //发送完成
    printf("puts: file already upload.\n");
    close(fd);

    close_mysql_tools(conn, res);
}

// 客户端下载文件
void getsCommand(task_t *task)
{

    printf("execute gets command.\n");
    ListNode * pNode = userList;
    while(pNode != NULL) {
        user_info_t * user = (user_info_t *)pNode->val;
        if(user->sockfd == task->peerfd) {
            printf("===============================\n");
            handle_gets_file(user, task->data);
            return;
            
            printf("===============================\n");
        }
        pNode = pNode->next;
    }
}

void userLoginCheck1(task_t *task)
{
    // printf("userLoginCheck1.\n");
    ListNode *pNode = userList;
    while (pNode != NULL)
    {
        user_info_t *user = (user_info_t *)pNode->val;
        if (user->sockfd == task->peerfd)
        {
            // 拷贝用户名
            strcpy(user->name, task->data);
            // loginCheck1(user);
            loginCheck1_version2(user, task->data);
            return;
        }
        pNode = pNode->next;
    }
}

void userLoginCheck2(task_t *task)
{
    // printf("userLoginCheck2.\n");
    ListNode *pNode = userList;
    while (pNode != NULL)
    {
        user_info_t *user = (user_info_t *)pNode->val;
        if (user->sockfd == task->peerfd)
        {
            // 拷贝加密密文
            loginCheck2_version2(user, task->data);

            return;
        }
        pNode = pNode->next;
    }
}

void set_user_status_zero(int sockfd)
{
    ListNode *pNode = userList;
    while (pNode != NULL)
    {
        user_info_t *user = (user_info_t *)pNode->val;
        if (user->sockfd == sockfd)
        {
            char *curtime = get_nowtime();

            // 创建mysql连接
            MYSQL *conn = mysql_tools_connect(mysql_tools_ip, mysql_tools_port, mysql_tools_username, mysql_tools_password, mysql_tools_database);
            char sql[1024] = {0};
            sprintf(sql, "update user set time = '%s' and status = 0 where name = '%s' and password = '%s';",
                    user->name, user->encrypted);
            // 更新结果
            int ret = execute_sql(conn, sql);
            printf("update user status: %d\n", ret);

            // 释放资源
            close_mysql_tools(conn, NULL);
            free(curtime);
            return;
        }
        pNode = pNode->next;
    }
}

void userRegister(task_t *task)
{
    printf("Execute userRegister.\n");
    ListNode *pNode = userList;
    while (pNode != NULL)
    {
        user_info_t *user = (user_info_t *)pNode->val;
        if (user->sockfd == task->peerfd)
        {
            // 注册用户信息
            rigister_user_info(user, task->data);
            return;
        }
        pNode = pNode->next;
    }
}
#ifndef __USER_H__
#define __USER_H__

typedef enum {
    STATUS_LOGOFF = 0,
    STATUS_LOGIN
}LoginStatus;

typedef struct {
    int sockfd;//套接字文件描述符
    int user_id;
    int user_file_id;
    LoginStatus status;//登录状态
    char name[50];//用户名(客户端传递过来的)
    char encrypted[100];
}user_info_t;

void loginCheck1(user_info_t * user);
void loginCheck2(user_info_t * user, const char * encrypted);

void loginCheck1_version2();
void loginCheck2_version2();

#endif


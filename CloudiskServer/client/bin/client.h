#ifndef __CLIENT_H_
#define __CLIENT_H_

#include <dirent.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <openssl/evp.h>

#define USER_NAME "请输入用户名："
#define PASSWORD "请输入密码："
#define REGISTER_NAME "请输入注册用户名："
#define REGISTER_PASSWORD "请输入注册密码："
#define SALT_LENGTH 16
#define CHARSET "charset"
#define CLIENT_DATA "../data"

# define SPLICE_F_MOVE 1

typedef enum {
    CMD_TYPE_PWD=1,
    CMD_TYPE_LS,
    
    CMD_TYPE_CD,
    CMD_TYPE_CD_SUCCESS,
    CMD_TYPE_CD_ERROR,

    CMD_TYPE_MKDIR,
    CMD_TYPE_RMDIR,

    CMD_TYPE_PUTS,
    CMD_TYPE_PUTS_SHA256,
    CMD_TYPE_PUTS_SHA256_EXISTS,
    CMD_TYPE_PUTS_SHA256_NOEXISTS,
    CMD_TYPE_PUTS_SUCCESS,
    CMD_TYPE_PUTS_ERROR,

    CMD_TYPE_GETS,
    CMD_TYPE_GETS_SHA256_EXISTS,
    CMD_TYPE_GETS_SHA256_NOEXISTS,
    CMD_TYPE_GETS_TYPE_ERROR,
    
    CMD_TYPE_NOTCMD,  //不是命令

    TAST_REGISTER = 50,
    TAST_REGISTER_RESP_OK,
    TAST_REGISTER_RESP_ERROR,

    TASK_LOGIN_SECTION1 = 100,
    TASK_LOGIN_SECTION1_RESP_OK,
    TASK_LOGIN_SECTION1_RESP_ERROR,
    TASK_LOGIN_SECTION2,
    TASK_LOGIN_SECTION2_RESP_OK,
    TASK_LOGIN_SECTION2_RESP_ERROR,

    //客户端命令
    CLIENT_TYPE_LS,
    CLIENT_TYPE_PWD,
    CLIENT_TYPE_CLEAR
}CmdType;

typedef enum{
    Default,
    Login_System,
    Register_User,
    Exit_System
}LoginType;

typedef struct 
{
    int len;//记录内容长度
    CmdType type;//消息类型
    char buff[1000];//记录内容本身
}train_t;

int tcpConnect(const char * ip, unsigned short port);
int recvn(int sockfd, void * buff, int len);
int sendn(int sockfd, const void * buff, int len);

int userLogin(int sockfd);
// 生成随机盐值
void generate_salt(char *salt, size_t length, const char * charset);
// sha256加密 
void sha256(const char *str, char outputBuffer[65]); 
// sha512加密                            
void sha512(const char *str, char outputBuffer[129]);
// 用户注册
int register_user(int sockfd, const char *charset);

int parseCommand(const char * input, int len, train_t * pt);

//判断一个字符串是什么命令
int getCommandType(const char * str);
//执行上传文件操作
void putsCommand(int sockfd, train_t * pt);
// 创建文件夹
int create_directory(const char *path);

// 查看当前文件夹所在路径
void pwdCommand();
// 查看当前文件夹内容
void lsCommand();
// 清空终端
void clearCommand();

#endif
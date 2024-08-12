#ifndef __THREAD_POOL_H__
#define __THREAD_POOL_H__

#define _GNU_SOURCE
#include <arpa/inet.h>
#include <assert.h>

#include <dirent.h>

#include <errno.h>
#include <error.h>

#include <fcntl.h>

#include <netdb.h>
#include <netinet/in.h>

#include <openssl/evp.h>

#include <pthread.h>

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>

#include <sys/epoll.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/sendfile.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <sys/wait.h>

#include <time.h>

#include <unistd.h>

#define SIZE(a) (sizeof(a)/sizeof(a[0]))
#define USER_FILEPATH "/home/mentha/C60_net"

#define CHARSET_FILE "dwa@$!$23142w/\\dwao8hdi|}}}+_';]idcomzow9amudm(&*(&@&(*3e8798)))"
#define SALT_LENGTH 16

typedef void (*sighandler_t)(int);

#define ARGS_CHECK(argc, num)   {\
    if(argc != num){\
        fprintf(stderr, "ARGS ERROR!\n");\
        return -1;\
    }}

#define ERROR_CHECK(ret, num, msg) {\
    if(ret == num) {\
        perror(msg);\
        return -1;\
    }}

#define THREAD_ERROR_CHECK(ret, func) {\
    if(ret != 0) {\
        fprintf(stderr, "%s:%s\n", func, strerror(ret));\
    }}

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

}CmdType;


typedef struct 
{
    int len;//记录内容长度
    CmdType type;
    char buff[1000];//记录内容本身
}train_t;

typedef struct task_s{
    int peerfd;//与client进行通信的套接字
    int epfd;//epoll的实例
    CmdType type;
    char data[1000];
    struct task_s * pNext;
}task_t;

typedef struct task_queue_s
{
    task_t * pFront;
    task_t * pRear;
    int queSize;//记录当前任务的数量
    pthread_mutex_t mutex; 
    pthread_cond_t cond;
    int flag;//0 表示要退出，1 表示不退出

}task_queue_t;

typedef struct threadpool_s {
    pthread_t * pthreads;
    int pthreadNum;
    task_queue_t que;//...任务队列
}threadpool_t;

int queueInit(task_queue_t * que);
int queueDestroy(task_queue_t * que);
int queueIsEmpty(task_queue_t * que);
int taskSize(task_queue_t * que);
int taskEnque(task_queue_t * que, task_t * task);
task_t * taskDeque(task_queue_t * que);
int broadcastALL(task_queue_t* que);

int threadpoolInit(threadpool_t *, int num);
int threadpoolDestroy(threadpool_t *);
int threadpoolStart(threadpool_t *);
int threadpoolStop(threadpool_t *);


int tcpInit(const char * ip, const char * port);
int addEpollReadfd(int epfd, int fd);
int delEpollReadfd(int epfd, int fd);
int transferFile(int sockfd);
int sendn(int sockfd, const void * buff, int len);
int recvn(int sockfd, void * buff, int len);

//处理客户端发过来的消息
void handleMessage(int sockfd, int epfd, task_queue_t * que);
//执行任务的总的函数
void doTask(task_t * task);
//每一个具体命令的执行
void cdCommand(task_t * task);
void lsCommand(task_t * task);
void pwdCommand(task_t * task);
void mkdirCommand(task_t * task);
void rmdirCommand(task_t * task);
void notCommand(task_t * task);
void putsCommand(task_t * task);
void getsCommand(task_t * task);

//用户登录的操作
void userLoginCheck1(task_t * task);
void userLoginCheck2(task_t * task);
// 用户注册操作
void userRegister(task_t * task);

// sha256
void sha256(const char *str, char outputBuffer[65]);
// sha512
void sha512(const char *str, char outputBuffer[129]);


#endif
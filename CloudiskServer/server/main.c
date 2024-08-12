#include "config.h"
#include "linked_list.h"
#include "thread_pool.h"
#include "user.h"

#define EPOLL_ARR_SIZE 100

int exitPipe[2];
//定义存储用户信息的链表
ListNode * userList = NULL;

void sigHandler(int num)
{
    printf("\n sig is coming.\n");
    //激活管道, 往管道中写一个1
    int one = 1;
    write(exitPipe[1], &one, sizeof(one));
}

int main(int argc, char ** argv)
{   
    signal(SIGPIPE, SIG_IGN);
    //命令行参数要读取配置文件
    ARGS_CHECK(argc, 2);
    //创建匿名管道
    pipe(exitPipe);

    //fork之后，将创建了子进程
    pid_t pid = fork();
    if(pid > 0) {//父进程
        close(exitPipe[0]);//父进程关闭读端
        signal(SIGUSR1, sigHandler);
        wait(NULL);//等待子进程退出，回收其资源
        close(exitPipe[1]);
        printf("\nparent process exit.\n");
        exit(0);//父进程退出
    }
    //以下都是子进程中的执行流程
    close(exitPipe[1]);//子进程关闭写端

    //初始化hash表，用来存储配置信息
    HashTable ht;
    initHashTable(&ht);
    //读取服务器配置文件
    readConfig(argv[1], &ht);

    //创建线程池结构体
    threadpool_t threadpool;
    memset(&threadpool, 0, sizeof(threadpool));
    //初始化线程池
    threadpoolInit(&threadpool, atoi((const char*)find(&ht, THREAD_NUM)));
    //启动线程池
    threadpoolStart(&threadpool);

    //创建监听套接字
    int listenfd = tcpInit(find(&ht, IP), find(&ht, PORT));

    //创建epoll实例
    int epfd = epoll_create1(0);
    ERROR_CHECK(epfd, -1, "epoll_create1");

    //对listenfd进行监听
    addEpollReadfd(epfd, listenfd);
    addEpollReadfd(epfd, exitPipe[0]);

    // 在堆上创建epoll_event结构体，通过calloc初始化epoll_event堆空间
    struct epoll_event * pEventArr = (struct epoll_event*)
        calloc(EPOLL_ARR_SIZE, sizeof(struct epoll_event));

    while(1) {
        int nready = epoll_wait(epfd, pEventArr, EPOLL_ARR_SIZE, -1);
        if(nready == -1 && errno == EINTR) {
            continue;
        } else if(nready == -1) {
            ERROR_CHECK(nready, -1, "epoll_wait");
        } else {
            //大于0
            for(int i = 0; i < nready; ++i) {
                int fd = pEventArr[i].data.fd;
                if(fd == listenfd) {//对新连接进行处理
                    int peerfd = accept(listenfd, NULL, NULL);
                    printf("\n conn %d has conneted.\n", peerfd);
                    //userCheck();
                    //将新连接添加到epoll的监听红黑树上
                    addEpollReadfd(epfd, peerfd);
                    //添加用户节点
                    user_info_t * user = (user_info_t*)calloc(1, sizeof(user_info_t));
                    user->sockfd = peerfd;
                    appendNode(&userList, user);
                } else if(fd == exitPipe[0]) {
                    //线程池要退出
                    int howmany = 0;
                    //对管道进行处理
                    read(exitPipe[0], &howmany, sizeof(howmany));
                    //主线程通知所有的子线程退出
                    threadpoolStop(&threadpool);
                    //子进程退出前，回收资源
                    threadpoolDestroy(&threadpool);
                    close(listenfd);
                    close(epfd);
                    close(exitPipe[0]);
                    printf("\nchild process exit.\n");
                    destroyHashTable(&ht);
                    exit(0);
                } else {//客户端的连接的处理
                    handleMessage(fd, epfd, &threadpool.que);
                }
            }
        }
    }

    return 0;
}


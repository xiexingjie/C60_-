#include "client.h"
#include "hashtable.h"
#include "config.h"

#include <func.h>
extern char username[50];

int main()
{
    srand(time(NULL));
    //初始化hash表，用来存储配置信息
    HashTable ht;
    
    initHashTable(&ht);
    //读取服务器配置文件
    readConfig("../conf/client.conf", &ht);

    int clientfd = tcpConnect("127.0.0.1", 8080);
    if (clientfd == -1)
    {
        return 0;
    }
    // 清空终端信息
    printf("\033[H\033[2J");
    
    while(1)
    {
begin:
        fflush(stdin);
        // 清空终端信息
        printf("\033[H\033[2J");
        int choose = 0;
        char input[100];  // 足够大以容纳用户输入
        printf("**************************** 百度网盘项目 ****************************\n");
        printf("****                                                              ****\n");
        printf("**********************************************************************\n");
        printf("****                                                              ****\n");
        printf("*************************** 1、账户登录 ******************************\n");
        printf("****                                                              ****\n");
        printf("**********************************************************************\n");
        printf("****                                                              ****\n");
        printf("*************************** 2、注册账户 ******************************\n");
        printf("****                                                              ****\n");
        printf("**********************************************************************\n");
        printf("****                                                              ****\n");
        printf("*************************** 3、退出系统 ******************************\n");
        printf("****                                                              ****\n");
        printf("**********************************************************************\n");
        printf("请选择功能：");
        fflush(stdout);
        // 使用 fgets 读取用户输入
        if (fgets(input, sizeof(input), stdin) != NULL) {
            // 尝试解析输入
            if (sscanf(input, "%d", &choose) != 1) {
                printf("无效输入，请输入数字。\n");
                continue;  // 重新显示菜单
            }
        } else {
            printf("输入错误，请重试。\n");
            continue;
        }

        switch (choose)
        {
            case Login_System:
                {
                    // 用户登录操作
                    // 登录成功时，确认用户名以及客户端存储数据，放置data文件夹下
                    userLogin(clientfd);
                    goto system_start;
                }
            case Register_User:
                {
                    int result = register_user(clientfd, find(&ht, CHARSET));
                    if(result == -1)
                        continue;
                    goto system_start;
                }
            case Exit_System:
                {
                    printf("Byebye.\n");
                    goto end;
                }
            default:
                {
                    printf("'%d': command not found\n", choose);
                }
        }
    }

system_start:
   
    create_directory(CLIENT_DATA); //在当前文件夹上一层创建目录
    // 进入存放数据客户端文件
    chdir(CLIENT_DATA);

    char curpath[128] = "/";
    char buf[128] = {0};
    // 4. 使用select进行监听
    fd_set rdset;
    train_t t;

    int ret = -1;
    int nready = 0;

    FD_ZERO(&rdset);
    FD_SET(STDIN_FILENO, &rdset);
    FD_SET(clientfd, &rdset);
    int max = clientfd;

    // 清空终端信息
    printf("\033[H\033[2J");
    while (1)
    {
        fd_set temp = rdset;
    
        // 初始打印提示符
        printf("%s@%s:%s$ ", username, username, curpath);
        fflush(stdout);
        nready = select(max + 1, &temp, NULL, NULL, NULL);

        if (nready == -1)
        {
            close(clientfd);
            error(EXIT_FAILURE, errno, "select");
        }

        if (FD_ISSET(STDIN_FILENO, &temp))
        {
            // 读取标准输入中的数据
            memset(buf, 0, sizeof(buf));
            ret = read(STDIN_FILENO, buf, sizeof(buf));
            if (strncmp(buf, "exit", 4) == 0)
            {
                printf("byebye.\n");
                goto end;
            }

            if(strcmp(buf, "\n") == 0)
                continue;
            memset(&t, 0, sizeof(t));
            // 解析命令行
            buf[strlen(buf) - 1] = '\0';
            parseCommand(buf, strlen(buf), &t);
            // 优先执行客户端命令
            if(t.type == CLIENT_TYPE_LS){
                lsCommand();
                continue;
            }
            else if(t.type == CLIENT_TYPE_PWD){
                
                pwdCommand();
                continue;
            }
            else if(t.type == CLIENT_TYPE_CLEAR){
                clearCommand();
                continue;
            }
            // 服务器命令
            sendn(clientfd, &t, 4 + 4 + t.len);

            if (t.type == CMD_TYPE_PUTS)
            {
                putsCommand(clientfd, &t);
            }
            else if(t.type == CMD_TYPE_GETS)
            {
                getsCommand(clientfd, t.buff);
            }
            else if(t.type == CMD_TYPE_CD)
            {
                // 断开select对该节点的监听
                FD_CLR(clientfd, &temp);
                
                //接收返回的数据
                memset(&t, 0, sizeof(t));
                recvn(clientfd, &t.len, sizeof(int));
                recvn(clientfd, &t.type, sizeof(int));
                recvn(clientfd, t.buff, t.len);

                if(t.type == CMD_TYPE_CD_SUCCESS)
                    strcpy(curpath, t.buff); // 如果是cd命令，需要更新当前路径
                else
                    printf("%s\n", t.buff);
                // 重新添加select对该节点的监听
                FD_SET(clientfd, &temp);

            }
        }
        else if (FD_ISSET(clientfd, &temp))
        {
            // 清除提示符行
            printf("\33[2K\r");

            memset(buf, 0, sizeof(buf));
            ret = recv(clientfd, buf, sizeof(buf), 0);
            if (ret == 0)
            {
                printf("The server has closed.\n");
                goto end;
            }
            printf("%s\n", buf);
            fflush(stdout);
        }
    }

end:
    close(clientfd);
    return 0;
}

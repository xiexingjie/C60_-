#include "client.h"
#include "str_util.h"
#define CHUNK_SIZE 4096

char username[50] = { 0 };

int tcpConnect(const char * ip, unsigned short port)
{
    //1. 创建TCP的客户端套接字
    int clientfd = socket(AF_INET, SOCK_STREAM, 0);
    if(clientfd < 0) {
        perror("socket");
        return -1;
    }
 
    //2. 指定服务器的网络地址
    struct sockaddr_in serveraddr;
    //初始化操作,防止内部有脏数据
    memset(&serveraddr, 0, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;//指定IPv4
    serveraddr.sin_port = htons(port);//指定端口号
    //指定IP地址
    serveraddr.sin_addr.s_addr = inet_addr(ip);

    //3. 发起建立连接的请求
    int ret = connect(clientfd, (const struct sockaddr*)&serveraddr, sizeof(serveraddr));
    if(ret < 0) {
        perror("connect");
        close(clientfd);
        return -1;
    }
    return clientfd;
}

static int userLogin1(int sockfd, train_t *t);
static int userLogin2(int sockfd, train_t *t);

int userLogin(int sockfd)
{
    int ret;
    train_t t;
    memset(&t, 0, sizeof(t));
    userLogin1(sockfd, &t);
    userLogin2(sockfd, &t);
    return 0;
}

static int userLogin1(int sockfd, train_t *pt)
{
    /* printf("userLogin1.\n"); */
    train_t t;
    memset(&t, 0, sizeof(t));
    while(1) {
        printf(USER_NAME); // 提示用户输入账户名
        fflush(stdout);

        int ret = read(STDIN_FILENO, username, sizeof(username));
        username[ret - 1] = '\0';
        t.len = strlen(username);
        t.type = TASK_LOGIN_SECTION1;
        strncpy(t.buff, username, t.len);

        ret = sendn(sockfd, &t, 8 + t.len);
        /* printf("login1 send %d bytes.\n", ret); */

        //接收信息
        memset(&t, 0, sizeof(t));
        ret = recvn(sockfd, &t.len, 4);
        /* printf("length: %d\n", t.len); */
        ret = recvn(sockfd, &t.type, 4);
        if(t.type == TASK_LOGIN_SECTION1_RESP_ERROR) {
            //无效用户名, 重新输入
            printf("user name not exist.\n");
            continue;
        }
        //用户名正确，读取setting
        ret = recvn(sockfd, t.buff, t.len);
        break;
    }
    memcpy(pt, &t, sizeof(t));
    
    return 0;
}

static int userLogin2(int sockfd, train_t * pt)
{
    /* printf("userLogin2.\n"); */
    int ret;
    train_t t;
    memset(&t, 0, sizeof(t));
    while(1) {
        char * passwd = getpass(PASSWORD);
        fflush(stdout);

        // 用于接收转换后的密码
        char output[65] = { 0 };
        // 创建字符数组接收密码盐值结合，注意顺序
        char buff[50 + SALT_LENGTH] = { 0 };

        // 结合密码和盐值
        // pt->buff是服务端传输过来的盐值
        sprintf(buff, "%s%s", passwd, pt->buff);
        // printf("buff: %s\n", buff);
        // printf("pt->buff: %s\n", pt->buff);
        // 获取sha256密码
        sha256(buff, output);

        t.len = strlen(output);
        t.type = TASK_LOGIN_SECTION2;
        strncpy(t.buff, output, t.len);
        ret = sendn(sockfd, &t, 8 + t.len);
        /* printf("userLogin2 send %d bytes.\n", ret); */

        memset(&t, 0, sizeof(t));
        ret = recvn(sockfd, &t.len, 4);
        /* printf("2 length: %d\n", t.len); */
        ret = recvn(sockfd, &t.type, 4);
        if(t.type == TASK_LOGIN_SECTION2_RESP_ERROR) {
            //密码不正确
            printf("sorry, password is not correct.\n");
            continue;
        } else {
            ret = recvn(sockfd, t.buff, t.len);
            printf("Login Success.\n");
            break;
        } 
    }
    return 0;
}

//作用：确定接收len字节的数据
int recvn(int sockfd, void * buff, int len)
{
    int left = len;//还剩下多少个字节需要接收
    char * pbuf = buff;
    int ret = -1;
    while(left > 0) {
        ret = recv(sockfd, pbuf, left, 0);
        if(ret == 0) {
            break;
        } else if(ret < 0) {
            perror("recv");
            return -1;
        }

        left -= ret;
        pbuf += ret;
    }
    //当退出while循环时，left的值等于0
    return len - left;
}

// 生成随机盐值
void generate_salt(char *salt, size_t length, const char * charset) {
    for (size_t i = 0; i < length; i++) {
        salt[i] = charset[rand() % (sizeof(charset) - 1)];
    }
    salt[length] = '\0';
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

// 生成sha256码
void sha256(const char *str, char outputBuffer[65]) {
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

    if (EVP_DigestUpdate(ctx, str, strlen(str)) != 1) {
        fprintf(stderr, "EVP_DigestUpdate failed\n");
        EVP_MD_CTX_free(ctx);
        return;
    }

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

// 生成sha512码
void sha512(const char *str, char outputBuffer[129]) {
    EVP_MD_CTX *ctx = EVP_MD_CTX_new();
    if (ctx == NULL) {
        fprintf(stderr, "EVP_MD_CTX_new failed\n");
        return;
    }

    const EVP_MD *md = EVP_sha512();
    unsigned char hash[EVP_MAX_MD_SIZE];
    unsigned int lengthOfHash = 0;

    if (EVP_DigestInit_ex(ctx, md, NULL) != 1) {
        fprintf(stderr, "EVP_DigestInit_ex failed\n");
        EVP_MD_CTX_free(ctx);
        return;
    }

    if (EVP_DigestUpdate(ctx, str, strlen(str)) != 1) {
        fprintf(stderr, "EVP_DigestUpdate failed\n");
        EVP_MD_CTX_free(ctx);
        return;
    }

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
    outputBuffer[128] = '\0'; // Null-terminate the string
}

int register_user(int sockfd, const char *charset){

    int ret;
    // 创建盐值随机参数
    char salt[SALT_LENGTH + 1] = {0};
    // 生成随机盐值
    generate_salt(salt, SALT_LENGTH, charset);
   
    // 获取账户
    printf(REGISTER_NAME); // 提示用户输入账户名
    fflush(stdout);
    scanf("%s", username);

    // 获取密码
    char *password = getpass(REGISTER_PASSWORD);

    // 用于接收转换后的密码
    char output[65] = { 0 };
    // 创建字符数组接收密码盐值结合，注意顺序
    char buff[50 + SALT_LENGTH] = { 0 };

    // 结合密码和盐值
    sprintf(buff, "%s%s", password, salt);
    // 获取sha256密码
    sha256(buff, output);

    // 创建消息体
    train_t rt;
    memset(&rt, 0, sizeof(rt));
    // 结构体初始化
    rt.type = TAST_REGISTER;
    sprintf(rt.buff, "%s;%s;%s", username, output, salt);
    rt.len = strlen(rt.buff);
    ret = sendn(sockfd, &rt, 4 + 4 + rt.len);

    //接收信息
    memset(&rt, 0, sizeof(rt));
    ret = recvn(sockfd, &rt.len, 4);
    /* printf("length: %d\n", t.len); */
    ret = recvn(sockfd, &rt.type, 4);

    printf("----%d\n", rt.type);
    
    if(rt.type == TAST_REGISTER_RESP_ERROR) {
        // 注册失败
        printf("register failed.\n");
        return -1;
    }

    printf("register successful.\n");
    return 0;

}

//作用: 确定发送len字节的数据
int sendn(int sockfd, const void * buff, int len)
{
    int left = len;
    const char * pbuf = buff;
    int ret = -1;
    while(left > 0) {
        ret = send(sockfd, pbuf, left, 0);
        if(ret < 0) {
            perror("send");
            return -1;
        }

        left -= ret;
        pbuf += ret;
    }
    return len - left;
}

//解析命令
int parseCommand(const char * pinput, int len, train_t * pt)
{
    char * pstrs[10] = {0};
    int cnt = 0;
    splitString(pinput, " ", pstrs, 10, &cnt);
    pt->type = getCommandType(pstrs[0]);
    //暂时限定命令行格式为：
    //1. cmd
    //2. cmd content
    if(cnt > 1) {
        pt->len = strlen(pstrs[1]);
        strncpy(pt->buff, pstrs[1], pt->len);
    }
    return 0;
}

int getCommandType(const char * str)
{
    if(!strcmp(str, "pwd")) 
        return CMD_TYPE_PWD;
    else if(!strcmp(str, "ls"))
        return CMD_TYPE_LS;
    else if(!strcmp(str, "cd"))
        return CMD_TYPE_CD;
    else if(!strcmp(str, "mkdir"))
        return CMD_TYPE_MKDIR;
    else if(!strcmp(str,"rmdir"))
        return CMD_TYPE_RMDIR;
    else if(!strcmp(str, "puts"))
        return CMD_TYPE_PUTS;
    else if(!strcmp(str, "gets"))
        return CMD_TYPE_GETS;
    else if(!strcmp(str, "cls"))
        return CLIENT_TYPE_LS;
    else if(!strcmp(str, "cpwd"))
        return CLIENT_TYPE_PWD;
    else if(!strcmp(str, "clear"))
        return CLIENT_TYPE_CLEAR;
    else
        return CMD_TYPE_NOTCMD;
}

void putsCommand(int sockfd, train_t * pt)
{
    train_t t;
    char filepath[128] = { 0 };
    snprintf(filepath, sizeof(filepath), "%s/%s", CLIENT_DATA, pt->buff);
    printf("filepath: %s\n", filepath);

    // 计算文件的sha256码
    char output[65] = { 0 };
    sha256_file(filepath, output);
    memset(&t, 0, sizeof(t));
    strcpy(t.buff, output);
    t.len = strlen(t.buff);
    t.type = CMD_TYPE_PUTS_SHA256;
    sendn(sockfd, &t, 8 + t.len);
    
    memset(&t, 0, sizeof(t));
    recvn(sockfd, &t.type, 4);
    if(t.type == CMD_TYPE_PUTS_SHA256_EXISTS)
    {
        printf("puts: file already upload.\n");
        return;
    }

    memset(&t, 0, sizeof(t));
    recvn(sockfd, &t.type, sizeof(int));
    if(t.type == CMD_TYPE_PUTS_ERROR)
    {
        printf("puts file faild\n");
        return;
    }
    // ======================================================准备工作完成===========================================================
    // ======================================================准备发送文件===========================================================
    //打开文件
    int fd = open(filepath, O_RDONLY);

    //获取文件大小
    struct stat st;
    memset(&st, 0, sizeof(st));
    // 获取文件的stat信息
    fstat(fd, &st);
    printf("%s: size: %ld\n", filepath, st.st_size);
    //发送文件大小
    sendn(sockfd, &st.st_size, sizeof(st.st_size));

    // 需要接收服务端的文件大小，判断是否之前上传过
    off_t system_filesize;
    recvn(sockfd, &system_filesize, sizeof(system_filesize));
    if(system_filesize == st.st_size)
    {
        //发送完成
        printf("puts: file already upload.\n");
        close(fd);
        return;
    }

    off_t cur = 0;
    char buff[1000] = {0};
    int ret = 0;
    
    ssize_t bytes_sent;
    size_t total_sent = 0;

    while (total_sent < st.st_size) {
        size_t bytes_to_send = CHUNK_SIZE;
        if (total_sent + bytes_to_send > st.st_size) {
            bytes_to_send = st.st_size - total_sent;
        }

        bytes_sent = sendfile(sockfd, fd, &system_filesize, bytes_to_send);
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
}

void getsCommand(int sockfd, char * filename) {
    train_t t;
    memset(&t, 0, sizeof(t));

    recvn(sockfd, &t.type, sizeof(int));
    if(t.type == CMD_TYPE_GETS_SHA256_NOEXISTS)
    {
        // 未找到该文件
        printf( "not find file: %s\n", filename);
        return;
    }
    else if(t.type == CMD_TYPE_GETS_TYPE_ERROR)
    {
        // 该文件是文件夹，不允许下载
        printf( "The file: %s is a folder and cannot be downloaded\n", filename);
        return;
    }    
    // ================================================准备工作就绪，准备接收文件============================================================
    off_t filesize;
    recvn(sockfd, &filesize, sizeof(off_t));

    char filepath[256] = {0};
    snprintf(filepath, sizeof(filepath), "%s/%s", CLIENT_DATA, filename);

    int fd = open(filepath, O_RDWR | O_CREAT, 0644);
    
    // 创建管道
    int pipe_fd[2];
    pipe(pipe_fd);

    off_t bytes_received = 0;
    off_t total_bytes = 0;

    while (total_bytes < filesize) {
        // 计算剩余字节数
        ssize_t remaining_bytes = filesize - total_bytes;
        ssize_t to_read = remaining_bytes < CHUNK_SIZE ? remaining_bytes : CHUNK_SIZE;

        // 从网络接收数据并存入管道
        ssize_t bytes_received = splice(sockfd, NULL, pipe_fd[1], NULL, to_read, SPLICE_F_MOVE);
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
    printf("gets file: %s successful.\n", filename);
}

int create_directory(const char *path) {
    char tmp[256];
    char *p = NULL;
    size_t len;

    // 复制路径到临时缓冲区
    snprintf(tmp, sizeof(tmp), "%s", path);
    len = strlen(tmp);

    // 如果路径以斜杠结尾，则移除它
    if (tmp[len - 1] == '/') {
        tmp[len - 1] = 0;
    }

    // 从头到尾依次创建每一层目录
    for (p = tmp + 1; *p; p++) {
        if (*p == '/') {
            *p = 0; // 暂时终止字符串
            if (mkdir(tmp, 0755) != 0 && errno != EEXIST) {
                // 如果文件存在，不做处理
            }
            *p = '/'; // 恢复字符
        }
    }

    // 最后创建路径的最后一部分
    if (mkdir(tmp, 0755) != 0 && errno != EEXIST) {
        perror("Error creating directory");
        return -1;
    }

    return 0;
}

// =============客户端指令
void pwdCommand(){
    char * curpath = getcwd(NULL, 0);
    printf("%s\n", curpath);
    free(curpath);
}

void clearCommand(){
    // 清空终端信息
    printf("\033[H\033[2J");
}

void lsCommand(){
    // printf("%s\n", user->pwd);
    char * curpath = getcwd(NULL, 0);
    
    char buff[1024] = {0};
    struct dirent * entry;
    // 打开目录流
    DIR* pdir = opendir(curpath);

    while((entry = readdir(pdir)) != NULL)
    {
        if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0)
        {
            // 说明是文件目录下的内容
            strcat(buff, entry->d_name);
            strcat(buff, "\t");
        }
    }
    // 设置字符串终止符
    buff[strlen(buff)] = '\0';
    printf("%s\n", buff);
    free(curpath);
    closedir(pdir);
}
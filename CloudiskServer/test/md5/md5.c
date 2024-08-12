#include <func.h>
#include <openssl/md5.h>

int main(int argc, char ** argv)
{
    int fd = open(argv[1], O_RDONLY);
    ERROR_CHECK(fd, -1, "open");
    char buff[1000] = {0};
    int ret = 0;
    
    MD5_CTX ctx;
    MD5_Init(&ctx);//1. 初始化MD5的结构体
    while(1) {
        memset(buff, 0, sizeof(buff));
        //2. 读取文件中的一段内容
        ret = read(fd, buff, sizeof(buff));
        if(ret == 0) {
            break;
        }
        //3. 对每一段内容调用更新函数
        MD5_Update(&ctx, buff, ret);
    }
    unsigned char md[16] = {0};
    MD5_Final(md, &ctx);//4. 结束
    char result[33] = {0};
    for(int i = 0; i < 16; ++i) {
        //printf("%2x", md[i]);
        char frag[3] = {0};
        sprintf(frag, "%02x", md[i]);
        strcat(result, frag);
    }
    printf("result:%s\n", result);

    return 0;
}

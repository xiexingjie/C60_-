#include "str_util.h"

// 假设max_tokens是数组tokens的最大大小
// 在使用后，记得要释放空间
void splitString(const char * pstrs, const char* delimiter, char *tokens[], int max_tokens, int * pcount) {
    int token_count = 0;
    char *token = strtok((char *)pstrs, delimiter); // 使用delimiter作为分隔符

    while (token != NULL && token_count < max_tokens - 1) { // 保留一个位置给NULL终止符
        char * pstr = (char*)calloc(1, strlen(token) + 1);
        strcpy(pstr, token);
        tokens[token_count] = pstr;//保存申请的堆空间首地址
        token_count++;
        token = strtok(NULL, delimiter); // 继续获取下一个token
    }
    // 添加NULL终止符
    tokens[token_count] = NULL;
    *pcount= token_count;
}

void freeStrs(char * pstrs[], int count)
{
    for(int i = 0; i < count; ++i) {
        free(pstrs[i]);
    }
}

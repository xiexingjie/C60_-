#pragma once
#include <func.h>

//作用： 分割字符串
//@pstr        原字符串
//@delimiter   分隔符
//@ptokens     字符串数组首地址
//@max_tokens  指定最大值
//@pcount      传出参数，获取分割后的字符串个数
void splitString(const char *pstr, const char * delimiter, char *ptokens[], int max_tokens, int * pcount);
void freeStrs(char * pstrs[], int count);

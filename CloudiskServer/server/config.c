#include "config.h"

void readConfig(const char* filename, HashTable * ht)
{
    FILE * fp = fopen(filename, "rw");
    if(fp == NULL) {
        printf("open file %s error.\n", filename);
        return;
    }

    char buff[128] = {0};
    while(fgets(buff, sizeof(buff), fp) != NULL) {
        char * strs[3] = {0};
        int cnt = 0;
        splitString(buff, "=", strs, 3, &cnt);
        /* printf("cnt: %d\n", cnt); */
        /* for(int i = 0; i < cnt; ++i) { */
        /*     printf("strs[%d]: %s\n", i, strs[i]); */
        /* } */
        /* printf("\n"); */
        char * value = (char*)calloc(1, strlen(strs[1]) + 1);
        strcpy(value, strs[1]);
        insert(ht, strs[0], value);
        freeStrs(strs, cnt);
    }

    fclose(fp);
}

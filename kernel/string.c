/************************** 字符串操作函数 *******************************
 * Author：Joker001014
 * 2025.03.13
***********************************************************************/

#include "types.h"
#include "def.h"

// 字符串长度
int
strlen(char *str)
{
    int num = 0;
    while (*str != '\0') {
        num ++;
        str ++;
    }
    return num;
}

// 字符串对比
int
strcmp(char *str1, char *str2)
{
    if(strlen(str1) != strlen(str2)) return 1;
    int i, len = strlen(str1);
    for(i = 0; i < len; i ++) {
        if(str1[i] != str2[i]) return 1;
    }
    return 0;
}
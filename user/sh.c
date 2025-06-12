/***************************** shell 终端 ******************************
 * Author：Joker001014
 * 2025.03.16
***********************************************************************/

#include "types.h"
#include "ulib.h"
#include "syscall.h"

// 定义一些控制字符常量
#define LF 0x0au        // 换行
#define CR 0x0du        // 回车
#define BS 0x08u        // 退格
#define DL 0x7fu        // 删除

// 判断当前行是否为空
int
isEmpty(char *line, int length) {
    int i;
    for(i = 0; i < length; i ++) {
        if(line[i] == 0) break;
        if(line[i] != ' ' && line[i] != '\t') {
            return 0;
        }
    }
    return 1;
}

// 将当前行置为空
void
empty(char *line, int length)
{
    int i;
    for(i = 0; i < length; i ++) {
        line[i] = 0;
    }
}

uint64
main()
{
    char line[256];
    int lineCount = 0;      // 当前输入行的字符数
    printf("Welcome to Moonix!\n");
    printf("$ ");
    while(1) {
        // 从标准输入缓存区获取一个字符
        uint8 c = getc();
        // 字符处理
        switch(c) {
            // 换行/回车
            case LF: case CR: 
                printf("\n");
                // 当前行不为空，则执行指令
                if(!isEmpty(line, 256)) {
                    sys_exec(line);     // 执行指令
                    lineCount = 0;      // 清空当前指令行
                    empty(line, 256);
                }
                printf("$ ");
                break;
            // 删除
            case DL:
                if(lineCount > 0) {
                    putchar(BS);        // 退格
                    putchar(' ');       // 输出空格覆盖
                    putchar(BS);        // 退格
                    line[lineCount-1] = 0;
                    lineCount -= 1;
                }
                break;
            // 其他字符正常输出
            default:
                if(lineCount < 255) {
                    line[lineCount] = c;    // 记录指令
                    lineCount += 1;
                    putchar(c);
                }
                break;
        }
    }
}

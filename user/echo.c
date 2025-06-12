/*********************** 将输入字符输出到屏幕上 ***************************
 * Author：Joker001014
 * 2025.03.16
***********************************************************************/

#include "types.h"
#include "ulib.h"

// 定义一些控制字符常量
#define LF 0x0au        // 换行
#define CR 0x0du        // 回车
#define BS 0x08u        // 退格
#define DL 0x7fu        // 删除

// 将读到的字符输出，其中退格键 DL 的写法来自 xv6
uint64
main()
{
    printf("Welcome to echo!\n");
    int lineCount = 0;      // 当前输入行的字符数
    while(1) {
        // 从标准输入缓存区获取一个字符
        uint8 c = getc();
        // 字符处理
        switch(c) {
            // 换行/回车
            case LF: case CR: 
                lineCount = 0;  // 重置计数
                putchar(LF);
                putchar(CR);
                break;
            // 删除
            case DL:
                // 若有字符输出
                if(lineCount > 0) {
                    putchar(BS);    // 退格
                    putchar(' ');   // 输出空格覆盖
                    putchar(BS);    // 退格
                    lineCount -= 1;
                }
                break;
            // 其他字符正常输出
            default:
                lineCount += 1;
                putchar(c);
                break;
        }
    }
}


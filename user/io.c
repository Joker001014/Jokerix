/************************ U-Mode 下的io操作 *****************************
 * Author：Joker001014
 * 2025.02.26
 * 大都拷贝自内核的 printf.c，OpenSBI 调用被替换为系统调用
***********************************************************************/

#include <stdarg.h>     // 对于参数不定场景，使用 va_list 迭代遍历采参数
#include "types.h"
#include "ulib.h"
#include "syscall.h"

// 提供 16 进制数字字符的映射，供 printint 和 printptr 使用
static char digits[] = "0123456789abcdef";

// 从标准输入缓存区获取一个字符
uint8 getc()
{
    uint8 c;
    // 读取的文件描述符，读取的字节数组，读取的长度
    sys_read(0, &c, 1);
    return c;
}

// 向终端输出一个字符
void putchar(int c)
{
    sys_write(c);
}

/*
    功能：将一个整数格式化为字符串，并输出到控制台
    输入：xx：要打印的整数；
        base：数字的进制，支持 10（十进制）和 16（十六进制）；
        sign：是否为有符号整数（1 表示有符号，0 表示无符号）
*/
static void
printint(int xx, int base, int sign)
{
    char buf[16];
    int i;
    uint x;

    if (sign && (sign = xx < 0))
        x = -xx;
    else
        x = xx;

    i = 0;
    do
    {
        buf[i++] = digits[x % base];
    } while ((x /= base) != 0);

    if (sign)
        buf[i++] = '-';

    while (--i >= 0)
        putchar(buf[i]);
}

/*
    功能：将指针（64 位地址）格式化为十六进制字符串并输出
    输入：x-要打印的指针地址
*/
static void
printptr(uint64 x)
{
    int i;
    putchar('0');
    putchar('x');
    for (i = 0; i < (sizeof(uint64) * 2); i++, x <<= 4)
        putchar(digits[x >> (sizeof(uint64) * 8 - 4)]);
}

/*
    功能：格式化输出到控制台，支持以下格式：
          %d：十进制整数。
          %x：十六进制整数。
          %p：指针。
          %s：字符串。
          %%：输出 % 本
    输入：fmt-格式化字符串；可变参数列表（...）-对应的值
*/
void printf(char *fmt, ...)
{
    va_list ap;
    int i, c;
    char *s;

    if (fmt == 0)
        panic("null fmt");

    va_start(ap, fmt);
    for (i = 0; (c = fmt[i] & 0xff) != 0; i++)
    {
        if (c != '%')
        {
            putchar(c);
            continue;
        }
        c = fmt[++i] & 0xff;
        if (c == 0)
            break;
        switch (c)
        {
        case 'd':
            printint(va_arg(ap, int), 10, 1);
            break;
        case 'x':
            printint(va_arg(ap, int), 16, 1);
            break;
        case 'p':
            printptr(va_arg(ap, uint64));
            break;
        case 's':
            if ((s = va_arg(ap, char *)) == 0)
                s = "(null)";
            for (; *s; s++)
                putchar(*s);
            break;
        case '%':
            putchar('%');
            break;
        default:
            putchar('%');
            putchar(c);
            break;
        }
    }
}

/*
    功能：打印紧急错误信息并冻结系统
    输入：s-错误消息字符串
*/
void panic(char *s)
{
    printf("panic: ");
    printf(s);
    printf("\n");
    sys_exit(1);
}




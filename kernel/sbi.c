/************************ 监管者二进制接口SBI ****************************
 * Author：Joker001014
 * 2025.02.26
 * 对 OpenSBI 提供的服务进行简单的包装
***********************************************************************/

#include "types.h"
#include "def.h"
#include "sbi.h"

// 向终端输出一个字符
void
consolePutchar(usize c)
{
    SBI_ECALL_1(SBI_CONSOLE_PUTCHAR, c);
}

// 从控制台读取一个字符
usize
consoleGetchar()
{
    return SBI_ECALL_0(SBI_CONSOLE_GETCHAR);
}

// 关闭系统
void
shutdown()
{
    SBI_ECALL_0(SBI_SHUTDOWN);
    while(1) {}
}

// 设置定时器
void
setTimer(usize time)
{
    SBI_ECALL_1(SBI_SET_TIMER, time);
}
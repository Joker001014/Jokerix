/************************** 保存上下文信息 ******************************
 * Author：Joker001014
 * 2025.02.26
***********************************************************************/

#ifndef CONTEXT_H
#define CONTEXT_H

#include "types.h"

/* 中断发生前后的程序上下文 */
typedef struct
{
    usize x[32];            /* 32 个通用寄存器 */
    usize sstatus;          /* S-Mode 状态寄存器 */
    usize sepc;             /* 中断处理结束后的返回地址 */
} InterruptContext;

/* 线程切换前后的线程上下文 */
typedef struct {
    usize ra;               /* ra 寄存器，保存线程当前线程函数调用返回地址 */
    usize satp;             /* satp 寄存器，保存线程使用的三级页表基址 */
    usize s[12];            /* 被调用者保存的 12 个函数调用保存的寄存器 */
    InterruptContext ic;    
} ThreadContext;


#endif
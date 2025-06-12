/************************ risc-v CSR寄存器指令 ****************************
 * Author：Joker001014
 * 2025.02.26
***********************************************************************/

#ifndef RISCV_H
#define RISCV_H

#include "types.h"

/*  常用寄存器：
    stvec   -   中断程序入口地址
    status  -   全局中断使能
    sie     -   具体中断使能
    scause  -   中断原因
    sepc    -   中断返回地址
    stval   -   中断辅助信息
*/

// 读取 scause 寄存器，中断原因
// static - 静态函数，只有包含riscv.h的源文件才可以调用函数，且每个源文件都会有一个独立的r_scause()函数副本
// inline - 内联函数，编译器会直接将代码插入到调用程序中，提高性能，减小开销
static inline usize
r_scause() 
{
    usize x;
    // 内嵌汇编，volatile 告诉编译器不要优化代码
    // 读取scause到任意分配的寄存器，%0占位符即表示第一个输出寄存器
    asm volatile("csrr %0, scause"
                    : "=r"(x));     // 将该任意寄存器输出到 x
    return x;
}

// 中断处理函数地址处理方式
#define MODE_DIRECT 0x0     // 中断跳转到 stvec 的 BASE
#define MODE_VECTOR 0x1     // 中断跳转到 stvec 的 BASE + scause*4
// 写 stvec，设置中断处理函数 地址 和 模式
static inline void
w_stvec(usize x)
{
    asm volatile("csrw stvec, %0" : : "r"(x));
}

// 读 sepc，中断返回地址
static inline usize
r_sepc()
{
    usize x;
    asm volatile("csrr %0, sepc" : "=r"(x));
    return x;
}

#define SIE_SEIE (1L << 9)  /* 外部中断 */
#define SIE_STIE (1L << 5)  /* 时钟中断 */
#define SIE_SSIE (1L << 1)  /* 软件中断 */
// 读 sie，具体中断使能状态
static inline usize
r_sie()
{
    usize x;
    asm volatile("csrr %0, sie" : "=r" (x) );
    return x;
}

// 写 sie
static inline void 
w_sie(usize x)
{
    asm volatile("csrw sie, %0" : : "r" (x));
}

#define SSTATUS_SUM (1L << 18)  /* 允许内核访问用户态 */
#define SSTATUS_SPP (1L << 8)   /* 上一个特权模式 */
#define SSTATUS_SPIE (1L << 5)  /* 中断处理发生前的SIE值 */
#define SSTATUS_SIE (1L << 1)   /* 监管者模式中断使能 */
#define SSTATUS_UIE (1L << 0)   /* 用户模式中断使能 */
// 读 sstatus，全局中断状态
static inline usize
r_sstatus()
{
    usize x;
    asm volatile("csrr %0, sstatus" : "=r" (x) );
    return x;
}

// 写 sstatus
static inline void 
w_sstatus(usize x)
{
    asm volatile("csrw sstatus, %0" : : "r" (x));
}

// 读 time，硬件时钟
static inline usize
r_time()
{
    usize x;
    asm volatile("csrr %0, time" : "=r" (x) );
    return x;
}

static inline uint64
r_satp()
{
    uint64 x;
    asm volatile("csrr %0, satp" : "=r" (x) );
    return x;
}

/* 打开异步中断，并等待中断 */
static inline void
enable_and_wfi()
{   
    // csrsi - 控制状态寄存器某个位， 1<<1 - 置位第二位SIE
    // wfi - Wait for Interrupt特殊指令，用于暂停 CPU 直到某个中断发生，CPU进入低功耗状态
    asm volatile("csrsi sstatus, 1 << 1; wfi");
}

/* 关闭异步中断并保存原先的 sstatus */
static inline usize
disable_and_store()
{
    usize x;        // 保存操作后的 sstatus 返回
    // csrrci - CSR read and clear with Immediate，清除SIE位并存储到%0（即x）
    asm volatile("csrrci %0, sstatus, 1 << 1" : "=r" (x) );
    return x;
}

/* 用 flags 的值恢复 sstatus */
static inline void
restore_sstatus(usize flags)
{
    // cars - CSR set with Immediate，用输入变量flags的值设置sstatus寄存器
    asm volatile("csrs sstatus, %0" :: "r"(flags) );
}

#endif
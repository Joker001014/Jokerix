/************************ 监管者二进制接口SBI ****************************
 * Author：Joker001014
 * 2025.02.26
***********************************************************************/

#ifndef SBI_H
#define SBI_H

// SBI 调用号
#define SBI_SET_TIMER               0x0
#define SBI_CONSOLE_PUTCHAR         0x1
#define SBI_CONSOLE_GETCHAR         0x2
#define SBI_CLEAR_IPI               0x3
#define SBI_SEND_IPI                0x4
#define SBI_REMOTE_FENCE_I          0x5
#define SBI_REMOTE_SFENCE_VMA       0x6
#define SBI_REMOTE_SFENCE_VMA_ASID  0x7
#define SBI_SHUTDOWN                0x8

// SBI系统调用（内核态调用ECALL）
// register声明四个寄存器变量，并通过asm与对应的寄存器绑定，然后赋值
// +表示a0是一个输入输出寄存器
// 输入操作数为a1、a2、a7，使用任意动态分配的寄存器
// 修饰寄存器memory，告诉编译器ecall指令可能会修改内存，即不要对内存优化
// a7 寄存器保存系统调用号，a0、a1、a2 和 a3 分别是系统调用的参数。
#define SBI_ECALL(__num, __a0, __a1, __a2)  \
( { \
    register unsigned long a0 asm("a0") = (unsigned long)(__a0);    \
    register unsigned long a1 asm("a1") = (unsigned long)(__a1);    \
    register unsigned long a2 asm("a2") = (unsigned long)(__a2);    \
    register unsigned long a7 asm("a7") = (unsigned long)(__num);   \
    asm volatile("ecall"    \
        : "+r"(a0)  \
        : "r"(a1), "r"(a2), "r"(a7) \
        : "memory");    \
a0;} )

// 不同参数个数宏拓展，没有参数时传递0
#define SBI_ECALL_0(__num) SBI_ECALL(__num, 0, 0, 0)
#define SBI_ECALL_1(__num, __a0) SBI_ECALL(__num, __a0, 0, 0)
#define SBI_ECALL_2(__num, __a0, __a1) SBI_ECALL(__num, __a0, __a1, 0)

#endif


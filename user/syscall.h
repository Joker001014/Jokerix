/************************** U-Mode 系统调用 ****************************
 * Author：Joker001014
 * 2025.03.08
***********************************************************************/

#ifndef SYSCALL_H
#define SYSCALL_H

// 系统调用号定义
typedef enum {
    Read = 63,      // 从标准输入读取字符
    Write = 64,     // 向屏幕输出字符
    Exit = 93,      // 退出当前线程
    Exec = 221,     // 执行程序系统调用
} SyscallId;

// 系统调用宏定义（用户态调用ECALL）
// register声明四个寄存器变量，并通过asm与对应的寄存器绑定，然后赋值
// +表示a0是一个输入输出寄存器
// 输入操作数为a1、a2、a3、a7，使用任意动态分配的寄存器
// 修饰寄存器memory，告诉编译器ecall指令可能会修改内存，即不要对内存优化
// a7 寄存器保存系统调用号，a0、a1、a2 和 a3 分别是系统调用的参数。
#define sys_call(__num, __a0, __a1, __a2, __a3)                          \
({                                                                  \
    register unsigned long a0 asm("a0") = (unsigned long)(__a0);    \
    register unsigned long a1 asm("a1") = (unsigned long)(__a1);    \
    register unsigned long a2 asm("a2") = (unsigned long)(__a2);    \
    register unsigned long a3 asm("a3") = (unsigned long)(__a3);    \
    register unsigned long a7 asm("a7") = (unsigned long)(__num);   \
    asm volatile("ecall"                                            \
                : "+r"(a0)                                          \
                : "r"(a1), "r"(a2), "r"(a3), "r"(a7)                         \
                : "memory");                                        \
    a0;                                                             \
})

// 不同参数个数系统调用宏拓展，没有参数时传递0
#define sys_read(__a0, __a1, __a2) sys_call(Read, __a0, __a1, __a2, 0)
#define sys_write(__a0) sys_call(Write, __a0, 0, 0, 0)
#define sys_exit(__a0) sys_call(Exit, __a0, 0, 0, 0)
#define sys_exec(__a0) sys_call(Exec, __a0, 0, 0, 0)

#endif
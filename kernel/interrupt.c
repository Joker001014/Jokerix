/**************************** 中断处理程序 ******************************
 * Author：Joker001014
 * 2025.02.26
***********************************************************************/

#include "types.h"
#include "def.h"
#include "riscv.h"
#include "context.h"
#include "interrupt.h"
#include "consts.h"
#include "stdin.h"

// 引入中断处理程序汇编，保存和恢复上下文
asm(".include \"kernel/interrupt.S\"");

// 打开 OpenSBI 的 VIRT_PLIC 外部中断响应
void
initExternalInterrupt()
{
    *(uint32 *)(0x0C002080 + KERNEL_MAP_OFFSET) = 1 << 0xa;
    *(uint32 *)(0x0C000028 + KERNEL_MAP_OFFSET) = 0x7U;
    *(uint32 *)(0x0C201000 + KERNEL_MAP_OFFSET) = 0x0U;
}
// 打开 OpenSBI 的 VIRT_UART0 串口设备响应
void
initSerialInterrupt()
{
    *(uint8 *)(0x10000004 + KERNEL_MAP_OFFSET) = 0x0bU;
    *(uint8 *)(0x10000001 + KERNEL_MAP_OFFSET) = 0x01U;
}

// 中断初始化
// __attribute__((aligned(4))) void
void
initInterrupt()
{
    extern void __interrupt();  // 全局中断处理，保存 Context 并跳转到 handleInterrupt() 处
    // 写 stvec 寄存器。设置中断处理程序入口 和 模式
    w_stvec((usize)__interrupt | MODE_DIRECT);  

    // 开启外部中断
    w_sie(r_sie() | SIE_SEIE);

    /* 打开 OpenSBI 的外部中断响应和串口设备响应 */
    initExternalInterrupt();
    initSerialInterrupt();

    printf("==== Init Interrupt ====\n");
}

// 断点中断处理：输出断点信息并跳转到下一条指令
void
breakpoint(InterruptContext *context) 
{
    printf("Breakpoint at %p\n", context->sepc);
    // 跳过断点，继续执行下一条指令，修改中断返回地址指向下一条指令
    context->sepc += 2;     /* 注意：EV64位宽4字节，但指令并非全是4字节， ebreak 指令长度 2 字节 */
}

// 时钟中断处理：设置下一次时钟中断时间
void
supervisorTimer()
{
    extern void tick(); tick();         // 设置下一次时钟中断时间
    extern void tickCPU(); tickCPU();   // 检查当前线程的时间片是否用完
}

// 未知中断处理：直接打印信息并关机
void
fault(InterruptContext *context, usize scause, usize stval)
{
    printf("Unhandled interrupt!\nscause\t= %p\nsepc\t= %p\nstval\t= %p\n",
                scause,
                context->sepc,
                stval
            );
    panic("");
}

// 系统调用中断处理
void
handleSyscall(InterruptContext *context)
{
    context->sepc += 4;     // 跳过 ecall 指令
    extern usize syscall(usize id, usize args[3], InterruptContext *context);
    // 处理系统调用
    usize ret = syscall(
        // 传入 a7 系统调用号
        context->x[17],     
        // 传入 a0、a1、a2 系统调用参数
        (usize[]){context->x[10], context->x[11], context->x[12]},
        context
    );
    context->x[10] = ret;   // 将 a0 寄存器设置为系统调用处理的返回值
}

// 外部中断处理
void external()
{
    // printf("externel interrupt happened!\n Input char is ");
    // // 目前只处理串口中断
    // usize ret = consoleGetchar();   // 从控制台读取一个字符
    // consolePutchar(ret);            // 向终端输出一个字符

    // 将输入的字符放到输ss入缓冲区
    usize ret = consoleGetchar();   // 从控制台获取一个字符
    // 若获取不到字符 ret = -1
    if(ret != -1) {
        char ch = (char)ret;
        // 将字符放到缓冲区
        if(ch == '\r') {
            pushChar('\n');
        } else {
            pushChar(ch);
        }
    }
}

// 中断处理函数，接受interrupt.S传递过来的三个参数 sp, scause, stval
// sp保存上下文向下移动34个usize，所以sp也是一个指向InterruptContext的指针！
void 
handleInterrupt(InterruptContext *context, usize scause, usize stval)
{
    switch(scause) {
        case BREAKPOINT:            // 断点中断
            breakpoint(context);
            break;
        case USER_ENV_CALL:         // U-Mode 系统调用
            handleSyscall(context);
            break;
        case SUPERVISOR_TIMER:      // 时钟中断
            supervisorTimer();
            break;
        case SUPERVISOR_EXTERNAL:   // 外部中断
            external();
            break;
        default:                    // 未知中断
            fault(context, scause, stval);
            break;
    }
}




































/**************************** 时钟中断程序 ******************************
 * Author：Joker001014
 * 2025.02.26
***********************************************************************/

#include "types.h"
#include "riscv.h"
#include "def.h"

static const usize INTERVAL = 100000;   // 时钟中断为 100000 个CPU周期

void setTimerout();

// 初始化时钟中断
void 
initTimer()
{
    // 写 sie 时钟中断使能
    w_sie(SIE_STIE);
    // 写 scause 监管者模式中断使能（因为时钟中断还需打断内核线程）
    w_sstatus(SSTATUS_SIE);
    // 初始化时设置第一次时钟中断
    setTimerout();
}

// 设置下一次时钟中断时间为 当前时间 + INTERVAL个CPU周期
void 
setTimerout()
{
    setTimer(r_time() + INTERVAL); // r_time() 读取硬件时间（kernel/riscv.h）
}

// 触发时钟中断，并记录 TICKS
// static usize TICKS = 0;
void 
tick()
{
    setTimerout();
    // TICKS += 1;
    // if(TICKS % 100 == 0) {
    //     printf("** TICKS = %d **\n", TICKS);    // 每 100 次时钟中断输出 TICKS
    // }
}























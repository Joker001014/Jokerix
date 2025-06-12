/******************************* main *********************************
 * Author：Joker001014
 * 2025.02.26
***********************************************************************/

#include "types.h"
#include "def.h"

// asm(".include \"kernel/entry.asm\"");
asm(".include \"kernel/linkFS.asm\"");

void
testAlloc()
{
    printf("alloc %p\n", allocFrame());
    usize f = allocFrame();
    printf("alloc %p\n", f);
    printf("alloc %p\n", allocFrame());
    printf("dealloc %p\n", f);
    deallocFrame(f);
    printf("alloc %p\n", allocFrame());
    printf("alloc %p\n", allocFrame());
}

void main()
{
    // extern void initInterrupt();    initInterrupt();    // 设置中断处理程序入口 和 模式
    // extern void initTimer();        initTimer();        // 时钟中断初始化
    // extern void initMemory();       initMemory();       // 初始化 页分配 和 动态内存分配
    // extern void mapKernel();        mapKernel();        // 内核重映射，三级页表机制

    // extern void initFs();           initFs();           // 初始化文件系统
    // extern void initThread();       initThread();       // 初始化线程管理
    // extern void runCPU();           runCPU();           // 切换到 idle 调度线程，表示正式由 CPU 进行线程管理和调度
    
    extern void initMemory();       initMemory();       // 初始化 页分配 和 动态内存分配
    extern void initInterrupt();    initInterrupt();    // 设置中断处理程序入口 和 模式
    extern void initFs();           initFs();           // 初始化文件系统
    extern void initThread();       initThread();       // 初始化线程管理
    extern void initTimer();        initTimer();        // 时钟中断初始化
    extern void runCPU();           runCPU();           // 切换到 idle 调度线程，表示正式由 CPU 进行线程管理和调度
 
    while(1) {}
}
/****************************** 线程管理 ********************************
 * Author：Joker001014
 * 2025.03.05
 ***********************************************************************/

#include "types.h"
#include "def.h"
#include "consts.h"
#include "thread.h"
#include "riscv.h"
#include "mapping.h"
#include "elf.h"
#include "fs.h"

/*
 * 构建内核线程的内核栈
 * 输出栈空间的起始地址
 */
usize newKernelStack()
{
    /* 将内核线程的线程栈分配在内核堆中 */
    usize bottom = (usize)kalloc(KERNEL_STACK_SIZE); // 在内核堆上分配内存
    return bottom;
}

/*
 * 该函数用于切换上下文，保存当前函数的上下文，并恢复目标函数的上下文
 * 输入：线程上下文存储的地址（由于目标线程切换前最后是保存在栈上的，所以该地址即栈顶地址）
 * 输出：函数返回时即返回到了新线程的运行位置
 * naked 防止 gcc 在函数执行前后自动插入开场白和结束语，函数调用保存寄存器和栈指针这部分我们自行设计
 * noinline 防止 gcc 将函数内联，有些编译器为了避免跳转、返回，会将函数优化为内联，上下文切换借助了函数调用返回，不应内联
 */
__attribute__((naked, noinline)) void
switchContext(usize *self, usize *target)
{
    asm volatile(".include \"kernel/switch.S\""); // 调用汇编指令切换线程
}

// 线程切换
void switchThread(Thread *self, Thread *target)
{
    switchContext(&self->contextAddr, &target->contextAddr);
}

/*
 * 将线程上下文依次压入栈顶
 * 并返回新的栈顶地址，即线程上下文地址
 */
usize pushContextToStack(ThreadContext self, usize stackTop)
{
    // 分配栈空间 -> 转换指针类型
    ThreadContext *ptr = (ThreadContext *)(stackTop - sizeof(ThreadContext));
    *ptr = self;
    return (usize)ptr;
}

/*
 * 创建新的内核线程上下文，并将线程上下文入栈
 * 借助中断恢复机制进行线程的初始化工作，即从中断恢复结束时即跳转到sepc，就是线程的入口点
 * 输入：线程入口点；内核线程线程栈顶；内核线程页表
 * 输出：线程上下文地址
 */
usize newKernelThreadContext(usize entry, usize kernelStackTop, usize satp)
{
    InterruptContext ic;
    ic.x[2] = kernelStackTop; // 设置sp寄存器为内核栈顶
    ic.sepc = entry;          // 中断返回地址为线程入口点
    ic.sstatus = r_sstatus();
    /* 内核线程，返回后特权级为 S-Mode */
    ic.sstatus |= SSTATUS_SPP;
    /* 开启 新线程 异步中断使能 */
    ic.sstatus |= SSTATUS_SPIE; // 保存中断处理发生前的SIE值
    ic.sstatus &= ~SSTATUS_SIE; // 禁用中断使能，因为下文将借助中断恢复机制
    // ！下文借助中断恢复机制，即将SIE=SPIE，成功设置中断使能
    // 创建新线程
    ThreadContext tc;
    // 借助中断的恢复机制，来初始化新线程的每个寄存器，从 Context 中恢复所有寄存器
    extern void __restore();
    tc.ra = (usize)__restore;
    // 即 switchContext切换完后回收ra、satp、s[12]，栈顶只剩tc.ic，和中断处理程序返回是一样的
    // 所以设置 switchContext 的返回地址为__restore，借助中断恢复机制来恢复所有寄存器
    tc.satp = satp; // 设置页表
    tc.ic = ic;
    return pushContextToStack(tc, kernelStackTop);
}

/*
 * 创建新的用户线程上下文，并将线程上下文入栈
 * 借助中断恢复机制进行线程的初始化工作，即从中断恢复结束时即跳转到sepc，就是线程的入口点
 * 输入：线程入口点；用户线程栈顶；内核线程线程栈顶；内核线程页表
 * 输出：线程上下文地址
 */
usize newUserThreadContext(usize entry, usize ustackTop, usize kstackTop, usize satp)
{
    InterruptContext ic;
    ic.x[2] = ustackTop; // 设置sp寄存器为用户栈顶
    ic.sepc = entry;     // 中断返回地址为线程入口点
    ic.sstatus = r_sstatus();
    // 设置返回后的特权级为 U-Mode
    ic.sstatus &= ~SSTATUS_SPP;
    // 异步中断使能
    ic.sstatus |= SSTATUS_SPIE;
    ic.sstatus &= ~SSTATUS_SIE;
    // 创新新线程上下文
    ThreadContext tc;
    // 借助中断的恢复机制，来初始化新线程的每个寄存器，从 Context 中恢复所有寄存器
    extern void __restore();
    tc.ra = (usize)__restore;
    tc.satp = satp; // 设置页表
    tc.ic = ic;
    return pushContextToStack(tc, kstackTop);
}

// 为线程传入初始化参数
// 我们利用中断恢复过程来填充寄存器，所以将参数保存到ic
void appendArguments(Thread *thread, usize args[8])
{
    ThreadContext *ptr = (ThreadContext *)thread->contextAddr;
    ptr->ic.x[10] = args[0];
    ptr->ic.x[11] = args[1];
    ptr->ic.x[12] = args[2];
    ptr->ic.x[13] = args[3];
    ptr->ic.x[14] = args[4];
    ptr->ic.x[15] = args[5];
    ptr->ic.x[16] = args[6];
    ptr->ic.x[17] = args[7];
}

/*
 * 创建新的内核线程
 * 创建内核栈，创建上下文
 */
Thread
newKernelThread(usize entry)
{
    // 构建内核线程的内核栈
    usize stackBottom = newKernelStack();
    // 创建新的内核线程上下文
    usize contextAddr = newKernelThreadContext(
        entry,                           // 线程入口点
        stackBottom + KERNEL_STACK_SIZE, // 内核栈顶
        r_satp()                         // 创建的内核线程与启动线程同属于一个进程，所以直接获取satp赋值
    );
    Thread t = {// 线程上下文地址， 线程栈底地址
                contextAddr, stackBottom};
    return t;
}

// 映射一个未被分配物理内存的段
void mapFramedSegment(Mapping m, Segment segment)
{
    usize startVpn = segment.startVaddr / PAGE_SIZE;       // 起始虚拟页
    usize endVpn = (segment.endVaddr - 1) / PAGE_SIZE + 1; // 结束虚拟页
    usize vpn;
    for (vpn = startVpn; vpn < endVpn; vpn++)
    {
        // 根据给定的虚拟页号寻找三级页表项
        PageTableEntry *entry = findEntry(m, vpn);
        if (*entry != 0)
        {
            panic("Virtual address already mapped!\n");
        }
        // 分配一个物理页并设置标志位
        *entry = (allocFrame() >> 2) | segment.flags | VALID;
    }
}

/*
 * 创建新的用户线程
 * 加载ELF用户程序，创建用户栈，创建内核栈，创建上下文
 */
Thread
newUserThread(char *data)
{
    // 解析 ELF 文件，完成内核和可执行程序各个段的映射,data为指向 ELF 文件的首字节的指针
    Mapping m = newUserMapping(data);
    usize ustackBottom = USER_STACK_OFFSET;                // 用户栈底
    usize ustackTop = USER_STACK_OFFSET + USER_STACK_SIZE; // 用户栈顶
    // 将用户栈映射到未被分配物理内存的段(添加到页表上、分配物理页映射)
    Segment s = {ustackBottom, ustackTop, 1L | USER | READABLE | WRITABLE};
    mapFramedSegment(m, s);

    // 构建用户线程的内核栈
    usize kstack = newKernelStack();
    usize entryAddr = ((ElfHeader *)data)->entry;
    Process p = {m.rootPpn | (8L << 60)}; // 构造进程（根页表地址，mode为sv39）
    // 创建新的用户线程上下文
    usize context = newUserThreadContext(
        entryAddr,                  // 线程入口点
        ustackTop,                  // 用户线程栈顶
        kstack + KERNEL_STACK_SIZE, // 内核线程线程栈顶
        p.satp                      // 内核线程页表
    );
    Thread t = {context, kstack, p}; // 线程上下文地址，线程栈底地址，所属进程
    return t;
}

// 测试函数，作为新线程入口点
void tempThreadFunc(Thread *from, Thread *current, usize c)
{
    printf("The char passed by is ");
    consolePutchar(c); // 向终端输出字符
    consolePutchar('\n');
    printf("Hello world from tempThread!\n");
    switchThread(current, from); // 手动线程切换回去
}

// 线程测试函数，作为入口点
void helloThread(usize arg)
{
    printf("Begin of thread %d\n", arg);
    int i;
    // 将传入的参数输出800遍
    for (i = 0; i < 800; i++)
    {
        printf("%d", arg);
    }
    printf("\nEnd of thread %d\n", arg);
    exitFromCPU(0); // 退出
    while (1)
    {
    }
}

// 构造空结构Thread表示当前启动线程
// 在调用 switchThread() 函数时，会将当前线程的上下文信息保存到这个线程结构中
Thread
newBootThread()
{
    // Thread t = {0L, 0L};
    /*
     * 该函数调用时，应当处于启动线程内
     * 只需要创建一个空结构，在进行切换时即可自动填充
     */
    Thread t;
    t.contextAddr = 0L;
    t.kstack = 0L;
    t.wait = -1;
    return t;
}

// 创建线程池
ThreadPool
newThreadPool(Scheduler scheduler)
{
    ThreadPool pool;
    pool.scheduler = scheduler;
    return pool;
}

// 初始化线程
void initThread()
{
    // 1.创建调度函数实现
    Scheduler s = {
        schedulerInit,
        schedulerPush,
        schedulerPop,
        schedulerTick,
        schedulerExit};
    s.init(); // 初始化调度器
    // 2.创建线程池
    ThreadPool pool = newThreadPool(s);
    // 3.构建idle调度线程
    Thread idle = newKernelThread((usize)idleMain);
    // 4.初始化CPU调度器
    initCPU(idle, pool);
    // 5.构造线程并添加到CPU中
    // usize i;
    // for (i = 0; i < 5; i++)
    // {
    //     Thread t = newKernelThread((usize)helloThread); // 构造新内核线程
    //     usize args[8];
    //     args[0] = i;
    //     appendArguments(&t, args); // 为线程传入初始化参数
    //     // 6.启动
    //     addToCPU(t); // 将线程添加到调度队列中
    // }

    // 从文件系统中读取 elf 文件
    Inode *helloInode = lookup(0, "/bin/sh");     // 查找文件inode
    char *buf = kalloc(helloInode->size);           // 分配内存
    readall(helloInode, buf);                       // 读取文件所有数据到buf
    Thread t = newUserThread(buf);                  // 创建新的用户线程
    kfree(buf);                                     // 释放内存
    addToCPU(t);                                    // 添加到线程池
    printf("***** init thread *****\n");
}
// 测试线程切换 5.4
// void
// initThread()
// {
//     // 构建新的启动线程
//     Thread bootThread = newBootThread();
//     // 构建新内核线程，入口点为测试函数
//     Thread tempThread = newKernelThread((usize)tempThreadFunc);
//     usize args[8];
//     args[0] = (usize)&bootThread;
//     args[1] = (usize)&tempThread;
//     args[2] = (long)'M';
//     // 新内核线程参数初始化，将参数传入到tc.ic，switchContext恢复完后会借助中断初始化恢复寄存器，实现传参
//     appendArguments(&tempThread, args);
//     switchThread(&bootThread, &tempThread); // 线程切换，即线程栈和上下文切换
//     printf("I'm back from tempThread!\n");
// }

// 遍历线程池，寻找未被使用的tid
int allocTid(ThreadPool *pool)
{
    int i;
    for (i = 0; i < MAX_THREAD; i++)
    {
        if (!pool->threads[i].occupied)
            return i;
    }
    panic("Alloc tid failed!\n");
    return -1;
}

// 将线程添加到线程池中
void addToPool(ThreadPool *pool, Thread thread)
{
    int tid = allocTid(pool); // 遍历线程池，寻找未使用tid
    // 配置线程信息
    pool->threads[tid].status = Ready;  // 就绪
    pool->threads[tid].occupied = 1;    // 占用
    pool->threads[tid].thread = thread; // 线程上下文地址和栈底地址
    pool->scheduler.push(tid);          // 将线程加入参与调度
}

// 向线程池获取一个可以运行的线程，若没有返回-1
RunningThread
acquireFromPool(ThreadPool *pool)
{
    int tid = pool->scheduler.pop(); // 从就绪线程中获取一个可运行线程
    RunningThread rt;
    rt.tid = tid;
    if (tid != -1)
    {
        ThreadInfo *ti = &pool->threads[tid]; // 从线程池取出线程
        // 修改取出线程在线程池的状态（上行代码用&引用传入的）
        ti->status = Running; // 由于调用该函数的下一步就要直接切换到这个线程，所以在线程池中直接标记为 Running 状态
        ti->tid = tid;        // 线程ID（因为将线程添加到线程池中时没用设置ThreadInfo.tid，所以这里初始化）
        rt.thread = ti->thread;
    }
    return rt;
}

// 修改线程池内的线程信息：在一个线程停止运行，切换回调度线程后调用
// 线程停止运行有两种情况
//      一种是线程运行结束
//      一种是还没有运行完，但是时间片用尽，这种情况就需要重新将线程加入调度器
void retrieveToPool(ThreadPool *pool, RunningThread rt)
{
    int tid = rt.tid;
    // 若线程不被占用了，即线程运行结束
    if (!pool->threads[tid].occupied)
    {
        // 表明刚刚这个线程退出了，回收栈空间（传入栈底地址，根据HEAP维护的二叉树，即可知道回收多大空间）
        kfree((void *)pool->threads[tid].thread.kstack);
        return;
    }
    // 线程时间片用完，重新加入调度器
    ThreadInfo *ti = &pool->threads[tid];
    ti->thread = rt.thread; // 更新线程上下文、栈地址
    if (ti->status == Running)
    {
        ti->status = Ready;        // 更新线程状态
        pool->scheduler.push(tid); // 加入线程调度
    }
}

// 对调度器的 tick() 函数包装，用于查看当前正在运行的线程是否需要切换
int tickPool(ThreadPool *pool)
{
    // 提醒调度算法当前线程又运行了一个 tick，返回的 int 表示调度算法认为当前线程是否需要被切换出去
    return pool->scheduler.tick();
}

// 线程退出，释放该 tid 线程信息的占用位，并且通知调度器让这个 tid 不再参与调度
void exitFromPool(ThreadPool *pool, int tid)
{
    pool->threads[tid].occupied = 0; // 清除占用标志
    pool->scheduler.exit(tid);       // 告诉调度算法线程已经结束
}

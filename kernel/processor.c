/**************************** 调度器线程 ********************************
 * Author：Joker001014
 * 2025.03.06
***********************************************************************/

#include "types.h"
#include "def.h"
#include "consts.h"
#include "thread.h"
#include "riscv.h"
#include "fs.h"

// 全局唯一的 Processor 调度线程实例
static Processor CPU;

// 对CPU（调度线程）初始化
// 使用 idle 线程和 pool 线程池来对 CPU 进行初始化
// 参数 pool 主要就是为了指定这个 Processor 所使用的调度算法
void
initCPU(Thread idle, ThreadPool pool)
{
    CPU.idle = idle;    // 调度线程
    CPU.pool = pool;    // 线程池
    CPU.occupied = 0;   // 当前没有线程在运行
}

// 将线程添加到CPU管理的线程池中（对 addToPool() 进行包装）
void
addToCPU(Thread thread)
{
    addToPool(&CPU.pool, thread);
}

// 线程主动退出，通知 CPU 这个线程运行结束
// CPU 会通知线程池释放资源，并切换到 idle 线程进行下一步调度
// 输入：退出代码code
void
exitFromCPU(usize code)
{
    disable_and_store();            // 关闭异步中断
    int tid = CPU.current.tid;      // 当前运行线程tid
    exitFromPool(&CPU.pool, tid);   // 清除线程池中占用标记，告诉调度算法线程已经结束

    // 如果有线程在等待其退出，则将其唤醒
    if(CPU.current.thread.wait != -1) {
        wakeupCPU(CPU.current.thread.wait);
    }

    printf("Thread %d exited, exit code = %d\n", tid, code);
    switchThread(&CPU.current.thread, &CPU.idle);   // 切换到调度器线程
}

// 切换到 idle 线程，表示正式由 CPU 进行线程管理和调度，这个函数通常在启动线程中调用
// 由于启动线程被构造为一个局部变量，我们再也无法切换回启动线程，相当于操作系统的初始化工作已经结束
void
runCPU()
{   
    // Thread boot = {0L, 0L};         // 启动线程
    /*
     * 在启动线程的最后调用
     * 从启动线程切换进 idle，boot 线程信息丢失，不会再回来
     */
    Thread boot;
    boot.contextAddr = 0;
    boot.kstack = 0;
    boot.wait = -1;
    switchThread(&boot, &CPU.idle); // 从启动线程切换进 idle，boot 线程信息丢失，不会再回来
}

// 当前线程主动放弃 CPU，并进入休眠状态
void
yieldCPU()
{
    if(CPU.occupied) {
        usize flags = disable_and_store();          // 关闭异步中断

        int tid = CPU.current.tid;                      // 当前线程PID
        ThreadInfo *ti = &CPU.pool.threads[tid];        // 从线程池获取该线程
        ti->status = Sleeping;                          // 睡眠
        switchThread(&CPU.current.thread, &CPU.idle);   // 切换到idle调度线程

        restore_sstatus(flags);                     // 恢复中断
    }
}

// 将某个线程唤醒，将其参与调度
void
wakeupCPU(int tid)
{
    ThreadInfo *ti = &CPU.pool.threads[tid];    // 获取线程
    ti->status = Ready;                         // 唤醒
    schedulerPush(tid);                         // 加入线程调度
}

// 线程调度的入口点函数，是调度线程最核心的函数
void
idleMain()
{
    // 进入 idle 时禁用异步中断
    disable_and_store();
    while(1) {
        // 向线程池获取一个可以运行的线程
        RunningThread rt = acquireFromPool(&CPU.pool);
        if(rt.tid != -1) {
            // 有线程可以运行
            CPU.current = rt;       // 设置调度器当前线程
            CPU.occupied = 1;       // 标志线程正在运行
            // printf("\n>>>> will switch_to thread %d in idle_main!\n", CPU.current.tid);
            // 从调度器线程 切换到 当前线程
            switchThread(&CPU.idle, &CPU.current.thread);  

            // 切换回 idle 线程处
            // printf("<<<< switch_back to idle in idle_main!\n");
            CPU.occupied = 0;       // 标记当前没有线程正在运行
            // 修改线程池内的线程信息：在一个线程停止运行，切换回调度线程后调用
            retrieveToPool(&CPU.pool, CPU.current);
        } else {
            // 无可运行线程，短暂开启异步中断并处理
            enable_and_wfi();
            disable_and_store();
        }
    }
}

// 在时钟中断时被调用，每当时钟中断发生时，如果当前有正在运行的线程，
// 都会检查一下当前线程的时间片是否用完，如果用完了就需要切换到调度线程
void
tickCPU()
{
    // 判断当前是否有正在运行线程（不是 idle）
    if(CPU.occupied) {
        // 当前线程运行时间片是否耗尽
        if(tickPool(&CPU.pool)) {
            // 关闭中断
            usize flags = disable_and_store();
            // 切换到 idle 调度器线程
            switchThread(&CPU.current.thread, &CPU.idle);

            // 某个时刻再切回此线程时从这里开始
            restore_sstatus(flags);
        }
    }
}

/*
 * 执行一个用户进程
 * path 为可执行文件在文件系统的路径, hostTid 为需要暂停的线程的 tid
 */
int
executeCPU(char *path, int hostTid)
{
    Inode *res = lookup(0, path);   // 查找文件inode
    if(res == 0) {
        printf("Command not found!\n");
        return 0;
    }
    char *buf = kalloc(res->size);  // 分配内存
    readall(res, buf);              // 读取文件所有数据到buf
    Thread t = newUserThread(buf);  // 创建新的用户线程
    t.wait = hostTid;               // 记录等待其退出的进程tid
    kfree(buf);                     // 释放内存
    addToCPU(t);                    // 添加到线程池中
    return 1;
}

// 获取当前正在运行的线程TID
int
getCurrentTid()
{
    return CPU.current.tid;
}

// 获取当前正在运行的线程
Thread
*getCurrentThread()
{
    return &CPU.current.thread;
}



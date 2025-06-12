/****************************** 线程管理 ********************************
 * Author：Joker001014
 * 2025.03.05
***********************************************************************/

#ifndef THREAD_H
#define THREAD_H

#include "types.h"
#include "consts.h"
#include "context.h"

// 进程结构体，为资源分配的最小单位
// 保存线程共享资源
typedef struct {
    // 页表寄存器
    usize satp;
} Process;

// 线程结构体
typedef struct {
    usize contextAddr;  /* 线程上下文存储的地址 */
    usize kstack;       /* 线程栈底地址 */
    Process process;    /* 所属进程 */
    int wait;           /* 等待其退出的Tid,当没有线程等待时，wait 被赋值为-1 */
} Thread;

/* 线程状态 */
typedef enum {
    Ready,      // 就绪
    Running,    // 运行
    Sleeping,   // 休眠
    Exited      // 退出
} Status;

// 调度器算法实现（函数指针）
typedef struct {
    
    void    (* init)(void);     // 初始化调度器
    void    (* push)(int);      // 将一个线程加入线程调度
    int     (* pop) (void);     // 从就绪线程中选择一个运行，如果没有可运行的线程则返回 -1
    int     (* tick)(void);     // 提醒调度算法当前线程又运行了一个 tick，返回的 int 表示调度算法认为当前线程是否需要被切换出去
    void    (* exit)(int);      // 告诉调度算法某个线程已经结束
} Scheduler;

/* 线程池中的线程信息槽 */
typedef struct {
    Status status;      // 线程状态
    int tid;            // 线程ID
    int occupied;       // 该槽位是否被占用
    Thread thread;      // 线程
} ThreadInfo;

// 线程池
typedef struct {
    ThreadInfo threads[MAX_THREAD];
    Scheduler scheduler;
} ThreadPool;

// 正在运行的线程
typedef struct {
    int tid;
    Thread thread;
} RunningThread;

// 调度线程参与调度所需要的所有信息
typedef struct {
    ThreadPool pool;        // 线程池
    Thread idle;            // 调度线程
    RunningThread current;  // 当前运行线程信息
    int occupied;           // 当前是否有线程（除了调度线程）正在运行
} Processor;

/* 线程相关函数 */
void switchThread(Thread *self, Thread *target);
Thread newUserThread(char *data);
int allocFd(Thread *thread);
void deallocFd(Thread *thread, int fd);

/* 线程池相关函数 */
ThreadPool newThreadPool(Scheduler scheduler);
void addToPool(ThreadPool *pool, Thread thread);
RunningThread acquireFromPool(ThreadPool *pool);
void retrieveToPool(ThreadPool *pool, RunningThread rt);
int tickPool(ThreadPool *pool);
void exitFromPool(ThreadPool *pool, int tid);

/* Processor 相关函数 */
void initCPU(Thread idle, ThreadPool pool);
void addToCPU(Thread thread);
void idleMain();
void tickCPU();
void exitFromCPU(usize code);
void runCPU();
void yieldCPU();
void wakeupCPU(int tid);
int executeCPU(char *path, int hostTid);
int getCurrentTid();
Thread *getCurrentThread();

/* 调度器相关函数 */
void schedulerInit();
void schedulerPush(int tid);
int  schedulerPop();
int  schedulerTick();
void schedulerExit(int tid);



#endif


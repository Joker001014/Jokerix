/*************************** RR轮转调度算法 *****************************
 * Author：Joker001014
 * 2025.03.06
 * 实现 thread.h 中定义的 Scheduler 结构体中的五个调度器算法
***********************************************************************/

#include "types.h"
#include "def.h"
#include "thread.h"

// 双向环形链表来实现队列,队列元素如下
// 链表的节点按照 tid + 1 都存放在数组中，其中下标 0 处为 Dummy Head，用于快速找到队列头
typedef struct
{
    int valid;      // 标记线程是否有效
    usize time;     // 线程剩余时间片
    int prev;       // 前一个线程tid
    int next;       // 后一个线程tid
} RRInfo;

// 调度器信息结构体
struct
{
    RRInfo threads[MAX_THREAD + 1]; // 优先级调度队列（由于 0 号位有个 Dummy Head，所以 threads 数组的长度为 MAX_THREAD + 1）
    usize maxTime;                  // 最大时间片
    int current;                    // 当前正在运行的tid
} rrScheduler;

// 初始化调度器
void
schedulerInit()
{
    rrScheduler.maxTime = 1;        // 设置最大时间片为1
    rrScheduler.current = 0;        // 当前没有线程运行，设置当前线程为0
    /* 第 0 个位置为 Dummy head，用于快速找到链表头和尾 */
    RRInfo ri = {0, 0L, 0, 0};      // 初始化一个无效的线程信息结构
    rrScheduler.threads[0] = ri;
}

// 将一个线程加入线程调度，即加入调度队列尾部
void
schedulerPush(int tid)
{
    tid += 1;       // 调整索引
    if(tid + 1 > MAX_THREAD + 1) {
        panic("Cannot push to scheduler!\n");
    }
    // 若线程没有时间片，初始化为最大时间片
    if(rrScheduler.threads[tid].time == 0) {
        rrScheduler.threads[tid].time = rrScheduler.maxTime;
    }
    // 获取但前队列尾部
    int prev = rrScheduler.threads[0].prev;
    // 将线程加入队列尾部
    rrScheduler.threads[tid].valid = 1;     // 标记线程有效
    rrScheduler.threads[prev].next = tid;   // 尾部next指向当前线程
    rrScheduler.threads[tid].prev = prev;   // 当前线程prev指向尾部线程
    rrScheduler.threads[0].prev = tid;      // 头部prev指向当前线程
    rrScheduler.threads[tid].next = 0;      // 当前线程next指向头部
}

// 从就绪线程中选择一个运行，如果没有可运行的线程则返回 -1
int
schedulerPop()
{
    // 获取队列一个有效线程
    int ret = rrScheduler.threads[0].next;  
    if(ret != 0) {
        // 若有可用线程，则从队列头部弹出
        int next = rrScheduler.threads[ret].next;   // 获取该线程的下一个线程
        int prev = rrScheduler.threads[ret].prev;   // 获取该线程的上一个线程
        rrScheduler.threads[next].prev = prev;      // 更新下一个线程的prev
        rrScheduler.threads[prev].next = next;      // 更新上一个线程的next
        rrScheduler.threads[ret].prev = 0;          // 清空当前线程的prev
        rrScheduler.threads[ret].next = 0;          // 清空当前线程的next
        rrScheduler.threads[ret].valid = 0;         // 标记当前线程为无效
        rrScheduler.current = ret;                  // 设置调度器当前线程为弹出线程
    }
    return ret-1;   // 调整索引
}

// 提醒调度算法当前线程又运行了一个 tick
// 输出：1-表示调度算法认为当前线程需要被切换出去，0-不需要切换出去
int
schedulerTick()
{
    int tid = rrScheduler.current;  // 获取当前线程tid
    if(tid != 0) {
        // 当前线程有效
        rrScheduler.threads[tid].time -= 1;     // 当前线程时间片-1
        if(rrScheduler.threads[tid].time == 0) {    
            return 1;       // 时间片用尽则切换出去
        } else {
            return 0;       // 否则不切换
        }
    }
    return 1;   // 如果当前线程也进行切换
}

// 告诉调度算法某个线程已经结束
void
schedulerExit(int tid)
{
    tid += 1;   // 调整索引
    // 判断结束的线程是否为当前正在运行的线程
    if(rrScheduler.current == tid) {
        rrScheduler.current = 0;    // 将当前线程设置为0，表示没有线程在运行
    }
}
/******************************* 条件变量 ******************************
 * Author：Joker001014
 * 2025.03.16
 * 线程自动放弃 CPU，等待条件满足再被唤醒的机制
***********************************************************************/

#include "condition.h"
#include "thread.h"

// 将当前线程加入到等待队列中
void
waitCondition(Condvar *self)
{
    pushBack(&self->waitQueue, getCurrentTid());    // 插入等待队列
    yieldCPU();     // 主动放弃 CPU，并进入休眠状态
}

// 从等待队列中唤醒一个线程
void
notifyCondition(Condvar *self)
{
    // 判断等待队列是否为空
    if(!isEmpty(&self->waitQueue)) {
        int tid = (int)popFront(&self->waitQueue);  // 从等待队列获取一个线程
        wakeupCPU(tid);     // 唤醒线程，参与调度
    }
}

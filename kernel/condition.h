/******************************* 条件变量 ******************************
 * Author：Joker001014
 * 2025.03.16
 * 线程自动放弃 CPU，等待条件满足再被唤醒的机制
***********************************************************************/

#ifndef _CONDITION_H
#define _CONDITION_H

#include "queue.h"

// 条件变量，内部为等待该条件满足的等待线程队列
typedef struct {
    Queue waitQueue;
} Condvar;

void waitCondition(Condvar *self);
void notifyCondition(Condvar *self);

#endif
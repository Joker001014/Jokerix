/**************************** 链表实现队列 ******************************
 * Author：Joker001014
 * 2025.03.16
***********************************************************************/

#include "types.h"
#include "queue.h"
#include "def.h"

// 向队尾插入新节点
void
pushBack(Queue *q, usize data)
{
    // 分配一个节点内存
    Node *n = kalloc(sizeof(Node));
    n->item = data;
    if(q->head == q->tail && q->head == 0) {
        // 队列为空
        q->head = n;    // 头尾节点均为当前节点
        q->tail = n;
    } else {
        // 队列不为空
        q->tail->next = n;  // 插入尾部
        q->tail = n;        // 重新设置队尾
    }
}

// 移除队列头节点（默认队列不为空）
usize
popFront(Queue *q)
{
    Node *n = q->head;      // 获取头节点
    usize ret = n->item;    // 返回值为头节点值
    if(q->head == q->tail) {
        // 队列只有一个元素
        q->head = 0;
        q->tail = 0;
    } else {
        // 队列元素大于1
        q->head = q->head->next;    // 设置新的头节点
    }
    kfree(n);       // 回收内存
    return ret;
}

// 判断队列是否为空
// 返回：1-空，0-不为空
int
isEmpty(Queue *q)
{
    if(q->head == q->tail && q->head == 0) {
        return 1;
    } else {
        return 0;
    }
}



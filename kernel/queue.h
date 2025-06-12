/**************************** 链表实现队列 ******************************
 * Author：Joker001014
 * 2025.03.16
***********************************************************************/

#ifndef _QUEUE_H
#define _QUEUE_H

#include "types.h"

// 队列节点
typedef struct n {
    usize item;     // 节点值
    struct n *next; // 下一个节点
} Node;

// 队列结构
typedef struct {
    Node *head;     // 队首
    Node *tail;     // 队尾
} Queue;

void pushBack(Queue *q, usize data);
usize popFront(Queue *q);
int isEmpty(Queue *q);

#endif
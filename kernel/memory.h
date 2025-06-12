/************************* 内存页的分配和回收 ****************************
 * Author：Joker001014
 * 2025.03.01
***********************************************************************/

#ifndef MEMORY_H
#define MRMORY_H

#include "types.h"

/* 具体的页帧分配/回收算法实现结构体 */
typedef struct
{
    usize (*alloc)(void);           // 分配页帧的函数指针
    void (*dealloc)(usize index);   // 回收页帧的函数指针
} Allocator;

/* 页帧分配/回收管理器 */
typedef struct
{
    usize startPpn;         /* 可用空间的起始 */
    Allocator allocator;    /* 具体的分配/回收实现算法 */
} FrameAllocator;


#endif
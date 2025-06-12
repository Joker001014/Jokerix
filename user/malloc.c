/************************ U-Mode动态内存分配 ****************************
 * Author：Joker001014
 * 2025.03.08
 * 使用 Buddy System Allocation 算法分配
***********************************************************************/

#include "types.h"
#include "ulib.h"

/* 动态内存分配相关常量 */
#define USER_HEAP_SIZE      0x1000          /* 堆空间大小 4K */
#define MIN_BLOCK_SIZE      0x20            /* 最小分配的内存块大小 32bytes */
#define HEAP_BLOCK_NUM      0x80            /* 管理的总块数 96 */
#define BUDDY_NODE_NUM      0xff            /* 二叉树节点个数 */

#define LEFT_LEAF(index) ((index) * 2 + 1)
#define RIGHT_LEAF(index) ((index) * 2 + 2)
#define PARENT(index) ( ((index) + 1) / 2 - 1)

#define IS_POWER_OF_2(x) (!((x)&((x)-1)))
#define MAX(a, b) ((a) > (b) ? (a) : (b))

static uint8 HEAP[USER_HEAP_SIZE];          /*  用于分配的堆空间，4 KBytes */

/* 
 * Buddy System Allocation 的具体实现
 * 使用一棵数组形式的完全二叉数来监控内存
 */
struct
{
    uint32 size;                    /* 管理的总块数 */
    uint32 longest[BUDDY_NODE_NUM]; /* 每个节点表示范围内空闲块个数 */
} buddyTree;

// 二叉树初始化
void
buddyInit(int size)
{
    buddyTree.size = size;
    uint32 nodeSize = size << 1;
    int i;
    /* 初始化每个节点，此时每一块都是空闲的 */
    for(i = 0; i < (size << 1) - 1; i ++) {
        if(IS_POWER_OF_2(i+1)) {
            nodeSize /= 2;
        }
        buddyTree.longest[i] = nodeSize;
    }
}

// 初始化堆空间
void
initHeap()
{
    buddyInit(HEAP_BLOCK_NUM);
}

/*
 * 获得大于等于 size 的最小的 2 的幂级数
 * 算法来自于 Java 的 Hashmap
 */
uint32
fixSize(uint32 size)
{
    uint32 n = size - 1;
    n |= n >> 1;
    n |= n >> 2;
    n |= n >> 4;
    n |= n >> 8;
    n |= n >> 16;
    return n + 1;
}

/* 
 * 分配 size 大小的块(单位为MIN_BLOCK_SIZE)，通过二叉树进行管理
 * 返回空闲块的第一块在堆上的偏移（0~HEAP_BLOCK_NUM），单位为MIN_BLOCK_SIZE
 */
uint32
buddyAlloc(uint32 size)
{
    uint32 index = 0;
    uint32 nodeSize;
    uint32 offset;

    if(size <= 0) size = 1;
    else if(!IS_POWER_OF_2(size)) size = fixSize(size);

    /* 一共也没有那么多空闲块 */
    if(buddyTree.longest[0] < size) {
        return -1;
    }
    
    /* 寻找大小最符合的节点 */
    for(nodeSize = buddyTree.size; nodeSize != size; nodeSize /= 2) {
        uint32 left = buddyTree.longest[LEFT_LEAF(index)];
        uint32 right = buddyTree.longest[RIGHT_LEAF(index)];
        /* 优先选择最小的且满足条件的分叉，小块优先，尽量保留大块 */
        if(left <= right) {
            if(left >= size) index = LEFT_LEAF(index);
            else index = RIGHT_LEAF(index);
        } else {
            if(right >= size) index = RIGHT_LEAF(index);
            else index = LEFT_LEAF(index);
        }
    }

    /* 标记为占用 */
    buddyTree.longest[index] = 0;
    /* 获得这一段空闲块的第一块在堆上的偏移 */
    offset = (index + 1) * nodeSize - buddyTree.size;

    /* 向上修改父节点的值 */
    while(index) {
        index = PARENT(index);
        buddyTree.longest[index] = 
            MAX(buddyTree.longest[LEFT_LEAF(index)], buddyTree.longest[RIGHT_LEAF(index)]);
    }

    return offset;
}

/* 
 * 在堆上分配内存,利用 buddyAlloc 进行分配
 * 输入：size，单位为 Byte
 * 输出：分配空间的起始地址
*/
void *
malloc(uint32 size)
{
    if(size == 0) return 0;

    /* 获得所需要分配的块数 */
    uint32 n = (size - 1) / MIN_BLOCK_SIZE + 1;
    uint32 block = buddyAlloc(n);
    if(block == -1) panic("Malloc failed!\n");

    /* 清除这一段内存空间 */
    uint32 totalBytes = fixSize(n) * MIN_BLOCK_SIZE;
    uint8 *beginAddr = (uint8 *)((usize)HEAP + (usize)(block * MIN_BLOCK_SIZE));
    uint32 i;
    for(i = 0; i < totalBytes; i ++) {
        beginAddr[i] = 0;
    }
    
    return (void *)beginAddr;
}

/* 
 * 根据 offset 回收空间
 * offset单位为块（MIN_BLOCK_SIZE） 指在二叉树节点中的偏移 
*/
void
buddyFree(uint32 offset)
{
    uint32 nodeSize, index = 0;
    
    nodeSize = 1;
    index = offset + buddyTree.size - 1;

    /* 向上回溯到之前分配块的节点位置 */
    for( ; buddyTree.longest[index]; index = PARENT(index)) {
        nodeSize *= 2;
        if(index == 0) {
            return;
        }
    }
    buddyTree.longest[index] = nodeSize;

    /* 继续向上回溯，合并连续的空闲区间 */
    while(index) {
        index = PARENT(index);
        nodeSize *= 2;

        uint32 leftLongest, rightLongest;
        leftLongest = buddyTree.longest[LEFT_LEAF(index)];
        rightLongest = buddyTree.longest[RIGHT_LEAF(index)];

        if(leftLongest + rightLongest == nodeSize) {
            buddyTree.longest[index] = nodeSize;
        } else {
            buddyTree.longest[index] = MAX(leftLongest, rightLongest);
        }
    }
}

/* 
 * 回收被分配出去的内存 
 * 输入：回收空间的起始地址
*/
void
free(void *ptr)
{
    if((usize)ptr < (usize)HEAP) return;
    if((usize)ptr > (usize)HEAP + USER_HEAP_SIZE - MIN_BLOCK_SIZE) return;
    /* 相对于堆空间起始地址的偏移 */
    uint32 offset = (usize)((usize)ptr - (usize)HEAP);
    buddyFree(offset / MIN_BLOCK_SIZE);
}









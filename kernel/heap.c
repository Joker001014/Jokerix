/*************************** 堆的动态内存分配 ****************************
 * Author：Joker001014
 * 2025.03.01
 * 使用 Buddy System Allocation 算法分配
***********************************************************************/

#include "types.h"
#include "def.h"
#include "consts.h"

#define LEFT_LEAF(index) ((index) * 2 + 1)      // 计算一个节点的左子节点索引
#define RIGHT_LEAF(index) ((index) * 2 + 2)     // 计算一个节点的右子节点索引
#define PARENT(index) ( ((index) + 1) / 2 - 1)  // 计算父节点索引

#define IS_POWER_OF_2(x) (!((x)&((x)-1)))       // 判断数x是否为2的幂次方
#define MAX(a, b) ((a) > (b) ? (a) : (b))       // 计算a、b的最大值

/* 用于分配的堆空间，存放在 .bss 段，8M bytes */
static uint8 HEAP[KERNEL_HEAP_SIZE];

void buddyInit(int size);
int buddyAlloc(int size);
void buddyFree(int offset);

// 初始化堆空间
void
initHeap()
{
    buddyInit(HEAP_BLOCK_NUM);
}

/*
 * 获得大于等于某个数的最小的 2 的幂级数
 * 算法来源于 Java HashMap 实现
 */
int
fixSize(int size)
{
    int n = size - 1;
    n |= n >> 1;
    n |= n >> 2;
    n |= n >> 4;
    n |= n >> 8;
    n |= n >> 16;
    return n + 1;
}

/* 
 * 在内核堆上分配内存
 * 输入：size，单位为 Byte
 * 输出：分配空间的起始地址
*/
void *
kalloc(int size)
{
    if(size <= 0) return 0;

    uint32 n;
    // 计算需要分配的块数
    if((size && (MIN_BLOCK_SIZE-1)))    // 分配块数 与 最小分配块数-1 相与不为零，说明右余数，块数+1
        n = size / MIN_BLOCK_SIZE + 1;
    else
        n = size / MIN_BLOCK_SIZE;
    // 分配块，得到偏移地址（单位为MIN_BLOCK_SIZE）
    int block = buddyAlloc(n);
    if(block == -1) panic("Malloc failed!\n");

    /* 清零被分配的内存空间 */
    int totalBytes = fixSize(n) * MIN_BLOCK_SIZE;   // 计算所分配的大小，单位bytes
    // 计算分配空间的起始地址(usize)，声明指向一个字节(uint8)的指针指向这个地址
    uint8 *beginAddr = (uint8 *)((usize)HEAP + (usize)(block * MIN_BLOCK_SIZE));
    int i;
    for(i = 0; i < totalBytes; i ++) {  // 逐字节清零分配的内存空间
        beginAddr[i] = 0;
    }
    
    return (void *)beginAddr;   
}

/* 
 * 回收被分配出去的内存 
 * 输入：回收空间的起始地址
*/
void
kfree(void *ptr)
{
    // 验证地址是否在堆空间
    if((usize)ptr < (usize)HEAP) return;
    if((usize)ptr > (usize)HEAP + KERNEL_HEAP_SIZE - MIN_BLOCK_SIZE) return;
    /* 相对于堆空间起始地址的偏移 */
    usize offset = (usize)((usize)ptr - (usize)HEAP);
    buddyFree(offset / MIN_BLOCK_SIZE);
}

/* 
 * Buddy System Allocation 的具体实现
 * 使用一棵数组形式的完全二叉数来监控内存
 */
struct
{
    int size;                    /* 管理的总块数 */
    int longest[BUDDY_NODE_NUM]; /* 每个节点表示范围内最大连续空闲块个数 */
    // 总节点个数 BUDDY_NODE_NUM = size*2 - 1
} buddyTree;

// 二叉树初始化
void
buddyInit(int size)
{
    buddyTree.size = size;      // 管理的总块数
    int nodeSize = size << 1;   // 因为下为对于i=0节点也会判断为2的幂，所以先乘2来抵消下文的除2
    int i;
    /* 初始化每个节点，此时每一块都是空闲的 */
    for(i = 0; i < (size << 1) - 1; i ++) {
        // 若当前节点为2的幂，则除以2
        if(IS_POWER_OF_2(i+1)) {
            nodeSize /= 2;
        }
        buddyTree.longest[i] = nodeSize;    
        // 数组存储例：1024 512 512 256 256 256 256 128 128...
    }
}

/* 
 * 分配 size 大小的块(单位为MIN_BLOCK_SIZE)，通过二叉树进行管理
 * 返回空闲块的第一块在堆上的偏移（0~HEAP_BLOCK_NUM），单位为MIN_BLOCK_SIZE
 */
int
buddyAlloc(int size)
{
    int index = 0;
    int nodeSize;
    int offset;

    /* 调整内存块数量到 2 的幂 */
    if(size <= 0) size = 1;
    else if(!IS_POWER_OF_2(size)) size = fixSize(size);

    /* 超过总内存分配大小 */
    if(buddyTree.longest[0] < size) {
        return -1;
    }
    
    /* 从二叉树根开始，寻找大小最符合的节点 */
    for(nodeSize = buddyTree.size; nodeSize != size; nodeSize /= 2) {   // 不断减小nodeSize，直到和size一样大
        int left = buddyTree.longest[LEFT_LEAF(index)];     // 左子节点索引结果，即大小
        int right = buddyTree.longest[RIGHT_LEAF(index)];   // 右子结点索引结果，即大小
        /* 优先选择最小的且满足条件的分叉，小块优先，尽量保留大块 */
        if(left <= right) {
            if(left >= size) index = LEFT_LEAF(index);  // 大小满足，继续向下找
            else index = RIGHT_LEAF(index);
        } else {
            if(right >= size) index = RIGHT_LEAF(index);
            else index = LEFT_LEAF(index);
        }
    }

    /* 
     * 将该节点标记为占用
     * 
     * 注意这里标记将该节点的下级节点，便于回收时确定内存块数量
     */
    buddyTree.longest[index] = 0;

    /* 获得这一段空闲块的第一块在堆上的偏移 */
    // 此处非常巧妙，如对于二叉树 1024 512 512 256 256 256 256 128 128...
    //                         0   1   2   3   4   5   6   7
    // 如果index=4， 5*256-1024=256，就知道偏移是256
    offset = (index + 1) * nodeSize - buddyTree.size;

    /* 向上调整父节点的值,即空闲个数 */
    while(index) {
        index = PARENT(index);
        buddyTree.longest[index] = 
            MAX(buddyTree.longest[LEFT_LEAF(index)], buddyTree.longest[RIGHT_LEAF(index)]);
    }

    return offset;
}

/* 
 * 根据 offset 回收空间
 * offset单位为块（MIN_BLOCK_SIZE） 指在二叉树节点中的偏移 
*/
void
buddyFree(int offset)
{
    int nodeSize, index = 0;
    
    nodeSize = 1;
    index = offset + buddyTree.size - 1;

    /* 
     * 向上回溯到之前分配块的节点位置
     * 由于分配时没有标记下级节点，这里只需要向上寻找到第一个被标记的节点就是当时分配的节点
     */
    for( ; buddyTree.longest[index]; index = PARENT(index)) {  // 直到被标记为0则退出
        nodeSize *= 2;
        if(index == 0) {    // 到根节点还没有被占用，直接退出
            return;
        }
    }
    buddyTree.longest[index] = nodeSize;    // 恢复空闲大小

    /* 继续向上回溯，合并连续的空闲区间，直到根节点 */
    while(index) {
        index = PARENT(index);
        nodeSize *= 2;

        int leftLongest, rightLongest;
        leftLongest = buddyTree.longest[LEFT_LEAF(index)];
        rightLongest = buddyTree.longest[RIGHT_LEAF(index)];

        // 如果左、右根节点的空闲大小之和等于父节点空闲大下
        if(leftLongest + rightLongest == nodeSize) {
            buddyTree.longest[index] = nodeSize;    // 则合并（修改父节点最大连续大小）
        } else {
            buddyTree.longest[index] = MAX(leftLongest, rightLongest);  // 否则父节点取子结点最大连续大小
        }
    }
}

// 动态内存分配测试函数
void 
testHeap()
{
    printf("Heap:\t%p\n", HEAP);
    void *a = kalloc(100);
    printf("a:\t%p\n", a);
    void *b = kalloc(60);
    printf("b:\t%p\n", b);
    void *c = kalloc(100);
    printf("c:\t%p\n", c);
    kfree(a);
    void *d = kalloc(30);
    printf("d:\t%p\n", d);
    kfree(b);
    kfree(d);
    kfree(c);
    a = kalloc(60);
    printf("a:\t%p\n", a);
    kfree(a);
}


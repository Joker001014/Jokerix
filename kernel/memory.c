/************************* 内存页的分配和回收 ****************************
 * Author：Joker001014
 * 2025.03.01
 * 基于线段树的页帧分配
***********************************************************************/

#include "types.h"
#include "def.h"
#include "memory.h"
#include "consts.h"
#include "riscv.h"

/* 全局唯一的页帧分配器 */
FrameAllocator frameAllocator;

/* 分配算法需要实现的三个函数 */
Allocator newAllocator(usize startPpn, usize endPpn);
usize alloc();                                           
void dealloc(usize ppn);

// 初始化全局页帧分配器
void
initFrameAllocator(usize startPpn, usize endPpn)
{
    frameAllocator.startPpn = startPpn;     // 设置分配器的起始页帧号
    frameAllocator.allocator = newAllocator(startPpn, endPpn);  // 初始化页帧分配器
}

/*
 * 分配一个物理页
 * 返回物理页的起始地址
 */
usize
allocFrame()
{
    usize start = alloc() << 12;
    int i;
    /*
     * 清空被分配的区域
     * 这里访问需要通过虚拟地址
     */
    char *vStart = (char *)(start + KERNEL_MAP_OFFSET);
    for(i = 0; i < PAGE_SIZE; i ++) {
        vStart[i] = 0;
    }
    return (usize)start;
}
// usize
// allocFrame()
// {
//     // 分配一个物理页并计算地址
//     char *start = (char *)(alloc() << 12);
//     int i;
//     // 初始化页的每个字节为零
//     for(i = 0; i < PAGE_SIZE; i ++) {
//         start[i] = 0;
//     }
//     return (usize)start;
// }


/*
 * 回收一个物理页
 * 输入为物理页的起始物理地址
 */
void
deallocFrame(usize startAddr)
{
    dealloc(startAddr >> 12);
}


/*
 * 初始化 页分配 和 动态内存分配
 */
void
initMemory()
{
    /* 初始化 .bss 段 */
    // uint64 *bss_start_init = (uint64 *) bss_start, *bss_end_init = (uint64 *) bss_end;
    // for (volatile uint64 *bss_mem = bss_start_init; bss_mem < bss_end_init; ++bss_mem) {
    //     *bss_mem = 0;
    // }

    /* 
     * 开启 sstatus 的 SUM 位
     * 允许内核访问用户内存
     */
    w_sstatus(r_sstatus() | SSTATUS_SUM);
    // 初始化全局页帧分配器
    initFrameAllocator(
        // 起始PPN 是内核结束虚拟地址-内核起始虚拟地址 再加上 内核起始物理地址 = 内核结束物理地址
        (((usize)(kernel_end) - KERNEL_BEGIN_VADDR + KERNEL_BEGIN_PADDR) >> 12) + 1,
        // 终止页是物理内存的最后一个页
        MEMORY_END_PADDR >> 12
    );
    extern void initHeap();     initHeap();     // 初始化动态内存分配器
    extern void mapKernel();    mapKernel();    // 内核重映射，三级页表机制
    printf("***** Init Memory *****\n");
}


/* 最大可用的内存长度，从 0x80000000 ~ 0x88000000 */
#define MAX_PHYSICAL_PAGES 0x8000   // 页带小为4K

// 管理页栈分配的线段树结构
struct
{
    uint8 node[MAX_PHYSICAL_PAGES << 1];    /* 线段树的节点，每个都表示该范围内是否有空闲页 */
    usize firstSingle;                      /* 第一个单块节点的下标，即最后一层叶子节点的下标 */
    usize length;                           /* 分配区间长度，表示可分配的页帧数量 */
    usize startPpn;                         /* 分配的起始 ppn */
} sta;

// 初始化页帧分配器（线段数初始化）
Allocator
newAllocator(usize startPpn, usize endPpn)
{
    sta.startPpn = startPpn - 1;
    sta.length = endPpn - startPpn;     // 设置页帧分配的区间长度
    sta.firstSingle = 1;

    // 找到合适的树大小，树的节点数量必须是2的幂
    while(sta.firstSingle < sta.length + 2) {   // 推测：+1是因为尾-头，头和尾本身也是节点；又+1是因为firstSingle要大于区间长度
        sta.firstSingle <<= 1;
    }

    // 初始化线段树每个节点为1，表示已分配了
    usize i = 1;
    for(i = 1; i < (sta.firstSingle << 1); i ++) {  // 完全二叉树总节点个数为管理节点数乘2
        sta.node[i] = 1;
    }
    // 初始化叶子节点为0（线段树末端），表示页帧是空闲的
    for(i = 1; i < sta.length; i ++) {
        sta.node[sta.firstSingle + i] = 0;
    }
    // 构建线段树，将左右子节点的值与运算，如果左右子节点有一个是空闲的，那父节点也是空闲块
    for(i = sta.firstSingle - 1; i >= 1; i --) {
        sta.node[i] = sta.node[i << 1] & sta.node[(i << 1) | 1];
    }
    Allocator ac = {alloc, dealloc};    // 创建分配器
    return ac;
}

/*
 * 分配一个物理页
 * 返回物理页号 PPN 
 */
usize
alloc()
{
    // 如果根节点已分配，表示没有空闲页
    if(sta.node[1] == 1) {
        panic("Physical memory depleted!\n");
    }
    // 查找空闲页
    usize p = 1;
    while(p < sta.firstSingle) {    // p 小于单节点地址
        if(sta.node[p << 1] == 0) { // 判断左子节点是否空闲
            p = p << 1;
        } else {                    // 判断右子节点是否空闲
            p = (p << 1) | 1;
        }
    }
    // 计算分配的物理页号 PPN
    usize result = p - sta.firstSingle + sta.startPpn;
    sta.node[p] = 1;    // 标记为已分配
    // 回溯更新父节点
    p >>= 1;            
    while(p >> 0) {
        sta.node[p] = sta.node[p << 1] & sta.node[(p << 1) | 1];    // 若左右子节点都不空闲，则父结点也不空闲
        p >>= 1;
    }
    return result;
}

/*
 * 回收物理页
 * 输入物理页号 PPN
 */
void
dealloc(usize ppn)
{
    // 计算页帧在线段树中的索引
    usize p = ppn - sta.startPpn + sta.firstSingle;
    // 如果页表已经空闲则不需要回收
    if(sta.node[p] != 1) {
        printf("The page is free, no need to dealloc!\n");
        return;
    }
    // 标记为空闲
    sta.node[p] = 0;
    // 回溯更新父节点
    p >>= 1;
    while(p > 0) {
        sta.node[p] = sta.node[p << 1] & sta.node[(p << 1) | 1];
        p >>= 1;
    }
}


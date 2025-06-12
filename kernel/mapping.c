/*************************** 虚拟内存映射 *******************************
 * Author：Joker001014
 * 2025.03.04
***********************************************************************/

#include "types.h"
#include "def.h"
#include "consts.h"
#include "mapping.h"

/* 
 * 根据虚拟页号得到其对应页表项在三级页表中的位置
 * 输入：vpn-27位虚拟页号VPN，存放三级页表索引的数组
 */
void
getVpnLevels(usize vpn, usize *levels)
{
    levels[0] = (vpn >> 18) & 0x1ff;
    levels[1] = (vpn >> 9) & 0x1ff;
    levels[2] = vpn & 0x1ff;
}

/* 
 * 创建一个有根页表的映射，只分配了三级页表的空间
 * 输出：根页表的物理页号PPN
 */
Mapping
newMapping()
{
    usize rootPaddr = allocFrame();
    Mapping m = {rootPaddr >> 12};
    return m;
}

/* 
 * 根据给定的虚拟页号寻找三级页表项
 * 如果某一级页表项为空，会创建下一级页表并填充
 * 输入：self-44位根页表物理页号PPN，vpn-27位虚拟页号VPN
 * 输出：一级页表页表项
 */
PageTableEntry
*findEntry(Mapping self, usize vpn)
{
    // 获得 根页表 线性映射后的虚拟地址
    PageTable *rootTable = (PageTable *)accessVaViaPa(self.rootPpn << 12);
    // 计算虚拟页号对应三级页表位置
    usize levels[3]; 
    getVpnLevels(vpn, levels);
    // 计算获取三级页表项PTE
    PageTableEntry *entry = &(rootTable->entries[levels[0]]);
    int i;
    for(i = 1; i <= 2; i ++) {
        /* 页表不存在，创建新页表 */
        if(*entry == 0) {   
            usize newPpn = allocFrame() >> 12;  // 分配一个空闲物理页，返回分配页物理地址
            *entry = (newPpn << 10) | VALID;    // 设置页表项指向分配页，并设置Flags有效位
        }
        // 计算下一级页表位置，PTE获取高44位，左移两位对应的页即为其物理地址
        usize nextPageAddr = (*entry & PDE_MASK) << 2;
        // 索引下一级页表的页表项PTE
        // 注意！从页表和页表项中取出的地址都是物理地址，但内核已经被映射到了高地址空间，所以只能通过虚拟地址来访问
        entry = &(((PageTable *)accessVaViaPa(nextPageAddr))->entries[levels[i]]);
    }
    return entry;
}

/*
 * 线性映射一个段到三级页表上
 * 段中的每一个虚拟地址都会按照固定偏移量线性映射到一个物理地址
 */
void
mapLinearSegment(Mapping self, Segment segment)
{
    usize startVpn = segment.startVaddr / PAGE_SIZE;        // 段起始页虚拟页号VPN（4K对齐）
    usize endVpn = (segment.endVaddr - 1) / PAGE_SIZE + 1;  // 段结束页虚拟页号VPN（4K对齐）
    usize vpn;
    for(vpn = startVpn; vpn < endVpn; vpn ++) {
        PageTableEntry *entry = findEntry(self, vpn);
        if(*entry != 0) {
            panic("Virtual address already mapped!\n");
        }
        // 修改三级页表映射到实际物理地址上，并设置flags权限，及有效位
        *entry = ((vpn - KERNEL_PAGE_OFFSET) << 10) | segment.flags | VALID;
    }
}


// 映射一个未被分配物理内存的段，并复制数据到新分配的内存
// m-新分配的根页表物理页号，segment-需要拷贝的段，data-拷贝的数据，length-拷贝的长度
void
mapFramedAndCopy(Mapping m, Segment segment, char *data, usize length)
{
    usize s = (usize)data, l = length;
    usize startVpn = segment.startVaddr / PAGE_SIZE;        // 起始地址虚拟页号
    usize endVpn = (segment.endVaddr - 1) / PAGE_SIZE + 1;  // 结束地址虚拟页号
    usize vpn;
    // 遍历每一页
    for(vpn = startVpn; vpn < endVpn; vpn ++) {
        // 根据给定的虚拟页号寻找三级页表项
        PageTableEntry *entry = findEntry(m, vpn);
        if(*entry != 0) {
            panic("Virtual address already mapped!\n");
        }
        // 分配一个物理页
        usize pAddr = allocFrame();
        // 设置页表项PTE
        *entry = (pAddr >> 2) | segment.flags | VALID;
        // 复制数据到目标位置
        char *dst = (char *)accessVaViaPa(pAddr);   /* 获得线性映射后的虚拟地址 */
        // 拷贝一页
        if(l >= PAGE_SIZE) {
            char *src = (char *)s;
            int i;
            for(i = 0; i < PAGE_SIZE; i ++) {
                dst[i] = src[i];    // 逐字节拷贝
            }
        } else {
            // 拷贝剩余不足一页的数据
            char *src = (char *)s;
            int i;
            for(i = 0; i < l; i ++) {
                dst[i] = src[i];    // 逐字节拷贝
            }
            for(i = l; i < PAGE_SIZE; i ++) {
                dst[i] = 0;         // 最后一页剩下字节置零
            }
        }
        // 继续拷贝下一页
        s += PAGE_SIZE;
        if(l >= PAGE_SIZE) l -= PAGE_SIZE;
        else l = 0;
    }
}

/*
 * 将页表地址写入 satp 中
 * 设置 satp 为 SV39，并刷新 TLB
 */
void
activateMapping(Mapping self)
{
    usize satp = self.rootPpn | (8L << 60); // 设置 PPN 和 MODE
    asm volatile("csrw satp, %0" : : "r" (satp));
    asm volatile("sfence.vma":::);
}

/* 
 * 创建一个映射了内核的虚拟地址空间，即创建根页表
 * 在该地址空间中，内核的各个段按照固定的偏移被映射到虚拟地址空间的高地址空间处
 */
Mapping
newKernelMapping()
{
    Mapping m = newMapping();   // 创建根页表
    
    /* .text 段，r-x */
    Segment text = {
        (usize)text_start,
        (usize)rodata_start,
        1L | READABLE | EXECUTABLE
    };
    mapLinearSegment(m, text);  // 将段映射到三级页表上

    /* .rodata 段，r-- */
    Segment rodata = {
        (usize)rodata_start,
        (usize)data_start,
        1L | READABLE
    };
    mapLinearSegment(m, rodata);

    /* .data 段，rw- */
    Segment data = {
        (usize)data_start,
        (usize)bss_start,
        1L | READABLE | WRITABLE
    };
    mapLinearSegment(m, data);

    /* .bss 段，rw- */
    Segment bss = {
        (usize)bss_start,
        (usize)kernel_end,
        1L | READABLE | WRITABLE
    };
    mapLinearSegment(m, bss);

    /* 剩余空间，rw- */     // 内核结束到内存结束空间，按页分配内存分配的就是这一段空间
    Segment other = {
        (usize)kernel_end,
        (usize)(MEMORY_END_PADDR + KERNEL_MAP_OFFSET),
        1L | READABLE | WRITABLE
    };
    mapLinearSegment(m, other);

    return m;
}

// 映射外部中断相关区域到虚拟内存
void
mapExtInterruptArea(Mapping m)
{
    // VIRT_PLIC
    Segment s1 = {
        (usize)0x0C000000 + KERNEL_MAP_OFFSET,
        (usize)0x0C001000 + KERNEL_MAP_OFFSET,
        1L | READABLE | WRITABLE
    };
    mapLinearSegment(m, s1);    // 将段映射到三级页表上

    Segment s2 = {
        (usize)0x0C002000 + KERNEL_MAP_OFFSET,
        (usize)0x0C003000 + KERNEL_MAP_OFFSET,
        1L | READABLE | WRITABLE
    };
    mapLinearSegment(m, s2);

    Segment s3 = {
        (usize)0x0C201000 + KERNEL_MAP_OFFSET,
        (usize)0x0C202000 + KERNEL_MAP_OFFSET,
        1L | READABLE | WRITABLE
    };
    mapLinearSegment(m, s3);

    // VIRT_UART
    Segment s4 = {
        (usize)0x10000000 + KERNEL_MAP_OFFSET,
        (usize)0x10001000 + KERNEL_MAP_OFFSET,
        1L | READABLE | WRITABLE
    };
    mapLinearSegment(m, s4);
}

/* 重映射内核,写入satp */
void
mapKernel()
{
    Mapping m = newKernelMapping();     // 创建一个映射了内核(0x80200000后地址）的虚拟地址空间
    mapExtInterruptArea(m);             // 创建一个映射了PLIC和UART地址
    activateMapping(m);                 // 将根页表地址写入 satp
    printf("***** Remap Kernel *****\n");
}

/* 获得线性映射后的虚拟地址 */
usize
accessVaViaPa(usize pa)
{
    return pa + KERNEL_MAP_OFFSET;
}


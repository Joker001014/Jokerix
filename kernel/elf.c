/********************* 映射ELF文件各个段到内存 ***************************
 * Author：Joker001014
 * 2025.03.09
***********************************************************************/

#include "types.h"
#include "def.h"
#include "mapping.h"
#include "elf.h"
#include "consts.h"

// 将 ELF 权限标志位转换为页表项属性
usize
convertElfFlags(uint32 flags)
{
    usize ma = 1L;  // 设置有效位
    ma |= USER;     // 设置 USER 属性，以保证 U-Mode 下的程序可以访问
    if(flags & ELF_PROG_FLAG_EXEC) {
        ma |= EXECUTABLE;
    }
    if(flags & ELF_PROG_FLAG_WRITE) {
        ma |= WRITABLE;
    }
    if(flags & ELF_PROG_FLAG_READ) {
        ma |= READABLE;
    }
    return ma;
}

// 新建用户进程页映射，遍历ELF文件所有程序段并映射到虚拟内存空间
// 函数传入指向 ELF 文件的首字节的指针
Mapping
newUserMapping(char *elf)
{
    // 创建一个映射了内核的虚拟地址空间(创建根页表、映射程序各个段)
    Mapping m = newKernelMapping();
    ElfHeader *eHeader = (ElfHeader *)elf;
    // 校验 ELF 头
    if(eHeader->magic != ELF_MAGIC) {
        panic("Unknown file type!");
    }
    // 通过 e_phoff 可以找到文件的程序头
    ProgHeader *pHeader = (ProgHeader *)((usize)elf + eHeader->phoff);
    usize offset;
    int i;
    // 遍历所有的程序段，将类型为 LOAD 的段全部映射到虚拟内存空间
    for(i = 0, offset = (usize)pHeader; i < eHeader->phnum; i ++, offset += sizeof(ProgHeader)) {
        pHeader = (ProgHeader *)offset;
        //  判断该段的类型，对于操作系统来说，我们需要关注类型为 LOAD 的段
        if(pHeader->type != ELF_PROG_LOAD) {
            continue;
        }
        // 将 ELF 权限标志位转换为页表项属性
        usize flags = convertElfFlags(pHeader->flags);
        // 获取段映射到内存空间的起始虚拟地址、结束虚拟地址
        usize vhStart = pHeader->vaddr, vhEnd = vhStart + pHeader->memsz;
        // 创建描述映射到虚拟内存的一个段
        Segment segment = {vhStart, vhEnd, flags};
        // 计算段数据的起始位置
        char *source = (char *)((usize)elf + pHeader->off);
        // 映射一个未被分配物理内存的段，并复制数据到新分配的内存
        mapFramedAndCopy(m, segment, source, pHeader->filesz);
    }
    return m;
}







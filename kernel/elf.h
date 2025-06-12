/********************* 映射ELF文件各个段到内存 ***************************
 * Author：Joker001014
 * 2025.03.09
***********************************************************************/

#ifndef _ELF_H
#define _ELF_H

#include "types.h"

// ELF 魔数   F L E 7f
#define ELF_MAGIC 0x464C457FU 

// ELF 文件头
typedef struct {
  uint magic;
  uchar elf[12];        /* Magic number and other info */
  ushort type;          /* Object file type */
  ushort machine;       /* Architecture */
  uint version;         /* Object file version */
  uint64 entry;         /* Entry point virtual address */
  uint64 phoff;         /* Program header table file offset */
  uint64 shoff;         /* Section header table file offset */
  uint flags;           /* Processor-specific flags */
  ushort ehsize;        /* ELF header size in bytes */
  ushort phentsize;     /* Program header table entry size */
  ushort phnum;         /* Program header table entry count */
  ushort shentsize;     /* Section header table entry size */
  ushort shnum;         /* Section header table entry count */
  ushort shstrndx;      /* Section header string table index */
} ElfHeader;

// 程序段头
typedef struct {
  uint32 type;      // 描述该段的类型
  uint32 flags;     // 以p_type而定
  uint64 off;       // 该段的开始相对于文件开始的偏移量
  uint64 vaddr;     // 段加在到虚拟内存空间的地址
  uint64 paddr;     // 段的虚拟地址 
  uint64 filesz;    // 文件映像中该段的字节数
  uint64 memsz;     // 内存映像中该段的字节数
  uint64 align;     // 描述要对齐的段在内存中如何对齐，该值是2的整数次幂 
} ProgHeader;

// 程序段头类型
#define ELF_PROG_LOAD           1   /* 程序段头类型 LOAD */

/* 程序段头权限 */
#define ELF_PROG_FLAG_EXEC      1   /* 程序段头属性，可执行 */
#define ELF_PROG_FLAG_WRITE     2   /* 程序段头属性，可写 */
#define ELF_PROG_FLAG_READ      4   /* 程序段头属性，可读 */

Mapping newUserMapping(char *data);

#endif
/***************************** 程序入口 ********************************
 * Author：Joker001014
 * 2025.02.26
 * 设置内核栈，跳转到main函数
***********************************************************************/

    .section .text.entry    // 内核的入口点
    .globl _start
_start:
    # 计算 bootpagetable 的物理页号（satp 中保存的页表基地址是物理页号）
    lui t0, %hi(bootpagetable)      # Load Upper Immediate，将立即数的高20位(%hi)加载到t0寄存器的高20位
    li t1, 0xffffffff00000000       # Load Immediate，加载立即数到t1
    sub t0, t0, t1                  # t0 - t1，即将虚拟地址转为物理地址
    srli t0, t0, 12                 # shift right logical immediate，t0右移12位，即页号PPN
    # 设置使用 SV39（8 << 60）
    li t1, (8 << 60)                # t1 = 2^63,即高四位=8，即MODE=8，即Sv39模式
    or t0, t0, t1                   # 设置satp的PPN，MODE=8，ASID=0
    # 写入 satp 并刷新 TLB
    csrw satp, t0                   # 写satp
    sfence.vma                      # 刷新TLB，使新配置页表生效

    # 加载栈地址，I 型指令只支持最多 32 位立即数，操作地址时需要分两次装载
    lui sp, %hi(bootstacktop)       # 将栈顶地址高部分加载到sp
    addi sp, sp, %lo(bootstacktop)  # 将栈顶地址低部分加载到sp

    # 跳转到 main
    lui t0, %hi(main)               # 同理加载main地址
    addi t0, t0, %lo(main)
    jr t0                           # 跳转到main函数


# 以下 4096 × 16 字节的空间作为 OS 的启动栈
    .section .bss.stack
    .align 12
    .global bootstack
bootstack:
    .space 4096 * 16
    .global bootstacktop
bootstacktop:

# 初始内核映射所用的页表
    .section .data
    .align 12       # 4K地址对齐
bootpagetable:
    .quad 0         # 定义一个8字节的数据项
    .quad 0
    # 第 2 项：0x80000000 -> 0x80000000，0xcf 表示 VRWXAD 均为 1，该页表项用于支持跳转到 main 之前刷新 TLB 之后的代码工作
    .quad (0x80000 << 10) | 0xcf    
    .zero 507 * 8   # 填充507个8字节的零数据
    # 第 510 项：0xffffffff80000000 -> 0x80000000，0xcf 表示 VRWXAD 均为 1
    .quad (0x80000 << 10) | 0xcf    # PPN | Flags，63~39位等于38位，所以表示 0xffffffff 80000000
    .quad 0   










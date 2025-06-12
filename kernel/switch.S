/**************************** 线程上下文切换 ****************************
 * Author：Joker001014
 * 2025.03.05
***********************************************************************/

.equ XLENB, 8   # 寄存器字节数为8

    addi  sp, sp, (-XLENB*14)   # 分配栈空间
    sd sp, 0(a0)                # a0为传入的“当前线程上下文存储地址“，栈指针移动后更新上下文地址
    sd ra, 0*XLENB(sp)          # 将寄存器 ra 保存到栈上
    sd s0, 2*XLENB(sp)          # s0-s11
    sd s1, 3*XLENB(sp)
    sd s2, 4*XLENB(sp)
    sd s3, 5*XLENB(sp)
    sd s4, 6*XLENB(sp)
    sd s5, 7*XLENB(sp)
    sd s6, 8*XLENB(sp)
    sd s7, 9*XLENB(sp)
    sd s8, 10*XLENB(sp)
    sd s9, 11*XLENB(sp)
    sd s10, 12*XLENB(sp)
    sd s11, 13*XLENB(sp)
    csrr  s11, satp
    sd s11, 1*XLENB(sp)         # satp

    ld sp, 0(a1)                # 将“目标线程上下文存储地址”传入sp
    ld s11, 1*XLENB(sp)         
    csrw satp, s11              # 恢复 satp
    sfence.vma                  # 刷新TLB，使新配置页表生效
    ld ra, 0*XLENB(sp)          # 恢复 ra
    ld s0, 2*XLENB(sp)          # 恢复s0-s11
    ld s1, 3*XLENB(sp)
    ld s2, 4*XLENB(sp)
    ld s3, 5*XLENB(sp)
    ld s4, 6*XLENB(sp)
    ld s5, 7*XLENB(sp)
    ld s6, 8*XLENB(sp)
    ld s7, 9*XLENB(sp)
    ld s8, 10*XLENB(sp)
    ld s9, 11*XLENB(sp)
    ld s10, 12*XLENB(sp)
    ld s11, 13*XLENB(sp)
    addi sp, sp, (XLENB*14)     # 释放目标线程存储寄存器的栈空间

    sd zero, 0(a1)              # 清除a1寄存器
    ret

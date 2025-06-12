/************************** 中断处理程序的入口 ***************************
 * Author：Joker001014
 * 2025.02.26
 * 在中断发生时将当前程序的上下文保存在内核栈上，并在中断处理完成后恢复
***********************************************************************/

# 启用替代宏.macro,允许编写宏替代复杂指令
.altmacro
# 寄存器宽度 8 字节
.set    REG_SIZE, 8
# Context 大小为 34 个数据
.set    CONTEXT_SIZE, 34

# 宏：保存寄存器reg到栈上
.macro SAVE reg, offset
    sd  \reg, \offset*8(sp)     # 将reg寄存器存储到sp+offset*8 （"\"是宏参数的占位符）
.endm

# 宏：调用 SAVE 批量保存不同宏定义，n=1时，SAVE x1, 1 -> sd x1, 8(sp)
.macro SAVE_N n
    SAVE  x\n, \n
.endm

# 宏：从栈中恢复寄存器
.macro LOAD reg, offset
    ld  \reg, \offset*8(sp)
.endm

.macro LOAD_N n
    LOAD  x\n, \n
.endm


    .section .text          # 代码段
    .globl __interrupt
    .balign 4               # 中断处理函数需要 4 字节对齐
# 全局中断处理，保存 Context 并跳转到 handleInterrupt() 处
__interrupt:
    # 移动栈指针，留出 Context 的空间
    addi    sp, sp, -34*REG_SIZE
    
    # 保存通用寄存器，其中 x0 固定为 0
    SAVE    x1, 1
    # 将原来的 sp 写入 2 位置
    addi    x1, sp, 34*REG_SIZE   # 先计算原sp位置，再保存在x1中
    SAVE    x1, 2
    # 循环保存 x3 至 x31
    .set    n, 3            # 定义一个符号常量 = 3
    .rept   29
        SAVE_N  %n
        .set    n, n + 1
    .endr

    # 保存 CSR
    csrr    s1, sstatus
    csrr    s2, sepc
    SAVE    s1, 32
    SAVE    s2, 33

    # 调用 handleInterrupt()
    # 将 Context 的地址(栈顶)和 scause、stval 作为参数传入
    mv      a0, sp
    csrr    a1, scause
    csrr    a2, stval
    jal     handleInterrupt


    .globl __restore
# 从 handleInterrupt 返回
# 从 Context 中恢复所有寄存器，并跳转至 Context 中 sepc 的位置
__restore:
    # 恢复 CSR
    LOAD    s1, 32
    LOAD    s2, 33
    csrw    sstatus, s1
    csrw    sepc, s2

    # 恢复通用寄存器
    LOAD    x1, 1
    # 恢复 x3 至 x31
    .set    n, 3
    .rept   29
        LOAD_N  %n
        .set    n, n + 1
    .endr

    # 恢复 sp，必须最后执行栈指针恢复（即释放栈指针），才不会影响上述LOAD宏的偏移位置
    LOAD    x2, 2
    sret            # 返回中断发生前位置

/********************* 链接脚本：定义程序内存布局 *************************
 * Author：Joker001014
 * 2025.02.26
***********************************************************************/

/* 目标架构 */
OUTPUT_ARCH(riscv)

/* 执行入口 */
ENTRY(_start)

/* 数据存放起始虚拟地址 */
BASE_ADDRESS = 0xffffffff80200000;

SECTIONS
{
    /* . 表示当前地址（location counter） */
    . = BASE_ADDRESS;

    /* start 符号表示全部的开始位置 */
    kernel_start = .;

    . = ALIGN(4K);
    text_start = .;

    /* .text 字段 */
    .text : {
        /* 把 entry 函数放在最前面 */
        *(.text.entry)
        /* 要链接的文件的 .text 字段集中放在这里 */
        *(.text .text.*)
    }

    . = ALIGN(4K);
    rodata_start = .;

    /* .rodata 字段 */
    .rodata : {
        /* 要链接的文件的 .rodata 字段集中放在这里 */
        *(.rodata .rodata.*)
    }

    . = ALIGN(4K);
    data_start = .;

    /* .data 字段 */
    .data : {
        *(.data .data.*)
    }

    . = ALIGN(4K);
    bss_start = .;

    /* .bss 字段 */
    .bss : {
        *(.sbss .bss .bss.*)
    }

    /* 内核结束地址 */
    . = ALIGN(4K);
    kernel_end = .;
}
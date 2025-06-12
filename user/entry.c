/************************** 用户程序入口点 ******************************
 * Author：Joker001014
 * 2025.03.08
 * 初始化堆空间
***********************************************************************/

#include "types.h"
#include "ulib.h"
#include "syscall.h"

/*
 * 弱链接 main 函数
 * 当没有 main 函数被链接时会链接此函数
 */
__attribute__((weak)) uint64
main()
{
    panic("No main linked!\n");
    return 1;
}

/*
 * 用户程序入口点
 * gcc 默认的编译配置中，当 _start() 函数存在时，会将 EntryPoint 设置为 _start()，而不是 main()
 * 初始化堆并调用 main
 */
void
_start(uint8 _args, uint8 *_argv)
{
    // 初始化用户堆空间
    extern void initHeap();     initHeap();
    sys_exit(main());
}
/************************** U-Mode 系统调用 ****************************
 * Author：Joker001014
 * 2025.03.08
***********************************************************************/

#include "types.h"
#include "def.h"
#include "context.h"
#include "stdin.h"
#include "thread.h"
#include "fs.h"

const usize SYS_READ = 63;
const usize SYS_WRITE = 64;
const usize SYS_EXIT = 93;
const usize SYS_EXEC     = 221;


// 只将 READ 系统调用实现读取输入缓冲区的功能，所以 fd 和 len 都不会被使用
usize
sysRead(usize fd, uint8 *base, usize len) {
    *base = (uint8)popChar();
    return 1;
}

// 执行程序
usize
sysExec(char *path)
{
    if(executeCPU(path, getCurrentTid())) {
        // 若执行成功则让当前进程进入休眠
        yieldCPU();
    }
    return 0;
}

// 内核处理系统调用
usize
syscall(usize id, usize args[3], InterruptContext *context)
{
    switch (id)
    {
    case SYS_WRITE:     // 系统写
        consolePutchar(args[0]);
        return 0;
    case SYS_EXIT:      // 系统线程退出
        exitFromCPU(args[0]);
        return 0;
    case SYS_READ:      // 系统读
        return sysRead(args[0], (uint8 *)args[1], args[2]);
    case SYS_EXEC:      // 系统执行
        sysExec((char *)args[0]);
        return 0;
    default:
        printf("Unknown syscall id %d\n", id);
        panic("");
        return -1;
    }
}













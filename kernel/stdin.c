/**************************** 标准输入缓冲区 *****************************
 * Author：Joker001014
 * 2025.03.16
***********************************************************************/

#include "queue.h"
#include "condition.h"

// 标准输入缓冲区，buf 为输入字符缓冲，pushed 为条件变量（等待输入的线程）
struct
{
    Queue buf;          // 队列存储字符
    Condvar pushed;     // 条件变量保存等待输入的线程id
} STDIN;

/*
 * 将一个字符放入标准输入缓冲区
 * 并唤醒一个等待字符的线程
 */
void
pushChar(char ch)
{
    pushBack(&STDIN.buf, (usize)ch);    // 将字符放入标准输入缓冲区
    notifyCondition(&STDIN.pushed);     // 从等待队列中唤醒一个线程
}

/*
 * 线程请求从 stdin 中获取一个输入的字符
 * 如果当前缓冲区为空，线程会进入等待队列，并挂起自己
 * 在之后的某个时刻被唤醒时，缓冲区必然有字符，就可以顺利返回
 */
char
popChar()
{
    while(1) {
        // 判断标准输入缓冲区是否有数据
        if(!isEmpty(&STDIN.buf)) {
            char ret = (char)popFront(&STDIN.buf);  // 获取数据
            return ret;
        } else {
            // 没有数据则将当前线程加入到等待队列中
            waitCondition(&STDIN.pushed);
        }
    }
}

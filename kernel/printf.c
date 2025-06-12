/************************ 监管者二进制接口SBI ****************************
 * Author：Joker001014
 * 2025.02.26
 * 对 OpenSBI 提供的服务进行简单的包装
***********************************************************************/

#include <stdarg.h>     // 对于参数不定场景，使用 va_list 迭代遍历采参数
#include "types.h"
#include "def.h"

// 提供 16 进制数字字符的映射，供 printint 和 printptr 使用
static char digits[] = "0123456789abcdef";

/*
    功能：将一个整数格式化为字符串，并输出到控制台
    输入：xx：要打印的整数；
        base：数字的进制，支持 10（十进制）和 16（十六进制）；
        sign：是否为有符号整数（1 表示有符号，0 表示无符号）
*/
static void printint(int xx, int base, int sign)
{
  char buf[16];
  int i;
  uint x;

  // 如果是负数，记录符号并取绝对值
  if(sign && (sign = xx < 0)) 
    x = -xx;
  else
    x = xx;

  i = 0;
  do {
    buf[i++] = digits[x % base];  // 获取当前最低位数字
  } while((x /= base) != 0);      // 依次处理更高位

  // 如果是负数，添加符号
  if(sign)
    buf[i++] = '-';

  // 从高位到低位输出
  while(--i >= 0)
    consolePutchar(buf[i]); // 调用 SBI 接口向终端输出字符
}

/*
    功能：将指针（64 位地址）格式化为十六进制字符串并输出
    输入：x-要打印的指针地址
*/
static void printptr(uint64 x)
{
  int i;
  // 添加 "0x" 前缀
  consolePutchar('0');
  consolePutchar('x');
  for (i = 0; i < (sizeof(uint64) * 2); i++, x <<= 4)       // 每次处理 4 位
    consolePutchar(digits[x >> (sizeof(uint64) * 8 - 4)]);  // 输出最高 4 位
}

/*
    功能：格式化输出到控制台，支持以下格式：
          %d：十进制整数。
          %x：十六进制整数。
          %p：指针。
          %s：字符串。
          %%：输出 % 本
    输入：fmt-格式化字符串；可变参数列表（...）-对应的值
*/
void printf(char *fmt, ...)
{
  va_list ap;
  int i, c;
  char *s;

//   // 加锁保护
//   locking = pr.locking;
//   if(locking)
//     acquire(&pr.lock);

  if (fmt == 0)
    panic("null fmt");

  va_start(ap, fmt);
  for(i = 0; (c = fmt[i] & 0xff) != 0; i++){
    if(c != '%'){   // 非格式化符号，直接输出
      consolePutchar(c);
      continue;
    }
    c = fmt[++i] & 0xff;  // 获取格式化符号
    if(c == 0)
      break;
    switch(c){
    case 'd':   // 十进制整数
      printint(va_arg(ap, int), 10, 1);
      break;
    case 'x':   // 十六进制整数
      printint(va_arg(ap, int), 16, 1);
      break;
    case 'p':   // 指针
      printptr(va_arg(ap, uint64));
      break;
    case 's':   // 字符串
      if((s = va_arg(ap, char*)) == 0)
        s = "(null)";   // 空字符串处理
      for(; *s; s++)
        consolePutchar(*s);
      break;
    case '%':
      consolePutchar('%');    // 输出 %
      break;
    default:
      // 未知格式化符号
      consolePutchar('%');
      consolePutchar(c);
      break;
    }
  }
  va_end(ap);

//   // 解锁
//   if(locking)
//     release(&pr.lock);
}

/*
    功能：打印紧急错误信息并冻结系统
    输入：s-错误消息字符串
*/
void panic(char *s)
{
//   pr.locking = 0;     // 禁用 `printf` 锁，防止死锁
  printf("panic: ");
  printf(s);          // 打印错误信息
  printf("\n");
//   panicked = 1;       // 标记系统进入 panic 状态
//   for(;;)             // 无限循环，冻结系统
//     ;
  shutdown();
}





































/************************** 内核文件驱动 *******************************
 * Author：Joker001014
 * 2025.03.12
 * 根据一个路径，从文件系统中找到这个文件的 Inode，并且读取它的所有数据到一个字节数组中
***********************************************************************/

#include "types.h"
#include "def.h"
#include "fs.h"

Inode *ROOT_INODE;      // 声明一个指针 ROOT_INODE，指向根目录的 Inode 结构
char *FREEMAP;          // 声明一个指针 FREEMAP，指向文件系统的 freemap

/* 
 * 目前文件系统被装载到 .data 段中
 * _fs_img_start 为文件系统部分内存的起始符号
 */
extern void _fs_img_start();

/* 根据块号获取 Image 中的块的起始地址 */
usize
getBlockAddr(int blockNum) {
    void *addr = (void *)_fs_img_start;
    addr += (blockNum * BLOCK_SIZE);
    return (usize)addr;
}

// 初始化文件系统（只需要找到 root inode 即可）
void
initFs()
{
    // 获取 freemap 块地址
    FREEMAP = (char *)getBlockAddr(1);      
    // 获取超级块的起始地址，_fs_img_start 是文件系统镜像的起始位置（在 linkUser 中定义）
    SuperBlock* spBlock = (SuperBlock *)_fs_img_start;  
    // 根目录的 Inode 紧接在 freemap 块之后
    ROOT_INODE = (Inode *)getBlockAddr(spBlock->freemapBlocks + 1);
}

/*
 * 传入的 Inode 作为当前目录，从当前目录下根据路径查找文件
 * 若当前目录为 /usr，传入 ./hello 和 hello 甚至 ../usr/hello 都可以找到可执行文件 hello
 * 如果传入的路径以 / 开头，函数会忽略当前路径而从根目录下开始查找
 * 如果传入的 Inode 为 null，函数也会从根目录开始查找 
*/
Inode *
lookup(Inode *node, char *filename)
{
    // 如果文件名以 '/' 开头，表示从根目录开始查找
    if(filename[0] == '/') {
        node = ROOT_INODE;      // 将当前节点指向根目录的 Inode
        filename ++;            // 跳过 '/' 字符
    }
    // 如果当前节点为 NULL，则设为根目录 Inode
    if(node == 0) node = ROOT_INODE;
    // 如果文件名为空（表示已找到目标 Inode），返回当前节点
    if(*filename == '\0') return node;
    // 如果当前节点不是目录类型，则返回 NULL
    if(node->type != TYPE_DIR) return 0;
    // 创建一个字符串变量用来存储目标文件名
    char cTarget[strlen(filename) + 1];
    int i = 0;
    // 从文件名中提取出一个部分（直到遇到 '/' 或字符串结束）
    while (*filename != '/' && *filename != '\0') {
        cTarget[i] = *filename;
        filename ++;
        i ++;
    }
    cTarget[i] = '\0';
    // 如果文件名后面还有 '/'，则跳过它
    if(*filename == '/') filename ++;
    // 如果目标文件名是 "."，表示当前目录，递归调用 lookup 查找下一个部分
    if(!strcmp(".", cTarget)) {
        return lookup(node, filename);
    }
    // 如果目标文件名是 ".."，表示父目录，递归查找父目录
    if(!strcmp("..", cTarget)) {
        Inode *upLevel = (Inode *)getBlockAddr(node->direct[1]);    // 获取父目录的 Inode
        return lookup(upLevel, filename);       // 递归查找父目录
    }
    // 当前节点的块数
    int blockNum = node->blocks;
    // 如果块数小于等于 12，则直接在 direct 数组中查找文件
    if(blockNum <= 12) {
        for(i = 2; i < blockNum; i ++) {
            Inode *candidate = (Inode *)getBlockAddr(node->direct[i]);  // 获取当前块的 Inode
            // 如果文件名匹配，则递归查找该文件
            if(!strcmp((char *)candidate->filename, cTarget)) {
                return lookup(candidate, filename);
            }
        }
        return 0;   // 如果没有找到匹配的文件，则返回 NULL
    } else {
        // 如果块数大于 12，先在 direct 数组中查找
        for(i = 2; i < 12; i ++) {
            Inode *candidate = (Inode *)getBlockAddr(node->direct[i]);  // 获取当前块的 Inode
            // 如果文件名匹配，则递归查找该文件
            if(!strcmp((char *)candidate->filename, cTarget)) {
                return lookup(candidate, filename);
            }
        }
        // 如果文件仍然没有找到，则在间接块（indirect）中查找
        uint32 *indirect = (uint32 *)getBlockAddr(node->indirect);      // 获取间接块的地址
        for(i = 12; i < blockNum; i ++) {
            Inode *candidate = (Inode *)getBlockAddr(indirect[i-12]);   // 获取间接块中的 Inode
            // 如果文件名匹配，则递归查找该文件
            if(!strcmp((char *)candidate->filename, cTarget)) {
                return lookup(candidate, filename);
            }
        }
        return 0;   // 如果没有找到匹配的文件，则返回 NULL
    }
}

// 将数据从块中复制到 buf 中
void
copyByteToBuf(char *src, char *dst, int length)
{
    int i;
    for(i = 0; i < length; i ++) {
        dst[i] = src[i];
    }
}

/* 读取一个表示文件的 Inode 的所有字节到 buf 中 */
void
readall(Inode *node, char *buf) {
    // 检查 Inode 类型是否为文件，如果不是文件则触发 panic
    if(node->type != TYPE_FILE) {
        panic("Cannot read a directory!\n");
    }
    // 获取文件的大小和文件占用的块数
    int l = node->size, b = node->blocks;
    // 如果文件占用的块数小于或等于 12，则直接在 direct 数组中读取数据
    if(b <= 12) {
       int i;
       for(i = 0; i < b; i ++) {
           // 获取当前块的地址
           char *src = (char *)getBlockAddr(node->direct[i]);
           // 拷贝大小判断，大于一页取4096
           int copySize = l >= 4096 ? 4096 : l;
           // 将数据从块中复制到 buf 中
           copyByteToBuf(src, buf, copySize);
           // 更新 buf 指针，指向下一个写入的位置
           buf += copySize;
           // 更新剩余大小
           l -= copySize;
       }
    } else {
        // 如果文件占用的块数大于 12，先处理前 12 块
        int i;
        // 同上
        for(i = 0; i < 12; i ++) {
            char *src = (char *)getBlockAddr(node->direct[i]);
            int copySize = l >= 4096 ? 4096 : l;
            copyByteToBuf(src, buf, copySize);
            buf += copySize;
            l -= copySize;
        }
        // 获取间接块地址
        uint32 *indirect = (uint32 *)getBlockAddr(node->indirect);
        // 处理所有间接块数据
        for(i = 0; i < b-12; i ++) {
            char *src = (char *)getBlockAddr(indirect[i]);
            int copySize = l >= 4096 ? 4096 : l;
            copyByteToBuf(src, buf, copySize);
            buf += copySize;
            l -= copySize;
        }
    }
}















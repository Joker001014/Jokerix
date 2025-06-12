/************************ 打包 Image 镜像文件 ***************************
 * Author：Joker001014
 * 2025.03.11
***********************************************************************/

/*
 * mksfs.c 用于将一个文件夹作为根目录打包成一个 SimpleFS 镜像文件
 * 
 * SimpleFS 镜像组成如下：
 * +-------+------+     +------+------+------+     +------+
 * | Super | Free | ... | Free | Root |Other | ... |Other |
 * | Block | Map  |     | Map  |Inode |Inode |     |Inode |
 * +-------+------+     +------+------+------+     +------+
 */

#include "types.h"
#include "simplefs.h"
// 标准库
#include <stdio.h>
#include <dirent.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <string.h>

// 总块数 256 块，大小为 1M （256 * 4k = 2^20)
#define BLOCK_NUM       256
// Freemap 块的个数
#define FREEMAP_NUM     1

// 定义 Image 字节数组，它的大小和文件系统一致
// 所有的块都首先写入 Image 中，最后再将 Image 保存成文件
// 最终的镜像数据
char Image[BLOCK_SIZE * BLOCK_NUM];

// 临时的 freemap，最后需要写入 Image，此时一个 char 代表一块
char freemap[BLOCK_NUM];
uint32 freenum = BLOCK_NUM;

// 被打包的文件夹名称
char *rootdir = "rootfs";

void walk(char *dirName, Inode *nowInode, uint32 nowInodeNum);
uint64 getBlockAddr(int blockNum);
int getFreeBlock();
void copyInodeToBlock(int blockNum, Inode *in);

// 初始化各个块，并写入 Image 中
void
main()
{
    // 最开始的几块分别是 超级块，freemap 块 和 root 文件夹所在的 inode
    freemap[0] = 1;
    int i;
    // 设置 freemap 块为已占用
    for(i = 0; i < FREEMAP_NUM; i ++) freemap[1+i] = 1;
    freemap[FREEMAP_NUM + 1] = 1;   // 设置根目录块为已占用
    freenum -= (FREEMAP_NUM + 2);   // 更新空闲块数量
    
    // 填充 superblock 信息
    SuperBlock spBlock;
    spBlock.magic = MAGIC_NUM;              // 魔数
    spBlock.blocks = BLOCK_NUM;             // 文件系统总块数
    spBlock.freemapBlocks = FREEMAP_NUM;    // 空闲块数
    char *info = "SimpleFS By JokerDebug";  // 文件系统信息
    for(i = 0; i < strlen(info); i ++) {
        spBlock.info[i] = info[i];
    }
    spBlock.info[i] = '\0';
    
    // 设置根 inode
    Inode rootInode;
    rootInode.size = 0;         // 目录文件大小为0
    rootInode.type = TYPE_DIR;  // 文件类型为目录
    rootInode.filename[0] = '/'; rootInode.filename[1] = '\0';  // 根目录文件名
    // 递归遍历根文件夹，并设置和填充数据

    // 递归遍历文件夹，为每个文件和文件夹创建 Inode 并填充信息
    walk(rootdir, &rootInode, FREEMAP_NUM+1);

    spBlock.unusedBlocks = freenum; // 设置超级块中未使用的块数

    // 将超级块写入 Image
    char *ptr = (char *)getBlockAddr(0), *src = (char *)&spBlock;
    for(i = 0; i < sizeof(spBlock); i ++) {
        ptr[i] = src[i];
    }

    // 将 freemap 写入 Image
    ptr = (char *)getBlockAddr(1);
    for(i = 0; i < BLOCK_NUM/8; i ++) {
        char c = 0;
        int j;
        for(j = 0; j < 8; j ++) {
            if(freemap[i*8+j]) {
                c |= (1 << j);
            }
        }
        *ptr = c;
        ptr ++;
    }

    // 将 rootInode 写入 Image
    copyInodeToBlock(FREEMAP_NUM+1, &rootInode);

    // 将 Image 写到磁盘上
    FILE *img = fopen("fs.img", "w+b");
    fwrite(Image, sizeof(Image), 1, img);
    fflush(img); fclose(img);
}

/* 根据块号获取 Image 中的块的起始地址 */
uint64
getBlockAddr(int blockNum) {
    void *addr = (void *)Image;         // Image起始地址
    addr += (blockNum * BLOCK_SIZE);    // 加上偏移
    return (uint64)addr;
}

// 递归遍历文件夹，为每个文件和文件夹创建 Inode 并填充信息，将文件的 inode 写入磁盘块
// dirName 当前文件夹名，nowInode 为当前文件夹的 Inode，nowInodeNum 为其 Inode 号
void
walk(char *dirName, Inode *nowInode, uint32 nowInodeNum)
{
    // 打开当前文件夹
    DIR *dp = opendir(dirName);
    struct dirent *dirp;

    // 文件夹下第一个文件为其自己
    nowInode->direct[0] = nowInodeNum;
    if(!strcmp(dirName, rootdir)) {     // 判断是否为根目录
        // 若在根目录，则无上一级，上一级文件夹也为其自己
        nowInode->direct[1] = nowInodeNum;
    }
    // 下一个文件的序号
    int emptyIndex = 2;     // 初始化空闲位置索引，从第 2 个位置开始

    // 遍历当前文件夹下所有文件，创建 Inode 并填充信息
    while((dirp = readdir(dp))) {
        if(!strcmp(dirp->d_name, ".") || !strcmp(dirp->d_name, "..")) {
            // 跳过 . 和 ..
            continue;
        }
        int blockNum;
        if(dirp->d_type == DT_DIR) {
            // 文件夹处理，递归遍历
            Inode dinode;
            dinode.size = 0;            // 设置文件夹 inode 大小为 0
            dinode.type = TYPE_DIR;     // 设置文件夹类型
            int i;
            for(i = 0; i < strlen(dirp->d_name); i ++) {
                dinode.filename[i] = dirp->d_name[i];   // 将文件夹名称复制到 inode 的 filename
            }
            dinode.filename[i] = '\0';
            blockNum = getFreeBlock();          // 获取一个空闲的块来存储文件夹
            // 文件夹的前两个文件分别为 . 和 ..
            dinode.direct[0] = blockNum;        // 当前文件夹
            dinode.direct[1] = nowInodeNum;     // 父文件夹
            char *tmp = (char *)malloc(strlen(dirName) + strlen(dirp->d_name) + 1);
            sprintf(tmp, "%s/%s", dirName, dirp->d_name);   // 拼接文件夹的完整路径
            walk(tmp, &dinode, blockNum);                   // 递归处理子文件夹

            copyInodeToBlock(blockNum, &dinode);            // 将文件夹的 inode 写入磁盘块
        } else if(dirp->d_type == DT_REG) {
            // 普通文件处理
            Inode finode;
            finode.type = TYPE_FILE;
            int i;
            for(i = 0; i < strlen(dirp->d_name); i ++) {
                finode.filename[i] = dirp->d_name[i];       // 将文件名称复制到 inode 的 filename
            }
            finode.filename[i] = '\0';
            char *tmp = (char *)malloc(strlen(dirName) + strlen(dirp->d_name) + 1);
            sprintf(tmp, "%s/%s", dirName, dirp->d_name);   // 拼接文件的完整路径
            // 获取文件信息
            struct stat buf;
            stat(tmp, &buf);            // 获取文件的状态信息
            finode.size = buf.st_size;  // 设置文件大小
            finode.blocks = (finode.size - 1) / BLOCK_SIZE + 1; // 计算文件所占的块数
            
            blockNum = getFreeBlock();  // 获取一个空闲的块来存储文件数据

            // 将文件数据复制到对应的块
            uint32 l = finode.size;         // 剩余未拷贝的大小
            int blockIndex = 0;
            FILE *fp = fopen(tmp, "rb");    // 以二进制读取文件
            while(l) {
                int ffb = getFreeBlock();                   // 获取空闲块
                char *buffer = (char *)getBlockAddr(ffb);   // 获取块的地址
                size_t size;
                if(l > BLOCK_SIZE) size = BLOCK_SIZE;       // 如果剩余数据大于块大小，则取块大小
                else size = l;                              // 否则剩余的大小为实际大小
                fread(buffer, size, 1, fp);                 // 将文件内容读取到 buffer
                l -= size;                                  // 减少剩余大小
                if(blockIndex < 12) {
                    finode.direct[blockIndex] = ffb;        // 前 12 块直接存储数据
                } else {
                    // 12 个间接块均已使用
                    if(finode.indirect == 0) {
                        finode.indirect = getFreeBlock();   // 如果间接块未分配，分配一个空闲块
                    }
                    uint32 *inaddr = (uint32 *)getBlockAddr(finode.indirect);   // 获取间接块的地址
                    inaddr[blockIndex - 12] = ffb;          // 将块号保存到间接块中（共可存储1024个块，减去12的直接块偏移）
                }
                blockIndex ++;                              // 增加块索引
            }
            fclose(fp);                                     // 关闭文件
            copyInodeToBlock(blockNum, &finode);            // 将文件的 inode 写入磁盘块
        } else {
            continue;   // 跳过其他类型的文件
        }
        
        // 更新当前文件夹 nowInode 的信息
        if(emptyIndex < 12) {
            // 如果直接块未满，直接将块号存储到 nowInode 的 direct 数组中
            nowInode->direct[emptyIndex] = blockNum;
        } else {
            if(nowInode->indirect == 0) {
                nowInode->indirect = getFreeBlock();    // 如果间接块未分配，分配一个空闲块
            }
            uint32 *inaddr = (uint32 *)getBlockAddr(nowInode->indirect);    // 获取间接块的地址
            inaddr[emptyIndex - 12] = blockNum;         // 将块号存储到间接块中
        }
        emptyIndex ++;              // 增加空闲位置索引
    }
    closedir(dp);                   // 关闭当前文件夹
    nowInode->blocks = emptyIndex;  // 更新当前文件夹所占的块数
}

// 从 freemap 中找到一个空闲的块，将其标记为占用并返回其块号
int getFreeBlock() {
    int i;
    // 遍历freemap标志位
    for(i = 0; i < BLOCK_NUM; i ++) {
        if(!freemap[i]) {
            freemap[i] = 1;     // 标记为占用
            freenum --;         // 更新空闲块数
            return i;
        }
    }
    printf("get free block failed!\n");
    exit(1);
}

// 将 Inode 存储在 block 中
void copyInodeToBlock(int blockNum, Inode *in) {
    /* 根据块号获取 Image 中的块的起始地址 */
    Inode *dst = (Inode *)getBlockAddr(blockNum);
    *dst = *in;
}
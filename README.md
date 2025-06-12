# Jokerix

![linux](http://img.shields.io/badge/-Linux-FCC624?style=flat-square&logo=linux&logoColor=ffffff)
![c](http://img.shields.io/badge/-C-A8B9CC?style=flat-square&logo=c&logoColor=ffffff)
![asm](http://img.shields.io/badge/-ASM-6E4C37?style=flat-square&logo=assemblyscript&logoColor=ffffff)
![Ubuntu](http://img.shields.io/badge/-Ubuntu-E95420?style=flat-square&logo=ubuntu&logoColor=ffffff)


从零开始编写操作系统

> 相信每个自学操作系统的同学，大致学习路线都离不开 HIT-OS、MIT-6.S081、MIT-6.824、MIT-6.828等经典的公开课。但学习完这些经典公开课并完成相应的Lab，很多同学脑海中对于操作系统的知识其实都是零散的，让你从头开始编写一个操作系统，我相信大部分人还是无从下手。因为Lab只是修改相应的核心模块，对于整体系统的组织、模块间的处理等细节，往往没有人去关注，也就是说我们还需要进一步把这些概念串起来、巩固起来。那么，我相信大部分人都有过一个想法：“**我能不能自己写一个操作系统**”，这可能是大部分操作系统开发人员的梦想吧。

> 因此！本项目将展示如何从零开始使用 ANSI C 编写出一个基于 64 位 RISC-V 架构的操作系统——**Jokerix**，该系统支持在内核上运行用户态（User/Application mode）的终端，并输入命令执行其他程序。




## 具体各个实验都提供了非常详细的讲解注释：

**目录**：
- 0 前置知识 [【Create my OS】0 前置知识 | JokerDebug (joker001014.github.io)](https://joker001014.github.io/blog/025_Create_OS_0/)
- 1 最小内核 [【Create my OS】1 最小内核 | JokerDebug (joker001014.github.io)](https://joker001014.github.io/blog/026_Create_OS_1/)

- 2 开启中断 [【Create my OS】2 开启中断 | JokerDebug (joker001014.github.io)](https://joker001014.github.io/blog/027_Create_OS_2/)

- 3 内存管理 [【Create my OS】3 内存管理 | JokerDebug (joker001014.github.io)](https://joker001014.github.io/blog/028_Create_OS_3/)
- 4 虚拟内存 [【Create my OS】4 虚拟内存 | JokerDebug (joker001014.github.io)](https://joker001014.github.io/blog/029_Create_OS_4/)
- 5 内核线程 [【Create my OS】5 内核线程 | JokerDebug (joker001014.github.io)](https://joker001014.github.io/blog/030_Create_OS_5/)
- 6 线程调度 [【Create my OS】6 线程调度 | JokerDebug (joker001014.github.io)](https://joker001014.github.io/blog/031_Create_OS_6/)
- 7 用户线程 [【Create my OS】7 用户线程 | JokerDebug (joker001014.github.io)](https://joker001014.github.io/blog/032_Create_OS_7/)
- 8 文件系统 [【Create my OS】8 文件系统 | JokerDebug (joker001014.github.io)](https://joker001014.github.io/blog/033_Create_OS_8/)
- 9 实现终端 [【Create my OS】9 实现终端 | JokerDebug (joker001014.github.io)](https://joker001014.github.io/blog/034_Create_OS_9/)


---

ps若无法科学上网的同学，可参考 CSDN 博文（与上述个人博客内容一致）：
- [【哈工大_操作系统实验】Lab1 熟悉实验环境](https://blog.csdn.net/weixin_53159274/article/details/137797821?spm=1001.2014.3001.5501)
- [【哈工大_操作系统实验】Lab2 操作系统的引导](https://blog.csdn.net/weixin_53159274/article/details/142370848?spm=1001.2014.3001.5501)
- [【哈工大_操作系统实验】Lab3 系统调用](https://blog.csdn.net/weixin_53159274/article/details/142684778?spm=1001.2014.3001.5501)
- [【哈工大_操作系统实验】Lab4 进程运行轨迹的跟踪与统计](https://blog.csdn.net/weixin_53159274/article/details/142732749?spm=1001.2014.3001.5501)
- [【哈工大_操作系统实验】Lab5 基于内核栈切换的进程切换](https://blog.csdn.net/weixin_53159274/article/details/143029437?spm=1001.2014.3001.5501)
- [【哈工大_操作系统实验】Lab6 信号量的实现和应用](https://blog.csdn.net/weixin_53159274/article/details/143059098?spm=1001.2014.3001.5501)
- [【哈工大_操作系统实验】Lab7 地址映射与共享](https://blog.csdn.net/weixin_53159274/article/details/143059233?spm=1001.2014.3001.5501)
- [【哈工大_操作系统实验】Lab8 终端设备的控制](https://blog.csdn.net/weixin_53159274/article/details/143158944?spm=1001.2014.3001.5501)
- [【哈工大_操作系统实验】Lab9 proc文件系统的实现](https://blog.csdn.net/weixin_53159274/article/details/143194730?spm=1001.2014.3001.5501)



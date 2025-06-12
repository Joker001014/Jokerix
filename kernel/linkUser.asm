# 目前我们的操作系统还没有文件系统，文件系统会在下一节讲解。
# 我们如果想在操作系统中运行上一节编写的用户程序，就只能暂时把它和内核合并在一起，
# 这样在最开始 OpenSBI 就会将内核和应用程序一并加载到内存中了。
# 具体的做法就是将编译出的目标文件直接链接到 .data 段，一个字节都不改动


# 将用户程序链接到 .data 段

.section .data
    .global _user_img_start
    .global _user_img_end
_user_img_start:
    .incbin "User"
_user_img_end:
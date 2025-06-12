# Makefile

K=kernel
U=user

# 后续添加的源文件需要在这里添加，否则不会参与连接
OBJS = 					\
	$K/entry.o  		\
	$K/main.o			\
	$K/sbi.o 			\
	$K/printf.o 		\
	$K/interrupt.o 		\
	$K/timer.o 			\
	$K/heap.o 			\
	$K/memory.o 		\
	$K/mapping.o 		\
	$K/thread.o 		\
	$K/processor.o 		\
	$K/rrscheduler.o 	\
	$K/syscall.o		\
	$K/elf.o			\
	$K/string.o		 	\
	$K/fs.o 			\
	$K/queue.o			\
	$K/condition.o		\
	$K/stdin.o			\

# UPROS =                        \
# 	$U/entry.o                \
# 	$U/malloc.o                \
# 	$U/io.o                    \
# 	$U/hello.o

# 依赖
UPROSBASE =           		\
	$U/entry.o              \
	$U/malloc.o             \
	$U/io.o              	\

# 用户编写的用户程序
UPROS =                     \
	hello                   \
	hello2					\
	echo					\
	sh 						\

# 设置交叉编译工具链
TOOLPREFIX := riscv64-linux-gnu-
# $(shell uname) 会执行 uname 命令，返回当前操作系统的名称，如果为Darwin(即macOS)则执行以下语句
ifeq ($(shell uname),Darwin)
	TOOLPREFIX=riscv64-unknown-elf-
endif
CC = $(TOOLPREFIX)gcc
AS = $(TOOLPREFIX)gas
LD = $(TOOLPREFIX)ld
OBJCOPY = $(TOOLPREFIX)objcopy
OBJDUMP = $(TOOLPREFIX)objdump

# QEMU 虚拟机
QEMU = qemu-system-riscv64

# gcc 编译选项
# 开启warning、将警告当成错误处理、O1优化、保留函数调用栈指针、产生GDB所需的调试信息
CFLAGS = -Wall -Werror -O -fno-omit-frame-pointer -ggdb
# 在编译过程中生成依赖文件
CFLAGS += -MD
# 设置代码模型为 medany，要求程序和相关符号都被定义在 2 GB 的地址空间中
CFLAGS += -mcmodel=medany
# 设置环境为Freestanding（不一定以main为入口）、未初始化全局变量放在bss段、链接时不使用标准库、减少获取符号地址所需的指令数
CFLAGS += -ffreestanding -fno-common -nostdlib -mno-relax
CFLAGS += -I.
# 关闭 gcc 的栈溢出保护机制
CFLAGS += $(shell $(CC) -fno-stack-protector -E -x c /dev/null >/dev/null 2>&1 && echo -fno-stack-protector)

# ld 链接选项
LDFLAGS = -z max-page-size=4096

# QEMU 启动选项
# 通过 `-bios` 指定 Bootloader 为 default 时默认使用为 OpenSBI
# -device loader 表示将后面的内容直接加载到内存中的某个地址处，并不做其他动作。这里我们加载的文件为 Image，加载到 0x80200000
QEMUOPTS = -machine virt -bios default -device loader,file=Image,addr=0x80200000 --nographic

all: Image

Image: Kernel

# 链接
Kernel: User $(subst .c,.o,$(wildcard $K/*.c)) $(subst .S,.o,$(wildcard $K/*.S))
	$(LD) $(LDFLAGS) -T $K/kernel.ld -o $K/Kernel $(OBJS) # 生成 elf 格式目标文件
	$(OBJCOPY) $K/Kernel -O binary Image  # 生成二进制文件



# User: $(subst .c,.o,$(wildcard $U/*.c))
# 	$(LD) $(LDFLAGS) -o $U/User $(UPROS)
# 	cp $U/User User
# 将每一个用户程序都编译成一个可执行文件
User: mksfs $(subst .c,.o,$(wildcard $U/*.c))
	mkdir -p rootfs/bin
	for file in $(UPROS); do                                            \
		$(LD) $(LDFLAGS) -o rootfs/bin/$$file $(UPROSBASE) $U/$$file.o;    \
	done
	./mksfs

# 编译打包工具
mksfs:
	gcc mkfs/mksfs.c -o mksfs
	
# compile all .c file to .o file
$K/%.o: $K/%.c    # kernel/目录下的所有.o文件  和 所有.c文件
	$(CC) $(CFLAGS) -c $< -o $@

$U/%.o: $U/%.c
	$(CC) $(CFLAGS) -c $< -o $@

# compile all .S file to .o file
$K/%.o: $K/%.S    # kernel/目录下的所有.o文件  和 所有.s文件
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f */*.d */*.o $K/Kernel Image Image.asm mksfs fs.img

# riscv64-linux-gnu-objdump -x kernel
asm: Kernel
	$(OBJDUMP) -S $K/Kernel > Image.asm

qemu: Image
	$(QEMU) $(QEMUOPTS)


GDBPORT = $(shell expr `id -u` % 5000 + 25000)
QEMUGDB = $(shell if $(QEMU) -help | grep -q '^-gdb'; \
	then echo "-gdb tcp::$(GDBPORT)"; \
	else echo "-s -p $(GDBPORT)"; fi)

qemu-gdb: Image asm
	$(QEMU) $(QEMUOPTS) -S $(QEMUGDB)
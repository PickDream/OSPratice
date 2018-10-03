#include <asm.h>

# Start the CPU: switch to 32-bit protected mode, jump into C
# BIOS 从硬盘的第一扇区初始化这份代码到物理地址 0x7c00的地方,此时CPU是实模式
# 此时CS与IP寄存器的状态 %cs=0 %ip=7c00.

# .set是起到定义宏的作用，在这里预先定义个几个宏
.set PROT_MODE_CSEG,        0x8                     # 内核代码段描述符地址，在GDT表建立之后会使用
.set PROT_MODE_DSEG,        0x10                    # 内核数据段描述符地址，在GDT表建立之后会使用
.set CR0_PE_ON,             0x1                     # CR0控制寄存器最后一位设置为0，代表进入保护模式

# start 开始的地方在实模式下的 0:7c00
# start 是一个符号，符号在汇编程序表示一个地址。 .globl 告诉汇编器，之后的start标号是全局的，是会被连接器用到的
.globl start
# start符号代指的程序开始
start:
.code16                                             # 伪指令，标明使用16位汇编
    cli                                             # cl (clear) i(interupt) 清除标志寄存器的中断标志位，不再接受中断
    cld                                             # d(direction) 串操作使得地址自动增加

    # 将所有重要的寄存器全部清零 (DS, ES, SS).
    xorw %ax, %ax                                   # 抑或操作设置为0
    movw %ax, %ds                                   # -> AT&A 语法 从左到右 设置Data Segment
    movw %ax, %es                                   # -> 设置Extra Segment
    movw %ax, %ss                                   # -> 设置Stack Segment

    # 为A20信号使能,在8086CPU只有A0~A19号地址线，超出会取余，之后推出可以寻址16MB空间的CPU
    # 但是为了兼容，它提供可选择开启A20的功能，开启A20就意味着开启了其他所有的地址线，当时的
    # 8042键盘控制器恰好有空闲的端口引脚，就用空闲引脚作为A20开启信号。
    # 首先是开启A20，根据上文我们知道需要将第20位为1即可，但是我们需要知道在UCore里是如何将A20置为1的。
    # 根据说明书我们可以知道，A20地址线由键盘控制器8042进行控制，我们的A20所对应的是8042里面的P21引脚，所以问题就变成了我们需要将P21引脚置1。
    # 对于8042芯片来说，有两个端口地址60h和64h。对于这两个端口来说，0x64用来发送一个键盘控制命令，0x60用来传递参数，所以将P21引脚置1的操作就变成了，我们首先利用0x64输入一个写入的指令，然后由0x60读进去相应的参数来将P21置1。由以下的资料我们可以知道，我们首先要先向64h发送0xd1的指令，然后向60h发送0xdf的指令。
    # A20的步骤是，在键盘缓冲区为空的前提下，1.发送0xd1命令到0x64端口(seta20.1)2.发送0xdf到0x60(seta20.2)
seta20.1:
    inb $0x64, %al                                  # 在发送开启A20的命令之前，要等待键盘缓冲区为空，通过检测0x64的第二个比特是否为1，如果为0代表缓存区为空
    testb $0x2, %al                                 # 测试第二位，如果8024值不为0，就需要重新检测
    jnz seta20.1

    movb $0xd1, %al                                 # 0xd1 -> port 0x64
    outb %al, $0x64                                 # 将0xd1送到0x64口（开启A20的第一个步骤）

seta20.2:
    inb $0x64, %al                                  # 再次检测.
    testb $0x2, %al
    jnz seta20.2

    movb $0xdf, %al                                 # 将0xdf发生到0x60，来开启对应的A20
    outb %al, $0x60                                 # 0xdf = 11011111, means set P2's A20 bit(the 1 bit) to 1

    # 从实模式切换到保护模式，需要指定一个临时的GDT(全局描述符)表，加载一个已经设置好的GDT表
    # 这个表的地址是gdtdesc 标号下的。(在代码末尾有定义)

    # l(load) gdt 加载gdtr寄存器，LGDT指令加载48位，高16位表示表的最后一个子字节的偏移（表的大小-1）
    # 低32位表示起始的地址
    lgdt gdtdesc                                   
    movl %cr0, %eax                                 # 将CR0的值先保存到eax
    orl $CR0_PE_ON, %eax                            # 之后设置保护模式开启位
    movl %eax, %cr0                                 # 重新赋值CR0寄存器，至此，开启保护模式

    # 跳转到下一条指令，但是是使用保护模式32位下的，前半部分指明寻找第二个GDT表项
    ljmp $PROT_MODE_CSEG, $protcseg                 # 使用长跳转，对应GDT表项的BASE为0

.code32                                             # 32位汇编模式
protcseg:
    # 建立在保护模式下的数据段寄存器的值，段寄存器是16位的
    movw $PROT_MODE_DSEG, %ax                       # 初始化除代码段的其他段为数据段
    movw %ax, %ds                                   # -> DS: Data Segment
    movw %ax, %es                                   # -> ES: Extra Segment
    movw %ax, %fs                                   # -> FS
    movw %ax, %gs                                   # -> GS
    movw %ax, %ss                                   # -> SS: Stack Segment

    # 设置堆栈部分. 堆栈区域从 0--start(0x7c00)
    movl $0x0, %ebp
    movl $start, %esp
    call bootmain

    # If bootmain returns (it shouldn't), loop.
spin:
    jmp spin

# Bootstrap GDT
.p2align 2                                          # 强制使用4字节对齐，而一个GDT表项就是4字节
# GDT中使用了宏是在asm.h中定义的
gdt:
    SEG_NULLASM                                     # 全为0的GDT表项
    SEG_ASM(STA_X|STA_R, 0x0, 0xffffffff)           # bootloader和kernel的代码段，可读可执行
    SEG_ASM(STA_W, 0x0, 0xffffffff)                 # bootloader和kernel的数据段, 可写

gdtdesc:
    .word 0x17                                      # sizeof(gdt) - 1 (0x17 = 23 = 3*8 - 1)
    .long gdt                                       # 指定GDT表存在的位置

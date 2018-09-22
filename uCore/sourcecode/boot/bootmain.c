#include <defs.h>
#include <x86.h>
#include <elf.h>

/* *********************************************************************
 * 这是一个很简单的引导加载程序, 他的工作就是引导来自第一个IDE
 * 硬盘的ELF文件格式的内核镜像
 *
 * 磁盘布局
 *  * 程序(bootasm.S and bootmain.c)是 bootloader.
 *    他们被存储在磁盘的第一个扇区
 *
 *  * 第二个扇区之后是内核镜像
 *
 *  * 内核镜像必须是ELF文件格式的
 *
 * 启动的步骤
 *  * 当CPU启动后它会架子啊BIOS到内存中并且执行它
 *
 *  * BOS初始化设备，设置中断向量，并且读启动设备的第一个扇区
 * 
 *
 *  * 加载在第硬盘第一个扇区的bootloader加载
 *
 *  * bootasm.S控制启动保护模式，设置堆栈，为C代码之后的执行做准备工作，之后运行bootmain()
 *
 *  * 在这个文件的bootmain()工作完成之后，加载内核到内存，之后将控制权转交给内核
 * */

#define SECTSIZE        512
#define ELFHDR          ((struct elfhdr *)0x10000)      // scratch space

/* 等待硬盘空闲,*/
static void
waitdisk(void) {
    /*读I/O地址0x1f7 等待磁盘准备好*/
    while ((inb(0x1F7) & 0xC0) != 0x40)
        /* 如果忙则做空操作*/;
}

/** readsect - 读一个扇区从通过扇区号@secno读入内存地址@dst
 * 需要阅读访问硬盘的LBA和CHS方式，在这里使用LBA方式
 * CHS 是一个三元组，构成24位的，前10位是cylinder是磁道，中间8位是head,是磁头，后6位是sector
 * 表示扇区,而LBA采用48位寻址，最大寻址空间位128PB
 * 读一个扇区的大致流程如下：等待准备好->发送读取指令(相关参数)->等待读入硬盘缓存区->读入内存
 */
static void
readsect(void *dst, uint32_t secno) {
    //等待磁盘准备好
    waitdisk();

    outb(0x1F2, 1);                           // 表明要读写几个扇区。最小是1个扇区
    outb(0x1F3, secno & 0xFF);                //拿到0-7位
    outb(0x1F4, (secno >> 8) & 0xFF);         //拿到8-15位
    outb(0x1F5, (secno >> 16) & 0xFF);        //拿到16-23位
    outb(0x1F6, ((secno >> 24) & 0xF) | 0xE0);//拿到24-27位，高位全部置为一
    outb(0x1F7, 0x20);                        // cmd 0x20 - 读取扇区指令

    //等待磁盘完成操作
    waitdisk();

    // 按照双字读取文件到目标地址，硬盘有数据缓存
    insl(0x1F0, dst, SECTSIZE / 4);
}

/* *
 * readseg @va 指明了链接地址，@count指明了读多少，而@offset则指明了从文件开头偏移多少去读
 * 可能会多度，主要是因为最终调用的是readsect函数去读取
 * 下面对一些概念做一个解释，以便于更好的理解
 * */
static void
readseg(uintptr_t va, uint32_t count, uint32_t offset) {
    //找到内存中加载地址的末端
    uintptr_t end_va = va + count;

    //将起始地址进行对齐读入，头部可能会多，但整体的地址不会收到影响
    //offset偏移是从文件开始位置进行的偏移
    va -= offset % SECTSIZE;

    // 将字节数转换为扇区的个数; 由于内核可执行程序是在第二个扇区开始的，因此需要+1
    uint32_t secno = (offset / SECTSIZE) + 1;

    // 如果太慢了，就一次多读几个扇区         
    // 通过递增扇区号，将程序头部指明的节全部读取到内存中来
    // 需要注意的是，这一部分代码可能会多读一部分，但是没有什么问题 --
    for (; va < end_va; va += SECTSIZE, secno ++) {
        readsect((void *)va, secno);
    }
}

/* bootmain - bootloader的入口 */
void
bootmain(void) {
    // 将文件的前4KB读取内存
    readseg((uintptr_t)ELFHDR, SECTSIZE * 8, 0);

    // 检测加载进内存的内核模块是否是ELF文件
    if (ELFHDR->e_magic != ELF_MAGIC) {  
        goto bad;
    }

    struct proghdr *ph, *eph;

    // 通过结构体的偏移找到程序头表的入口，并加载程序段(ignores ph flags)
    ph = (struct proghdr *)((uintptr_t)ELFHDR + ELFHDR->e_phoff);
    // 通过指针的偏移,找到程序头表下的具体节的地址，
    eph = ph + ELFHDR->e_phnum;
    //开始对程序头表包含的具体的信息进行遍历处理
    for (; ph < eph; ph ++) {
        //用readreg这个函数将文件的每一个段都读到内存中由程序头表的相应位置
        //由于kernel 程序的链接地址太高，而实际内存没有那么大的情况下，实际地址就是&0xFFFFFF的结果(Mark，这个还不是太懂)
        readseg(ph->p_va & 0xFFFFFF, ph->p_memsz, ph->p_offset);
    }

    // 调用elf文件格式指明的程序入口点地址
    // note: 不会返回
    ((void (*)(void))(ELFHDR->e_entry & 0xFFFFFF))();

/*加载失败跳转到这里*/
bad:
    outw(0x8A00, 0x8A00);
    outw(0x8A00, 0x8E00);

    /* 无限循环 */
    while (1);
}


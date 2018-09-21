#include <defs.h>
#include <x86.h>
#include <elf.h>

/* *********************************************************************
 * This a dirt simple boot loader, whose sole job is to boot
 * an ELF kernel image from the first IDE hard disk.
 *
 * DISK LAYOUT
 *  * This program(bootasm.S and bootmain.c) is the bootloader.
 *    It should be stored in the first sector of the disk.
 *
 *  * The 2nd sector onward holds the kernel image.
 *
 *  * The kernel image must be in ELF format.
 *
 * BOOT UP STEPS
 *  * when the CPU boots it loads the BIOS into memory and executes it
 *
 *  * the BIOS intializes devices, sets of the interrupt routines, and
 *    reads the first sector of the boot device(e.g., hard-drive)
 *    into memory and jumps to it.
 *
 *  * Assuming this boot loader is stored in the first sector of the
 *    hard-drive, this code takes over...
 *
 *  * control starts in bootasm.S -- which sets up protected mode,
 *    and a stack so C code then run, then calls bootmain()
 *
 *  * bootmain() in this file takes over, reads in the kernel and jumps to it.
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
 * 读一个扇区的大致流程如下：
 * 1. 等待磁盘准备好
 * 2. 发出读取磁盘的命令
 * 3. 等待磁盘准备好
 * 4. 把磁盘扇区数据读到指定内存
 */
static void
readsect(void *dst, uint32_t secno) {
    //等待磁盘准备好
    waitdisk();

    outb(0x1F2, 1);                         // 表明要读写几个扇区。最小是1个扇区
    outb(0x1F3, secno & 0xFF);
    outb(0x1F4, (secno >> 8) & 0xFF);
    outb(0x1F5, (secno >> 16) & 0xFF);
    outb(0x1F6, ((secno >> 24) & 0xF) | 0xE0);
    outb(0x1F7, 0x20);                      // cmd 0x20 - 读取扇区指令

    //等待磁盘完成操作
    waitdisk();

    // 按照双字读取文件到目标地址，硬盘有数据缓存
    insl(0x1F0, dst, SECTSIZE / 4);
}

/* *
 * readseg - read @count bytes at @offset from kernel into virtual address @va,
 * might copy more than asked.
 * */
static void
readseg(uintptr_t va, uint32_t count, uint32_t offset) {
    uintptr_t end_va = va + count;

    // round down to sector boundary
    va -= offset % SECTSIZE;

    // translate from bytes to sectors; kernel starts at sector 1
    uint32_t secno = (offset / SECTSIZE) + 1;

    // If this is too slow, we could read lots of sectors at a time.
    // We'd write more to memory than asked, but it doesn't matter --
    // we load in increasing order.
    for (; va < end_va; va += SECTSIZE, secno ++) {
        readsect((void *)va, secno);
    }
}

/* bootmain - the entry of bootloader */
void
bootmain(void) {
    // 将第一个扇区，在LBA表示号位0,的扇区读到0x10000地址的地方
    readseg((uintptr_t)ELFHDR, SECTSIZE * 8, 0);

    // 检测加载进内存的内核模块是否是ELF文件
    if (ELFHDR->e_magic != ELF_MAGIC) {
        goto bad;
    }

    struct proghdr *ph, *eph;

    // load each program segment (ignores ph flags)
    ph = (struct proghdr *)((uintptr_t)ELFHDR + ELFHDR->e_phoff);
    eph = ph + ELFHDR->e_phnum;
    for (; ph < eph; ph ++) {
        readseg(ph->p_va & 0xFFFFFF, ph->p_memsz, ph->p_offset);
    }

    // call the entry point from the ELF header
    // note: does not return
    ((void (*)(void))(ELFHDR->e_entry & 0xFFFFFF))();

bad:
    outw(0x8A00, 0x8A00);
    outw(0x8A00, 0x8E00);

    /* do nothing */
    while (1);
}


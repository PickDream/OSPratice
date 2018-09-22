/**
 * ELF文件结构体部分，用于加载
*/
#ifndef __LIBS_ELF_H__
#define __LIBS_ELF_H__

#include <defs.h>

#define ELF_MAGIC    0x464C457FU              // "\x7FELF" in little endian

/*ELF文件头部分结构体*/
struct elfhdr {
    uint32_t e_magic;     // 必须和ELF_MAGIC相等
    uint8_t e_elf[12];
    uint16_t e_type;      // 文件类型 1=relocatable, 2=executable, 3=shared object, 4=core image
    uint16_t e_machine;   // 针对的体系结构 3=x86, 4=68K, etc.
    uint32_t e_version;   // 版本信息，总是为1
    uint32_t e_entry;     // [*]可执行程序的入口点
    uint32_t e_phoff;     // 程序头表偏移量
    uint32_t e_shoff;     // 节头表偏移量
    uint32_t e_flags;     // 处理器特定标志
    uint16_t e_ehsize;    // 文件头长度
    uint16_t e_phentsize; // 程序头部长度
    uint16_t e_phnum;     // 程序头部个数
    uint16_t e_shentsize; // 节头部长度
    uint16_t e_shnum;     // 节头部个数
    uint16_t e_shstrndx;  // 节头部字符索引
};

/*ELF文件程序头表*/
struct proghdr {
    uint32_t p_type;   // 段类型
    uint32_t p_offset; // 段位置相对于文件开始处的偏移
    uint32_t p_va;     // 段在内存中的地址（虚拟地址）
    uint32_t p_pa;     // 段的物理地址
    uint32_t p_filesz; // 段在文件状态下的长度
    uint32_t p_memsz;  // 段在内存中的长度(如果包含.bss段的话就会变得更大）
    uint32_t p_flags;  // 段标志 read/write/execute bits
    uint32_t p_align;  // 段在内存中的对齐, invariably hardware page size
};

#endif /* !__LIBS_ELF_H__ */


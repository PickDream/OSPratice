#ifndef __LIBS_X86_H__
#define __LIBS_X86_H__
/**
 * 阅读x86.h需要对GCC的内联汇编有足够的了解，相关资料整理如下:
 * GCC-Inline-Assembly-HowTo http://www.ibiblio.org/gferg/ldp/GCC-Inline-Assembly-HOWTO.html
 * 有对应的中文翻译版作为参照  https://www.linuxprobe.com/gcc-how-to.html
*/
#include <defs.h>

#define do_div(n, base) ({                                        \
    unsigned long __upper, __low, __high, __mod, __base;        \
    __base = (base);                                            \
    asm("" : "=a" (__low), "=d" (__high) : "A" (n));            \
    __upper = __high;                                            \
    if (__high != 0) {                                            \
        __upper = __high % __base;                                \
        __high = __high / __base;                                \
    }                                                            \
    asm("divl %2" : "=a" (__low), "=d" (__mod)                    \
        : "rm" (__base), "0" (__low), "1" (__upper));            \
    asm("" : "=A" (n) : "a" (__low), "d" (__high));                \
    __mod;                                                        \
 })

/*函数的声明部分*/
static inline uint8_t inb(uint16_t port) __attribute__((always_inline));                    //
static inline void insl(uint32_t port, void *addr, int cnt) __attribute__((always_inline)); //
static inline void outb(uint16_t port, uint8_t data) __attribute__((always_inline));        //
static inline void outw(uint16_t port, uint16_t data) __attribute__((always_inline));       //
static inline uint32_t read_ebp(void) __attribute__((always_inline));                       //

/* Pseudo-descriptors used for LGDT, LLDT(not used) and LIDT instructions. */
/*__attribute__((packed))告诉编译器不要对结构体做对齐处理，好比这个结构体如果对齐(默认四字节)就是8字节，不对齐就是6字节*/
struct pseudodesc {
    uint16_t pd_lim;        // Limit
    uint32_t pd_base;        // Base address
} __attribute__ ((packed));

static inline void lidt(struct pseudodesc *pd) __attribute__((always_inline));
static inline void sti(void) __attribute__((always_inline));
static inline void cli(void) __attribute__((always_inline));
static inline void ltr(uint16_t sel) __attribute__((always_inline));

/*inb指令对应的函数*/
/**
 * 1. volatile 阻止编译器可能为指令做过度优化(指令重排序)
 * 2. %1,%2 是样板操作数，编译器会自动选择合适的寄存器完成1号寄存器到2号寄存器的赋值
 * 3. "d"表示先将port的值赋给al寄存器
 * 4. "=a"表示目标操作数放在al中，并且最终将al的值防止到port的地址中去
 * 5. 输入和输出都对应这样板操作数的输入操作数和输出操作数
*/
static inline uint8_t
inb(uint16_t port) {
    uint8_t data;
    asm volatile ("inb %1, %0" : "=a" (data) : "d" (port)); //
    return data;
}

/**
 * 从I/O端口port 读取cnt个长度为32位数写到内存中
*/
static inline void
insl(uint32_t port, void *addr, int cnt) {
    asm volatile (
            "cld;"                                      //清除方向标志位，代表在串操作时向高地址移动
            "repne; insl;"                              //repne 表示当ecx不为0的时候执行相应的操作,insl指令代表
            : "=D" (addr), "=c" (cnt)                   //将addr原来的值
            : "d" (port), "0" (addr), "1" (cnt)         //"0"代表这与第一个输出变量一样的约束，"1"也相同
            : "memory", "cc");                          //代表内存会发生改变"memory",控制寄存器会发生改变"cc"(由于cld的设置)
}

/**
 * 以下两个函数都是低级输出端口信息的方法，一个是输出一个字节
 * 另一个是输出一个字(两字节)
*/

static inline void
outb(uint16_t port, uint8_t data) {
    asm volatile ("outb %0, %1" :: "a" (data), "d" (port));
}

static inline void
outw(uint16_t port, uint16_t data) {
    asm volatile ("outw %0, %1" :: "a" (data), "d" (port));
}

/*读取ebp的值*/
static inline uint32_t
read_ebp(void) {
    uint32_t ebp;
    asm volatile ("movl %%ebp, %0" : "=r" (ebp));
    return ebp;
}

static inline void
lidt(struct pseudodesc *pd) {
    asm volatile ("lidt (%0)" :: "r" (pd));
}

static inline void
sti(void) {
    asm volatile ("sti");
}

/*清除中断标志位*/
static inline void
cli(void) {
    asm volatile ("cli");
}


static inline void
ltr(uint16_t sel) {
    asm volatile ("ltr %0" :: "r" (sel));
}

static inline int __strcmp(const char *s1, const char *s2) __attribute__((always_inline));
static inline char *__strcpy(char *dst, const char *src) __attribute__((always_inline));
static inline void *__memset(void *s, char c, size_t n) __attribute__((always_inline));
static inline void *__memmove(void *dst, const void *src, size_t n) __attribute__((always_inline));
static inline void *__memcpy(void *dst, const void *src, size_t n) __attribute__((always_inline));

#ifndef __HAVE_ARCH_STRCMP
#define __HAVE_ARCH_STRCMP
static inline int
__strcmp(const char *s1, const char *s2) {
    int d0, d1, ret;
    asm volatile (
            "1: lodsb;"
            "scasb;"
            "jne 2f;"
            "testb %%al, %%al;"
            "jne 1b;"
            "xorl %%eax, %%eax;"
            "jmp 3f;"
            "2: sbbl %%eax, %%eax;"
            "orb $1, %%al;"
            "3:"
            : "=a" (ret), "=&S" (d0), "=&D" (d1)
            : "1" (s1), "2" (s2)
            : "memory");
    return ret;
}

#endif /* __HAVE_ARCH_STRCMP */

#ifndef __HAVE_ARCH_STRCPY
#define __HAVE_ARCH_STRCPY
static inline char *
__strcpy(char *dst, const char *src) {
    int d0, d1, d2;
    asm volatile (
            "1: lodsb;"
            "stosb;"
            "testb %%al, %%al;"
            "jne 1b;"
            : "=&S" (d0), "=&D" (d1), "=&a" (d2)
            : "0" (src), "1" (dst) : "memory");
    return dst;
}
#endif /* __HAVE_ARCH_STRCPY */

#ifndef __HAVE_ARCH_MEMSET
#define __HAVE_ARCH_MEMSET
static inline void *
__memset(void *s, char c, size_t n) {
    int d0, d1;
    asm volatile (
            "rep; stosb;"
            : "=&c" (d0), "=&D" (d1)
            : "0" (n), "a" (c), "1" (s)
            : "memory");
    return s;
}
#endif /* __HAVE_ARCH_MEMSET */

#ifndef __HAVE_ARCH_MEMMOVE
#define __HAVE_ARCH_MEMMOVE
static inline void *
__memmove(void *dst, const void *src, size_t n) {
    if (dst < src) {
        return __memcpy(dst, src, n);
    }
    int d0, d1, d2;
    asm volatile (
            "std;"
            "rep; movsb;"
            "cld;"
            : "=&c" (d0), "=&S" (d1), "=&D" (d2)
            : "0" (n), "1" (n - 1 + src), "2" (n - 1 + dst)
            : "memory");
    return dst;
}
#endif /* __HAVE_ARCH_MEMMOVE */

#ifndef __HAVE_ARCH_MEMCPY
#define __HAVE_ARCH_MEMCPY
static inline void *
__memcpy(void *dst, const void *src, size_t n) {
    int d0, d1, d2;
    asm volatile (
            "rep; movsl;"
            "movl %4, %%ecx;"
            "andl $3, %%ecx;"
            "jz 1f;"
            "rep; movsb;"
            "1:"
            : "=&c" (d0), "=&D" (d1), "=&S" (d2)
            : "0" (n / 4), "g" (n), "1" (dst), "2" (src)
            : "memory");
    return dst;
}
#endif /* __HAVE_ARCH_MEMCPY */

#endif /* !__LIBS_X86_H__ */


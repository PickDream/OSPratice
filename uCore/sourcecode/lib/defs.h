#ifndef __LIBS_DEFS_H__
#define __LIBS_DEFS_H__

/*如果没有定义NULL这个宏的话，就将NULL设置为空指针并指向0的位置*/
#ifndef NULL
#define NULL ((void *)0)
#endif
/*inline __attribute__((always_inline)) 告诉编译器使用强制内联*/
#define __always_inline inline __attribute__((always_inline))
/*告诉编译器不要内联这个函数*/
#define __noinline __attribute__((noinline))
/*这个attribute属性主要描述的是函数，表示函数不会返回,所谓不返回，
用于编译器在检测到可能会有存在没有返回值的分支的时候，而不提示警告
相关链接:http://www.unixwiz.net/techtips/gnu-c-attributes.html*/
#define __noreturn __attribute__((noreturn))

/* bool类型设置位int */
typedef int bool;

/* 以下定义主要是将常用的整形都标记其长度 */
typedef char int8_t;
typedef unsigned char uint8_t;
typedef short int16_t;
typedef unsigned short uint16_t;
typedef int int32_t;
typedef unsigned int uint32_t;
typedef long long int64_t;
typedef unsigned long long uint64_t;

/* *
 * 定义32位的指针长度，另外定义的ptr_t为后缀的是指针
 * */
typedef int32_t intptr_t;
typedef uint32_t uintptr_t;

/* size_t 用于表示一个结构的大小 */
typedef uintptr_t size_t;

/* ppn_t 用于表示页号 */
typedef size_t ppn_t;

/* *
 * 进行舍入操作，当传入的n是2的幂次是有效的
 * 这里使用到了typeof,typeof是GNU标准的类型推导操作
 * 
 * Round down to the nearest multiple of n
 * */
#define ROUNDDOWN(a, n) ({                                          \
            size_t __a = (size_t)(a);                               \
            (typeof(a))(__a - __a % (n));                           \
        })

/* Round up to the nearest multiple of n */
#define ROUNDUP(a, n) ({                                            \
            size_t __n = (size_t)(n);                               \
            (typeof(a))(ROUNDDOWN((size_t)(a) + __n - 1, __n));     \
        })

/* 返回一个成员相对于一个结构开始的偏移，这里主要使用了指针的移位 */
#define offsetof(type, member)                                      \
    ((size_t)(&((type *)0)->member))

/* *
 * 已知一个结构指针，通过拿到该结构作为成员相对于父结构的偏移来得到父结构
 * 开始的位置
 * @ptr:    成员结构指针
 * @type:   父结构类型
 * @member: 成员结构体的名字
 * */
#define to_struct(ptr, type, member)                               \
    ((type *)((char *)(ptr) - offsetof(type, member)))

#endif /* !__LIBS_DEFS_H__ */


# OSPratice
学习操作系统做的作业与练习，作业按照章节分类
## 第二章 操作系统结构
+ 2.15 利用POSIX的API，编写从一个文件复制到另一个目标文件的程序[->转到](CH2/2.15.c)
+ 创建一个简单的内核模块
## uCore源代码阅读与实践
### boot
+ [asm.h](uCore/sourcecode/boot/asm.h) 通过宏定义了一些方便操作并且生成GDT表项的方法
+ [bootasm.S](uCore/sourcecode/boot/bootasm.S) 初始化寄存器，发送A20开始信号，修改CR0激活保护模式，初始化
### libs
+ [defs.h](uCore/sourcecode/lib/defs.h) 定义一些明确语义的数据类型，还定义一些工具函数
+ [x86.h](uCore/sourcecode/lib/x86.h) 正在学习中..
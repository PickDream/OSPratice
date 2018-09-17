/**
 * 使用POSIX API将一个文件的内容复制到另一个目标文件中
 * 作者：maoxin
 * ---------------------------------------------
 * NOTE:
 * 1. open
 *   1.1 函数原型
 *       open(const char *pathname,int flags);
 *       open(const char *pathname,int flags,mode_t mode) 
 *   1.2 open是变参函数，所以看似就像是函数重载
 *   1.3 返回文件描述符成功，返回-1失败，同时修改errno
 *   1.4 flags:
 *       必选项： O_RDONLY,O_WRONLY,O_RDWD
 *       可选项： O_CREATE(没有则创建),O_APPEND(附加),O_TRUNK(清空文件),O_NONBLOCK(设置非阻塞)
 *   1.5 mode:是权限 mode = mode&~umask
 * 2. read
 *    2.1函数原型
 *       ssize_t read(int fd,void* buf,size_t count);
 *    2.2参数
 *       ssize_t 是有符号整形,size_t 是不带符号的整形，传入一个缓冲区，指明长度
 *    2.3返回值
 *       返回实际读取的长度,-1代表失败>0代表读出的字节数，=0代表文件读完了
 * 3. write
 *    3.1 函数原型
 *        ssize_t write(int fd,const void *buf,size_t count)
 *    3.2 buf是要写入文件的数据，count是缓冲区的大小
 *    3.3 返回值
 *        -1失败,>0:写入到文件的数据
 *          
*/
#include<sys/types.h>
#include<sys/stat.h>
#include<fcntl.h>
#include<unistd.h>
#include<stdio.h>

int main(int argc,char* argv[]){
    char* source = NULL;
    char* dest =NULL;
    char buf[256]={0};
    ssize_t count = 0;

    if(argc!=3){
        fprintf(stderr,"参数不对");
        return -1;
    }
    source = argv[1];
    dest = argv[2];

    int source_fd = open(source,O_RDONLY);
    int aim_fd = open(dest,O_WRONLY|O_CREAT);
    if(source_fd==-1){
        fprintf(stderr,"源文件不存在！");
        return -2;
    }
    while((count = read(source_fd,buf,sizeof(buf)))>0){
        if(write(aim_fd,buf,count)==-1){
            fprintf(stderr,"写入文件失败");
            break;
        }
    }
    if(count==-1){
        fprintf(stderr,"读取失败");
    }
    close(source_fd);
    close(aim_fd);
    return 0;
}

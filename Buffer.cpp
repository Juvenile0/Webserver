#include<Buffer.h>
#include<unistd.h>
#include<cstring>
#include <sys/uio.h> 

using namespace webserver;

ssize_t Buffer::writeFd(int Fd,int *savedErrno){
    size_t bytes = readableBytes();
    char* bufPtr = begin() + readIndex_;
    if(ssize_t result=::write(Fd,bufPtr,bytes)<=0){
        if(result<0&& result==EINTR) //中断，返回0
            return 0;
        else {  //失败，返回-1
            printf("[Buffer:writeFd]fd = %d write : %s\n", Fd, strerror(errno));  
            *savedErrno = errno;
            return -1;
        }
    }else{
        readIndex_+=result;
        return result;
    }
    
}
/* 从fd中写入缓冲区中 */
ssize_t Buffer::readFd(int Fd,int *savedErrno){
    char extrbuffer[65536];
    struct iovec vec[2];
    size_t bytes  = writableBytes();
    vec[0].iov_base = begin()+writeIndex_;
    vec[0].iov_len = bytes;
    vec[1].iov_base = extrbuffer;
    vec[1].iov_len = sizeof(extrbuffer);
    const ssize_t result=::readv(Fd,vec,2);
    if(result<0){
        printf("[Buffer:readFd]fd = %d readv : %s\n", Fd, strerror(errno));
        *savedErrno = errno;
    }else if(static_cast<size_t>(result)<bytes){
        writeIndex_+=result;
    }else{
        writeIndex_ = buffer_.size();
        /* 超出部分再写入 */  
        append(extrbuffer,result-bytes);
    }


}
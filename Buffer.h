#ifndef __BUFFER_H__
#define __BUFFER_H__

#include <algorithm>
#include <string>
#include <cassert>
#include <vector>

namespace webserver
{

    class Buffer
    {
    public:
        /* 预留缓冲区大小和初始可用大小 */
        static const size_t PreparedSize = 8;
        static const size_t InitSize = 1024;

        /* 可读、写区、预留大小 */
        size_t writableBytes() const
        {
            return buffer_.size() - writeIndex_;
        }
        size_t readableBytes() const
        {
            return writeIndex_ - readIndex_;
        }
        size_t prepareBytes() const
        {
            return readIndex_;
        }

        /* 读、写区首地址 */
        const char* peek() const
        {
            return begin() + readIndex_;
        }
        const char* writeaddr() const
        {
            return begin() + writeIndex_;
        }

        /* 往缓冲区中读取数据 */
        void retrieveBylen(size_t len)
        {
            assert(readableBytes() > len);
            readIndex_ += len;
        }
        void retrieveByaddr(const char *end)
        {
            assert(peek() < end);
            assert(end < writeaddr());
            readIndex_ += end - peek();
        }
        void retrieveAll()
        {
            writeIndex_ = 0;
            readIndex_ = 0;
        }

        /* 往缓冲区中写数据 */
        void append(const void* data,size_t len){
            append(static_cast<const char* >(data),len);
        }
        void append(const char* data,size_t len){
            ensureEnoughbytes(len);        
            std::copy(data,data+len,writeaddr());
            writeIndex_+=len;
        }
        void append(const std::string& str){
            append(str.data(),str.length());
        }
        void append(const Buffer& otherBuffer){
            append(otherBuffer.peek(),otherBuffer.readableBytes());
        }

        /* 足够的写入空间  */
        void ensureEnoughbytes(size_t len){
            // 如果没有足够空间扩容
            if(writableBytes()<len){
                dilatation(len);
            };
            assert(writableBytes()>=len);
        }
        
        /* 查找crlf */
        const char* findCrlf() const {
            const char crlf[] = "\r\n";
            const char* result=std::search(begin(),writeaddr(),crlf,crlf+2);
            return result == writeaddr() ? nullptr:crlf;
        }
        const char* findCrlf(const char* start ) const{
            assert(peek()<start);
            assert(start<=writeaddr());
            const char crlf[] = "\r\n";
            const char* result=std::search(start,writeaddr(),crlf,crlf+2);
            return result == writeaddr() ? nullptr:crlf;
        }

        /* readFd:Fd写数据到缓冲区中,writeFd缓冲区写数据到Fd中 */
        ssize_t readFd(int Fd,int* savedErrno);
        ssize_t writeFd(int Fd,int* savedErrno);

    private:
        std::vector<char> buffer_;
        size_t writeIndex_;
        size_t readIndex_;

        /* 首地址 */
        char *begin()
        {
            return &*buffer_.begin();
        }
        const char *begin() const
        {
            return &*buffer_.begin();
        }
       
        /* 扩充空间 */
        void dilatation(size_t len){
            /* 没有足够的空间就resize */
            if(writableBytes()+prepareBytes()<len+PreparedSize){
                buffer_.resize(writeIndex_+len);
            }else{
                assert(PreparedSize<readIndex_);
                size_t rBytes = readableBytes();
                std::copy(buffer_.begin()+readIndex_,buffer_.begin()+writeIndex_,buffer_.begin()+PreparedSize);
                readIndex_ = PreparedSize;
                writeIndex_ = PreparedSize+rBytes;
                assert(rBytes==readableBytes());
            }
        }
    };

}

#endif
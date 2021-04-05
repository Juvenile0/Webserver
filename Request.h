#ifndef __REQUEST_H__
#define __REQUEST_H__

#include<Buffer.h>
#include<string>
#include<vector>
#include<map>
namespace webserver{
    
class TimeManager;
class Buffer;
class Request
{
public:
    enum Method{ Get,Post,Patch,Delete,Put,Head,Invalid};  // HTTP method
    enum Version{ Unknown, HTTP10, HTTP11}; //HTTPversion
    enum ParseStatus{       
        ParseRequest,
        ParseHeaders,
        ParseBody,
        ParseFinish}; 
    
    Request(int fd);
    ~Request();

    int fd(){return fd_;};
    int read(int* Errno);   //读数据, request.fd->inbuffer
    int write(int* Errno);  //写数据, outbuffer-> fd

    int append(Buffer otherBuffer){outBuffer_.append(otherBuffer);}; // ->outbuffer ，接受request请求后，创建response对象，将其写入outbuffer中
    int writeableBytes(){return outBuffer_.readableBytes();}    //返回outbuffer可以输出的字节数
    
    bool parse();    //分析整个报文，函数体内调用paresline
    bool isparseFinish(){ return parsestatus_==ParseFinish;};    
    void  resetparse();
    

    void setworking(){workstatus_ = true;}
    void setnowworking(){workstatus_ = false;};
    bool getworkstatus(){return workstatus_;};


    std::string getUrl() const { return url_; }
    std::string getUrlargs() const { return urlargs_; }
    std::string getHeader(const std::string& field) const;
    std::string getMethod() const;
    bool keepConnect() const; //是否保持连接
private:
    int fd_;
    Buffer inBuffer_;   //fd->buffer    request
    Buffer outBuffer_;  // buffer->fd  response

    TimeManager* tmanager_;
    bool workstatus_;   //是否处于工作状态

    /* http属性 */    
    Method method_;
    Version version_;
    ParseStatus parsestatus_;
    std::string url_;    //url 地址
    std::string urlargs_;   //url查询的参数
    std::map<std::string,std::string> headers_;  //http头

    bool parseLine(const char* begin, const char* end);
    bool setMethod(const char* begin, const char* end);
    void addHeader(const char* begin, const char* colon, const char* end);
    void setUrl(const char* begin, const char* end);
    void setUrlargs(const char* begin, const char* end);
    void setVersion(Version version){version_ =version;};
};
}
#endif
#include<Response.h>
#include<Buffer.h>


#include<string>
#include<sys/mman.h>
#include<sys/unistd.h>
#include<sys/stat.h>
#include<fcntl.h>
#include<cassert>


using namespace webserver;


const std::map<int, std::string>  Response::statusCodes = {
    {200, "OK"},
    {400, "Bad Request"},
    {403, "Forbidden"},
    {404, "Not Found"}
};

const std::map<std::string, std::string> Response::type = {
    {".html", "text/html"},
    {".xml", "text/xml"},
    {".xhtml", "application/xhtml+xml"},
    {".txt", "text/plain"},
    {".rtf", "application/rtf"},
    {".pdf", "application/pdf"},
    {".word", "application/nsword"},
    {".png", "image/png"},
    {".gif", "image/gif"},
    {".jpg", "image/jpeg"},
    {".jpeg", "image/jpeg"},
    {".au", "audio/basic"},
    {".mpeg", "video/mpeg"},
    {".mpg", "video/mpeg"},
    {".avi", "video/x-msvideo"},
    {".gz", "application/x-gzip"},
    {".tar", "application/x-tar"},
    {".css", "text/css"}
};

/*  */

Buffer Response::responseBuffer(){
    Buffer buffer;
    /* 400 */
    if(statuscode_ == 400){
        doErrorResponse(buffer,"can't parse the message");
        return buffer;
    };

    /* 找不到文件 */
    struct stat stbuffer;
    if(stat(url_.data(),&stbuffer)<0){
        statuscode_ = 404;
        doErrorResponse(buffer,"can't find the resource");
        return buffer;
    }

    /* 权限错误 */
    if(!(S_ISREG(stbuffer.st_mode) || !(S_IRUSR & stbuffer.st_mode))) {
        statuscode_= 403;
        doErrorResponse(buffer, "Swings can't read the file");
        return buffer;
    }
    staticResponse(buffer,stbuffer.st_size);
    return buffer;

};

/* 获取文件类型，用在报文中 */
std::string Response::getFileType(){
    int index=url_.find_last_of(".");
    std::string suffix;
    /* 纯文本 */
    if(index == std::string::npos){
        return "text/plan";
    }
    suffix = url_.substr(index);
    auto it = type.find(suffix);
    if(it==type.end()){
        return "text/plan";
    };
    return it->second;

};


/* 4XX*/
void Response::doErrorResponse(Buffer &buffer,std::string errorMessage){

    std::string body;
    auto it = statusCodes.find(statuscode_);
    if(it==statusCodes.end()){return;};
    body += "<html><title>Swings Error</title>";
    body += "<body bgcolor=\"ffffff\">";
    body += std::to_string(statuscode_) + " : " + it -> second + "\n";
    body += "<p>" + errorMessage + "</p>";
    body += "<hr><em>Swings web server</em></body></html>";

    // 响应行
    buffer.append("HTTP/1.1 " + std::to_string(statuscode_) + " " + it -> second + "\r\n");
    // 报文头
    buffer.append("Server: Swings\r\n");
    buffer.append("Content-type: text/html\r\n");
    buffer.append("Connection: close\r\n");
    buffer.append("Content-length: " + std::to_string(body.size()) + "\r\n\r\n");
    // 报文体
    buffer.append(body);


};


/* 访问静态资源 */
void Response::staticResponse(Buffer &buffer,long fileSize){
    assert(fileSize>0);
    auto it= statusCodes.find(statuscode_);
    if(it==statusCodes.end()){
        statuscode_ = 400;
        doErrorResponse(buffer,"unknow statuscode");
        return;
    }

        buffer.append("HTTP/1.1 " + std::to_string(statuscode_) + " " + it -> second + "\r\n");
    // 报文头
    if(keepConnect_) {
        buffer.append("Connection: Keep-Alive\r\n");
        buffer.append("Keep-Alive: timeout=" + std::to_string(CONNECT_TIMEOUT) + "\r\n");
    } else {
        buffer.append("Connection: close\r\n");
    }
    buffer.append("Content-type: " + getFileType() + "\r\n");
    buffer.append("Content-length: " + std::to_string(fileSize) + "\r\n");
    // TODO 添加头部Last-Modified: ?
    buffer.append("Server: Swings\r\n");
    buffer.append("\r\n");
    
    int fd=open(url_.data(),O_RDONLY, 0); //打开文件

    void* mfile  = mmap(NULL, fileSize, PROT_READ, MAP_PRIVATE, fd, 0);
    if(mfile == (void*)-1){ //创建失败
        munmap(mfile,fileSize);
        statuscode_ = 400;
        doErrorResponse(buffer,"can't find the file");
        return;
    };
    buffer.append(mfile,fileSize);  //写入缓冲区中
    munmap(mfile,fileSize); //清除内存
};

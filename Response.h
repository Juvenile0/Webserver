#ifndef __RESPONSE_H__
#define __RESPONSE_H__

#include<map>
#define CONNECT_TIMEOUT 500 // 非活跃连接500ms断开

/* response:当请求接受时,根据响应创建response对象，写入outbuffer中，但线程doresponse时从outbuffer中取出 */
namespace webserver{
class Buffer;
class Response
{

public:
    Response(int statuscode,std::string url,bool keepConnect)
        :statuscode_(statuscode),url_(url),keepConnect_(keepConnect){};
    ~Response(){};
    
    //访问资源对应的类型
    static const std::map<std::string,std::string> type;
    // 4XX 等状态码对应的信息
    static const std::map<int,std::string> statusCodes;


    /* 当request接收时，会创建response对象，将response写入outbuffer中，该函数是将response对象转化为buffer */
    Buffer responseBuffer();

    /* 根据错误状态码4xx 做出响应 */
    void doErrorResponse(Buffer &buffer,std::string errorMessage);
    /* 访问静态资源 */
    void staticResponse(Buffer &buffer,long fileSize);
    

private:
    std::string getFileType();

    std::string url_;   //路由
    int statuscode_;     //状态码
    std::map<std::string,std::string> headers_; //报文头
    bool keepConnect_;   //长连接
    
};




}

#endif
#include<Request.h>

#include<unistd.h>

using namespace webserver;

Request::Request(int fd):
    fd_(fd),workstatus_(false),
    method_(Invalid),version_(Unknown),
    parsestatus_(ParseRequest),tmanager_(nullptr){
        assert(fd>=0);
    }
Request::~Request(){
    close(fd_);
}


// fd->inbuffer
int Request::read(int* Errno){
    return inBuffer_.readFd(fd_,Errno);
}

//inbuffer->outbuffer
int Request::write(int* Errno){
    return outBuffer_.writeFd(fd_,Errno);
}
/* 解析请求头 */
bool Request::parse(){
 bool ok= true;
    bool hasMore=true;
    while(hasMore)
    {
        //解析请求行
        if(parsestatus_==ParseRequest)
        {
            //
            const char* crlf = inBuffer_.findCrlf();
            if(crlf)
            {
                //开始解析请求行
                ok=parseLine(inBuffer_.peek(),crlf);
                if(ok)
                {
                    //解析成功
                    //回收请求行buffer
                    inBuffer_.retrieveByaddr(crlf + 2);//回收
                    parsestatus_=ParseHeaders;
                }
                else
                {
                    hasMore=false;
                }
            }
            else
            {
                hasMore=false;
            }
        }
        //解析请求头
        else if(parsestatus_==ParseHeaders)
        {
            const char* crlf = inBuffer_.findCrlf();
            if (crlf)
            {
                //冒号
                const char* colon = std::find(inBuffer_.peek(), crlf, ':');
                if (colon != crlf)
                {
                    addHeader(inBuffer_.peek(), colon, crlf);
                }
                else
                {
                // empty line, end of header
                // FIXME:
                    parsestatus_ = ParseFinish;
                    hasMore = false;
                }
                inBuffer_.retrieveByaddr(crlf + 2);//回收
            }
            else
            {
                hasMore = false;
            }
        }
        else if(parsestatus_==ParseBody)
        {
            // FIXME:
        }
    }//endwhile
    return ok;
}

/* 解析请求头的一行 */
bool Request::parseLine(const char* begin,const char* end){
bool succeed=false;
    const char*start=begin; //防止修改
    const char*space = std::find(start,end,' ');
    //设置请求方法//method_
    if(space!=end&&setMethod(start,space))
    {
        start=space+1;
        space = std::find(start,end,' ');
        if(space !=end)
        {
            //解析URI
            const char* question = std::find(start, space, '?');
            if (question != space)
            {
                setUrl(start, question);
                setUrlargs(question, space);
            }
            else
            {
                setUrl(start, space);
            }
            //解析HTTP版本号
            start = space+1;
            succeed = end-start == 8 && std::equal(start, end-1, "HTTP/1.");
            if (succeed)
            {
                if (*(end-1) == '1')
                {
                    setVersion(HTTP11);
                }
                else if (*(end-1) == '0')
                {
                    setVersion(HTTP10);
                }
                else
                {
                    succeed = false;
                }
            }//endif
        }//endif
    }//endif
    return succeed;

}

/* colon 为 : */
void Request::addHeader(const char* begin,const char* colon , const char* end){
    std::string field(begin, colon);
    ++colon;
    while (colon < end && isspace(*colon))
    {
        ++colon;
    }
    std::string value(colon, end);
    //while循环用来去掉后导的空格
    while (!value.empty() && isspace(value[value.size()-1]))
    {
        value.resize(value.size()-1);
    }
    headers_[field] = value;//将解析出来的头信息放入map中


}


bool Request::setMethod(const char* begin,const char* end){
    std::string method(begin,end);
    if(method == "GET")
        method_ = Get;
    else if(method == "POST")
        method_ = Post;
    else if(method == "HEAD")
        method_ = Head;
    else if(method == "PUT")
        method_ = Put;
    else if(method == "DELETE")
        method_ = Delete;
    else if(method == "PATCH")
        method_ = Patch;
    else
        method_ = Invalid;

    return method_ != Invalid;
}

void Request::setUrl(const char* begin,const char* end){
    std::string path;
    path.assign(begin,end);
    url_ = path=="/"?path:"/index";
};

void Request::setUrlargs(const char* begin,const char* end){
    urlargs_.assign(begin,end);
}


std::string Request::getMethod() const{
    std::string method = "Invalid";
    switch(method_){
        case Get: method = "GET";break;
        case Post: method = "POST";break;
        case Delete: method = "DELETE";break;
        case Put: method = "PUT";break;
        case Head: method = "HEAD";break;
        default: break;
    }
    return method;
}

std::string Request::getHeader(const std::string& field) const{
    auto it = headers_.find(field);
    if(it == headers_.end()){
        return "";

    }
    return it->second;

}

bool Request::keepConnect() const{
    std::string str=getHeader("Connection");
    if(str=="") 
        return false;
    return ((str=="Keep-Alive")||(version_==HTTP11&&str!="close"));    

}

/* 重置状态 */
void Request::resetparse(){
    method_ = Invalid;
    version_ = Unknown;
    parsestatus_ = ParseRequest;
    url_="";
    urlargs_ = "";
    headers_.clear();
}

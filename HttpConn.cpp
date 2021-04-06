#include <HttpConn.h>
#include <Epoll.h>
#include <Request.h>
#include <Response.h>
#include <TimeManager.h>
#include <ThreadPool.h>

#include<sys/types.h>
#include<sys/socket.h>
#include<arpa/inet.h>
#include<fcntl.h>
#include<unistd.h>
#include <cassert>
#include<iostream>
#include<cstring>

using namespace webserver;

HttpConn::HttpConn(int port, int num)
    : port_(port), listenfd_(getListenfd(port)),
      request_(new Request(listenfd_)), timemanager_(new TimeManager()),
      epoll_(new Epoll()), threadpool_(new ThreadPool(num))
{
    assert(listenfd_ >= 0);
}
HttpConn::~HttpConn(){
    close(listenfd_);
}

void HttpConn::run()
{
    /* epoll监听 */
    epoll_->add(listenfd_,request_.get(),(EPOLLIN | EPOLLET));
    epoll_->setCFunction(std::bind(&acceptConnection,this));
    epoll_->setCCFunction(std::bind(&closeConnection,this,std::placeholders::_1));
    epoll_->setRqFunction(std::bind(&doRequest,this,std::placeholders::_1));
    epoll_->setRsFunction(std::bind(&doResponse,this,std::placeholders::_1));

    while(1){

        int times = timemanager_-> getNextExpireTime();
        int eventnums = epoll_->wait(times);
        if(eventnums>0){
            epoll_->handleEvents(listenfd_,threadpool_,eventnums);
        }
        timemanager_->handleExpireTimers(); //处理超时事件
    }
}

void HttpConn::acceptConnection()
{
    while(1) {
        int acceptFd = ::accept4(listenfd_, nullptr, nullptr, SOCK_NONBLOCK | SOCK_CLOEXEC);
        if(acceptFd == -1) {
            if(errno == EAGAIN)
                break;
            std::cout<<"when accept connection met error:"<<strerror(errno)<<std::endl;
            break;
        }
        // 创建新的request
        Request* request = new Request(acceptFd);
        /* 遇到新的request，添加定时器，当定时结束时，执行closeConnection函数 */
        timemanager_ -> addTimer(request, CONNECT_TIMEOUT, std::bind(&HttpConn::closeConnection, this, request));
        // 添加io复用
        epoll_ -> add(acceptFd, request, (EPOLLIN | EPOLLONESHOT));
    }
}

void HttpConn::closeConnection(Request *request)
{
    //是否工作
    if(request->getworkstatus()){
        return;
    };
    int fd = request->fd();
    epoll_->del(fd,request,0);
    timemanager_->delTimer(request);
    delete request;
    request = nullptr;
}

void HttpConn::doRequest(Request *request)
{
    timemanager_->delTimer(request);
    assert(request!=nullptr);
    int fd = request->fd();

    int readErrno;
    // read:   fd->request.inbuffer
    int readable = request->read(&readErrno);
    // 没有数据可读    
    if(readable == 0) {
        request -> setnowworking();
        closeConnection(request);
        return; 
    }

    // 非EAGAIN错误，断开连接
    if(readable < 0 && (readErrno != EAGAIN)) {
        request -> setnowworking();
        closeConnection(request);
        return; 
    }

    // EAGAIN错误则释放线程使用权，并监听下次可读事件epoll_ -> mod(...)
    if(readable < 0 && readErrno == EAGAIN) {
        epoll_ -> mod(fd, request, (EPOLLIN | EPOLLONESHOT));
        request -> setnowworking();
        timemanager_ -> addTimer(request, CONNECT_TIMEOUT, std::bind(&HttpConn::closeConnection, this, request));
        return;
    }
    // 解析失败
    if(!request->parse()){
        Response* response = new Response(400,"",false);
        request->setnowworking();
        int writeErrno;
        //将response写入 request->outbuffer中
        request->append(response->responseBuffer());
        request->write(&writeErrno);
        closeConnection(request);
        return;
    }

    //解析成功
    if(request->isparseFinish()){
        Response* response = new Response(200,request->getUrl(),request->keepConnect());
        
        request->append(response->responseBuffer());
        epoll_->mod(fd,request,(EPOLLIN | EPOLLOUT | EPOLLONESHOT));
        return;
    }

}

void HttpConn::doResponse(Request *request)
{
    timemanager_->delTimer(request);
    assert(request!=nullptr);
    //可以从outbuffer中读的字节数
    int bytes=request->writeableBytes();
    int fd = request->fd();
    //不可读
    if(bytes==0){
        epoll_->mod(fd,request,(EPOLLIN|EPOLLONESHOT));
        request->setnowworking();
        timemanager_->addTimer(request,CONNECT_TIMEOUT,std::bind(&closeConnection,this,request));
        return ;
    }
    // outbuffer->fd 读数据
    int writeErrno;
    int ret = request -> write(&writeErrno);

    if(ret < 0 && writeErrno == EAGAIN) {
        epoll_ -> mod(fd, request, (EPOLLIN | EPOLLOUT | EPOLLONESHOT));
        return;
    }
    if(ret<0 && writeErrno!= EAGAIN){
        request->setnowworking();
        closeConnection(request);
        return ;

    }
    //全部读完
    if(ret == bytes){
        // 是否长连接
        if(request->keepConnect()){
            request->resetparse();
            epoll_->mod(fd,request,(EPOLLIN|EPOLLONESHOT));
            request->setnowworking();// setnoworking后当timemanage执行closeconnection关闭连接
            timemanager_->addTimer(request,CONNECT_TIMEOUT,std::bind(&closeConnection,this,request));
        }else{
            request->setnowworking();    
            closeConnection(request);
        }
        return;
    }
    epoll_ -> mod(fd, request, (EPOLLIN | EPOLLOUT | EPOLLONESHOT));
    request -> setnowworking();
    timemanager_ -> addTimer(request, CONNECT_TIMEOUT, std::bind(&closeConnection, this, request));
    return;
}


int HttpConn::getListenfd(int port)
{   
    int listenfd=0;
    /* create socket */
    if((listenfd = socket(PF_INET,SOCK_NONBLOCK|SOCK_STREAM,0))==-1){
        std::cout<<"while create listenfd"<<listenfd<<" error: "<<strerror(errno) <<"\n"<<std::endl;
        return -1;
    };
    /* change options of socket
     * SO-REUSEADDR: create the socket when port is reused  
     */
    bool option = true;
    if((setsockopt(listenfd,SOL_SOCKET,SO_REUSEADDR,(const void*)option,sizeof(option)))){
        std::cout<<"whem add option \"socket_reuse\" "<<"error:"<<strerror(errno)<<std::endl;
        return -1;
    };

    /* bind  address */
    struct sockaddr_in address;
    bzero(&address,sizeof(address));    
    address.sin_family = AF_INET;       
    address.sin_addr.s_addr = ::htonl(INADDR_ANY);  //ip
    address.sin_port = htons(port);
    if((bind(listenfd,(struct sockaddr*)&address,sizeof(address)))){
        std::cout<<"when bind address error:"<<strerror(errno)<<std::endl;
        return -1;
    }
    if((listenfd=listen(listenfd,BACKLOG))==-1){
        std::cout<<"while listen  fd"<<listenfd<<" error: "<<strerror(errno) <<"\n"<<std::endl;
        return -1;
    }

    if(listenfd == -1) {
        close(listenfd);
        return -1;
    }
    return listenfd;
}

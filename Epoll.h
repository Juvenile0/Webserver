#ifndef __EPOLL_H__
#define __EPOLL_H__

#include <memory>
#include <functional>
#include <vector>
#include <sys/epoll.h>

#define MAX_EVENT_NUMBER  1024
namespace webserver
{
    class Request;
    class ThreadPool;
    class Epoll
    {
        /* 建立、关闭连接，请求、响应的回调函数 */
        using ConnectionFunction = std::function<void()>;
        using CloseConnectionFunction = std::function<void(Request *)>;
        using RequestFunction = std::function<void(Request *)>;
        using ResponseFunction = std::function<void(Request *)>;
        using EventList = std::vector<struct epoll_event>; //epoll队列
    private:
        int epollFd_;
        EventList eventList_;
        ConnectionFunction connectionfc_;
        CloseConnectionFunction cconnectionfc_;
        RequestFunction requestfc_;
        ResponseFunction responsefc_;

    public:
        Epoll();
        ~Epoll();
        /* 当wait到event后调用，分派事件 */
        void handleEvents(int listenFd, std::shared_ptr<ThreadPool> &threadPool, int eventNums); //事件处理
        /* 设置回调函数 */
        void setCFunction(const ConnectionFunction &cf) { connectionfc_ = cf; };
        void setCCFunction(const CloseConnectionFunction &ccf){ cconnectionfc_ = ccf;};
        void setRqFunction(const RequestFunction &rf){requestfc_ = rf;};
        void setRsFunction(const  ResponseFunction &rf){responsefc_ = rf;};

        int add(int fd,Request* request,int events)
            { return base(fd,request,events,EPOLL_CTL_ADD);};   //添加监听的文件描述符
        int del(int fd,Request* request,int events)
            { return base(fd,request,events,EPOLL_CTL_DEL);};   //删除
        int mod(int fd,Request* request,int events)
            { return base(fd,request,events,EPOLL_CTL_MOD);};   //修改
        int base(int fd,Request* request,int event,int op); //epoll操作
        int wait(int timeoutMs);   //io复用监听 


    };

}

#endif
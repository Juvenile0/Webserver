#include<Epoll.h>
#include<Request.h>
#include<ThreadPool.h>

#include<sys/epoll.h>
#include<cassert>
#include<fcntl.h>
#include<unistd.h>
#include<iostream>
#include<cstring>

using namespace webserver;

Epoll::Epoll():epollFd_(epoll_create(5)),
eventList_(MAX_EVENT_NUMBER)
{
    assert(epollFd_>=0);
}


Epoll::~Epoll()
{
    close(epollFd_);
}

int Epoll::base(int fd,Request* request,int events,int op){
    epoll_event event;
    event.events = events;
    event.data.ptr = (void*) request; 
    return epoll_ctl(epollFd_,op,fd,&event);
}

int Epoll::wait(int timeoutMs){
    int num = epoll_wait(epollFd_,&*eventList_.begin(),static_cast<int>(eventList_.size()),timeoutMs);
    if(num ==0){
        //std::cout<<"not event to do"<<std::end;
    };
    if(num <0){
        std::cout<<"met error when epoll wait the event ,erorr:"<<strerror(errno);
        
    }
    return num;
}

void Epoll::handleEvents(int ListenFd,std::shared_ptr<ThreadPool>& threadPool,int eventNums){
    assert(eventNums>0);
    for(int i=0;i<eventNums;++i){
        Request* request = (Request *)eventList_[i].data.ptr;
        int fd = request->fd();
        int events = eventList_[i].events;
        if(fd == ListenFd) {
            // 新连接回调函数
            connectionfc_();
        } else {
            if((events&& EPOLLERR )|| (events&& EPOLLRDHUP) || (events && EPOLLHUP)){
                request ->setnowworking();
                cconnectionfc_(request);    //关闭
                std::cout<<"when handle event met error: "<<strerror(errno)<<std::endl;
            }else if(events && EPOLLIN){
                request->setworking();
                threadPool->addtask(std::bind(requestfc_,request));
            }else if(events && EPOLLOUT){
                request->setworking();
                threadPool->addtask(std::bind(responsefc_,request));
            }else{
                std::cout<<"when handle event met error:"<<strerror(errno)<<std::endl;
            }
        }

    }


}

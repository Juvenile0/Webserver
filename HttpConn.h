#ifndef __Server_H__ 
#define __Server_H__

#include<memory>

#define BACKLOG 1024 //内核监听队列的最大长度
namespace webserver{
    class Epoll;
    class Request;
    class TimeManager;
    class ThreadPool;
    using EpollPtr = std::unique_ptr<Epoll>;
    using RequestPtr = std::unique_ptr<Request>;
    using TimeManagerPtr = std::unique_ptr<TimeManager>;
    using ThreadPoolPtr = std::shared_ptr<ThreadPool>;
    class HttpConn{
        public:
            HttpConn(int port,int num );
            ~HttpConn();
            void run();
      
        private:
            void acceptConnection();    //接受连接
            void closeConnection(Request* request ); //关闭连接
            void doRequest(Request* request);
            void doResponse(Request* request);
            int getListenfd(int port);
            
            int port_;   //端口
            int listenfd_;   //监听的文件描述符
            EpollPtr epoll_; //epoll io复用
            RequestPtr request_; //请求对象
            TimeManagerPtr timemanager_; //定时器
            ThreadPoolPtr threadpool_;   //线程池



    };
}
#endif
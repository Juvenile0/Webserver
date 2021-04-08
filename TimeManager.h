#ifndef __TIMEMANAGER_H__
#define __TIMEMANAGER_H__

#include <functional>
#include <time.h>
#include <vector>
#include <mutex>
namespace webserver
{

    class Request;
    using TimeoutFunc = std::function<void()>;

    // 定时器
    class heap_timer
    {
    public:
        time_t expire_;
        heap_timer(const int delay, const TimeoutFunc &func)
            : expire_(time(NULL) + delay), func_(func), delete_(false){};
        void del() { delete_ = true; }
        bool isDeleted() { return delete_; }
        void runFunc(){ func_();};
        
    private:
        bool delete_;
        TimeoutFunc func_;
    };
    //时间堆

    // 也可以采用 优先队列实现
    class TimeManager
    {

    public:
        TimeManager();
        ~TimeManager();

        int getNextExpireTime();
        void handleExpireTimers();
        void addTimer(Request *request, const int &timeout, const TimeoutFunc &tf);
        void delTimer(Request *request);

    private:
        std::vector<heap_timer *> heap_; //时间堆数组
        std::mutex lock_;

        void upHeap(int index);   //最小堆上浮操作
        void downHeap(int index); //最小堆下沉操作
        void popHeap();
        void swap(int index1, int index2);
        heap_timer *topHeap();
    };

}

#endif
#ifndef __TIMEMANAGER_H__
#define __TIMEMANAGER_H__

#include<functional>

namespace webserver{

class TimeManager
{
private:
    /* data */
public:
    TimeManager(/* args */);
    ~TimeManager();
    using TimeoutFunc = std::function<void()>; 

    int getNextExpireTime();
    void handleExpireTimers();
    void addTimer(Request* request, const int& timeout, const TimeoutFunc& tf);
    void delTimer(Request* request);
};




}
#endif
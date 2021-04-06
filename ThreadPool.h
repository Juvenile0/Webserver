
#include<functional>
#include<vector>
#include<mutex>
#include<queue>
#include<thread>
#include<condition_variable>

namespace webserver{
class ThreadPool
{

public:

    ThreadPool(int num);
    ~ThreadPool();

    using taskfunction = std::function<void()>;
    void addtask(const taskfunction& taskf);
private:
    bool quit_;
    std::vector<std::thread> threads_;
    std::mutex lock_;
    /* 由taskfunction执行 onrequest和onresponse  */
    std::queue<taskfunction> taskqueue_;
    //A condition variable is an object able to block the calling thread until notified to resume.
    // notify_all notify_one
    std::condition_variable condv_;  

};




}
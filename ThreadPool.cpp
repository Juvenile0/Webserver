

#include<thread>
#include<cassert>

#include<ThreadPool.h>
using namespace webserver;
ThreadPool::ThreadPool(int num):quit_(false){
    threads_.emplace_back([this](){

        while(1){
            taskfunction tf;
                {
                    std::unique_lock<std::mutex> lock(lock_);    
                    while(!quit_&& taskqueue_.empty())
                        condv_.wait(lock);
                    if(taskqueue_.empty() && quit_) {
                        // printf("[ThreadPool::ThreadPool] threadid = %lu return\n", pthread_self());
                        return;
                    }
                    // if(!taskqueue_.empty()) {
                    tf= taskqueue_.front();
                    taskqueue_.pop();
                    // }
                }
                if(tf) {
                    // printf("[ThreadPool::ThreadPool] threadid = %lu get a job\n", pthread_self()/*std::this_thread::get_id()*/);
                    tf();
                    // printf("[ThreadPool::ThreadPool] threadid = %lu job finish\n", pthread_self()/*std::this_thread::get_id()*/);
                } 

        }
    });
};

ThreadPool::~ThreadPool(){
    {
        std::unique_lock<std::mutex> lock(lock_);
        quit_ = true;
    } 
    condv_.notify_all();
    for(auto& thread: threads_)
        thread.join();
    // printf("[ThreadPool::~ThreadPool] threadpool is remove\n");
}


void ThreadPool::addtask(const taskfunction& taskf){
    std::unique_lock<std::mutex> lock(lock_);
    taskqueue_.push(taskf);
        // printf("[ThreadPool::pushJob] push new job\n");
    condv_.notify_one();


}
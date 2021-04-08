#include <TimeManager.h>
#include <assert.h>
#include <Request.h>
using namespace webserver;

TimeManager::~TimeManager()
{
    for (int i = 0; i < heap_.size(); ++i)
    {
        delete heap_[i];
    }
    heap_.clear();
}

void TimeManager::swap(int index1, int index2)
{
    heap_timer *temp = heap_[index1];
    heap_[index1] = heap_[index2];
    heap_[index2] = temp;
}
// 与父节点交换 不断上浮
void TimeManager::upHeap(int index)
{
    if (index > heap_.size() - 1 || index == 0)
    {
        return;
    }
    int parent = (index - 1) / 2;
    while (index > 0 && heap_[index] < heap_[parent])
    {
        swap(index,parent);
        index = parent;
        parent = (index - 1) / 2;
    };
}

//下浮操作 找到孩子节点中最小的一个 不断往下交换
void TimeManager::downHeap(int index)
{
    int size = heap_.size();
    if (index > size - 1)
        return;
    heap_timer *temp = heap_[index];
    int child = 0;
    for (; (index * 2 + 1) < size - 1; index = child)
    {
        child = index * 2 + 1;
        if (child + 1 <= size - 1; heap_[child + 1]->expire_ < heap_[child]->expire_)
        {
            child++;
        }
        if (heap_[child]->expire_ < heap_[index]->expire_)
        {
            heap_[index] = heap_[child];
        }
        else
            return;
    }
    heap_[index] = temp; //赋值交换后的值
}

heap_timer *TimeManager::topHeap()
{
    return heap_.empty() ? NULL : heap_[0];
}

void TimeManager::popHeap()
{
    int size = heap_.size();
    if(size<=0)
        return;
    swap(0,size-1);
    delete heap_[size-1];
    heap_.pop_back();
    downHeap(0);    //从根节点下浮
}

void TimeManager::addTimer(Request* request,const int& timeout,const TimeoutFunc& fc){
    std::unique_lock<std::mutex> lock(lock_);
    assert(request!=nullptr);

    heap_timer* timer = new heap_timer(timeout,fc);
    // request 一次只能绑定一个timer
    if(request->getTimer()!=nullptr){
        delTimer(request);
    }
    request->setTimer(timer);

    heap_.push_back(timer);
    upHeap(heap_.size()-1); //插入到最后一个位置 上浮

}

// 将timer的delete字段设置为flase  (本来可以给每个timer设置index，通过index和堆直接删除  但是懒得改了) 现在通过manager获取下一个时间的时候判断是否删除
void TimeManager::delTimer(Request *request){
    assert(request!=nullptr);
    heap_timer* timer = request->getTimer();
    
    if(timer == nullptr)
        return;
    timer -> del();
    // 防止request -> getTimer()访问到垂悬指针
    request -> setTimer(nullptr);

}

int TimeManager::getNextExpireTime(){
    std::unique_lock<std::mutex> lock(lock_);
    while(!heap_.empty()){
        heap_timer* timer = topHeap();
        assert(timer!=nullptr);
        if(timer->isDeleted()){
            popHeap();
            continue;
        }
        int res = timer->expire_-time(NULL);
        return res>0?res:0;
    }
}
// 
void TimeManager::handleExpireTimers(){
    std::unique_lock<std::mutex> lock(lock_);
    
    if(getNextExpireTime()>0)   return;
    if(!heap_.empty()){
        heap_[0]->runFunc();
        popHeap();
    }
}
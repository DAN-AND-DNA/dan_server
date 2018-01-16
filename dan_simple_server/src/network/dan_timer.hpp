#pragma once


#include<stdint.h>
#include<algorithm>
#include<vector>
#include<chrono>
namespace dan
{
//================================
struct danFd_t;
class danTimers
{
public:
    danTimers(){}
    ~danTimers(){}
    static const uint64_t getTimeStampNow();
    void addTimer(uint64_t startTime,danFd_t*danFd);
    void popTimer();
    const bool empty()const{return timers_.empty();}
    danFd_t* frontTimer();
    const uint64_t gap()const;//现在距离下一次操作超时还差多少毫秒
    std::vector<danFd_t*> getWaitingFds(); //获得等待执行但没超时的fd
private:
    struct Timer
    {
        Timer(uint64_t startTime,danFd_t*danFd):
            startTime_(startTime),danFd_(danFd)
        {}
        bool operator> (const struct Timer&t)const{return startTime_ > t.startTime_;}
        uint64_t startTime_; //开始时间
        danFd_t*danFd_;
    };


    class TimerPredicte
    {
    public:
        bool operator()(const struct Timer& t1,const struct Timer& t2)
        {
            return t1>t2;
        }
    };

    typedef std::vector<Timer> timers_t;
private:
    timers_t timers_;
};
//============================================
std::vector<danFd_t*> danTimers::getWaitingFds()
{
    std::vector<danFd_t*> waitingFds;
    for(auto&obj : timers_)
        waitingFds.push_back(obj.danFd_);
    return waitingFds;
}
//当前的时间戳
const uint64_t danTimers::getTimeStampNow()
{
    auto now=std::chrono::steady_clock::now();//毫秒级
    return std::chrono::duration_cast<std::chrono::microseconds>(now.time_since_epoch()).count();
}


const uint64_t danTimers::gap()const
{
    if(timers_.empty())
        return -1; //scheduler不执行定时器
    uint32_t gap=0;
    uint64_t now=getTimeStampNow();
    uint64_t next=timers_.front().startTime_;
    if (now<next)
        gap=now-next;
    //DAN_LOG_INFO<<"gap:"<<gap<<"now"<<now;
    return gap;
}

danFd_t*danTimers::frontTimer()
{
    if(!timers_.empty())
        return timers_.front().danFd_;
}
void danTimers::popTimer()
{
    if(timers_.empty())
        return;
    std::pop_heap(timers_.begin(),timers_.end(),TimerPredicte());
    timers_.pop_back();
}
void danTimers::addTimer(uint64_t startTime,danFd_t*danFd)
{
    timers_.push_back(Timer(startTime,danFd));
    std::push_heap(timers_.begin(),timers_.end(),TimerPredicte());
}



}

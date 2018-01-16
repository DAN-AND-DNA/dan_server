#ifndef DAN_LIGHT_THREAD_MGR_HPP
#define DAN_LIGHT_THREAD_MGR_HPP

#include<stdio.h>
#include<vector>
#include"dan_light_thread.hpp"
#include"common/logging.hpp"
enum
{
    WORKER_RUNNING,
    WORKER_SUSPEND,
    WORKER_DONE
};
//===========================================
namespace dan
{
class danLightThreadMgr
{
public:
    danLightThreadMgr(size_t heapSize,const bool needProtect);
    ~danLightThreadMgr();
    int danLightThreadMgrWorkerFactory(doFunc_t doFunc,void*args);
    bool danLightThreadMgrWorkerResume(size_t workerId); //切回协程
    bool danLightThreadMgrWorkerYield(); //切回主协程
    int danLightThreadMgrGetCurrentWorker(){return currentWorker_;}
    int danLightThreadMgrGetBusyWorkersCount(){return busyWorkersCount_;}
    bool danLightThreadMgrAllDone(){return busyWorkersCount_==0;}
private:
    void danLightThreadMgrWorkerDoneCallback();
private:
    struct worker
    {
        worker()
        {
            lightThreadPtr=nullptr;
            idleWorkerNext=-1;
        }
        danLightThread*lightThreadPtr;
        int idleWorkerNext;
        int workerStatus;
    };
    std::vector<worker> workers_;//拷贝方式
    int idleWorkerFirst_;
    size_t heapSize_;
    bool needProtect_;
    int busyWorkersCount_;
    int currentWorker_;
};

//===========================================
danLightThreadMgr::~danLightThreadMgr()
{
    for(auto &w:workers_)
        delete w.lightThreadPtr;
    DAN_LOG_INFO<<"lightThreadMrg清理完毕";
}
bool danLightThreadMgr::danLightThreadMgrWorkerYield()
{
    if(currentWorker_!=-1)
    {
        auto w=workers_[currentWorker_];
        currentWorker_=-1;
        w.workerStatus=WORKER_SUSPEND;
        w.lightThreadPtr->danLightThreadYield();
    }
    return true;
}
bool danLightThreadMgr::danLightThreadMgrWorkerResume(size_t id)
{
    if(id>=workers_.size())
        return false;
    auto w=workers_[id];
    if(w.workerStatus==WORKER_SUSPEND)
    {
        currentWorker_=id;
        w.workerStatus=WORKER_RUNNING;
        w.lightThreadPtr->danLightThreadResume();
        return true;
    }
    return false;
}

void danLightThreadMgr::danLightThreadMgrWorkerDoneCallback()
{
    if(currentWorker_!=-1)
    {
        worker& w=workers_[currentWorker_];
        w.idleWorkerNext=idleWorkerFirst_;
        w.workerStatus=WORKER_DONE;
        idleWorkerFirst_=currentWorker_;
        busyWorkersCount_--;
        printf("worker:%d完成工作\n",currentWorker_);
        currentWorker_=-1;
    }
}

danLightThreadMgr::danLightThreadMgr(size_t heapSize,const bool needProtect):
    heapSize_(heapSize),needProtect_(needProtect),idleWorkerFirst_(-1),
    busyWorkersCount_(0),currentWorker_(-1)
{
    DAN_LOG_INFO<<"成功初始化一个danLightThreadMgr";
}

int danLightThreadMgr::danLightThreadMgrWorkerFactory(doFunc_t dofunc,void*args)
{
    if(dofunc==nullptr)
        return -2;
    int id=-1;
    if(idleWorkerFirst_>-1)
    {
        //有闲置的
        id=idleWorkerFirst_;
        idleWorkerFirst_=workers_[id].idleWorkerNext;
        workers_[id].lightThreadPtr->danLightThreadSetJob(dofunc,args);
        DAN_LOG_INFO<<"取得1个闲置的lightThread Id为:"<<id;
    }
    else
    {
        //全线繁忙
        id=workers_.size();
        auto lightThreadPtr=danLightThread::danLightThreadFactory(heapSize_,needProtect_,dofunc,
                             std::bind(&danLightThreadMgr::danLightThreadMgrWorkerDoneCallback,this),args);
        assert(lightThreadPtr!=nullptr);
        worker w;
        w.lightThreadPtr=lightThreadPtr;
        workers_.push_back(w);
        DAN_LOG_INFO<<"新建造1个lightThread Id为:"<<id;
    }
    workers_[id].idleWorkerNext=-1;
    workers_[id].workerStatus=WORKER_SUSPEND;
    busyWorkersCount_++;
    return id;
}


}
#endif

#pragma once
#include<list>
#include"common/logging.hpp"
#include"common/settings.hpp"
#include"network/dan_timer.hpp"
namespace dan
{

template<typename T>
class danObjPool
{
public:
    danObjPool(std::string name);
    ~danObjPool(){clearPool();}

    std::string getName(){return poolName_;}
    void clearPool();
    T* getObj();//获取一个对象
    void reclaimObj(T*obj);//回收一个对象
    uint32_t getPoolSize(){return availableObjs_;}
private:
    void keepAvailableSize();//一直保持池中拥有一定数量的对象
private:
    typedef std::list<T*> objs_t; //可以快速增加和删除
    objs_t objs_; //底层存储
    std::string poolName_; // 对象池的名字
    uint64_t lastCheckTime_; //检查时间
    uint32_t totalObjs_;//对象池的总数
    uint32_t availableObjs_; //可用的对象数
};
//==========================================
//池对象接口
class danPoolObjInterface
{
public:
    danPoolObjInterface():
        isUsed_(false)
    {}
    virtual ~danPoolObjInterface(){}
    bool isUsed()const{return isUsed_;}
    void isUsed(bool isUsed){isUsed_=isUsed;}
private:
    bool isUsed_;
};

//===========================================
template<typename T>
void danObjPool<T>::clearPool()
{
    for(auto& t:objs_)
    {
        delete t;
    }
    std::cout<<"here1\n";
    objs_.clear();
    std::cout<<"here2\n";
    availableObjs_=0;
    std::cout<<"here3\n";
    DAN_LOG_INFO<<poolName_<<"池清理完毕";
}
template<typename T>
void danObjPool<T>::reclaimObj(T*p)
{
    if(p!=nullptr)
    {
        p->isUsed(false);
        if(getPoolSize() > DAN_POOL_MAX_SIZE)
        {   
            //到池上限
            //
            delete p;
            --totalObjs_;
        }
        else
        {
            //没有到池上限
            objs_.push_back(p);
            ++availableObjs_;
        }
    }
    //清理臃肿的可用对象
    uint64_t now=danTimers::getTimeStampNow();
    if(availableObjs_<=DAN_POOL_KEEP_SIZE)
    {
        //数量没超过要维持的数量
        lastCheckTime_=now;
    }
    else if(now-lastCheckTime_ > DAN_POOL_KEEP_TIME) 
    {
        //数量超过了要维持的数量 并且 清理时间到了
        //开始清理
        uint32_t needClearNum=(uint32_t)(availableObjs_-DAN_POOL_KEEP_SIZE); 
        DAN_LOG_INFO<<poolName_<<"对象池开始清理,清理的数量为:"<<needClearNum;
        while(needClearNum-- >0)
        {
            T*t=static_cast<T*>(*objs_.begin()); //向下转换
            objs_.pop_front();
            delete t;
            --availableObjs_;
        }
        lastCheckTime_=now;
    }
}

template<typename T>
T* danObjPool<T>::getObj()
{
    //循环的目的就是确保一定有个对象被返回
    while(true)
    {

        if(availableObjs_>0)
        {
            T*p=static_cast<T*>(*objs_.begin()); //向下转
            objs_.pop_front();
            --availableObjs_;
            p->isUsed(true);
            return p;
        }

        //保持制定的数量
        keepAvailableSize();
    }
    return nullptr;
}
template<typename T>
void danObjPool<T>::keepAvailableSize()
{
    for(int i=0;i<DAN_POOL_KEEP_SIZE;++i)
    {
        T*p=new T();
        p->isUsed(false);//池对象必须实现的
        objs_.push_back(p);
        ++totalObjs_;
        ++availableObjs_;
    }
}
template<typename T>
danObjPool<T>::danObjPool(std::string name):
    poolName_(name),
    objs_(),
    lastCheckTime_(danTimers::getTimeStampNow()),
    totalObjs_(0),
    availableObjs_(0)
{
}


}

#pragma once

#include<sys/socket.h>
#include<sys/epoll.h>
#include<signal.h>
#include<queue>
#include<fcntl.h>
#include<sys/types.h>
#include<netinet/in.h>
#include<netinet/tcp.h>
#include<string.h>
#include<arpa/inet.h>
#include"dan_light_thread_mgr.hpp"
#include"dan_timer.hpp"
#include"common/logging.hpp"

namespace dan
{


enum readyEvent_t
{
    EVENT_EPOLLCLOSE = -2,  //epoll主动关闭
    EVENT_EPOLLERROR = -1,  //epoll发生错误
    EVENT_TIMEOUT = 0,     //操作超时
};

class danScheduler;

typedef std::pair<danScheduler*,int> danArgs_t;

//fd结构
struct danFd_t
{
    danScheduler*scheduler;
    int readyEvents;    //就绪的事件
    int lightThreadId; //在该协程执行
    struct epoll_event waitingEvents; //监听的事件
    int epollFd; //所属的epoll
    int fd;  //监听的fd
    int connectTimeoutMs;// 用于连接操作的超时
    int ioTimeoutMs; //fd>0 io操作的超时 fd==-7 无绑定fd的定时器的开始时间
    void*args; //参数
    doFunc_t doFunc; //当fd == -7时,定时器执行
    uint64_t intervalMs; //当fd == -7时生效
};

//调度器
class danScheduler
{
public:
    danScheduler(size_t heapSize,int maxTasks,const bool needProtect); //监听maxTasks个fd
    ~danScheduler(){::close(epollFd_); DAN_LOG_INFO<<"成功关闭epoll";}
    void addTask(doFunc_t doFunc,void*args); //添加任务
    bool run();
    
    bool runForever(){runForever_=true;run();}
    int currentLightThreadId(){return lightThreadMgr_.danLightThreadMgrGetCurrentWorker();}
    void addTimer(danFd_t*danFd,int timeoutMs); //设置fd的操作的超时
    bool yieldToMain(); //返回主协程
    danFd_t* createFd(int fd,int ioTimeoutMs=5000,int connectTimeoutMs=200,bool noDelay=true);
    static void removeLightThread(int id);
    danFd_t* createTimerFd(uint64_t startMs,uint64_t intervalMs,doFunc_t func,void*args=nullptr);
private:
    void procTimers(uint64_t& gap);//处理定时器
    void broadcast(int eventStatus); //广播给每个协程
    void assignTasks();//给协程分配任务,协程立刻执行
    
    static bool setNonBlock(int fd,bool flag);
    static bool setNoDelay(int fd,bool falg);
    static void stopApp(int){stop_=true; DAN_LOG_INFO<<"准备关闭app";}

public:
    //协程向epoll进行注册和询问结果
    static int lightThreadPoll(danFd_t& danFd,int waitingEvents,int* readyEvents,int timeout);
    static int lightThreadAccept(danFd_t&danFd,struct sockaddr* addr,socklen_t* addrlen);
    static bool tcpListen(int * listenFd, const char * ip, unsigned short port);
    static ssize_t lightThreadSend(danFd_t& danFd,const void*buf,size_t len,int flags);
    static ssize_t lightThreadRecv(danFd_t& danFd,void*buf,size_t len,int flags);
private:
    static std::map<int,danFd_t*> waitingFds_; //协程Id和fd
    static bool stop_;
    int maxTasks_;
    bool runForever_; //是否要一直执行
    danTimers timerMgr_; //定时器管理器
    danLightThreadMgr lightThreadMgr_; //协程管理器
    int epollFd_; //内部唯一epoll
    typedef std::queue<std::pair<doFunc_t,void*>> undoTasksQueue_t; 
    undoTasksQueue_t undoTasksQueue_; //未执行的任务
};
//=======================================================================
void danScheduler::removeLightThread(int id)
{
    auto i=waitingFds_.find(id);
    if(i!=waitingFds_.end())
        waitingFds_.erase(i);
}
std::map<int,danFd_t*> danScheduler::waitingFds_;
ssize_t danScheduler::lightThreadSend(danFd_t& danFd,const void*buf,size_t len,int flags)
{
    int ret=::send(danFd.fd,buf,len,flags);
    // 发生错误ret==-1 errno=EAGAIN
    // 发送成功ret==发送数 errno=EAGAIN
    if(ret<0&&errno==EAGAIN)
    {
        //无阻塞fd正常的错误 缓冲区满了
        //向epoll注册
        int readyEvents=0;
        //返回主协程序 等待epoll通知
        int pRet=lightThreadPoll(danFd,EPOLLOUT,&readyEvents,danFd.ioTimeoutMs);
       
        //epoll通知到达 切回该协程
        if(pRet>0)
        {
            //有epoll事件触发
            ret=::send(danFd.fd,buf,len,flags);
        }
        else if(pRet==-2)
        {
            ret=-2; //服务器主动关闭
        }
        else if(pRet==0)
        {
            ret=0; //超时或者
        }
        else
        {
            ret=-1; //服务器错误
        }
    }
    return ret;
}
ssize_t danScheduler::lightThreadRecv(danFd_t& danFd,void* buf,size_t len,int flags)
{
    int ret=::recv(danFd.fd,buf,len,flags); //第一次尝试
    if(ret<0 && errno==EAGAIN)
    {
        int readyEvents=0;
        int pRet=lightThreadPoll(danFd,EPOLLIN,&readyEvents,danFd.ioTimeoutMs);//注册和等待通知
        if(pRet>0)
        {
            //有触发
            ret=::recv(danFd.fd,buf,len,flags); 
            if(ret == 0)
            {
                //客户端关闭
                removeLightThread(danFd.lightThreadId);
                ret=-3;
            }
        }
        else if(pRet==-2)
        {
            ret=-2; //正常关闭
        }
        else if(pRet==0)
        {
            ret=0; //超时
        }
        else 
            ret=-1;
    }
    return ret;
}

bool danScheduler::tcpListen(int * server, const char * ip, unsigned short port)
{
    int serverFd=::socket(AF_INET,SOCK_STREAM,0);
    if(serverFd<0)
    {
        DAN_LOG_ERROR<<"::socket() 错误! errno:"<<strerror(errno);
        return false;
    }
    int ret=0;
    int flags=1;
    if(::setsockopt(serverFd,SOL_SOCKET,SO_REUSEADDR,(char*) &flags,sizeof(flags))<0)
        DAN_LOG_ERROR<<"::setsockopt() 错误! errno:"<<strerror(errno);
    struct sockaddr_in addr;
    memset(&addr,0,sizeof(addr));
    addr.sin_family=AF_INET;
    addr.sin_port=htons(port);
    addr.sin_addr.s_addr=INADDR_ANY;

    if(ip)
    {
        addr.sin_addr.s_addr=inet_addr(ip);
        if(addr.sin_addr.s_addr==INADDR_NONE)
        {
            DAN_LOG_ERROR<<"地址错误:"<<ip;
            ret = -1;
        }
    }
    if(ret==0)
    {
        if(::bind(serverFd,(struct sockaddr*)&addr,sizeof(addr))<0)
        {
            DAN_LOG_ERROR<<"::bind() 错误! errno:"<<strerror(errno);
            ret = -1;
        }
    }
    
    if(ret==0)
    {
        if(::listen(serverFd,1024)<0)
        {
            DAN_LOG_ERROR<<"::listen() 错误! errno:"<<strerror(errno);
            ret = -1;
        }
    }

    if(ret!=0 && serverFd >= 0)
        ::close(serverFd);
    if(ret==0)
    {
        *server=serverFd;
        DAN_LOG_INFO<<"serverfd:"<<serverFd<<"监听 port:"<<port;
    }
    return ret==0;
}

int danScheduler::lightThreadAccept(danFd_t&danFd,struct sockaddr* addr,socklen_t* addrlen)
{
    int ret=::accept(danFd.fd,addr,addrlen);
    if(ret<0)
    {
        //非阻塞accept直接返回，会有以下错误,若不是就发生错误 
        if(errno!=EAGAIN &&errno!=EWOULDBLOCK)
        {
            return -1;
        }
    }
    int readyEvents=0;
    //不超时,向epoll注册,主动返回主协程
    int pRet=lightThreadPoll(danFd,EPOLLIN,&readyEvents,-1);
    if(pRet>0)
        //有触发，就从主协程中切回协程
        ret=::accept(danFd.fd,addr,addrlen);
    else if(pRet==-2)
        ret=-2; //正常关闭
    else if(pRet==0)
    {
        //处理超时
        ret=0;
    }
    else 
    {
        DAN_LOG_INFO<<"here---1001"<<pRet;
        ret=-1;
    }
    return ret;
}
//设置fd为不阻塞or阻塞
bool danScheduler::setNonBlock(int fd,bool flag)
{
    int ret=0;
    int oldSet=fcntl(fd,F_GETFL);//获取原始的
    if(flag)
        ret=fcntl(fd,F_SETFL,oldSet|O_NONBLOCK);
    else
        ret=fcntl(fd,F_SETFL,oldSet &(~O_NONBLOCK) );
    return ret==0;
}

//无延迟
bool danScheduler::setNoDelay(int fd,bool flag)
{
    int tmp=flag?1:0;
    int ret=::setsockopt(fd,IPPROTO_TCP,TCP_NODELAY,(char*)&tmp,sizeof(tmp));

    return ret==0;
}

danFd_t* danScheduler::createTimerFd(uint64_t startMs,uint64_t intervalMs,doFunc_t doFunc,void* args)
{
    danFd_t* pFd=(danFd_t*)calloc(1,sizeof(danFd_t));
    pFd->fd = -7; //-7说明只是个没有绑定fd的定时器定时执行
    pFd->doFunc = doFunc;
    pFd->ioTimeoutMs=startMs;
    pFd->intervalMs=intervalMs;
    pFd->args=args;
    return pFd;
}
danFd_t* danScheduler::createFd(int fd,int ioTimeoutMs,int connectTimeoutMs,bool noDelay)
{
    danFd_t* fdPtr=(danFd_t*)calloc(1,sizeof(danFd_t));
    setNonBlock(fd,true);
    if(noDelay)
        setNoDelay(fd,true);
    fdPtr->scheduler=this; //同一个调度器
    fdPtr->epollFd=epollFd_; //同一个epoll
    fdPtr->waitingEvents.data.ptr=fdPtr;//触发后可以取这个指针
    fdPtr->fd=fd;
    fdPtr->readyEvents=0; //
    fdPtr->args=nullptr;
    fdPtr->connectTimeoutMs=connectTimeoutMs;
    fdPtr->ioTimeoutMs=ioTimeoutMs;
    fdPtr->doFunc=nullptr;
    fdPtr->intervalMs=0;
    return fdPtr;
}
void danScheduler::procTimers(uint64_t& gap)
{
    // 让时间飞逝
    while(true)
    {
        gap=timerMgr_.gap();
        if(gap!=0)
            break;//gap>0 or =-1 不能执行
        
        //gap==0 可执行
        danFd_t*danFd=timerMgr_.frontTimer();
        if(danFd->fd != -7)
        {
            //fd超时事件
            timerMgr_.popTimer();
            danFd->readyEvents=EVENT_TIMEOUT; //超时
            lightThreadMgr_.danLightThreadMgrWorkerResume(danFd->lightThreadId); //切回协程
        }
        else
        {
            //定时事件
            timerMgr_.popTimer();
            if(danFd->doFunc)
                danFd->doFunc(danFd->args);
            timerMgr_.addTimer(danTimers::getTimeStampNow()+danFd->intervalMs,danFd);
        }
    }
}
bool danScheduler::yieldToMain()
{
    return lightThreadMgr_.danLightThreadMgrWorkerYield();
}

void danScheduler::addTimer(danFd_t*danFd,int timeoutMs)
{
    if(timeoutMs==-1)
        //无超时
        timerMgr_.addTimer(std::numeric_limits<uint64_t>::max(),danFd);
    else
        timerMgr_.addTimer(danTimers::getTimeStampNow()+timeoutMs,danFd);
}


int danScheduler::lightThreadPoll(danFd_t& danFd,int waitingEvents,int* readyEvents,int timeoutMs)
{
    //注册epoll并给danFd赋值
    
    //默认关闭
    int ret=-2;
    danFd.lightThreadId=danFd.scheduler->currentLightThreadId();
    danFd.waitingEvents.events=waitingEvents;
    waitingFds_[danFd.lightThreadId]=&danFd; //等待事件
    epoll_ctl(danFd.epollFd,EPOLL_CTL_ADD,danFd.fd,&danFd.waitingEvents);

    DAN_LOG_INFO<<"danScheduler::lightThreadPoll 向epoll注册完毕,切回主协程";
    danFd.scheduler->yieldToMain(); //切回主协程
    
    //恢复到协程，继续执行
    DAN_LOG_INFO<<"danScheduler::lightThreadPoll 协程接到就绪消息,切回该协程 id:"<<danFd.lightThreadId;
    epoll_ctl(danFd.epollFd,EPOLL_CTL_DEL,danFd.fd,&danFd.waitingEvents);//暂停监听
    *readyEvents=danFd.readyEvents;//就绪的事件
    std::cout<<"dan---1\n";
    if((*readyEvents)>0)
    {
        //epoll监听到正确的事件
        //
        if((*readyEvents)&waitingEvents)
            ret=1;//有触发事件
        else
        {
            //和注册监听的消息不符合
            DAN_LOG_INFO<<"epoll发生错误1";
            errno=EINVAL;//epoll发生错误
            ret=0;
        }

    }
    // 发生错误 广播的
    else if((*readyEvents)==EVENT_TIMEOUT)
    {
        DAN_LOG_INFO<<"超时";
        errno=ETIMEDOUT;//超时
        ret=0;
    }
    else if((*readyEvents)==EVENT_EPOLLERROR)
    {
        DAN_LOG_INFO<<"epoll发生错误2";
        errno=ECONNREFUSED;//连接失败
        ret=-1;
    }
    else 
    {
         DAN_LOG_INFO<<"正常关闭";
        //主动关闭
        errno=0;
        ret = -2;
    }
    return ret;
}

bool danScheduler::stop_=false;

void danScheduler::broadcast(int readyEvent)
{
    DAN_LOG_INFO<<"向协程广播消息:"<<readyEvent;
    for(auto& danFd:waitingFds_)
    {
        std::cout<<"准备关闭:"<<danFd.first<<std::endl;
    } 
    for(auto& danFd:waitingFds_)
    {
        if((danFd.second)->fd!=-7)
        {
            //fd操作
            (danFd.second)->readyEvents=readyEvent; //更新每个fd的就绪事件
            lightThreadMgr_.danLightThreadMgrWorkerResume(danFd.first); //切回协程，执行就绪事件
            std::cout<<"---------------------------3\n";
        }
    }
    std::cout<<"---------------------------3.1\n";      
}
bool danScheduler::run()
{
    assignTasks(); //先指派任务,任务都是非阻塞的,先切换到协程,执行完毕或主动放弃就返回这里(主协程)
                   //比如(accept+poll):协程执行完向epoll注册监听事件就返回主协
    struct epoll_event*events=(struct epoll_event*)calloc(maxTasks_,sizeof(struct epoll_event));//初始化过了
    uint64_t gap=timerMgr_.gap(); //距离下个定时的时间
    while(runForever_ || !lightThreadMgr_.danLightThreadMgrAllDone())
    {
        int nfds=epoll_wait(epollFd_,events,maxTasks_,DAN_EPOLL_WAIT);
        if(nfds!=-1)
        {
            //有触发的事件
            for(int i=0;i<nfds;i++)
            {
                DAN_LOG_INFO<<"epoll监听到一个事件";
                danFd_t*danFd=(danFd_t*)events[i].data.ptr;//成功触发取得数据
                danFd->readyEvents=events[i].events;//触发的事件
                DAN_LOG_INFO<<"danScheduler::run() 切回协程 Id:"<<danFd->lightThreadId;
                lightThreadMgr_.danLightThreadMgrWorkerResume(danFd->lightThreadId);//切回这个协程
                                //比如(accept+poll):该事件触发了，就返回这个协程,继续执行任务,执行完毕就返回这里(主协程)
            }

            if(stop_)
            {
                DAN_LOG_INFO<<"向协程广播关闭消息";
                broadcast(EVENT_EPOLLCLOSE);//广播给每个协程，切回协程，执行就该绪事件
                DAN_LOG_INFO<<"清理完协程 切回主协程序";
                break;
            }
            assignTasks();//分配新任务
        }
        else if(errno!=EINTR)
        {
            DAN_LOG_INFO<<"发生错误";
            broadcast(EVENT_EPOLLERROR);
            break;
        }
        //处理定时器
        procTimers(gap);
        //统计
    }
    std::cout<<"----------------------4\n";
    free(events);
    std::cout<<"----------------------5\n";
    return true;
}
void danScheduler::assignTasks()
{
    while(!undoTasksQueue_.empty())
    {
        auto task=undoTasksQueue_.front(); //取出还未执行的任务
        std::cout<<"cao: "<<((danArgs_t*)(task.second))->second<<std::endl;
        int workerId=lightThreadMgr_.danLightThreadMgrWorkerFactory(task.first,task.second);//新建或者取得闲置的协程
        DAN_LOG_INFO<<"danScheduler::assignTasks() 切回到协程 Id:"<<workerId;
        lightThreadMgr_.danLightThreadMgrWorkerResume(workerId);//协程直接执行任务,切换到协程
        undoTasksQueue_.pop();
    }
}


void danScheduler::addTask(doFunc_t doFunc,void*args)
{
    undoTasksQueue_.push(make_pair(doFunc,args));
}

danScheduler::danScheduler(size_t heapSize,int maxTasks,const bool needProtect):
lightThreadMgr_(heapSize,needProtect),epollFd_(-1),runForever_(false),maxTasks_(maxTasks)
{
    epollFd_=::epoll_create(maxTasks);
    assert(epollFd_);
    typedef void(*handler)(int);
    handler p=stopApp;
    signal(SIGINT,p);
    DAN_LOG_INFO<<"创建epoll成功,fd为:"<<epollFd_;
}

}

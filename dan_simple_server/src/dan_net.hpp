#pragma once

#include<map>
#include"network/dan_scheduler.hpp"
#include"dan_obj_pool.hpp"
#include"common/settings.hpp"
#include"dan_session.hpp"

namespace dan
{

class danNet
{
public:
    danNet();
    ~danNet();
    int run();
private:
    static void acceptFunc(void*args);
    static void recvMsgFunc(void*args);//接受消息
    static void procMsgFunc(void*args);//定时处理消息
    typedef std::map<int,danSession*> sessions_t;
private:
    danScheduler epoller_;
    int serverFd_;
    static sessions_t sessions_; //fd 和会话的映射
};

//=============================================
void danNet::procMsgFunc(void*args)
{
    if(sessions_.empty())
        return;
   sessions_t::iterator i=sessions_.begin();
   for(;i!=sessions_.end();++i)
   {
       if(i->second->getAlive()==true)
       {
           i->second->procMsg();
       }
       else
       {
           danSession::reclaimObj(i->second);
           sessions_.erase(i);
       }
   }
}

danNet::~danNet()
{
    sessions_t::iterator i=sessions_.begin();
    std::cout<<"sessions: "<<sessions_.size()<<std::endl;
    for(;i!=sessions_.end();++i)
        danSession::reclaimObj(i->second);
    sessions_.clear();
}
std::map<int,danSession*> danNet::sessions_;

void danNet::recvMsgFunc(void*args)
{
    danArgs_t* pArgs=(danArgs_t*)args;
    danFd_t*pFd=pArgs->first->createFd(pArgs->second);
    std::cout<<"recv:"<<pArgs->second<<std::endl;
    danSession*pSession=danSession::getObj(); //新建一个会话
    //设置
    pSession->bindFd(pFd);
    pSession->setAlive(true);
    sessions_[pArgs->second]=pSession; //添加和fd的映射
    pFd->ioTimeoutMs=-1; //不超时
    while(true)
    {
        ssize_t len=pSession->recvFromFd();
        std::cout<<len<<std::endl;
        if(len > 0)
        {
            DAN_LOG_INFO<<"收到"<<len<<"字节数据";
        }
        else if(len == 0)
        {
            DAN_LOG_ERROR<<"超时";
            break;
        }   
        else if(len == -1)
        {
            DAN_LOG_ERROR<<"发生错误";
            break;
        }
        else if(len == -2)  
        {
            DAN_LOG_WARNING<<"服务器主动关闭";
            break;
        }
        else
        {
            DAN_LOG_WARNING<<"客户端关闭";
            break;
        }
    }
    ::close(pFd->fd);
    ::free(pArgs);
    ::free(pFd);
}

void danNet::acceptFunc(void*args)
{
    danArgs_t* pArgs=(danArgs_t*)args;

    danFd_t* pFd=pArgs->first->createFd(pArgs->second);
    while(true)
    {
        struct sockaddr_in addr;
        socklen_t socklen=sizeof(sockaddr);
        int clientFd=danScheduler::lightThreadAccept(*pFd,(struct sockaddr*)&addr,&socklen); 
        //返回主协程等通知

        //从主协程得到通知
        if(clientFd>0)
        {
            DAN_LOG_INFO<<"new客户端"<<" ip:"<<inet_ntoa(addr.sin_addr)<<" port:"<<ntohs(addr.sin_port);
            danArgs_t* pArgs2=(danArgs_t*)calloc(1,sizeof(danArgs_t));
            pArgs2->first=pArgs->first;
            pArgs2->second=clientFd;

            //std::cout<<clientFd<<std::endl;
            pArgs->first->addTask(recvMsgFunc,pArgs2);
        }
        else if(clientFd==-1)
        {
            DAN_LOG_INFO<<"accept error:"<<strerror(errno);
            break;
        }
        else if(clientFd==-2)
            break; //正常退出,开始清理协程
    }
    std::cout<<"oo1\n";
    if(pArgs->second>0)
        close(pArgs->second);//关闭服务器
    std::cout<<"oo2\n";
    free(pFd);
    std::cout<<"oo3\n";
    free(pArgs);
}
int danNet::run()
{
    if(!danScheduler::tcpListen(&serverFd_,"127.0.0.1",(int)DAN_TCP_PORT))   
    {
        DAN_LOG_ERROR<<"监听失败";
        return -1;
    }
    danArgs_t*pArgs=(danArgs_t*)calloc(1,sizeof(danArgs_t));
    pArgs->first=&epoller_;
    pArgs->second=serverFd_;
    epoller_.addTask(acceptFunc,pArgs);
    danFd_t* msgTimer=epoller_.createTimerFd(0,DAN_MSG_TICK,procMsgFunc);//马上执行
    epoller_.addTimer(msgTimer,DAN_MSG_TICK);
    epoller_.run();
    free(msgTimer);
}

danNet::danNet():
    epoller_(DAN_HEAP_SIZE,DAN_MAX_TASKS,true)
{
}

}

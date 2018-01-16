#pragma once

#include<vector>
#include"network/dan_scheduler.hpp"
#include"dan_packet.hpp"
#include"dan.hpp"
#include"dan_message.hpp"

namespace dan
{

class danSession:public danPoolObjInterface
{
public:
    danSession();
    virtual ~danSession();
    static danObjPool<danSession>& getObjPool(){return g_objPool_;}
    static danSession* getObj(){return g_objPool_.getObj();}
    static void reclaimObj(danSession*s){g_objPool_.reclaimObj(s);}
    typedef std::queue<danPacket*> recvBuffer_t;
    typedef std::queue<danMessage*> sendBuffer_t;

    void bindFd(danFd_t*& pFd){pFd_=pFd;}
    ssize_t recvFromFd(); //从fd接收消息
    ssize_t sendToFd(uint8_t*data,size_t len); //发送给fd消息
    void procMsg(); //处理这个会话的消息 
    void setAlive(bool al){isAlive_=al;}
    bool getAlive(){return isAlive_;}
private:
    void onSend(void*args); //协程实际执行的发送方案
private:
    static danObjPool<danSession> g_objPool_;//会话对象池
    recvBuffer_t recvBuffer_; //包缓存
    sendBuffer_t sendBuffer_; //外发消息缓存
    uint16_t currMsgId_; //当前消息的ID
    uint32_t currMsgLen_; //当前消息的长度
    uint32_t readLen_; //已经读取的字节数
    uint32_t readyLen_; //已经到达的字节
    uint8_t* pCurrMsg_; //当前要处理的消息
    danFd_t*pFd_;
    bool isAlive_; //会话是否保活
};
//=======================================================
danSession::danSession():
    currMsgId_(0),currMsgLen_(0),readLen_(0),pCurrMsg_(nullptr),pFd_(nullptr),readyLen_(0),isAlive_(false)
{
}


void danSession::procMsg()
{
   
    //没消息返回
    if(recvBuffer_.empty())
        return;

    //处理
    while(!recvBuffer_.empty())
    {
        auto p=recvBuffer_.front();
        //拿消息id
        if(currMsgId_ == 0)
        {
            if(p->readableSize() >= 2)
            {
                (*p)>>currMsgId_;
            }
        }
        std::cout<<"getId:"<<currMsgId_<<std::endl;
        //拿消息len
        if(currMsgLen_ == 0)
        {
            if(p->readableSize() >= 4)
            {
                if(pCurrMsg_!=nullptr)
                    free(pCurrMsg_);
                
                (*p)>>currMsgLen_;
            }
        }
        std::cout<<"getLen:"<<currMsgLen_<<std::endl;
        //拿消息体
        if(currMsgId_!=0)
        {
            if(readyLen_ < currMsgLen_+6)
            {
                std::cout<<"readyLen_:"<<readyLen_<<"currMsgLen_+6:"<<currMsgLen_+6<<std::endl;
                return; //到达的字节不够一个消息
            }
            std::cout<<"herherhe"<<std::endl;
            //判断消息体是否需要多个包
            if(p->readableSize() >= currMsgLen_-readLen_)
            {
                //够了,处理消息
                pCurrMsg_=(uint8_t*)calloc(currMsgLen_,sizeof(uint8_t));
                memcpy(pCurrMsg_+readLen_,p->getData()+p->getRpos(),currMsgLen_-readLen_);
                p->setRpos(p->getRpos()+currMsgLen_-readLen_);
                //可能是其他别的消息的组合 
                //比如服务端缓冲为空(8K)，客户端的socket缓冲中有2个消息等待发送(各4k),
                //TCP的原则就是给多少就保证按顺序发到多少,不管ip分包,反正收到2个消息共8k

                //TODO 在这里解析消息
                std::string msgName = A::id_to_name(currMsgId_);
                std::cout<<"收到消息,其id: "<<int(currMsgId_)<<" 其消息名为: "<<msgName<<std::endl;
                const google::protobuf::Descriptor* d=
                    google::protobuf::DescriptorPool::generated_pool()->FindMessageTypeByName(msgName);
                const google::protobuf::Message* p=
                    google::protobuf::MessageFactory::generated_factory()->GetPrototype(d);
                
                google::protobuf::Message*pMsg=p->New();
                pMsg->ParseFromArray(pCurrMsg_,currMsgLen_);
               
                //工厂一个对应的处理模块
                danMod*p1=modFactory(currMsgId_,pMsg);
                if(p1!=nullptr)
                {
                    std::cout<<"开始处理消息\n";
                    p1->processMsg(); //处理消息

                    std::string reName=p1->getReName(); //拿结果
                    google::protobuf::Message*pReMsg=p1->getReMsg(); //拿结果
                    if(reName!="" && pReMsg!=nullptr)
                    {
                        // 序列化
                        uint16_t reId=dan::A::name_to_id(reName);
                        uint32_t reLen=pReMsg->ByteSize();
                        uint8_t*reBuffer=(uint8_t*)calloc(reLen,sizeof(uint8_t));
                        uint8_t*result=(uint8_t*)calloc(reLen+6,sizeof(uint8_t));
                        pReMsg->SerializeToArray(reBuffer,reLen);
                        std::cout<<"消息的长度为:"<<reLen+6<<std::endl;

                        //打包
                        memcpy(result,(uint8_t*)&reId,DAN_MSG_ID);
                        memcpy(result+DAN_MSG_ID,(uint8_t*)&reLen,DAN_MSG_LEN);
                        memcpy(result+DAN_MSG_ID+DAN_MSG_LEN,reBuffer,reLen);

                        std::cout<<"消息的长度为:"<<*((uint32_t*)(result+2))<<std::endl;
                       
                        //拷贝到发送缓存
                        sendToFd(result,reLen+6);
                        free(reBuffer);
                        free(result);
                    }
                    else
                    {
                        //结果为空
                    }
                    delete pMsg;
                    delete p1;
                }
                break;//处理完跳出,不清理当前包,可能有多个消息
                readyLen_-=currMsgLen_; //就绪字节数剔除已经完成的
                readyLen_-=6;
            }
            else
            {
                //需要多个包,继续下个包
                memcpy(pCurrMsg_+readLen_,p->getData()+p->getRpos(),p->readableSize());
                readLen_+=p->readableSize(); //更新已经读的字节数
                p->setRpos(p->getWpos()); //无可读
            } 
        }
        recvBuffer_.pop();
        danPacket::reclaimObj(p);
    }

    //1.包还没拿齐等下次网络收
    //2.处理完了当前消息，清理
    free(pCurrMsg_);
    pCurrMsg_=nullptr;

    currMsgLen_=0;
    currMsgId_=0;
    readLen_=0;

}

danSession::~danSession()
{
    while(!recvBuffer_.empty())
    {
        //归还包
        danPacket::reclaimObj(recvBuffer_.front());
        recvBuffer_.pop();
    }
    while(!sendBuffer_.empty())
    {
        //归还消息
        danMessage::reclaimObj(sendBuffer_.front());
        sendBuffer_.pop();
    }

    if(pCurrMsg_!=nullptr)
        free(pCurrMsg_);
}

ssize_t danSession::sendToFd(uint8_t*data,size_t len)
{
    if(pFd_==nullptr)
        return -1; //有绑定danfd 无法发送数据
   
    //压入消息缓冲
    danMessage* m=danMessage::getObj();
    m->fillMsg(data,len);
    m->finishMsg();
    sendBuffer_.push(m);

    danArgs_t* pArgs=(danArgs_t*)calloc(1,sizeof(danArgs_t));
    pArgs->first=pFd_->scheduler;
    pArgs->second=pFd_->fd;
    pFd_->scheduler->addTask(std::bind(&danSession::onSend,this,std::placeholders::_1),pArgs); 
    //因为是主协程序在处理消息时候去发,故应申请一个协程去发

}
void danSession::onSend(void*Args)
{
    danArgs_t*pArgs=(danArgs_t*)Args;
    //danFd_t* pFd=pArgs->first->createFd(pArgs->second);
    std::cout<<"包个数:"<<sendBuffer_.front()->getPacketCount()<<std::endl;
    std::cout<<"消息的字节数"<<sendBuffer_.front()->getMsgLen()<<std::endl;

    danMessage*pSendMsg=sendBuffer_.front();//第一个消息
    uint32_t currCount=0;
    uint32_t totalCount=pSendMsg->getPacketCount();
    while(totalCount-currCount > 0)
    {
        danPacket*pSendPacket=pSendMsg->getPacket(currCount);
        bool sendSuccess=false;
        while(pSendPacket->readableSize() > 0)
        {
            ssize_t len=danScheduler::lightThreadSend(*pFd_,pSendPacket->getData()+pSendPacket->getRpos(),pSendPacket->readableSize(),0);
            //注册完毕返回主协程

            //通知到了 进入该协程
            if(len>0)
            {
                pSendPacket->setRpos(pSendPacket->getRpos()+len);
            }
            else if(len == 0)
            {
                DAN_LOG_ERROR<<"发送超时";
                DAN_LOG_INFO<<strerror(errno);
                sendSuccess=false;
                break;
            }
            else if(len == -1)
            {
                //服务器内部错误或者发送错误
                DAN_LOG_ERROR<<"发送时发生错误";
                sendSuccess=false;
                break;
            }
            else if(len == -2)
            {
                DAN_LOG_WARNING<<"服务器主动关闭";
                sendSuccess=false;
                break;
            }
            sendSuccess=true;
        }

        if(!sendSuccess)
        {
            //TODO 该消息的这个包发送失败
            //可以考虑继续尝试,这里直接关闭这个fd,并将会话置为不可用
            
            isAlive_=false; //标记为false,等待回收
            ::close(pFd_->fd); 
            break;
        }
        else
            currCount++;
    }
    sendBuffer_.pop();
    danMessage::reclaimObj(pSendMsg);
    ::free(pArgs);
}

ssize_t danSession::recvFromFd()
{
   if(pFd_==nullptr)
       return -1; //没有绑定danfd 无法获取数据

   danPacket* p=danPacket::getObj(); //默认1460字节大
   
   //有多少读多少 可能多个消息
   //可能是多个消息的组合 
   //比如服务端缓冲为空(8K)，客户端的socket缓冲中有2个消息等待发送(各4k),
   //TCP的原则就是给多少就保证发到多少,不管ip分包,反正收到2个消息共8k
   int len=danScheduler::lightThreadRecv(*pFd_,(p->getData())+(p->getWpos()),(p->getMaxSize())-(p->getWpos()),0);
   readyLen_+=len; //就绪字节
   //返回主协程等通知
   //从主协程得到通知
   if(len > 0)
   {
       p->setWpos(p->getWpos()+len);
       recvBuffer_.push(p);
   }
   else if(len == 0)
   {
       //超时
       danPacket::reclaimObj(p);
   }
   else if(len == -1)
   {
       //错误
       danPacket::reclaimObj(p);
   }
   else if(len == -2)
   {
       // 服务器主动关闭
       danPacket::reclaimObj(p);
   }
   else
   {
       // 客户端主动关闭
       danPacket::reclaimObj(p);
   }
   return len;

}
//===========================================

danObjPool<danSession> danSession::g_objPool_("danSession");


}

#pragma once

#include<vector>
#include"dan_obj_pool.hpp"
#include"dan_packet.hpp"

namespace dan
{

//消息的抽象
//每个包最大1466个字节,超过就自动分成多个包
class danMessage:public danPoolObjInterface
{
public:
    static danObjPool<danMessage>& getObjPool();
    static void reclaimObj(danMessage*obj);
    static danMessage* getObj();
    danMessage();
    virtual ~danMessage();

    bool fillMsg(const uint8_t*value,size_t addLen);//填充字节
    void finishMsg();//完成这个消息,就是不不再修改其中内容了
    typedef std::vector<danPacket*> danPackets_t;
    danPacket* getPacket(uint32_t index){if(index<packetCount_) return packets_[index];return nullptr;}
    uint32_t getPacketCount()const {return packetCount_;}
    uint32_t getMsgLen()const{return msgLen_;}
private:
    static danObjPool<danMessage> g_objPool_;
    danPackets_t packets_; //一个消息所对应的包
    uint32_t packetCount_; //消息对应的包个数
    uint32_t msgLen_;//消息的字节数
    danPacket*pCurrPacket_; 
};
//===========================================================
danMessage::~danMessage()
{
    if(pCurrPacket_!=nullptr)
    {
        packets_.push_back(pCurrPacket_);
        pCurrPacket_=nullptr;
    }

    for(auto& p:packets_)
        danPacket::reclaimObj(p);
    packets_.clear();
}
void danMessage::finishMsg()
{
    if(pCurrPacket_->readableSize()>0)
    {
        packets_.push_back(pCurrPacket_);
        pCurrPacket_=nullptr;
        ++packetCount_;
    }
}
bool danMessage::fillMsg(const uint8_t*value,size_t addLen)
{
    int32_t currLen=(int32_t)addLen; //防止溢出
    int32_t totalLen=0;
    
    while(currLen>0)
    {
        if(pCurrPacket_==nullptr)
            pCurrPacket_=danPacket::getObj();

        int32_t usedLen=pCurrPacket_->readableSize();//已经使用的字节数

        if(usedLen >= DAN_TCP_MSS)
        {
            //当前的包已经满了就新包
            
            packets_.push_back(pCurrPacket_);
            packetCount_++;
            //新包
            pCurrPacket_=danPacket::getObj();
            usedLen=0;
        }
        int32_t leftLen=DAN_TCP_MSS-usedLen; //当前包可用的字节数
        int32_t realAddLen=addLen;
        if(addLen>leftLen)
            realAddLen=leftLen; //容量不足以塞下全部，就充满这个包
        pCurrPacket_->doAppend(value+totalLen,realAddLen);//加到满
        currLen-=realAddLen; //还要添加的字节数
        totalLen+=realAddLen; //保存已经添加的字节数
    }
    msgLen_+=addLen; //更新消息的字节数
}

danMessage::danMessage():
    packets_(),packetCount_(0),msgLen_(0),pCurrPacket_(nullptr)
{
    pCurrPacket_=danPacket::getObj(); //默认保持一个当前处理包
}

danObjPool<danMessage> danMessage::g_objPool_("danMessage");

danObjPool<danMessage>& danMessage::getObjPool()
{
    return g_objPool_;
}

danMessage* danMessage::getObj()
{
    return g_objPool_.getObj();
}

void danMessage::reclaimObj(danMessage*obj)
{
    g_objPool_.reclaimObj(obj);
}

}

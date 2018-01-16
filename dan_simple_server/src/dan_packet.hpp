#pragma once

#include"dan_byte_stream.hpp"
#include"common/settings.hpp"
namespace dan
{

class danPacket:public danByteStream
{
public:
    danPacket();
    virtual ~danPacket(){DAN_LOG_INFO<<"清理danPacket";}
    static danObjPool<danPacket>& getObjPool();
    static danPacket* getObj();
    static void reclaimObj(danPacket*t);
    void reset(){setWpos(0);setRpos(0);data_.clear();data_.reserve(256);}

private:
    static danObjPool<danPacket> g_objPool_;
};

//===============================================================================
danPacket::danPacket()
{
    resize(DAN_TCP_MSS); //保证单次不小于1460这个上限
}

danObjPool<danPacket> danPacket::g_objPool_("danPacket"); //局部于该编译单元


danObjPool<danPacket>& danPacket::getObjPool()
{
    return g_objPool_;
}

danPacket* danPacket::getObj()
{
    return g_objPool_.getObj();
}

void danPacket::reclaimObj(danPacket*t)
{
    std::cout<<"danpackt::reclaim\n";
    g_objPool_.reclaimObj(t);
}
     
}

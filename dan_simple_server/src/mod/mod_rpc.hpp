#pragma once

#include"dan_mod.hpp"
#include"proto/rpc.pb.h"

namespace dan
{
// mod_rpc 模块的消息处理
class mod_rpc:public danMod
{
public:
    //arg1:要处理的消息id
    //arg2:要处理的消息的基类
    mod_rpc(uint16_t id,google::protobuf::Message*pMsg):
    pMsg_(pMsg)
    {currId_=id; /*TODO 1 注册消息*/ msgMap_[1]=(std::bind(&mod_rpc::rpc_hello,this));}

    void processMsg()
    {
        return msgMap_[currId_]();
    }

  //TODO 2 实现消息
    void rpc_hello()
    {
        if(currId_!=1)
            return;

        // 1.拿参数
        rpc::hello*a=dynamic_cast<rpc::hello*>(pMsg_);

        // 2.处理逻辑
        std::cout<<"rpc.hello is done\n";
        
        // 3.设置结果
        rpc::hello_r*pR=new rpc::hello_r();
        pR->set_ret(7);
        pR->set_result("ok");
        pReMsg_=pR;
        reName_="rpc.hello_r";
    }
private:
    google::protobuf::Message*pMsg_; //参数消息
};

}

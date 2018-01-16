#pragma once

#include"dan_mod.hpp"
#include"proto/lulu.pb.h"

namespace dan
{
// mod_lulu 模块的消息处理
class mod_lulu:public danMod
{
public:
    //arg1:要处理的消息id
    //arg2:要处理的消息的基类
    mod_lulu(uint16_t id,google::protobuf::Message*pMsg):
    pMsg_(pMsg)
    {currId_=id; /*TODO 1 注册消息*/ msgMap_[1]=(std::bind(&mod_lulu::do_ask_me,this));}

    void processMsg()
    {
         msgMap_[currId_]();
    }

  //TODO 2 实现消息
  
    void do_ask_me()
    {
        if(currId_!=1)
            return;

        // 1.拿参数
        lulu::ask_me*a=dynamic_cast<lulu::ask_me*>(pMsg_);
        
        // 2.处理逻辑
        std::cout<<"do_ask_me is done\n";
        
        // 3.设置结果
        lulu::tell_you*x=new lulu::tell_you();
        x->set_ret(0);
        x->set_answer("ok");
        pReMsg_=x;
        reName_="lulu.tell_you";
    }

private:
    google::protobuf::Message*pMsg_; //参数消息
};

}

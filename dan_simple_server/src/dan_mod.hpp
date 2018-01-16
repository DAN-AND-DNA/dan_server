
//本文件,自动生成,没必要不需要改
//
#pragma once

#include<google/protobuf/message.h>
#include<map>
#include<string>

namespace dan
{

// 模块的基类
class danMod
{
public:
    danMod():currId_(0),msgMap_(),pReMsg_(nullptr),reName_(""){}
    virtual ~danMod(){if(pReMsg_!=nullptr) delete pReMsg_;}
    virtual void processMsg()=0;
    google::protobuf::Message* getReMsg(){return pReMsg_;} //取得结果消息
    const std::string& getReName(){return reName_;}//取得结果名字
protected:
    google::protobuf::Message* pReMsg_; // 结果消息
    std::string reName_; //结果消息名字
    uint16_t currId_; //当前要处理的消息id
    std::map<uint16_t,std::function<void(void)>> msgMap_; //消息id和消息处理函数直接
};

}


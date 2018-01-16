//本文件,自动生成,没必要不需要改
//
#pragma once

#include<map>
#include<stdint.h>
#include<functional>
namespace dan
{

//
class A
{
public:
    static std::string id_to_name(const uint16_t& id);
    static uint16_t name_to_id(const std::string &name);
private:
    static std::map<std::string,uint16_t> nameIdMap_;
    static std::map<uint16_t,std::string> idNameMap_;
};



std::map<std::string,uint16_t> A::nameIdMap_{
{"lulu.ask_me",257},
{"lulu.tell_you",258},
{"rpc.hello",513},
{"rpc.hello_r",514},
};
std::map<uint16_t,std::string> A::idNameMap_{
{257,"lulu.ask_me"},
{258,"lulu.tell_you"},
{513,"rpc.hello"},
{514,"rpc.hello_r"},
};


std::string A::id_to_name(const uint16_t& id)
{
    return idNameMap_[id];
}

uint16_t A::name_to_id(const std::string &name)
{
    return nameIdMap_[name];
}

}
#include"mod/mod_lulu.hpp"
#include"mod/mod_rpc.hpp"
namespace dan
{

static danMod* modFactory(uint16_t id,google::protobuf::Message*pMsg)
{
    uint16_t mainId=id/256;
    uint16_t secondId=id%256;
    
    switch(mainId)
    {
        case 1:
            return new dan::mod_lulu(secondId,pMsg);
        case 2:
            return new dan::mod_rpc(secondId,pMsg);
        default:
            return nullptr;
    }
}

}

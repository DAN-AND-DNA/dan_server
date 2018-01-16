#!/bin/bash

ROOT=$(cd `dirname $0`; pwd)
PROTO_ROOT=$ROOT/../proto/proto

rm -f dan.hpp

cat<< EOF >> dan.hpp
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
EOF
echo "std::map<uint16_t,std::string> A::idNameMap_{" >> tempdan.hpp

rm -f dan_mod.hpp

cat << EOF >> dan_mod.hpp

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

EOF

cat << EOF >> tempdan2.hpp
namespace dan
{

static danMod* modFactory(uint16_t id,google::protobuf::Message*pMsg)
{
    uint16_t mainId=id/256;
    uint16_t secondId=id%256;
    
    switch(mainId)
    {
EOF

 
# TODO 解析proto文件
for file in $PROTO_ROOT/*.proto
do
    if [ -f "$file" ]
    then
        
        #TODO 取包名
        B=`cat $file |grep -w "package" |head -n 1|sed "s/.*{package,\(.*\)}.*/\1/"`
        PACK_NAME=`echo "$B"| cut -d, -f 1`
        PACK_ID=`echo "$B" | cut -d, -f 2`

cat << EOF >> tempdan2.hpp
        case $PACK_ID:
            return new dan::mod_$PACK_NAME(secondId,pMsg);
EOF

        #TODO 消息总数
        NUM=`grep -w "//{message" $file |tail -n 1|sed "s/.*{message,\(.*\)}.*/\1/"|cut -d" " -f 1 | cut -d, -f 2`
        C=`grep -w "//{message" $file |head -n $NUM|sed "s/.*{message,\(.*\)}.*/\1/"`

        #TODO 初始化map
        START_ID=${PACK_ID}*256
        for((i=1;i<=${NUM};++i))
        do
             MSG_NAME=`echo $C|cut -d" " -f ${i} | cut -d, -f 1`
             MSG_ID=`echo $C|cut -d" " -f ${i} | cut -d, -f 2`
             ((MSG_ID= $MSG_ID+$START_ID))
             echo "{\"$PACK_NAME.$MSG_NAME\",$MSG_ID}," >> dan.hpp
        done
        
       
        #TODO 初始化map
        for((i=1;i<=${NUM};++i))
        do
             MSG_NAME=`echo $C|cut -d" " -f ${i} | cut -d, -f 1`
             MSG_ID=`echo $C|cut -d" " -f ${i} | cut -d, -f 2`
             ((MSG_ID= $MSG_ID+$START_ID))
             echo "{$MSG_ID,\"$PACK_NAME.$MSG_NAME\"}," >>tempdan.hpp
        done

    fi
done
echo "};" >>dan.hpp
echo "};" >>tempdan.hpp

cat<< EOF >>tempdan2.hpp
        default:
            return nullptr;
    }
}

}
EOF

cat tempdan.hpp >>dan.hpp
rm -f tempdan.hpp
cat<< EOF >> dan.hpp


std::string A::id_to_name(const uint16_t& id)
{
    return idNameMap_[id];
}

uint16_t A::name_to_id(const std::string &name)
{
    return nameIdMap_[name];
}

}
EOF

sh $ROOT/gen2.sh

cat tempdan2.hpp >> dan.hpp
rm -f tempdan2.hpp

#!/bin/bash

ROOT=$(cd `dirname $0`;pwd)
PROTO_ROOT=$ROOT/../proto/proto
PROTOBUF_ROOT=$ROOT/../proto/deps/protobuf

rm -f $ROOT/../CMakeLists.txt
rm -f $ROOT/../proto/*.pb.*

cat << EOF >> "$ROOT/../CMakeLists.txt"
cmake_minimum_required(VERSION 3.7)
set(CMAKE_CXX_FLAGS "-std=c++11")
add_subdirectory(proto)
INCLUDE_DIRECTORIES(\${CMAKE_CURRENT_SOURCE_DIR}/proto/deps/protobuf/include)
INCLUDE_DIRECTORIES(\${CMAKE_CURRENT_SOURCE_DIR})
ADD_EXECUTABLE(ddd main.cpp
EOF
 
for file in $PROTO_ROOT/*.proto
do 
    echo $file
    A=`echo $file | sed "s/.*\/\(.*\).proto/\1/"`

    echo "#include\"mod/mod_$A.hpp\"">>dan.hpp
    echo " \${CMAKE_CURRENT_SOURCE_DIR}/proto/$A.pb.cc" >> "$ROOT/../CMakeLists.txt"
    $PROTOBUF_ROOT/bin/protoc  -I$PROTO_ROOT --cpp_out=$ROOT/../proto $file

    if [ ! -f "$ROOT/../mod/mod_$A.hpp" ];then
cat << EOF >> "$ROOT/../mod/mod_$A.hpp"
#pragma once

#include"dan_mod.hpp"
#include"proto/$A.pb.h"

namespace dan
{
// mod_$A 模块的消息处理
class mod_$A:public danMod
{
public:
    //arg1:要处理的消息id
    //arg2:要处理的消息的基类
    mod_$A(uint16_t id,google::protobuf::Message*pMsg):
    pMsg_(pMsg)
    {currId_=id; /*TODO 1 注册消息 msgMap_[1]=(std::bind(&mod_$A::do_ask_me,this));*/}

    void processMsg()
    {
        return msgMap_[currId_]();
    }

  //TODO 2 实现消息
  /* 
    reMsg_t do_ask_me()
    {
        if(currId_!=1)
            return;

        // 1.拿参数
        lulu::ask_me*a=dynamic_cast<lulu::ask_me*>(pMsg_);
        
        // 2.处理逻辑
        std::cout<<"do_ask_me is done\n";
        
        // 3.设置结果
        pReMsg_=new lulu::tell_you();
        pReMsg_->set_ret(0);
        pReMsg_->set_answer("ok");
        reName_="lulu.tell_you";
    }

  */
private:
    google::protobuf::Message*pMsg_; //参数消息
};

}
EOF
    fi
done
cat<<EOF >> "$ROOT/../CMakeLists.txt"
)
TARGET_LINK_LIBRARIES(ddd \${CMAKE_CURRENT_SOURCE_DIR}/proto/deps/protobuf/lib/libprotobuf.a)
EOF

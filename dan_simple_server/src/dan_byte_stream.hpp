#pragma once

#include<cstring>
#include<vector>
#include"dan_obj_pool.hpp"
namespace dan
{

//字节流基类
class danByteStream:public danPoolObjInterface
{
public:
    danByteStream();
    ~danByteStream(){}
    size_t readableSize(){return wpos_-rpos_<0? 0:wpos_-rpos_;}
    size_t getMaxSize(){return data_.size();}
    size_t getWpos(){return wpos_;}
    void setWpos(size_t wpos){wpos_=wpos;}
    size_t getRpos(){return rpos_;}
    void setRpos(size_t rpos){rpos_=rpos;}
    uint8_t* getData(){return &data_[0];}
    void resize(size_t len){data_.resize(len);}

    template<typename T> void append(T value);
    template<typename T> T read();
    template<typename T> T doRead(size_t pos) const;
    void doAppend(const uint8_t* value,size_t size=0);
    danByteStream& operator<<(uint8_t value);
    danByteStream& operator>>(uint8_t& value);
    danByteStream& operator>>(uint16_t& value);
    danByteStream& operator<<(uint16_t value);
    danByteStream& operator>>(uint32_t& value);
protected:
    mutable size_t rpos_;
    mutable size_t wpos_;  //写位置//可用于const成员函数内
    std::vector<uint8_t> data_; //底层字节存储
};
//==========================================================
danByteStream& danByteStream::operator>>(uint32_t& value)
{
    value=read<uint32_t>();
    return *this;
}
danByteStream& danByteStream::operator>>(uint16_t& value)
{
    value=read<uint16_t>();
    return *this;
}

danByteStream& danByteStream::operator<<(uint16_t value)
{
    append(value);
    return *this;
}

danByteStream& danByteStream::operator>>(uint8_t& value)
{
    value=read<uint8_t>();
    return *this;
}
danByteStream& danByteStream::operator<<(uint8_t value)
{
    append(value);
    return *this;
}

template<typename T>
T danByteStream::read()
{
    T r=doRead<T>(rpos_);
    rpos_+=sizeof(r);
    return r;
}
template<typename T>
T danByteStream::doRead(size_t pos)const 
{
    T val=*((T*)&data_[pos]);
    return val;
}

template<typename T>
void danByteStream::append(T value)
{
    doAppend((uint8_t*) &value,sizeof(value));
}

void danByteStream::doAppend(const uint8_t* value,size_t size)
{
    if(size==0)
        return;

    if(data_.size()<wpos_+size)
        data_.resize(wpos_+size); //提前扩容和初始化
    memcpy(&data_[wpos_],value,size);
    wpos_+=size;
}

danByteStream::danByteStream():
    rpos_(0),wpos_(0)
{
    data_.reserve(256); // 设置默认容量,超过之后再自动调整
}


}

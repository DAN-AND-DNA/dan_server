#ifndef DAN_THREAD_STACK_MEMORY_HPP
#define DAN_THREAD_STACK_MEMORY_HPP
#include<sys/mman.h>
#include<assert.h>
#include<iostream>
#include<unistd.h>

//协程空间
namespace dan
{
class danHeap
{
public:
    danHeap(const size_t heapSize,const bool needProtect=true);
    ~danHeap();
    
    size_t danHeapAlloc(const size_t heapSize,const bool needProtect);
    size_t danHeapFree();
    void* danHeapGetPtr(){return heapPtr_;}
    size_t danHeapGetSize(){return heapSize_;}
private:
    void*rawHeapPtr_;
    void*heapPtr_;
    size_t heapSize_;
    bool needProtect_;
};
//===========================================================
danHeap::danHeap(const size_t heapSize,const bool needProtect):
    rawHeapPtr_(nullptr),heapPtr_(nullptr),heapSize_(0),needProtect_(false)
{
    size_t Size=this->danHeapAlloc(heapSize,needProtect);
   // std::cout<<"成功分配了"<<Size<<"个字节"<<std::endl;
}

size_t danHeap::danHeapAlloc(const size_t heapSize,const bool needProtect)
{
    needProtect_=needProtect;
    int pageSize=getpagesize(); //确定单分页字节大小
    //std::cout<<"pageSize:"<<pageSize<<std::endl;
    if((heapSize_%pageSize)!=0)
        heapSize_=(heapSize/pageSize+1)*pageSize; //向上取
    else
        heapSize_=heapSize; //小于一个页 向上取
    if(needProtect_)
    {
        //需要保护，所以多分配,并将地址位置移动到2页中间
        //给分配内存,匿名私有映射,直接初始化为0,该内存可读可写
        rawHeapPtr_=mmap(NULL,heapSize_+pageSize*2,PROT_READ|PROT_WRITE,MAP_ANONYMOUS|MAP_PRIVATE,-1,0);
        assert(rawHeapPtr_!=nullptr);
        assert(mprotect(rawHeapPtr_,pageSize,PROT_NONE)==0); //修改前一分页长度为不可操作
        assert(mprotect((void*)((char*)rawHeapPtr_+pageSize+heapSize_),pageSize,PROT_NONE)==0);//修改后一个分页长度不可操作
        heapPtr_=(void*)((char*)rawHeapPtr_+pageSize);//实际位置
    }
    else
    {
        //不用保护
        rawHeapPtr_=mmap(NULL,heapSize_,PROT_READ|PROT_WRITE,MAP_ANONYMOUS|MAP_PRIVATE,-1,0);
        assert(rawHeapPtr_!=nullptr);
        heapPtr_=rawHeapPtr_;
    }

    return heapSize_;
}
danHeap::~danHeap()
{
    size_t i=this->danHeapFree();
    std::cout<<"成功释放了"<<i<<"个字节"<<std::endl;
}
size_t danHeap::danHeapFree()
{
    int pageSize=getpagesize();
    if(needProtect_)
    {
        assert(mprotect(rawHeapPtr_,pageSize,PROT_READ|PROT_WRITE)==0); //修改前一分页长度为不可操作
        assert(mprotect((void*)((char*)rawHeapPtr_+pageSize+heapSize_),pageSize,PROT_READ|PROT_WRITE)==0);//修改后一个分页长度不可操作
        assert(munmap(rawHeapPtr_,heapSize_+pageSize*2)==0);//
    }
    else
        assert(munmap(rawHeapPtr_,heapSize_)==0);
    return heapSize_;
    
}




}

#endif

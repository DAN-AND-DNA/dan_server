#ifndef DAN_LIGHT_THREAD_HPP
#define DAN_LIGHT_THREAD_HPP
#include<functional>
#include<ucontext.h>
#include"dan_heap.hpp"
typedef std::function< void(void*) > doFunc_t;
typedef std::function< void() > doneCallback_t;

//协程
namespace dan
{

class danLightThread
{
public:
    danLightThread(size_t heapSize,const bool needHeapProtect,
                   doFunc_t doFunc,doneCallback_t doneCallback,
                   void*args);
    ~danLightThread(){}
    void danLightThreadSetJob(doFunc_t doFunc,void*args);
    bool danLightThreadYield();  //暂停线程执行
    bool danLightThreadResume(); //恢复线程执行

    static danLightThread*danLightThreadFactory(size_t heapSize,const bool needHeapProtect,
                                                doFunc_t doFunc,doneCallback_t doneCallback,void*args);
private:
    static ucontext_t*danLightThreadGetMain();
    static void doFuncWrapper(uintptr_t p);
private:
    danHeap heap_;
    doFunc_t doFunc_;
    doneCallback_t doneCallback_;
    void*args_;
    ucontext_t context_;
};
//=================================================
bool danLightThread::danLightThreadYield()
{
    //保存到本协程,切换到主协程，
    swapcontext(&context_,danLightThreadGetMain());
    return true;
}

bool danLightThread::danLightThreadResume()
{
    //保存到主协程，切换到本协程
    swapcontext(danLightThreadGetMain(),&context_);
    return true;
}
danLightThread* danLightThread::danLightThreadFactory(size_t heapSize,const bool needHeapProtect,
                                                    doFunc_t doFunc,doneCallback_t doneCallback,void*args)
{
        return new danLightThread(heapSize,needHeapProtect,doFunc,doneCallback,args);
}

void danLightThread::doFuncWrapper(uintptr_t p)
{
    danLightThread*pt=(danLightThread*)p;
    pt->doFunc_(pt->args_);
    if(pt->doneCallback_)
        pt->doneCallback_();
}
ucontext_t* danLightThread::danLightThreadGetMain()
{
    static __thread ucontext_t mainContext;
    return &mainContext;
}
void danLightThread::danLightThreadSetJob(doFunc_t doFunc,void*args)
{
    doFunc_=doFunc;
    args_=args;
    getcontext(&context_);
    context_.uc_stack.ss_sp=heap_.danHeapGetPtr();
    context_.uc_stack.ss_size=heap_.danHeapGetSize();
    context_.uc_stack.ss_flags=0;
    context_.uc_link=danLightThreadGetMain();
    uintptr_t ptr=uintptr_t(this);
    makecontext(&context_,(void(*)(void))doFuncWrapper,1,ptr);

}
danLightThread::danLightThread(size_t heapSize,const bool needHeapProtect,
                               doFunc_t doFunc,doneCallback_t doneCallback,
                               void*args):
    heap_(heapSize,needHeapProtect),doFunc_(nullptr),doneCallback_(doneCallback),args_(nullptr)
{
    danLightThreadSetJob(doFunc,args);
}


}
#endif

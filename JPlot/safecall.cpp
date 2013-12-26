#include "safecall.h"
#include <FL/Fl.H>
/*
void SafeCallAgentCLayer(void*context)
{
    if(((CallContext*)context)->calltype==0)
        SafeStdCallAgent(context);
    else
        SafeThisCallAgent(context);
    free(context);
}

void Invoke(void*proc,int argnum,...)
{
    CallContext*context=(CallContext*)malloc(sizeof(CallContext));
    context->proc=proc;
    context->argnum=argnum;
    context->calltype=0;
    va_list args;
    va_start(args,argnum);
    for(int i=0;i<argnum;++i)
        context->args[i]=va_arg(args,uint32_t);
    Fl::awake(SafeCallAgentCLayer,context);
}
*/
void SafeBindCallAgent(void*proc)
{
    std::function<void()>* wrapper = reinterpret_cast<std::function<void()>*>(proc);
    (*wrapper)();
    delete wrapper;
}

void Invoke(std::function<void()>*proc)
{
    Fl::awake(SafeBindCallAgent,(void*)proc);
}

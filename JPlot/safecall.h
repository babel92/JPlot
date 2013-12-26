#ifndef SAFECALL_H_INCLUDED
#define SAFECALL_H_INCLUDED

#include <functional>


void Invoke(std::function<void()>*proc);
void Invoke(void*proc,int argnum,...);

#define WRAPCALL(...) (new std::function<void()>(std::bind(__VA_ARGS__)))

#endif // SAFECALL_H_INCLUDED

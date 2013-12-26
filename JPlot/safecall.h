#ifndef SAFECALL_H_INCLUDED
#define SAFECALL_H_INCLUDED

#include <functional>

#define WRAPCALL(...) (new std::function<void()>(std::bind(__VA_ARGS__)))
void Invoke(std::function<void()>*proc);

#endif // SAFECALL_H_INCLUDED

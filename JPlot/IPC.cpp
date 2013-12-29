#include "IPC.h"

#ifdef LINUX
#include <semaphore.h>
	struct evntobj
	{
		IPCObj handle;
		std::string name;
	}; 
	static std::vector<evntobj*> M_evntlist;
#endif

	IPCObj IPCHelper::CreateIPCEvent(const char*Name)
	{
#ifdef WIN32
		HANDLE Event = ::CreateEventA(NULL, 0, 0, Name);
		if (::GetLastError() == ERROR_ALREADY_EXISTS)
		{
			::CloseHandle(Event);
			return (IPCObj)-1;
		}
		else
			return (IPCObj)Event;
#elif LINUX
		
		sem_t* ret=sem_open(Name,O_CREAT/*|O_EXCL*/,0777,0);
		if(ret==SEM_FAILED)
		{
			fprintf(stderr, "Failed to create semaphore: %d\n",errno);
			return (IPCObj)-1;
		}
		evntobj* ptr=new evntobj;
		ptr->handle=(IPCObj)ret;
		ptr->name=Name;
		M_evntlist.push_back(ptr);
		return (IPCObj)ret;
#endif
	}



	void IPCHelper::DestroyIPCEvent(IPCObj Event)
	{
#ifdef WIN32
		CloseHandle((HANDLE)Event);
#elif LINUX
		if(0==sem_close((sem_t*)Event))
		{
			for(auto it=M_evntlist.begin();it<M_evntlist.end();)
			{
				evntobj* Entry=(*it);
				if(Entry->handle==Event)
				{
					sem_unlink(Entry->name.c_str());
					it=M_evntlist.erase(it);
					delete Entry;
				}
				++it;
			}
		}
#endif
	}

IPCObj IPCHelper::FindIPCEvent(const char*Name)
	{
#ifdef WIN32
		HANDLE Event = ::CreateEventA(NULL, 0, 0, Name);
		if (::GetLastError() != ERROR_ALREADY_EXISTS)
		{
			::CloseHandle(Event);
			return IPC_FAIL;
		}
		else
			return (IPCObj)Event;
#elif LINUX
		sem_t*ret=sem_open(Name,O_RDWR,0777,0);
		return (ret!=SEM_FAILED)?(void*)ret:IPC_FAIL;
#endif
	}

int IPCHelper::WaitOnIPCEvent(IPCObj Event, int MilliSec)
	{
#ifdef WIN32
		return WAIT_FAILED != ::WaitForSingleObject((HANDLE)Event, MilliSec==0?INFINITE:MilliSec);
#elif LINUX
		timespec tm={0};
		if(MilliSec==0)
			return !sem_wait((sem_t*)Event);
		else
		{
			tm.tv_sec=MilliSec/1000;
			tm.tv_nsec=MilliSec%1000*1000000;
			return !sem_timedwait((sem_t*)Event,&tm);
		}
#endif
	}

void IPCHelper::SignalIPCEvent(IPCObj Event)
	{
#if WIN32
		PulseEvent((HANDLE)Event);
#elif LINUX
int val;
sem_getvalue((sem_t*)Event,&val);
printf("value:%d\n",val);
		if(0!=sem_post((sem_t*)Event))
		{
			fprintf(stderr,"Signaling failure: %d\n",errno);
		}
sem_getvalue((sem_t*)Event,&val);
printf("value:%d\n",val);
#endif
	}

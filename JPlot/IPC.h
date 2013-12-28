#ifndef IPCHELPER
#define IPCHELPER

#ifdef WIN32
#include <Windows.h>
#elif LINUX
#include <fcntl.h>
#include <errno.h>
#include <sys/stat.h>
#include <semaphore.h>
#include <stdio.h>
#endif

typedef void* IPCObj;
#define IPC_FAIL ((IPCObj)-1)

class IPCHelper
{
public:
	static IPCObj CreateIPCEvent(const char*Name)
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
		
		sem_t* ret=sem_open(Name,O_CREAT/*|O_EXCL*/,S_IRWXO,1);
		if(ret==SEM_FAILED)
		{
			fprintf(stderr, "Failed to create semaphore: %d\n",errno);
			return (IPCObj)-1;
		}
		return (IPCObj)ret;
#endif
	}

	static void DestroyIPCEvent(IPCObj Event)
	{
#ifdef WIN32
		CloseHandle((HANDLE)Event);
#elif LINUX
		sem_close((sem_t*)Event);
#endif
	}

	static IPCObj FindIPCEvent(const char*Name)
	{
#ifdef WIN32
		HANDLE Event = ::CreateEventA(NULL, 0, 0, Name);
		if (::GetLastError() != ERROR_ALREADY_EXISTS)
		{
			::CloseHandle(Event);
			return (IPCObj)-1;
		}
		else
			return (IPCObj)Event;
#elif LINUX
		sem_t*ret=sem_open(Name,0);
		return (ret!=SEM_FAILED)?(void*)ret:(void*)-1;
#endif
	}

	static int WaitOnIPCEvent(IPCObj Event, int MilliSec = 0)
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

	static void SignalIPCEvent(IPCObj Event)
	{
#if WIN32
		PulseEvent((HANDLE)Event);
#elif LINUX
		sem_post((sem_t*)Event);
#endif
	}
};

#endif

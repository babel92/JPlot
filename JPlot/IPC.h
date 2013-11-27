#ifndef IPCHELPER
#define IPCHELPER

#ifdef WIN32
#include <Windows.h>
#endif

class IPCHelper
{
public:
	static int CreateIPCEvent(const char*Name)
	{
		HANDLE Event = ::CreateEventA(NULL, 0, 0, Name);
		if (::GetLastError() == ERROR_ALREADY_EXISTS)
		{
			::CloseHandle(Event);
			return -1;
		}
		else
			return (int)Event;
	}

	static void DestroyIPCEvent(int Event)
	{
		CloseHandle((HANDLE)Event);
	}

	static int FindIPCEvent(const char*Name)
	{
		HANDLE Event = ::CreateEventA(NULL, 0, 0, Name);
		if (::GetLastError() != ERROR_ALREADY_EXISTS)
		{
			::CloseHandle(Event);
			return -1;
		}
		else
			return (int)Event;
	}

	static int WaitOnIPCEvent(int Event,int MilliSec=0)
	{
		return WAIT_FAILED == ::WaitForSingleObject((HANDLE)Event, MilliSec==0?INFINITE:MilliSec);
	}

	static void SignalIPCEvent(int Event)
	{
		PulseEvent((HANDLE)Event);
	}
};

#endif
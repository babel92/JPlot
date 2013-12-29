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
#include <vector>
#include <string>
#endif

typedef void* IPCObj;
#define IPC_FAIL ((IPCObj)-1)

class IPCHelper
{
public:
	static IPCObj CreateIPCEvent(const char*Name);

	static void DestroyIPCEvent(IPCObj Event);

	static IPCObj FindIPCEvent(const char*Name);

	static int WaitOnIPCEvent(IPCObj Event, int MilliSec = 0);

	static void SignalIPCEvent(IPCObj Event);
};

#endif

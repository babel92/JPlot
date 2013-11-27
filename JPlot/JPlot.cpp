// JPlot.cpp : Defines the entry point for the console application.
//

#include <thread>
#include <string>
#include <vector>
#include <mutex>
#include <condition_variable>

#include <FL/Fl.H>
#include <FL/Fl_Window.H>
#include <FL/Fl_Select_Browser.H>
#include <FL/Fl_Button.H>

#include "netlayer.h"
#include "Plotter.h"
#include "safecall.h"

#include <Windows.h>

class IPCHelper
{
public:
	static int CreateIPCEvent(const char*Name)
	{
		HANDLE Event = ::CreateEventA(NULL, 0, 0, Name);
		if (::GetLastError() == ERROR_ALREADY_EXISTS)
		{
			::CloseHandle(Event);
			return 0;
		}
		else
			return (int)Event;
	}

	static int FindNamedIPCEvent(const char*Name)
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

	static int WaitOnIPCEvent(int Event)
	{
		return WAIT_FAILED==::WaitForSingleObject((HANDLE)Event, INFINITE);
	}

	static void SignalIPCEvent(int Event)
	{
		PulseEvent((HANDLE)Event);
	}
};

std::vector<TCPSocket*> ClientList;
std::mutex MuClient;

std::condition_variable EvtNewClient;

void IPCListener()
{
	TCPSocket Server("localhost", "64129");
	Server.Bind();
	Server.Listen();
	int IPCEvent = IPCHelper::FindNamedIPCEvent("JPlotEVENT");
	if (IPCEvent != -1)
		IPCHelper::SignalIPCEvent(IPCEvent);

	for (;;)
	{
		TCPSocket* Incoming=Server.Accept();
		if (Incoming == NULL)
		{
			// Interrupted by Cleanup()
			return;
		}
		ClientList.push_back(Incoming);
		Incoming->Send("JPLOTIPC");

		{
			std::lock_guard<std::mutex> lock(MuClient);
			EvtNewClient.notify_one();
		}
	}
}

int RequestHandler(TCPSocket*Client, const char*Request, int ReqSize);
bool Exit = 0;

void RequestListener()
{
	Multiplexer Mul;
	std::unique_lock<std::mutex> LckNewClient(MuClient);

	while (!Exit)
	{
		if (ClientList.size() == 0)
		{
			EvtNewClient.wait(LckNewClient);
		}
		Mul.Zero();

		for (auto Client : ClientList)
			Mul.Add(*Client);
		Mul.Select();

		for (auto Client:ClientList)
		if (Mul.Check(*Client))
		{
			char Buffer[10240];
			int RecvSize = Client->Recv(Buffer, 10240);
			if (RecvSize < 0)
			{
				// Disconnect
			}
			else
			{
				RequestHandler(Client, Buffer, RecvSize);
			}
		}

	}
}

class FieldRetriver
{
	const char* M_ptr;
public:
	FieldRetriver(const char* Data) :M_ptr(Data){}
	std::string String(int CharCount)
	{
		std::string Ret = std::string(M_ptr, CharCount);
		M_ptr += CharCount;
		return Ret;
	}
	int Int()
	{
		int Ret = *(int*)M_ptr;
		M_ptr += 4;
		return Ret;
	}
	char Char()
	{
		return *M_ptr++;
	}
};

int RequestHandler(TCPSocket*Client, const char*Request, int ReqSize)
{
	FieldRetriver Header(Request);
	std::string&& Cmd = Header.String(4);
	if (Cmd == "NEWF")
	{
		char Type = Header.Char();
		std::string&& FigName = Header.String(16);
		std::string&& XName = Header.String(16);
		std::string&& YName = Header.String(16);
		Invoke(WRAPCALL([=]{new Plotter; Client->Send("UUID"); }));
	}
	else if (Cmd == "")
	{

	}
	else
	{

	}

	return 0;
}

Fl_Window* PtrWin;

BOOL WINAPI ExitHandler(DWORD Opt)
{
	switch (Opt)
	{
	case CTRL_CLOSE_EVENT:
		PtrWin->hide();
		return TRUE;
	case CTRL_C_EVENT:
		cout << "Ctrl+C dropped\n";
		return TRUE;
	}
	return FALSE;
}

int main(int argc, char* argv[])
{
	NetHelper::Init();

	std::thread ThrdReqListener(RequestListener);
	std::thread ThrdIPCListener(IPCListener);
	Fl::lock();
	Fl_Window Win(300, 200, "JPlot Manager");
	PtrWin = &Win;
	SetConsoleCtrlHandler(ExitHandler, TRUE);
	Fl_Select_Browser listview(0, 0, 300, 200);
	Win.resizable(Win);
	Win.show();
	
	int Ret = Fl::run();

	NetHelper::Cleanup();
	Exit = 1;
	EvtNewClient.notify_one();
	ThrdIPCListener.join();
	ThrdReqListener.join();

	return Ret;
}


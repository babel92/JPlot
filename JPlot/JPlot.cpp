// JPlot.cpp : Defines the entry point for the console application.
//

#include <thread>
#include <string>
#include <vector>
#include <mutex>
#include <condition_variable>

#include <FL/Fl.H>
#include <FL/Fl_Window.H>
#include <FL/Fl_Group.H>
#include <FL/Fl_Hold_Browser.H>
#include <FL/Fl_Button.H>

#include "netlayer.h"
#include "Plotter.h"
#include "safecall.h"
#include "IPC.h"

#include <Windows.h>

std::vector<TCPSocket*> ClientList;
std::mutex MuClient;

std::condition_variable EvtNewClient;

void IPCListener()
{
	TCPSocket Server("localhost", "64129");
	Server.Bind();
	Server.Listen();
	int IPCEvent = IPCHelper::FindIPCEvent("JPlotEVENT");
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
		Invoke(WRAPCALL([=]{new Plotter(FigName,XName,YName); Client->Send("UUID"); }));
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
	Fl_Window Win(400, 300, "JPlot Manager");
	PtrWin = &Win;
	SetConsoleCtrlHandler(ExitHandler, TRUE);

	Fl_Group Group(0, 0, 400, 300);
	Group.box(FL_DOWN_BOX);
	Group.align(FL_ALIGN_TOP | FL_ALIGN_INSIDE);
	Fl_Hold_Browser listview(10, 10, 380, 280);
	listview.align(FL_ALIGN_TOP | FL_ALIGN_INSIDE);
	Group.add_resizable(listview);
	Group.end();
	Win.resizable(Win);
	Win.size_range(300, 200);
	Win.show();
	
	int Ret = Fl::run();

	NetHelper::Cleanup();
	Exit = 1;
	EvtNewClient.notify_one();
	ThrdIPCListener.join();
	ThrdReqListener.join();

	return Ret;
}


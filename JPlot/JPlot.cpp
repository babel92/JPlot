// JPlot.cpp : Defines the entry point for the console application.
//

#include <thread>
#include <string>
#include <vector>
#include <mutex>
#include <condition_variable>

#include "netlayer.h"

#include <FL/Fl.H>
#include <FL/Fl_Window.H>
#include <FL/Fl_Group.H>
#include <FL/Fl_Hold_Browser.H>
#include <FL/Fl_Button.H>


#include "Plotter.h"
#include "PlotterFactory.h"
#include "safecall.h"
#include "IPC.h"
#include "JPlot.h"

#ifdef WIN32
#include <Windows.h>
#elif LINUX
#include <signal.h>
#endif

std::vector<TCPSocket*> ClientList;
std::mutex MuClient;

std::condition_variable EvtNewClient;

void IPCListener()
{
	TCPSocket Server("localhost", JPLOT_PORT);
	if (!Server.Bind())
	{
		cerr << "Port " << JPLOT_PORT << " occupied. Exit.\n";
		exit(1);
	}
	Server.Listen();
	void* IPCEvent = IPCHelper::FindIPCEvent("JPlotEVENT");
	if (IPCEvent != (void*)-1)
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

#define DELETE_ENTRY 10

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
		
		for (auto it = ClientList.begin(); it < ClientList.end();)
		{
			TCPSocket*Client = (*it);
			if (Mul.Check(*Client))
			{
				char Buffer[10240];
				int RecvSize = Client->Recv(Buffer, 10240);
				if (RecvSize < 0)
				{
					// Disconnect
					Client->Shutdown();
					it = ClientList.erase(it);
					continue;
				}
				else
				{
					RequestHandler(Client, Buffer, RecvSize);
				}
			}
			it++;
		}

	}
}

Fl_Hold_Browser* PtrBrow;

#define SIZE_GUARD(Size) \
if (ReqSize < Size)\
{\
	Client->Send("NEG SZERROR"); \
	return 0;\
}

int FindAndDeleteUIEntry(int FreeID,int ThreadSafe=0)
{
	for (int i = 1; i <= PtrBrow->size(); ++i)
	{
		if (PtrBrow->data(i) == (void*)FreeID)
		{
			if (ThreadSafe)
				Fl::lock();
			PtrBrow->remove(i);
			if (ThreadSafe)
				Fl::unlock();
			return 1;
		}
	}
	return 0;
}

int RequestHandler(TCPSocket*Client, const char*Request, int ReqSize)
{
	FieldRetriver Header(Request);
	std::string&& Cmd = Header.String(4);
	//cout<<Request<<endl;
	if (Cmd == "NEWF")
	{
		SIZE_GUARD(57);
		char Type = Header.Char();
		switch (Type)
		{
		case 'a':
			{
				std::string&& FigName = Header.String(16);
				std::string&& XName = Header.String(16);
				std::string&& YName = Header.String(16);
				int Arg = Header.Int();
				Invoke(WRAPCALL([=]{
					int ID = PlotterFactory::AllocPlotter(Type); 
					std::string&& IDStr = std::to_string(ID);
					Plotter* Wnd = PlotterFactory::GetPlotter(ID);
					Wnd->callback([](Fl_Widget*Wid, void*Arg){
						((Fl_Window*)Wid)->hide();
						PlotterFactory::FreePlotter((long)Arg);
						FindAndDeleteUIEntry((long)Arg);
					},(void*)ID);
					Wnd->SetTitle(FigName.c_str());
					Wnd->SetXText(XName.c_str());
					Wnd->SetYText(YName.c_str());
					PtrBrow->add((IDStr + '\t' + std::to_string(Type)).c_str(), (void*)ID);
					Client->Send(IDStr);
				}));
				break;
			}
		default:
			{
				Client->Send("NEG TYPEERR");
			}
			break;
		}

	}
	else if (Cmd == "FREE")
	{
		int FreeID = Header.Int();
		if (FindAndDeleteUIEntry(FreeID, 1)) // Thread safe flag set to 1
		{
			// Seems this is the only way to hide window from another thread
			Invoke(WRAPCALL(PlotterFactory::FreePlotter, FreeID));
			Client->Send("OK");
			return DELETE_ENTRY;
		}
		Client->Send("NEG NOFIG");
	}
	else if (Cmd == "DRAW")
	{
		SIZE_GUARD(22);
		int ID = Header.Int();
		Plotter* Plt = PlotterFactory::GetPlotter(ID);
		if (Plt == NULL)
		{
			Client->Send("NEG NOFIG");
			return 0;
		}
		int Arg = Header.Int();
		int DataType = Header.Int();
		int Size = Header.Int();

		Fl::lock();
		Plt->Plot((float*)Header.Ptr(), Size);
		if (Arg==1)
			Fl::awake();
		Fl::unlock();
		Client->Send("OK");
	}
	else
	{
		Client->Send("NEG NOCMD");
	}

	return 0;
}

Fl_Window* PtrWin;
#ifdef WIN32
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

#elif LINUX
void onExit()
{
	PtrWin->hide();
}

void sigint_handler(int s)
{
	cout << "Ctrl+C dropped\n";
}

#endif

void TimerFunc(void*Dummy)
{
	Fl::repeat_timeout(double(1.0) / 60, TimerFunc);
}

int main(int argc, char* argv[])
{
	NetHelper::Init();

	std::thread ThrdReqListener(RequestListener);
	std::thread ThrdIPCListener(IPCListener);
	
	Fl::lock();
	Fl_Double_Window Win(400, 300, "JPlot Manager");
	PtrWin = &Win;

#ifdef WIN32
	SetConsoleCtrlHandler(ExitHandler, TRUE);
#elif LINUX
	struct sigaction sigIntHandler;
	sigIntHandler.sa_handler = sigint_handler;
	sigemptyset(&sigIntHandler.sa_mask);
	sigIntHandler.sa_flags = 0;
	sigaction(SIGINT, &sigIntHandler, NULL);
	atexit(onExit);
#endif
	Win.callback([](Fl_Widget*Wid, void*Arg){
		PlotterFactory::Cleanup();
		((Fl_Window*)Wid)->hide();
	});

	Fl_Group Group(0, 0, 400, 300);
	Group.box(FL_DOWN_BOX);
	Group.align(FL_ALIGN_TOP | FL_ALIGN_INSIDE);
	Fl_Hold_Browser listview(10, 10, 380, 280);
	PtrBrow = &listview;
	listview.add("ID\tType",(void*)-1);
	listview.align(FL_ALIGN_TOP | FL_ALIGN_INSIDE);
	Group.add_resizable(listview);
	Group.end();
	Win.resizable(Win);
	Win.size_range(300, 200);
	Win.show();
	Fl::add_timeout(0, TimerFunc);

	int Ret = Fl::run();

	NetHelper::Cleanup();
	Exit = 1;
	EvtNewClient.notify_one();
	ThrdIPCListener.join();
	ThrdReqListener.join();

	return Ret;
}


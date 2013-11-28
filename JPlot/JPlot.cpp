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

class PlotterFactory
{
	struct PlotterEntry
	{
		Plotter* Wnd;
		int ID;
		char Type;
		PlotterEntry(Plotter* Plotter_, int ID_, char Type_) :Wnd(Plotter_), ID(ID_), Type(Type_){}
	};
	static const int MAX_PLOTTER = 16;
	static PlotterEntry* PlotterList[MAX_PLOTTER];
public:
	static int AllocPlotter(char Type)
	{
		for (int i = 0; i < MAX_PLOTTER; ++i)
		{
			if (PlotterList[i] == NULL)
			{
				PlotterList[i] = new PlotterEntry(new Plotter("A","B","C"), i, Type);
				return i;
			}
		}
		return -1;
	}
	static void FreePlotter(int ID)
	{
		if (PlotterList[ID] != NULL)
		{
			PlotterList[ID]->Wnd->hide();
			delete PlotterList[ID]->Wnd;
			delete PlotterList[ID];
			PlotterList[ID] = NULL;
			cout << "Plotter ID:" << ID << " freed\n";
		}
	}
	static Plotter* GetPlotter(int ID)
	{
		return (PlotterList[ID] != NULL) ? PlotterList[ID]->Wnd : NULL;
	}
	static void Cleanup()
	{
		for (int i = 0; i < MAX_PLOTTER; ++i)
			FreePlotter(i);
	}
};

PlotterFactory::PlotterEntry* PlotterFactory::PlotterList[MAX_PLOTTER] = { NULL };

Fl_Hold_Browser* PtrBrow;

int RequestHandler(TCPSocket*Client, const char*Request, int ReqSize)
{
	FieldRetriver Header(Request);
	std::string&& Cmd = Header.String(4);
	if (Cmd == "NEWF")
	{
		char Type = Header.Char();
		switch (Type)
		{
		case 'a':
			{
				std::string&& FigName = Header.String(16);
				std::string&& XName = Header.String(16);
				std::string&& YName = Header.String(16);
				Invoke(WRAPCALL([=]{
					int ID = PlotterFactory::AllocPlotter(Type); 
					std::string&& IDStr = std::to_string(ID);
					Plotter* Wnd = PlotterFactory::GetPlotter(ID);
					Wnd->SetTitle(FigName.c_str());
					Wnd->SetXText(XName.c_str());
					Wnd->SetYText(YName.c_str());
					PtrBrow->add((IDStr + '\t' + std::to_string(Type)).c_str(), (void*)ID);
					Client->Send(IDStr);
				}));
				break;
			}
		default:
			break;
		}

	}
	else if (Cmd == "FREE")
	{
		int FreeID = Header.Int();

		for (int i = 1; i <= PtrBrow->size(); ++i)
		{
			if (PtrBrow->data(i) == (void*)FreeID)
			{
				Fl::lock();
				PtrBrow->remove(i);
				Fl::unlock();
				// Seems this is the only way to hide window from another thread
				Invoke(WRAPCALL(PlotterFactory::FreePlotter, FreeID));
				Client->Send("OK");
				break;
			}
		}
		Client->Send("NEG");
	}
	else if (Cmd == "DRAW")
	{
		int ID = Header.Int();
		int Arg = Header.Int();
		int DataType = Header.Int();
		int Size = Header.Int();
		
		Plotter* Plt = PlotterFactory::GetPlotter(ID);
		Fl::lock();
		Plt->Plot((float*)Header.Ptr(), Size);
		Fl::unlock();
		Client->Send("OK");
	}
	else
	{
		Client->Send("NEG");
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
	SetConsoleCtrlHandler(ExitHandler, TRUE);

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
	//Fl::add_timeout(0, TimerFunc);
	int Ret = Fl::run();

	NetHelper::Cleanup();
	Exit = 1;
	EvtNewClient.notify_one();
	ThrdIPCListener.join();
	ThrdReqListener.join();

	return Ret;
}


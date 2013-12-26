#include "JPlot.h"
#include "netlayer.h"
#include "IPC.h"
#include <cstdarg>
#include <iostream>
#include <string>

#define INTERNAL

JPlot Instance;
const char* JPGraphName[] =
{
	"2D"
};

int JPlot_Run()INTERNAL
{
#ifdef WIN32
	HINSTANCE ret = ShellExecute(NULL, L"open", L"JPlot.exe", NULL, NULL, SW_NORMAL);
	return (int)ret > 32;
#elif LINUX

#endif
}

IPCObj JPlot_SetupEvent()INTERNAL
{
	return IPCHelper::CreateIPCEvent("JPlotEVENT");
}

void JPlot_DestroyEvent(IPCObj Event)INTERNAL
{
	IPCHelper::DestroyIPCEvent(Event);
}

int JPlot_WaitStartupWithTimeout(IPCObj Event, int Milli)INTERNAL
{
	return IPCHelper::WaitOnIPCEvent(Event, Milli);
}

int JPlot_Init()
{
	if (Instance)
		return 1;

	char buf[512];
	TCPSocket* Conn = new TCPSocket("localhost", JPLOT_PORT);
	if (!Conn->Connect())
	{
		IPCObj Evnt = JPlot_SetupEvent();
		JPlot_Run();
		int ret = JPlot_WaitStartupWithTimeout(Evnt, 100);
		Conn->Connect();
		Conn->Recv(buf, 512);
		Instance = ret ? Conn : NULL;
		JPlot_DestroyEvent(Evnt);
		return ret;
	}
	Conn->Recv(buf, 512);
	Instance = Conn;
	return 1;
}

#define ARGCHAR va_arg(args,char)
#define ARGINT va_arg(args,int)
#define ARGPTR va_arg(args,char*)
#define ARGDBL va_arg(args,double)
#define ARGFLT ((float)ARGDBL) // float is promoted to double

std::string JPlot_Command(int Command, ...)
{
	if (!Instance)
		throw;

	va_list args;
	
	char SendBuffer[8192];
	memset(SendBuffer, 0, 8192);
	TCPSocket* Socket = (TCPSocket*)Instance;
	FieldFiller MsgBuilder(SendBuffer);
	va_start(args, Command);
	switch (Command)
	{
	case JPNEWF:
		MsgBuilder.String("NEWF");
		MsgBuilder.Int(ARGINT);
		MsgBuilder.String(ARGPTR, 16);
		MsgBuilder.String(ARGPTR, 16);
		MsgBuilder.String(ARGPTR, 16);
		MsgBuilder.Int(ARGINT);
		break; 
	case JPDRAW:
		{
			MsgBuilder.String("DRAW");
			MsgBuilder.Int(ARGINT);
			MsgBuilder.Int(ARGINT);
			int Type = ARGINT; // Data type
			MsgBuilder.Int(Type); 
			int Size = ARGINT;
			MsgBuilder.Int(Size);
			// Buffer size needs to be handled according to type size here
			MsgBuilder.Buf(ARGPTR, Size * sizeof(float));
			break;
		}
	case JPFREE:
		MsgBuilder.String("FREE").Int(ARGINT);
		break;
	case JPSETF:
		{
			int Setting = ARGINT;
			MsgBuilder.String("SETF");
			MsgBuilder.Int(Setting);
			MsgBuilder.Int(ARGINT); // ID
			switch (Setting)
			{
			case JPXRANGE:
			case JPYRANGE:
				{
					float Para1 = ARGFLT;
					float Para2 = ARGFLT;
					MsgBuilder.Float(Para1);
					MsgBuilder.Float(Para2);
					break;
				}
			default:
				break;
			}
		}
	default:
		break;
	}
	va_end(args);
	Socket->Send(SendBuffer, MsgBuilder.Pos());
	size_t RetSize = Socket->Recv(SendBuffer, 8192);
	return{ SendBuffer, RetSize >= 16 || RetSize < 0 ? 0 : RetSize };
}

JGraph JPlot_NewPlot(string GraphName, string XLabel, string YLabel, int GraphType)
{
	int ID = std::stoi(JPlot_Command(JPNEWF, GraphType, GraphName.c_str(), XLabel.c_str(), YLabel.c_str(), 0));
	JGraph ret = new JGraph_ctx;
	ret->GraphType = GraphType;
	ret->ID = ID;
	return ret;
}

int JPlot_Draw(JGraph J, float* Buf, int Size)
{
	if (!J || Size <= 0)
		return 0;
	int Arg = 0;
	int Type = 0;
	JPlot_Command(JPDRAW, J->ID, Arg, Type, Size, Buf);
	return 1;
}

int JPlot_Close(JGraph J)
{
	if (!J)
		return 0;
	if (JPlot_Command(JPFREE, J->ID).find("OK") >= 0)
	{
		delete J;
		return 1;
	}
	return 0;
}

int JPlot_SetRange(JGraph J, int Axis, float Min, float Max)
{
	if (!J)
		return 0;
	JPlot_Command(JPSETF, Axis, J->ID, Min, Max);
	return 1;
}

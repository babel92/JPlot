#include "JPlot.h"
#include "netlayer.h"
#include "IPC.h"
#include <cstdarg>
#include <iostream>
#include <string>

JPlot Instance;

int JPlot_Run()
{
	HINSTANCE ret = ShellExecute(NULL, L"open", L"..\\..\\JPlot\\Debug\\JPlot.exe", NULL, NULL, SW_NORMAL);
	return (int)ret > 32;
}

IPCObj JPlot_SetupEvent()
{
	return IPCHelper::CreateIPCEvent("JPlotEVENT");
}

void JPlot_DestroyEvent(IPCObj Event)
{
	IPCHelper::DestroyIPCEvent(Event);
}

int JPlot_WaitStartupWithTimeout(IPCObj Event, int Milli)
{
	return IPCHelper::WaitOnIPCEvent(Event, 1000);
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

enum JPCOMMAND
{
	JPNEWF,
	JPDRAW,
	JPFREE
};

#define ARGCHAR va_arg(args,char)
#define ARGINT va_arg(args,int)
#define ARGPTR va_arg(args,char*)

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
		MsgBuilder.Char(ARGCHAR);
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
			MsgBuilder.Int(ARGINT);
			int Size = ARGINT;
			MsgBuilder.Int(Size);
			MsgBuilder.Buf(ARGPTR, Size);
			break;
		}
	case JPFREE:
		MsgBuilder.String("FREE").Int(ARGINT);
		break;
	default:
		break;
	}
	va_end(args);
	Socket->Send(SendBuffer, MsgBuilder.Pos());
	return{ SendBuffer, (size_t)Socket->Recv(SendBuffer, 8192) };
	
}

JGraph JPlot_NewPlot(char GraphType)
{
	JGraph ret = new JGraph_ctx;
	ret->GraphType = GraphType;
	ret->ID = std::stoi(JPlot_Command(JPNEWF, GraphType, "Nima", "X", "Y", 0));
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
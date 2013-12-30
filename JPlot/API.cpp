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
	pid_t plotproc = fork();
	if(plotproc<0)
	{
		fprintf(stderr, "Failed to fork\n");
		exit(1);
	}
	else if(plotproc==0)
	{
		execv("jplot",NULL);
		return 0;
	}
	else
		return 1;
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
		int ret = JPlot_WaitStartupWithTimeout(Evnt, 0);
		if(ret!=1)
		{
			fprintf(stderr,"Wait failed: %d\n",errno);
		}
		JPlot_DestroyEvent(Evnt);
		if(!Conn->Connect())
		{
			fprintf(stderr, "Failed to connect: %d\n", errno);
			exit(1);
		}
		if(-1==Conn->Recv(buf, 512))
			goto fail;
		Instance = ret ? Conn : NULL;
		return ret;
	}
	if(-1==Conn->Recv(buf, 512))
		goto fail;
	
	Instance = Conn;
	return 1;
fail:
	fprintf(stderr,"Socket failure: %d\n",errno);
	exit(1);
}

#define ARGCHAR va_arg(args,char)
#define ARGINT va_arg(args,int)
#define ARGPTR(TYPE) va_arg(args,TYPE)
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
		MsgBuilder.String(ARGPTR(char*), 16);
		MsgBuilder.String(ARGPTR(char*), 16);
		MsgBuilder.String(ARGPTR(char*), 16);
		MsgBuilder.Int(ARGINT);
		break; 
	case JPDRAW:
		{
			JGraph ctx = ARGPTR(JGraph); // Graph context
			int Arg = ARGINT; // Arg
			int Type = ARGINT; // Data type
			int Size = ARGINT; // Size
			MsgBuilder.String("DRAW");
			MsgBuilder.Int(ctx->ID); 
			MsgBuilder.Int(Arg); 
			MsgBuilder.Int(Type); 
			MsgBuilder.Int(Size);
			int CalculatedSize = Size;
			switch (Type)
			{
			case JPFLOAT:
				CalculatedSize *= sizeof(float);
				break;
			case JPDOUBLE:
				CalculatedSize *= sizeof(double);
				break;
			case JPINT:
				CalculatedSize *= sizeof(int);
				break;
			}
			switch (ctx->GraphType)
			{
			case JP2D:
				switch (Arg)
				{
				case JP2COORD:
					CalculatedSize *= 2;
				case JP1COORD:
					MsgBuilder.Buf(ARGPTR(char*), CalculatedSize);
					break;
				case JP2COORDSEP:
					{
						float** PtrArr = ARGPTR(float**);
						MsgBuilder.Buf(PtrArr[0], CalculatedSize);
						MsgBuilder.Buf(PtrArr[1], CalculatedSize);
						break;
					}
				}
				break;
			}
			
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
	int SendSize = MsgBuilder.Pos();
	int ActualSendSize = Socket->Send(SendBuffer, SendSize);
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

int JPlot_DrawBase(JGraph J, int Arg, int Type, void* Buf, int Size)INTERNAL
{
	if (!J || Size <= 0)
		return 0;
	JPlot_Command(JPDRAW, J, Arg, Type, Size, Buf);
	return 1;
}

int JPlot_Draw(JGraph J, float* Buf, int Size)
{
	return JPlot_DrawBase(J, JP1COORD, JPFLOAT, Buf, Size);
}

int JPlot_Draw2(JGraph J, float (*Buf)[2], int Size)
{
	return JPlot_DrawBase(J, JP2COORD, JPFLOAT, Buf, Size);
}

int JPlot_Draw2(JGraph J, float*X, float*Y, int Size)
{
	float* PtrArr[2] = { X, Y };
	return JPlot_DrawBase(J, JP2COORDSEP, JPFLOAT, PtrArr, Size);
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

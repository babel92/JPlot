#ifndef JPLOT
#define JPLOT

#include <string>
#include <vector>

#define JPLOT_PORT "64129"

typedef void* JPlot;

class TCPSocket;

typedef struct JGraph_ctx
{
	int ID;
	int GraphType;
}*JGraph;

struct Client
{
	TCPSocket* Socket;
	std::vector<int> GraphList;
};

enum JPCOMMAND
{
	JPNEWF,
	JPDRAW,
	JPFREE,
	JPSETF
};

enum JPSETTING
{
	JPXRANGE,
	JPYRANGE
};

enum JPDATATYPE
{
	JPFLOAT,
	JPDOUBLE,
	JPINT
};

enum JPGRAPHTYPE
{
	JP2D
};

enum JPARG
{
	JPFORCEREDRAW = 1,
	RSRV1 = 2,
	RSRV2 = 4,
	RSRV3 = 8,
	RSRV4 = 16
};

enum JP2DARG
{
	JP1COORD = 32,
	JP2COORD = 64,
	JP2COORDSEP = 128
};

extern const char* JPGraphName[];

int JPlot_Init();
std::string JPlot_Command(int Command, ...);
JGraph JPlot_NewPlot(std::string GraphName="Who Cares", std::string XLabel="X", std::string YLabel="Y", int GraphType = JP2D);
int JPlot_Draw(JGraph J, float* Buf, int Size);
int JPlot_Draw2(JGraph J, float(*Buf)[2], int Size);
int JPlot_Draw2(JGraph J, float*X, float*Y, int Size);
int JPlot_SetRange(JGraph J, int Axis, float Min, float Max);
int JPlot_Close(JGraph J);

#endif
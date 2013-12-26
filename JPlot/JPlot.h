#ifndef JPLOT
#define JPLOT

#include <string>
#include <vector>

#define JPLOT_PORT "64129"

typedef void* JPlot;

class TCPSocket;

struct Client
{
	TCPSocket* Socket;
	std::vector<int> GraphList;
};

typedef struct JGraph_ctx
{
	int ID;
	char GraphType;
}*JGraph;

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

enum JPGRAPHTYPE
{
	JP2D
};

extern const char* JPGraphName[];

int JPlot_Init();
std::string JPlot_Command(int Command, ...);
JGraph JPlot_NewPlot(std::string GraphName="Who Cares", std::string XLabel="X", std::string YLabel="Y", int GraphType = JP2D);
int JPlot_Draw(JGraph J, float* Buf, int Size);
int JPlot_SetRange(JGraph J, int Axis, float Min, float Max);
int JPlot_Close(JGraph J);

#endif
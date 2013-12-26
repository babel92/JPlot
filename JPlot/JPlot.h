#ifndef JPLOT
#define JPLOT

#include <string>

#define JPLOT_PORT "64129"

typedef void* JPlot;

typedef struct JGraph_ctx
{
	int ID;
	char GraphType;
}*JGraph;

enum JPCOMMAND
{
	JPNEWF,
	JPDRAW,
	JPFREE
};

int JPlot_Init();
std::string JPlot_Command(int Command, ...);
JGraph JPlot_NewPlot(std::string GraphName="Who Cares", std::string XLabel="X", std::string YLabel="Y", char GraphType = 'a');
int JPlot_Draw(JGraph J, float* Buf, int Size);
int JPlot_Close(JGraph J);

#endif
#ifndef JPLOT
#define JPLOT

#define JPLOT_PORT "64129"

typedef void* JPlot;

typedef struct JGraph_ctx
{
	int ID;
	char GraphType;
}*JGraph;

int JPlot_Init();
JGraph JPlot_NewPlot(char GraphType = 'a');
int JPlot_Draw(JGraph J, float* Buf, int Size);
int JPlot_Close(JGraph J);

#endif
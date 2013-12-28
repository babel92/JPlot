#ifndef PLOTTER_FACTORY
#define PLOTTER_FACTORY

#include "Plotter.h"

class PlotterFactory
{
	struct PlotterEntry
	{
		Plotter* Wnd;
		long ID;
		long Type;
		PlotterEntry(Plotter* Plotter_, long ID_, long Type_) :Wnd(Plotter_), ID(ID_), Type(Type_){}
	};
	static const int MAX_PLOTTER = 16;
	static PlotterEntry* PlotterList[MAX_PLOTTER];
public:
	static int AllocPlotter(long Type);
	static void FreePlotter(long ID);
	static Plotter* GetPlotter(long ID);
	static void Cleanup();

};

#endif

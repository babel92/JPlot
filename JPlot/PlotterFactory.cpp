#include "PlotterFactory.h"
#include <iostream>

using namespace std;

PlotterFactory::PlotterEntry* PlotterFactory::PlotterList[MAX_PLOTTER] = { NULL };

int PlotterFactory::AllocPlotter(char Type)
{
	for (int i = 0; i < MAX_PLOTTER; ++i)
	{
		if (PlotterList[i] == NULL)
		{
			PlotterList[i] = new PlotterEntry(new Plotter("A", "B", "C"), i, Type);
			return i;
		}
	}
	return -1;
}

void PlotterFactory::FreePlotter(int ID)
{
	static std::mutex mu;
	std::lock_guard<std::mutex> lock(mu);
	if (PlotterList[ID] != NULL)
	{
		PlotterList[ID]->Wnd->hide();
		delete PlotterList[ID]->Wnd;
		delete PlotterList[ID];
		PlotterList[ID] = NULL;
		cout << "Plotter ID:" << ID << " freed\n";
	}
}

Plotter* PlotterFactory::GetPlotter(int ID)
{
	return (PlotterList[ID] != NULL&&ID < MAX_PLOTTER) ? PlotterList[ID]->Wnd : NULL;
}

void PlotterFactory::Cleanup()
{
	for (int i = 0; i < MAX_PLOTTER; ++i)
		FreePlotter(i);
}
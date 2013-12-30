#include <FL/Fl.H>
#include <FL/Fl_Window.H>
#include <FL/Fl_Hold_Browser.H>
#include <string>
#include <algorithm>
#include "netlayer.h"
#include "PlotterFactory.h"
#include "safecall.h"
#include "JPlot.h"

extern Fl_Hold_Browser* PtrBrow;

int FindAndDeleteUIEntry(long FreeID, int ThreadSafe = 0)
{
	for (int i = 1; i <= PtrBrow->size(); ++i)
	{
		if (PtrBrow->data(i) == (void*)FreeID)
		{
			if (ThreadSafe)
				Fl::lock();
			PtrBrow->remove(i);
			if (ThreadSafe)
				Fl::unlock();
			return 1;
		}
	}
	return 0;
}

#define DELETE_ENTRY 10

#define SND(str)	Peer->Socket->Send(str);
#define SNDERR(err) SND("NEG " err)
#define SNDACK()	SND("OK")
#define SIZE_GUARD(Size) \
if (ReqSize < Size)\
{\
	SNDERR("SZERROR"); \
	return 0; \
}

int RequestHandler(Client*Peer, const char*Request, int ReqSize)
{

	FieldRetriver Header(Request);
	std::string&& Cmd = Header.String(4);
	Header.Int();
	//cout<<Request<<endl;
	if (Cmd == "NEWF")
	{
		SIZE_GUARD(60);
		long Type = Header.Int();
		switch (Type)
		{
		case JP2D:
			{
				std::string&& FigName = Header.String(16);
				std::string&& XName = Header.String(16);
				std::string&& YName = Header.String(16);
				int Arg = Header.Int();
				Invoke(WRAPCALL([=]{
					long ID = PlotterFactory::AllocPlotter(Type);
					std::string&& IDStr = std::to_string(ID);
					Plotter* Wnd = PlotterFactory::GetPlotter(ID);
					Wnd->SetData((void*)Type);
					Wnd->callback([](Fl_Widget*Wid, void*Arg){
						((Fl_Window*)Wid)->hide();
						PlotterFactory::FreePlotter((long)Arg);
						FindAndDeleteUIEntry((long)Arg);
					}, (void*)ID);
					Wnd->SetTitle(FigName.c_str());
					Wnd->SetXText(XName.c_str());
					Wnd->SetYText(YName.c_str());
					PtrBrow->add((IDStr + '\t' + JPGraphName[Type] + '\t' + FigName.c_str()).c_str(), (void*)ID);
					Peer->GraphList.push_back(ID);
					SND(IDStr);
				}));
				break;
				}
		default:
			SNDERR("TYPEERR");
			break;
		}

	}
	else if (Cmd == "SETF")
	{
		// SETTING, ID , PARA1 , PARA2
		SIZE_GUARD(16);
		int Setting = Header.Int();
		Plotter* Plot = PlotterFactory::GetPlotter(Header.Int());
		switch (Setting)
		{
		case JPXRANGE:
			{
				float XMin = Header.Float();
				float XMax = Header.Float();
				Fl::lock();
				Plot->SetXMin(XMin);
				Plot->SetXMax(XMax);
				Fl::unlock();
				SNDACK();
				break;
			}
		case JPYRANGE:
			{
				float YMin = Header.Float();
				float YMax = Header.Float();
				Fl::lock();
				Plot->SetYMin(YMin);
				Plot->SetYMax(YMax);
				Fl::unlock();
				SNDACK();
				break;
			}
		default:
			SNDERR("NOSETTING");
			break;
		}
	}
	else if (Cmd == "FREE")
	{
		int FreeID = Header.Int();
		if (FindAndDeleteUIEntry(FreeID, 1)) // Thread safe flag set to 1
		{
			// Seems this is the only way to hide window from another thread
			Invoke(WRAPCALL(PlotterFactory::FreePlotter, FreeID));
			Peer->GraphList.erase(std::remove(
				Peer->GraphList.begin(), 
				Peer->GraphList.end(), FreeID), 
				Peer->GraphList.end()
			);
			SNDACK();
			return DELETE_ENTRY;
		}
		SNDERR("NOFIG");
	}
	else if (Cmd == "DRAW")
	{
		SIZE_GUARD(22);
		int ID = Header.Int();
		Plotter* Plt = PlotterFactory::GetPlotter(ID);
		if (Plt == NULL)
		{
			SNDERR("NOFIG");
			return 0;
		}
		int Arg = Header.Int();
		int DataType = Header.Int();
		int Size = Header.Int();

		Fl::lock();
		switch ((long)Plt->GetData())
		{
		case JP2D:
			switch (Arg)
			{
			case JP1COORD:
				{
					float* buf = (float*)Header.Ptr();
					for (int i = 0; i < Size;++i)
						if (buf[i] > 1)
							throw;
					Plt->Plot((float*)Header.Ptr(), Size);
					break;
				}
			case JP2COORD:
				Plt->Plot2D((float(*)[2])Header.Ptr(), Size);
				break;
			case JP2COORDSEP:
				{
					float* X = (float*)Header.Ptr();
					Plt->Plot2D(X, X + Size, Size);
					break;
				}
			}
			break;
		}
		/*
		if (Arg == 1)
			Fl::awake();
		*/
		Fl::unlock();
		SNDACK();
	}
	else
	{
		SNDERR("NOCMD");
	}

	return 0;
}

#ifndef PLOTTER_H
#define PLOTTER_H

#include <queue>
#include <FL/Fl.H>
#include <FL/Fl_Overlay_Window.H>
#include <FL/Fl_Button.H>
#include <FL/fl_draw.H>
#include "Cartesian.H"
#include <thread>
#include <mutex>
#include <string>
#include <condition_variable>

typedef float Real;
using std::queue;
using std::string;

class Plotter:public Fl_Double_Window
{
    friend void PlotterThread(void*);
    friend void APCWrapper(void*);
    public:
		Plotter(const string FigName, const string XName, const string YName);

        void SetBGColor(float R,float G,float B);
        void Plot(Real*buf,int size);
        void Plot2D(Real*data1,Real*data2,int size);

        void SetXText(const char*label){m_x->copy_label(label);}
        void SetXMin(double num){m_x->minimum(num);}
        void SetXMax(double num){m_x->maximum(num);}
        void SetYText(const char*label){m_y->copy_label(label);}
        void SetYMin(double num){m_y->minimum(num);}
        void SetYMax(double num){m_y->maximum(num);}

        void SetTitle(const char*title){copy_label(title);}
        virtual ~Plotter();
    protected:
    private:

        double m_xmin,m_xmax,m_ymin,m_ymax;

        Ca_Canvas*m_canvas;
        Ca_X_Axis*m_x;
        Ca_Y_Axis*m_y;
        Fl_Group*m_group;
};

#define GUARD(func,...) {\
    if(InvokeRequired())\
    {\
        Invoke(WRAPCALL(&func,__VA_ARGS__));\
        return;\
    }}


#endif // PLOTTER_H

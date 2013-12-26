#include "Plotter.h"
#include <iostream>

using namespace std;
static const int FPS=30;

void Redraw(void*canvas)
{
    Fl::add_timeout(0,Redraw,canvas);
}

void APCWrapper(void* plotter);

void timer_cb(void *v) {
   Fl::repeat_timeout(double(1.0)/FPS,timer_cb);
}

void PlotterThread(void*plotter)
{
    Fl::lock();
    Plotter *ptr=(Plotter*)plotter;
    APCWrapper((void*)plotter);

    Fl::add_timeout(0, timer_cb);
    cout<<"Plotter thread finished initialization\n";
/*
    while(Fl::wait()>0)
    {
    }*/
    Fl::run();
    cout<<"Plotter thread quit\n";
}

void APCWrapper(void* plotter)
{
    Plotter *ptr=(Plotter*)plotter;

    ptr->size_range(450,250);
    ptr->callback([](Fl_Widget*w,void*arg) { w->hide();delete (Plotter*)arg; },(void*)ptr);

    ptr->m_group =new Fl_Group(0, 0, 580, 390 );
    ptr->m_group->box(FL_DOWN_BOX);
    ptr->m_group->align(FL_ALIGN_TOP|FL_ALIGN_INSIDE);

    ptr->m_canvas = new Ca_Canvas(50, 30, 480, 290, NULL);
    ptr->m_canvas->box(FL_DOWN_BOX);
    ptr->m_canvas->color(7);
    ptr->m_canvas->align(FL_ALIGN_TOP);
    Fl_Group::current()->resizable(ptr->m_canvas);
    ptr->m_canvas->border(15);

    ptr->m_x = new Ca_X_Axis(140, 335, 300, 30, "X");
    ptr->m_x->labelsize(14);
    ptr->m_x->align(FL_ALIGN_BOTTOM);
    ptr->m_x->scale(CA_LIN);
    ptr->m_x->minimum(ptr->m_xmin);
    ptr->m_x->maximum(ptr->m_xmax);
    ptr->m_x->label_format("%g");
    ptr->m_x->minor_grid_color(fl_gray_ramp(20));
    ptr->m_x->major_grid_color(fl_gray_ramp(15));
    ptr->m_x->label_grid_color(fl_gray_ramp(10));
    ptr->m_x->grid_visible(CA_MINOR_GRID|CA_MAJOR_GRID|CA_LABEL_GRID);
    ptr->m_x->major_step(10);
    ptr->m_x->label_step(10);
    ptr->m_x->axis_color(FL_BLACK);
    ptr->m_x->axis_align(CA_BOTTOM|CA_LINE);

    ptr->m_y = new Ca_Y_Axis(5, 30, 43, 235 /*, "I [mA]" */);
    ptr->m_y->label("Y");
    ptr->m_y->align(FL_ALIGN_LEFT|FL_ALIGN_TOP);
    //ptr->m_yalign(FL_ALIGN_TOP_RIGHT);
    ptr->m_y->minimum(ptr->m_ymin);
    ptr->m_y->maximum(ptr->m_ymax);
    ptr->m_y->minor_grid_style(FL_DASH);
    ptr->m_y->axis_align(CA_LEFT);
    ptr->m_y->axis_color(FL_BLACK);

    ptr->m_y->current();
    ptr->m_group->end();

    Fl_Group::current()->resizable(ptr->m_group);

    ptr->end();
    ptr->show();

}

Plotter::Plotter(const string FigName, const string XName, const string YName)
:Fl_Double_Window(580, 390, "Plotter"), m_xmin(0), m_ymin(0), m_xmax(100), m_ymax(100)
{
	APCWrapper(this);
	SetTitle(FigName.c_str());
	SetXText(XName.c_str());
	SetYText(YName.c_str());
}

void Plotter::Plot(Real*buf,int size)
{
    // Duplicate memory to ensure safety, we might use Ca_LinePoint
    /*
    if(InvokeRequired())
    {
        Real*newbuf=new Real[size];
        memcpy(newbuf,buf,size*sizeof(Real));
        Invoke(WRAPCALL(&Plotter::Plot,this,newbuf,size));
        return;
    }*/
	if (m_canvas == NULL)
		return;
    Ca_LinePoint* lp=NULL;
    Ca_Canvas::current(m_canvas);
    m_y->current();
    m_canvas->clear();
    for(int i=0;i<size;++i)
        lp=new Ca_LinePoint(lp,i,buf[i],0,FL_BLUE);
    redraw();
    //delete[] buf;
}

void Plotter::Plot2D(Real*data1,Real*data2,int size)
{
    Ca_LinePoint* lp=NULL;
    Ca_Canvas::current(m_canvas);
    m_y->current();
    m_canvas->clear();
    for(int i=0;i<size;++i)
        lp=new Ca_LinePoint(lp,data1[i],data2[i],0,FL_BLUE);
    m_canvas->redraw();
}

Plotter::~Plotter()
{
	// Destruction of axes are done by canvas' dtor
	delete m_canvas;
	m_canvas = NULL;
	delete m_group;
	m_group = NULL;
    //dtor
}

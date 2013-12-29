CPP=g++-4.8
CPPFLAGS=-c -IJPlot/ -std=c++11 -DLINUX -ggdb3 -Wall
LIBS=`fltk-config --ldflags`
OBJDIR=obj
SRCS=$(wildcard JPlot/*.cpp)
OBJS=$(addprefix $(OBJDIR)/,$(notdir $(SRCS:.cpp=.o)))

$(OBJDIR)/%.o: JPlot/%.cpp
	$(CPP) $(CPPFLAGS) -o$@ $<

all: $(OBJS)
	$(CPP) -g -o jplot $(OBJS) $(LIBS)
	ar rs JPLib/libjplot.a obj/API.o obj/netlayer.o obj/IPC.o

clean:
	rm -rf obj

$(OBJS): | $(OBJDIR)

$(OBJDIR):
	mkdir $(OBJDIR)


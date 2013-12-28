CPP=g++-4.8
CPPFLAGS=-c -IJPlot/ -std=c++11 -DLINUX -g
LIBS=`fltk-config --ldflags`
OBJDIR=obj
SRCS=$(wildcard JPlot/*.cpp)
OBJS=$(addprefix $(OBJDIR)/,$(notdir $(SRCS:.cpp=.o)))

$(OBJDIR)/%.o: JPlot/%.cpp
	$(CPP) $(CPPFLAGS) -o$@ $<

all: $(OBJS)
	$(CPP) $(OBJS) $(LIBS)

clean:
	rm -rf obj

$(OBJS): | $(OBJDIR)

$(OBJDIR):
	mkdir $(OBJDIR)


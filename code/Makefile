
EXE = ispd19dr 
CXX = g++
SYSTEM = x86-64_linux
LIBFORMAT = static_pic

#CXXOPTS = -m64 -O2 -pg -fopenmp -fPIC -fno-strict-aliasing -fexceptions -fpermissive -DNDEBUG -DIL_STD -Wno-ctor-dtor-privacy 
CXXOPTS = -m64 -O1 -fopenmp -fPIC -fno-strict-aliasing -fexceptions -fpermissive -DNDEBUG -DIL_STD -Wno-ctor-dtor-privacy 
#CFLAGS = $(CXXOPTS) $(LIBINCLUDE) -fopenmp -std=c++11

#CFLAGS_NW = -std=c++11 -static-libstdc++ -O2 -pg -fopenmp -fpermissive -g -ggdb -DNDEBUG -DIL_STD -Wno-ctor-dtor-privacy $(LIBINCLUDE)
#CFLAGS = -std=c++11 -static-libstdc++ -O2 -pg -fopenmp -fpermissive -g -W -ggdb -DNDEBUG -DIL_STD -Wno-ctor-dtor-privacy $(LIBINCLUDE)
CFLAGS_NW = -std=c++11 -static-libstdc++ -O2 -fopenmp -fpermissive -DNDEBUG -DIL_STD -Wno-ctor-dtor-privacy $(LIBINCLUDE)
CFLAGS = -std=c++11 -static-libstdc++ -O2 -fopenmp -fpermissive -DNDEBUG -DIL_STD -Wno-ctor-dtor-privacy $(LIBINCLUDE)

# include library settings..
LIBINCLUDE = -I$(SRCHASH)/include -I$(SRCDEF)/include -I$(SRCLEF)/include -I$(BOOST)/include -I$(CPLEXDIR) -I$(CONCERTDIR)

# linking flag settings
LINKCFLAGS = $(CFLAGS) -static-libstdc++ $(DEFLIBS) $(LEFLIBS) $(CPLEXLIBS) $(CONCERTLIBS) $(LIBGOMP)

# source settings.
SRCLEF = ../lib/lef/5.8-p027
SRCDEF = ../lib/def/5.8-p027
SRCHASH = /home/tool/cpplib/sparsehash/install/2.0.3
BOOST = /home/tool/cpplib/boost/install/1.67.0
CPLEXDIR = /home/tool/ibm/install/12.7.0/cplex/include 
CONCERTDIR = /home/tool/ibm/install/12.7.0/concert/include

DEFLIBS         = -L$(SRCDEF)/lib -ldef -ldefzlib -lcdef -lcdefzlib
LEFLIBS         = -L$(SRCLEF)/lib -llef -llefzlib -lclef -lclefzlib

CPLEXLIBS 		= -L/home/tool/ibm/install/12.7.0/cplex/lib/$(SYSTEM)/$(LIBFORMAT) -lilocplex -lcplex
CONCERTLIBS 	= -L/home/tool/ibm/install/12.7.0/concert/lib/$(SYSTEM)/$(LIBFORMAT) -lconcert -lm -pthread -fopenmp -static-libstdc++ -std=c++11

DEBUG = -g -gstrict-dwarf -gdwarf-2

LIBGOMP = -L/home/tool/gcc/install/5.4.0/lib64/libgomp.a -static

SRC = main.o lefreader.o defreader.o guidereader.o mymeasure.o \
	  hgFunc.o hgInit.o hgGrid.o hgRtree.o hgPinAccess.o hgGR.o hgNet.o \
	  hgPreroute.o hgDRC.o hgGeometry.o hgOverlap.o hgPinExt.o 

$(EXE): $(SRC)
	@echo " "
	@echo "Linking..."
	@echo " "
	#$(CXX) $(DEBUG) $(CXXOPTS) $^ -o $@ $(LINKCFLAGS)
	$(CXX) $(CXXOPTS) $^ -o $@ $(LINKCFLAGS)
	@echo " "
	@echo "Linking is Completed !!"
	@echo " "
	@echo "Tagging..."
	@echo " "
	ctags -R
	@echo " "
	@echo "Finished..!"
	@echo " "
	@echo "GNU GCC/G++ compiler has been used as"
	@g++ --version

defreader.o: defreader.cpp hgCircuit.h
	$(CXX) $(CFLAGS_NW) -c -o $@ $<

lefreader.o: lefreader.cpp hgCircuit.h
	$(CXX) $(CFLAGS_NW) -c -o $@ $<

guidereader.o: guidereader.cpp hgCircuit.h
	$(CXX) $(CFLAGS_NW) -c -o $@ $<

main.o: main.cpp hgCircuit.h 
	$(CXX) $(CFLAGS) -c -o $@ $<

hgFunc.o: hgFunc.cpp hgRouter.h
	$(CXX) $(CFLAGS) -c -o $@ $<

hgInit.o: hgInit.cpp hgRouter.h
	$(CXX) $(CFLAGS) -c -o $@ $<

hgGrid.o: hgGrid.cpp hgGrid.h hgRouter.h
	$(CXX) $(CFLAGS) -c -o $@ $<

hgNet.o: hgNet.cpp hgCircuit.h
	$(CXX) $(CFLAGS) -c -o $@ $<

hgRtree.o: hgRtree.cpp hgRtree.h hgRouter.h
	$(CXX) $(CFLAGS) -c -o $@ $<

hgPinAccess.o: hgPinAccess.cpp hgRouter.h
	$(CXX) $(CFLAGS) -c -o $@ $<

#shape.o: shape.cpp router.h
#	$(CXX) $(CFLAGS) -c -o $@ $<

hgGeometry.o: hgGeometry.cpp hgGeometry.h hgRouter.h
	$(CXX) $(CFLAGS) -c -o $@ $<

#topo.o: topo.cpp router.h topo.h
#	$(CXX) $(CFLAGS) -c -o $@ $<

hgDRC.o: hgDRC.cpp hgDRC.h hgRouter.h
	$(CXX) $(CFLAGS) -c -o $@ $<

hgPreroute.o: hgPreroute.cpp hgRouter.h
	$(CXX) $(CFLAGS) -c -o $@ $<

hgOverlap.o: hgOverlap.cpp hgRouter.h
	$(CXX) $(CFLAGS) -c -o $@ $<

hgPinExt.o: hgPinExt.cpp hgRouter.h
	$(CXX) $(CFLAGS) -c -o $@ $<

hgGR.o: hgGR.cpp hgRouter.h
	$(CXX) $(CFLAGS) -c -o $@ $<


mymeasure.o: mymeasure.cpp mymeasure.h
	$(CXX) $(CFLAGS) -c -o $@ $<


.PHONY: clean
clean: 
	/bin/rm -f *.o 2> /dev/null;  /bin/rm -f $(EXE) 2> /dev/null; /bin/rm -f defreader defwriter lefreader lefwriter 2> /dev/null; /bin/rm -f *.log;



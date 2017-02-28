CPPFLAGS = -Wall -O3 -std=c++11 -lm -w
PROGRAMS = main 

all: $(PROGRAMS)

main:main.cpp PCMSketch.h PCUSketch.h PCSketch.h PASketch.h \
	params.h BOBHash.h CMSketch.h CUSketch.h CSketch.h ASketch.h
	g++ -o main main.cpp $(CPPFLAGS)

clean:
	rm -f *.o $(PROGRAMS)

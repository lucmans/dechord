##
# Makefile: The makefile for latency, which plays audio in back with specified latency
# @author Luc de Jonckheere
##

# Min required -std=c++11


# General compiler flags
CXX = g++
CXXFLAGS = -std=c++11
WARNINGS = -Wall -Wextra -Wfloat-equal #-Wconversion -Warith-conversion
# FLAGS = -DCOLORED
OPTIMIZATIONS = -O3 -march=native -mtune=native # -mfma -mavx2 -ftree-vectorize -ffast-math
LIBS = -lSDL2
CORES = 20

BIN = latency
OBJ = main.o


.PHONY: all clean lines


all:
	make -j $(CORES) $(BIN)


$(BIN): $(OBJ)
	$(CXX) $(CXXFLAGS) $(WARNINGS) $(OPTIMIZATIONS) -o $@ $^ $(LIBS)


main.o: main.cpp
	$(CXX) $(CXXFLAGS) $(WARNINGS) $(FLAGS) $(OPTIMIZATIONS) -c $<

# %.o: %.cpp $(wildcard *.h)
# 	$(CXX) $(CXXFLAGS) $(WARNINGS) $(FLAGS) $(OPTIMIZATIONS) -c $<



# For studying the generated assembly
%.s: %.cpp  %.h
	$(CXX) -S -fverbose-asm -g -O2 $<


lines:
	wc -l *.h *.cpp


todo:
	grep -n TODO *.cpp *.h || echo -e "Nothing left to do!\n"


clean:
	rm -f *.o
	rm -f $(BIN)
	rm -f *.s
	rm -f vgcore*

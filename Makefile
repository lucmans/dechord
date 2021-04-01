##
# Makefile: The makefile for dechord, a guitar to MIDI program
# @author Luc de Jonckheere
##

# Min required -std=c++11


# General compiler flags
CXX = g++
CXXFLAGS = -std=c++11 -g -fsanitize=address
WARNINGS = -Wall -Wextra -Wfloat-equal #-Wconversion -Warith-conversion
# FLAGS = -DCOLORED
OPTIMIZATIONS = -O3 -march=native -mtune=native # -mfma -mavx2 -ftree-vectorize -ffast-math
LIBS = -lSDL2 -lfftw3f -lm -fopenmp
CORES = 20

BIN = dechord
OBJ = main.o gensound.o fourier.o config.o graphics.o find_peaks.o note_set.o music_file.o


.PHONY: all clean valgrind lines todo upload zip clean


all:
	make -j $(CORES) $(BIN)


$(BIN): $(OBJ)
	$(CXX) $(CXXFLAGS) $(WARNINGS) $(OPTIMIZATIONS) -o $@ $^ $(LIBS)


main.o: main.cpp gensound.h fourier.h config.h graphics.h music_file.h
	$(CXX) $(CXXFLAGS) $(WARNINGS) $(FLAGS) $(OPTIMIZATIONS) -c $<

gensound.o: gensound.cpp gensound.h config.h
	$(CXX) $(CXXFLAGS) $(WARNINGS) $(FLAGS) $(OPTIMIZATIONS) -c $<

fourier.o: fourier.cpp fourier.h config.h graphics.h find_peaks.h note_set.h music_file.h
	$(CXX) $(CXXFLAGS) $(WARNINGS) $(FLAGS) $(OPTIMIZATIONS) -c $<

config.o: config.cpp config.h
	$(CXX) $(CXXFLAGS) $(WARNINGS) $(FLAGS) $(OPTIMIZATIONS) -c $<

graphics.o: graphics.cpp graphics.h config.h
	$(CXX) $(CXXFLAGS) $(WARNINGS) $(FLAGS) $(OPTIMIZATIONS) -c $<

find_peaks.o: find_peaks.cpp find_peaks.h config.h
	$(CXX) $(CXXFLAGS) $(WARNINGS) $(FLAGS) $(OPTIMIZATIONS) -c $<

note_set.o: note_set.cpp note_set.h config.h
	$(CXX) $(CXXFLAGS) $(WARNINGS) $(FLAGS) $(OPTIMIZATIONS) -c $<

music_file.o: music_file.cpp
	$(CXX) $(CXXFLAGS) $(WARNINGS) $(FLAGS) $(OPTIMIZATIONS) -c $<

# %.o: %.cpp $(wildcard *.h)
# 	$(CXX) $(CXXFLAGS) $(WARNINGS) $(FLAGS) $(OPTIMIZATIONS) -c $<



# For studying the generated assembly
%.s: %.cpp  %.h
	$(CXX) -S -fverbose-asm -g -O2 $<


valgrind: valgrind.supp
	clear
	valgrind --leak-check=full --error-limit=no --suppressions=valgrind.supp ./$(BIN)  #--show-reachable=yes

# # Generate suppressions for SDL, X11, Intel i965 driver, AMD driver and many other shared libraries
# valgrind.supp:
# 	../gen_val_suppress.py


lines:
	wc -l *.h *.cpp


todo:
	grep -n TODO *.cpp *.h || echo -e "Nothing left to do!\n"


fourier.tar.gz zip:
	make clean
	tar -czvf ./fourier.tar.gz --exclude "./fourier.tar.gz" *

upload: fourier.tar.gz
	scp fourier.tar.gz s1685538@sshgw.leidenuniv.nl:homedir/fourier.tar.gz
	@echo Manually enter the scp commando in the remote server
	ssh s1685538@sshgw.leidenuniv.nl


clean:
	rm -f *.o
	rm -f $(BIN)
	rm -f *.s
	rm -f vgcore*
	rm -f ./fourier.tar.gz

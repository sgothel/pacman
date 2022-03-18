# A simple Makefile for compiling small SDL projects

# set the compiler
CPP		:= g++ -x c++ -std=c++17 -c
LN	    := g++

# set the compiler flags
# DBGFLAGS := -ggdb3 -O0
DBGFLAGS := -O3
CPPFLAGS := -Wall -Iinclude ${DBGFLAGS} `sdl2-config --cflags`
LNFLAGS := -Wall ${DBGFLAGS} -lm `sdl2-config --libs` -lSDL2_image -lSDL2_ttf -lSDL2_mixer

HEADERS := include/pacman/*

obj/%.o: src/%.cpp $(HEADERS) Makefile
	$(CPP) -o $@ $(CPPFLAGS) $<

# default recipe
all: obj bin bin/pacman

bin/pacman: obj/utils.o obj/graphics.o obj/audio.o obj/maze.o obj/pacman.o obj/ghost.o obj/game.o
	$(LN) -o $@ $^ $(LNFLAGS)

obj:
	mkdir -p $@

bin:
	mkdir -p $@

clean:
	rm -rf obj bin Debug

.PHONY: all clean 

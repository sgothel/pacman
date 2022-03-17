# Pacman, a naive implementation of the classic game in C++

[Original document location](https://jausoft.com/cgit/cs_class/pacman.git/about/).

## Git Repository
This project's canonical repositories is hosted on [Gothel Software](https://jausoft.com/cgit/cs_class/pacman.git/).

## Goals
This project has been created to demonstrate a complex system written in modern C++
for our computer science class.

Besides management of animated sprite graphics, maze environment and tile positioning,
the interesting part might be the ghost's state machine and their movements.

To implement the original pacman game behavior like weighted tile collision,
ghost algorithm, etc. - we have used Jamey Pittman's [The Pac-Man Dossier](https://www.gamedeveloper.com/design/the-pac-man-dossier)
and [Understanding Pac-Man Ghost Behavior](https://gameinternals.com/understanding-pac-man-ghost-behavior)
for reference.

## Supported Platforms
C++17 and better where the [SDL2 library](https://www.libsdl.org/) is supported.

## Building Binaries
The project requires make, g++ >= 8.3 and the libsdl2 for building.

Installing build dependencies on Debian (11 or better):
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~{.sh}
apt install git build-essential g++ gcc libc-dev make
apt install libsdl2-dev libsdl2-image-dev libsdl2-ttf-dev libsdl2-mixer-dev
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

For a generic build use:
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~{.sh}
git clone https://jausoft.com/cgit/cs_class/pacman.git
cd pacman
make
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The binary shall be build to `bin/pacman`.

## Changes

**0.0.1**

* Initial commit with working status and prelim ghost algorithm.


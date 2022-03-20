# Pacman, a naive implementation of the classic game in C++

[Original document location](https://jausoft.com/cgit/cs_class/pacman.git/about/).

## Git Repository
This project's canonical repositories is hosted on [Gothel Software](https://jausoft.com/cgit/cs_class/pacman.git/).

## Goals
This project likes to demonstrate a complex system written in modern C++
for our computer science class.

Besides management of animated sprite graphics, maze environment and tile positioning,
the interesting part might be the ghost's state machine and their movements.

To implement the original pacman game behavior like weighted tile collision,
ghost algorithm, etc. - we have used the following documents for reference
- Jamey Pittman's [The Pac-Man Dossier](https://www.gamedeveloper.com/design/the-pac-man-dossier)
- [Understanding Pac-Man Ghost Behavior](https://gameinternals.com/understanding-pac-man-ghost-behavior)
- [Why do Pinky and Inky have different behaviors when Pac-Man is facing up?](http://donhodges.com/pacman_pinky_explanation.htm)

The implementation is inspired by [Toru Iwatani](https://en.wikipedia.org/wiki/Toru_Iwatani)'s
original game [Puckman](https://en.wikipedia.org/wiki/Pac-Man).

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

## Usage

Following commandline arguments are supported
- `-step` to disable automatic moving forward of puckman
- `-show_fps` to periodically show the frames per seconds (fps) value on the console
- `-no_vsync` to force off hardware enabled vsync, which in turn enables manual fps synchronization
- `-fps <int>` to enforce a specific fps value, which will also set `-no_vsync` naturally
- `-speed <int>` to set the 100% player speed in fields per seconds
- `-wwidth <int> to set the initial window width
- `-wheight <int> to set the initial window height
- `-show_moves` to show all players' move criteria like distance and collisions incl. speed changes on the console
- `-no_ghosts` to disable all ghosts
- `-show_targets` to show the ghost's target position as a ray on the video screen
- `-show_debug_gfx` to show all debug gfx, including `-show_debug_gfx`
- `-bugfix` to turn off the original puckman's behavior (bugs), see `Bugfix Mode` below.
- `-use_audio` to turn on audio effects, i.e. playing the audio samples.

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~{.sh}
bin/pacman [-step] [-show_fps] [-no_vsync] [-fps <int>] [-speed <int>] [-wwidth <int>] [-wheight <int>] [-show_moves] [-no_ghosts] [-show_targets] [-show_debug_gfx] [-bugfix] [-use_audio]
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

## Bugfix Mode

With the `-bugfix` mode enabled, see `Usage` above,
the original puckman behavior (bugs) are disabled.

The list below shall be updated in case we further the implementation
and kept in sync with `include/pacman/globals.hpp`.

By default the original pacman behavior is being implemented:
- weighted (round) tile position for collision tests
- pinky's up-target not 4 ahead, but 4 ahead and 4 to the left
- ...

If false, a more accurate implementation, the pacman bugfix, is used:
- pixel accurate tile position for collision tests
- pinky's up-traget to be 4 ahead as intended
- ...

## Media Data

The pixel data in `media/playfield_pacman.png` and `media/tiles_all.png`
are copied from [Arcade - Pac-Man - General Sprites.png](https://www.spriters-resource.com/arcade/pacman/sheet/52631/),
which were submitted and created by `Superjustinbros`
and assumed to be in the public domain.

## Changes

**0.0.1**

* Initial commit with working status and prelim ghost algorithm.


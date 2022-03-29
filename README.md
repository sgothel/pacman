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
- `-no_vsync` to force off hardware enabled vsync, which in turn enables manual fps synchronization
- `-fps <int>` to enforce a specific fps value, which will also set `-no_vsync` naturally
- `-speed <int>` to set the 100% player speed in fields per seconds
- `-wwidth <int> to set the initial window width
- `-wheight <int> to set the initial window height
- `-show_fps` to show pacman's speed and to periodically show the frames per seconds (fps) value on the console
- `-show_modes` to show all players' mode changes
- `-show_moves` to show all players' move criteria like distance and collisions incl. speed changes on the console
- `-show_targets` to show the ghost's target position as a ray on the video screen
- `-show_debug_gfx` to show all debug gfx
- `-show_all` enable `-show_modes`, `-show_moves`, `-show_targets` and `-show_debug_gfx`
- `-no_ghosts` to disable all ghosts
- `-bugfix` to turn off the original puckman's behavior (bugs), see `Bugfix Mode` below.
- `-audio` to turn on audio effects, i.e. playing the audio samples.
- `-level <int>` to start at given level

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~{.sh}
bin/pacman [-no_vsync] [-fps <int>] [-speed <int>] [-wwidth <int>] [-wheight <int>] [-show_fps] [-show_modes] [-show_moves] [-show_targets] [-show_debug_gfx] [-show_all] [-no_ghosts] [-bugfix] [-audio] [-level <int>]
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

## Implementation Status 

### Done
- Maze
  - Read maze specification from text file
- Sprites
- Speed per tile accurate key-frame animation
  - Weighted tile position from floating position
  - Renderer fps derived *step width*, i.e. sub-tiles
  - Sync speed by dropping tick, every n-frames
- Ghost *AI*
    - Scared RNG target
    - Scatter and chase targets
    - Next move algo
    - Grouped wave switch of scatter, chase and frightened
    - Exit home using local and global pellet timer
    - PRNG with an identical seed value every new level and life for predictable results
- Level specification (per level)
  - Timings
    - scatter and chase duration per phase
    - frightening/powered duration
  - Speed
    - pacman normal on empty tile or eating
    - pacman powered on empty tile or eating
    - ghost speed normal, frightening or in tunnel
  - Score
- Score
    - show eaten ghosts score onscreen
- Sound
  - Using wav chunks, mixed from seperated channels

### To Do
- Ghost *AI*
  - *Elroy 1+2* mode
  - Tunnel speed
  - *Red-zones* w/o turning up
- Score
    - Fruits / Bonus
- Pacman lives
- Sound
  - Use lossy formats
  - Complete samples
- Extension
  - Second play moves a ghost
- Maze
  - Render maze itself from maze-spec file

## Media Data

The pixel data in `media/playfield_pacman.png` and `media/tiles_all.png`
are copied from [Arcade - Pac-Man - General Sprites.png](https://www.spriters-resource.com/arcade/pacman/sheet/52631/),
which were submitted and created by `Superjustinbros`
and assumed to be in the public domain.

## Changes

**0.0.1**

* Initial commit with working status and prelim ghost algorithm.


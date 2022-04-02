# Pacman, a naive implementation of the classic game in C++

[Original document location](https://jausoft.com/cgit/cs_class/pacman.git/about/).

## Git Repository
This project's canonical repositories is hosted on [Gothel Software](https://jausoft.com/cgit/cs_class/pacman.git/).

## Goals
This project likes to demonstrate a complex system written in modern C++
for our computer science class.

Besides management of animated sprite graphics, maze environment and tile positioning,
animation speed synchronization, 
the interesting part might be the ghost's state machine and their movements.

Potential code sections of interest
- Selecting a new direction [ghost_t::set_next_dir()](../tree/src/ghost.cpp#n275)
- Selecting the target [ghost_t::set_next_target()](../tree/src/ghost.cpp#n141)
- Pellet counter to leave home [ghost_t::pellet_...()](../tree/src/ghost.cpp#n884)
- Level Specification [game_level_spec_t](../tree/src/game.cpp#n126)
- Keyframe interval for animation [keyframei_t](../tree/include/pacman/utils.hpp#n96)
- Random number engine [random_engine_t](../tree/include/pacman/utils.hpp#n349)

## Previous Work
To implement the original pacman game behavior like weighted tile collision,
ghost algorithm, etc. - we have used the following documents for reference
- Jamey Pittman's [The Pac-Man Dossier](https://www.gamedeveloper.com/design/the-pac-man-dossier)
- Chad Birch's [Understanding Pac-Man Ghost Behavior](https://gameinternals.com/understanding-pac-man-ghost-behavior)
- Don Hodges's [Why do Pinky and Inky have different behaviors when Pac-Man is facing up?](http://donhodges.com/pacman_pinky_explanation.htm)

The implementation is inspired by [Toru Iwatani](https://en.wikipedia.org/wiki/Toru_Iwatani)'s
original game [Puckman](https://en.wikipedia.org/wiki/Pac-Man).

## License
This project is for educational purposes and shall be distributed in source form only.

This project is licensed under the MIT license.

Namco holds the copyright on the original Puckman since 1980.

## Supported Platforms
C++17 and better where the [SDL2 library](https://www.libsdl.org/) is supported.

## Building Binaries
The project requires make, g++ >= 8.3 and the libsdl2 for building.

Installing build dependencies on Debian (11 or better):
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~{.sh}
apt install git build-essential g++ gcc libc-dev libpthread-stubs0-dev make
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

### Commandline Arguments

Following commandline arguments are supported
- '-2p' to enable 2nd player controlling Blinky when chasing, scattering or scared using `W`, `A`, `S` and `D` for up, left, down and right.
- `-audio` to turn on audio effects, i.e. playing the audio samples.
- `-pixqual <int>` to increas pixel filtering quality, i.e. 0 = nearest (default), 1 = linear and 2 = anisotropic filtering. 
- `-no_vsync` to force off hardware enabled vsync, which in turn enables manual fps synchronization
- `-fps <int>` to enforce a specific fps value, which will also set `-no_vsync` naturally
- `-speed <int>` to set the 100% player speed in fields per seconds
- `-wwidth <int>` to set the initial window width
- `-wheight <int>` to set the initial window height
- `-show_fps` to show pacman's speed and to periodically show the frames per seconds (fps) value on the console
- `-show_modes` to show all players' mode changes
- `-show_moves` to show all players' move criteria like distance and collisions incl. speed changes on the console
- `-show_targets` to show the ghost's target position as a ray on the video screen
- `-show_debug_gfx` to show all debug gfx
- `-show_all` enable `-show_modes`, `-show_moves`, `-show_targets` and `-show_debug_gfx`
- `-no_ghosts` to disable all ghosts
- `-bugfix` to turn off the original puckman's behavior (bugs), see `Bugfix Mode` below.
- `-decision_on_spot` to enable ghot's deciding next turn on the spot with a more current position, otherwise one tile ahead.
- `-dist_manhatten` to use the Manhatten distance function instead of the Euclidean default
- `-level <int>` to start at given level
- `-record <basename-of-bmp-files>` to record each frame as a bmp file at known fps. The basename may contain folder names. The resulting bmp files may be converted to video using [scripts/bmps_to_mp4.sh](../tree/scripts/bmps_to_mp4.sh), see below.

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~{.sh}
bin/pacman [-2p] [-audio] [-pixqual <int>] [-no_vsync] [-fps <int>] [-speed <int>] [-wwidth <int>] [-wheight <int>] [-show_fps] [-show_modes] [-show_moves] [-show_targets] [-show_debug_gfx] [-show_all] [-no_ghosts] [-bugfix] [-decision_on_spot] [-dist_manhatten] [-level <int>] [-record <basename-of-bmp-files>]
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

### Keyboard Input

- General Control
  - End programm: `Q` or `ESCAPE`
  - Pause: `P`
  - Reset: `R`
  - Fullscreen: `F`
  - Snapshot: `F12` (saved as `puckman-snap-abcd.bmp>`)
- Player Control
  - Up: `UP` or `W`
  - Left: `LEFT` or `A`
  - Down: `DOWN` or `S`
  - Right: `RIGHT` or `D`

### Video Recording Example

We assume we are in the project folder having `bin/pacman` build and created a `video` folder.
 
- Use the first command to start the game while recording bmp snapshots each frame 
to `video/puckman-01-*.bmp`.
- Use the second command to convert same files to `video/puckman-01.mp4`.
- Use the third command to delete the bmp files
- Use the forth command to play the video with `mpv`
 
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~{.sh}
bin/pacman -record video/puckman-01 -wwidth 1044 -wheight 1080 -show_targets -show_debug_gfx
scripts/bmps_to_mp4.sh video/puckman-01
rm video/puckman-01*bmp
mpv video/puckman-01.mp4
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 
## Optional Deviations from the Original

### Decision on the Spot
As stated in [The Pac-Man Dossier](https://www.gamedeveloper.com/design/the-pac-man-dossier),
the ghosts select their next direction *one tile ahead of an intersection*.

With the `-decision_on_spot` mode enabled, see `Usage` above,
the ghosts decide their next direction for each field using a more current pacman position.

The default setting is to use the original *one tile ahead of an intersection*.
   
### Bugfix Mode

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
  - Scatter, chase and phantom targets
  - Scared RNG target
    - PRNG with an identical seed value every new level and life for predictable results
  - Next `direction_t` algo
    - No turning up in *Red-Zones* if chasing or scattering
    - Optionally use alternative Manhatten distance function instead of the Euclidean default.
  - Grouped wave switch of scatter, chase and frightened
  - Exit Home
    - Using local and global pellet timer
    - Additional maximum time when no pellets are eaten
  - Speed    
    - Adjust tunnel speed
    - *Elroy 1+2* mode
- Pacman
  - show eaten ghosts and fruit score onscreen (FREEZE)
  - Freeze pacman only for 3 frames after eating power pellet
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
    - ghost 1-4 per power pellet
    - Fruits / Bonus
- Sound
  - Using chunks (wav/mp3), mixed from seperated channels
  - Use lossy formats where applicable
- Persistent game state
  - Snapshot (screenshot)
  - Video recording via bmp snapshots each frame
- Extension
  - Second player controls Blinky when chasing, scattering or scared

### To Do
- Pacman
  - Lives
- Sound
  - Complete samples
- Maze
  - Render maze itself from maze-spec file
- Persistent game state
  - Save/load game state

## Media Data

The pixel data in `media/playfield_pacman.png` and `media/tiles_all.png`
are copied from [Arcade - Pac-Man - General Sprites.png](https://www.spriters-resource.com/arcade/pacman/sheet/52631/),
which were submitted and created by `Superjustinbros`
and assumed to be in the public domain.

Audio samples have been edited using any of the following sources
- [Pacman Waka Waka Sound - Seamless Loop - 1 Hour Edition](https://www.youtube.com/watch?v=xbR_-kVEYQ0)
- [PAC-MAN Namco Sounds - Start Music](https://www.youtube.com/watch?v=HwyAwPLHqnM)
- [Pac Man Ghost Noises](https://www.youtube.com/watch?v=6mkiaIxH0sU)
- [Pac Man Scared Ghosts](https://www.youtube.com/watch?v=JU3ZZtrqLZs)
- [Pacman - Game Over Sound Effect (HD)](https://www.youtube.com/watch?v=EqObchtcyKY)
- [Pac Man Sound Effects](https://www.youtube.com/watch?v=pIUeDjWZsEg)
- [Steve Dunn's Pacman-TypeScript](https://github.com/SteveDunn/Pacman-TypeScript/tree/master/snd)
- [Shaune Williams's Pac-Man](https://github.com/masonicGIT/pacman/tree/master/sounds)

The included audio samples are incomplete by design, only demonstrating utilizing
sound from a coding perspective using loops and concurrently mixed channels.

## Changes

**1.0.0**

* Reached clean code demo stage with functional implementation.

**0.2.0**

* Stability, next_target bug fix and alternative distance function

**0.1.0**

* Usable working state with most ghost algorithms for further analysis and demonstration

**0.0.1**

* Initial commit with working status and prelim ghost algorithm.


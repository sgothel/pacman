/*
 * Author: Sven Gothel <sgothel@jausoft.com> and Svenson Han Gothel
 * Copyright (c) 2022 Gothel Software e.K.
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
 * LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 * OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
#ifndef PACMAN_GLOBALS_HPP_
#define PACMAN_GLOBALS_HPP_

#include <pacman/maze.hpp>
#include <pacman/graphics.hpp>
#include <pacman/game.hpp>

#include <cmath>
#include <memory>

//
// globals
//

extern int win_pixel_width;
extern int win_pixel_scale;

extern int get_frames_per_sec();
inline int get_ms_per_frame() { return (int)std::round(1000.0 / (float)get_frames_per_sec()); }

extern std::unique_ptr<maze_t> pacman_maze;

extern std::shared_ptr<global_tex_t> global_tex;

typedef std::shared_ptr<ghost_t> ghost_ref;
extern std::vector<ghost_ref> ghosts;

typedef std::shared_ptr<pacman_t> pacman_ref;
extern pacman_ref pacman;

/**
 * By default the original pacman behavior is being implemented:
 * - weighted (round) tile position for collision tests
 * - pinky's up-target not 4 ahead, but 4 ahead and 4 to the left
 * - ...
 *
 * If false, a more accurate implementation, the pacman bugfix, is used:
 * - pixel accurate tile position for collision tests
 * - pinky's up-traget to be 4 ahead as intended
 * - ...
 */
extern bool use_original_pacman_behavior();

#endif /* PACMAN_GLOBALS_HPP_ */

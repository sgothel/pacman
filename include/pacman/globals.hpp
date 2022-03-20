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
#include <pacman/audio.hpp>
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

extern std::unique_ptr<maze_t> global_maze;

extern std::shared_ptr<global_tex_t> global_tex;

typedef std::shared_ptr<ghost_t> ghost_ref;
/**
 * ghosts are in proper ghost_t enum order for array access.
 */
extern std::vector<ghost_ref> ghosts;

typedef std::shared_ptr<pacman_t> pacman_ref;
extern pacman_ref pacman;

enum class audio_clip_t : int {
    INTRO = 0,
    MUNCH = 1,
    EAT_FRUIT = 2,
    EAT_GHOST = 3,
    EXTRA = 4,
    INTERMISSION = 5,
    DEATH = 6
};
constexpr int number(const audio_clip_t item) noexcept {
    return static_cast<int>(item);
}

typedef std::shared_ptr<audio_sample_t> audio_sample_ref;
extern std::vector<audio_sample_ref> audio_samples;

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
 *
 * TODO: Keep in sync with README.md
 */
extern bool use_original_pacman_behavior();

extern bool show_all_debug_gfx();

#endif /* PACMAN_GLOBALS_HPP_ */

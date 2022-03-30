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

int win_pixel_height() noexcept;
int win_pixel_width() noexcept;
int win_pixel_scale() noexcept;

int get_frames_per_sec() noexcept;
inline int get_ms_per_frame() noexcept { return (int)std::round(1000.0 / (float)get_frames_per_sec()); }

TTF_Font* font_ttf() noexcept;

text_texture_ref get_text_texture_cache(const std::string& key) noexcept;

void put_text_texture_cache(const std::string& key, text_texture_ref ttex) noexcept;

#endif /* PACMAN_GLOBALS_HPP_ */

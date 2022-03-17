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
#include <pacman/game.hpp>
#include <pacman/globals.hpp>

#include <limits>

#include <cstdio>
#include <time.h>

static constexpr const bool DEBUG_GFX_BOUNDS = false;

//
// globals across modules 'globals.hpp'
//

int win_pixel_width = 0;
int win_pixel_scale = 1;

static int frames_per_sec=0;
int get_frames_per_sec() { return frames_per_sec; }

std::unique_ptr<maze_t> pacman_maze;
std::shared_ptr<global_tex_t> global_tex;
std::vector<ghost_ref> ghosts;
pacman_ref pacman;

static bool original_pacman_behavior = true;
bool use_original_pacman_behavior() { return original_pacman_behavior; }

//
// score_t
//

score_t tile_to_score(const tile_t tile) {
    switch(tile) {
        case tile_t::PELLET: return score_t::PELLET;
        case tile_t::PELLET_POWER: return score_t::PELLET_POWER;
        case tile_t::CHERRY: return score_t::CHERRY;
        case tile_t::STRAWBERRY: return score_t::STRAWBERRY;
        case tile_t::ORANGE: return score_t::ORANGE;
        case tile_t::APPLE: return score_t::APPLE;
        case tile_t::MELON: return score_t::MELON;
        case tile_t::GALAXIAN: return score_t::GALAXIAN;
        case tile_t::BELL: return score_t::BELL;
        case tile_t::KEY: return score_t::KEY;
        default: return score_t::NONE;
    }
}

//
// global_tex_t
//

int global_tex_t::tile_to_texidx(const tile_t tile) const {
    if( tile_t::PELLET <= tile && tile <= tile_t::KEY ) {
        const int tile_i = static_cast<int>(tile);
        const int pellet_i = static_cast<int>(tile_t::PELLET);
        const int idx = tile_i - pellet_i;
        if( 0 <= idx && (size_t)idx < textures.size() ) {
            return idx;
        }
    }
    return -1;
}

int global_tex_t::validate_texidx(const int idx) const {
    if( 0 <= idx && (size_t)idx < textures.size() ) {
        return idx;
    }
    return -1;
}

global_tex_t::global_tex_t(SDL_Renderer* rend)
: all_images( std::make_shared<texture_t>(rend, "media/tiles_all.png") ),
  atex_pellet_power( "PP", rend, 250, all_images, 0, 0, 14, 14, { { 1*14, 0 }, { -1, -1} })
{
    add_sub_textures(textures, rend, all_images, 0, 0, 14, 14, {
            {  0*14, 0 }, {  1*14, 0 }, {  2*14, 0 }, {  3*14, 0 }, {  4*14, 0 }, {  5*14, 0 }, {  6*14, 0 },
            {  7*14, 0 }, {  8*14, 0 }, {  9*14, 0 }, { 10*14, 0 }, { 11*14, 0 }, { 12*14, 0 }, { 13*14, 0 },  } );
}

void global_tex_t::destroy() {
    for(size_t i=0; i<textures.size(); ++i) {
        textures[i]->destroy();
    }
    textures.clear();
    all_images->destroy();
}

std::shared_ptr<texture_t> global_tex_t::get_tex(const int idx) {
    const int idx2 = validate_texidx(idx);
    return 0 <= idx2 ? textures[idx2] : nullptr;
}
std::shared_ptr<const texture_t> global_tex_t::get_tex(const int idx) const {
    const int idx2 = validate_texidx(idx);
    return 0 <= idx2 ? textures[idx2] : nullptr;
}

std::shared_ptr<texture_t> global_tex_t::get_tex(const tile_t tile) {
    const int idx = tile_to_texidx(tile);
    return 0 <= idx ? textures[idx] : nullptr;
}
std::shared_ptr<const texture_t> global_tex_t::get_tex(const tile_t tile) const {
    const int idx = tile_to_texidx(tile);
    return 0 <= idx ? textures[idx] : nullptr;
}

void global_tex_t::draw_tile(const tile_t tile, SDL_Renderer* rend, const int x, const int y) {
    if( tile_t::PELLET_POWER == tile ) {
        atex_pellet_power.draw(rend, x, y, true /* maze_offset */);
    } else {
        std::shared_ptr<texture_t> tex = get_tex(tile);
        if( nullptr != tex ) {
            tex->draw(rend, x, y, true /* maze_offset */);
        }
    }
}

std::string global_tex_t::toString() const {
    return "tiletex[count "+std::to_string(textures.size())+"]";
}

//
// main
//

TTF_Font* font_ttf = nullptr;

static void on_window_resized(SDL_Renderer* rend, const int win_width_l, const int win_height_l) {
    int win_pixel_height=0;
    SDL_GetRendererOutputSize(rend, &win_pixel_width, &win_pixel_height);

    float sx = win_pixel_width / pacman_maze->get_pixel_width();
    float sy = win_pixel_height / pacman_maze->get_pixel_height();
    win_pixel_scale = static_cast<int>( std::round( std::fmin<float>(sx, sy) ) );

    if( nullptr != font_ttf ) {
        TTF_CloseFont(font_ttf);
        font_ttf = nullptr;
    }
    int font_height;
    {
        const std::string fontfilename = "fonts/freefont/FreeSansBold.ttf";
        font_height = pacman_maze->get_ppt_y() * win_pixel_scale;
        font_ttf = TTF_OpenFont(fontfilename.c_str(), font_height);
    }
    log_print("Window Resized: %d x %d pixel ( %d x %d logical ) @ %d hz\n",
            win_pixel_width, win_pixel_height, win_width_l, win_height_l, get_frames_per_sec());
    log_print("Pixel scale: %f x %f -> %d, font[ok %d, height %d]\n", sx, sy, win_pixel_scale, nullptr!=font_ttf, font_height);
}

static std::string get_usage(const std::string& exename) {
    // TODO: Keep in sync with README.md
    return "Usage: "+exename+" [-step] [-show_fps] [-no_vsync] [-fps <int>] [-speed <int>] [-wwidth <int>] [-wheight <int>] [-show_ghost_moves] [-show_targets] [-bugfix]";
}

int main(int argc, char *argv[])
{
    bool auto_move = true;
    bool show_fps = false;
    bool enable_vsync = true;
    int forced_fps = -1;
    float fields_per_sec=8;
    int win_width = 640, win_height = 720;
    bool show_ghost_moves = false;
    bool show_targets = false;
    {
        for(int i=1; i<argc; ++i) {
            if( 0 == strcmp("-step", argv[i]) ) {
                auto_move = false;
            } else if( 0 == strcmp("-show_fps", argv[i]) ) {
                show_fps = true;
            } else if( 0 == strcmp("-no_vsync", argv[i]) ) {
                enable_vsync = false;
            } else if( 0 == strcmp("-fps", argv[i]) && i+1<argc) {
                forced_fps = atoi(argv[i+1]);
                enable_vsync = false;
                ++i;
            } else if( 0 == strcmp("-speed", argv[i]) && i+1<argc) {
                fields_per_sec = atof(argv[i+1]);
                ++i;
            } else if( 0 == strcmp("-wwidth", argv[i]) && i+1<argc) {
                win_width = atoi(argv[i+1]);
                ++i;
            } else if( 0 == strcmp("-wheight", argv[i]) && i+1<argc) {
                win_height = atoi(argv[i+1]);
                ++i;
            } else if( 0 == strcmp("-show_ghost_moves", argv[i]) ) {
                show_ghost_moves = true;
            } else if( 0 == strcmp("-show_targets", argv[i]) ) {
                show_targets = true;
            } else if( 0 == strcmp("-bugfix", argv[i]) ) {
                original_pacman_behavior = false;
            }
        }
        log_print("%s\n", get_usage(argv[0]).c_str());
        log_print("- auto_move %d\n", auto_move);
        log_print("- show_fps %d\n", show_fps);
        log_print("- enable_vsync %d\n", enable_vsync);
        log_print("- forced_fps %d\n", forced_fps);
        log_print("- fields_per_sec %5.2f\n", fields_per_sec);
        log_print("- win size %d x %d\n", win_width, win_height);
        log_print("- show_ghost_moves %d\n", show_ghost_moves);
        log_print("- show_targets %d\n", show_targets);
        log_print("- use_bugfix_pacman %d\n", !use_original_pacman_behavior());
    }

    pacman_maze = std::make_unique<maze_t>("media/playfield_pacman.txt");

    if( !pacman_maze->is_ok() ) {
        log_print("Maze: Error: %s\n", pacman_maze->toString().c_str());
        return -1;
    }
    {
        log_print("--- 8< ---\n");
        const int maze_width = pacman_maze->get_width();
        pacman_maze->draw( [&maze_width](int x, int y, tile_t tile) {
            fprintf(stderr, "%s", to_string(tile).c_str());
            if( x == maze_width-1 ) {
                fprintf(stderr, "\n");
            }
            (void)y;
        });
        log_print("--- >8 ---\n");
        log_print("Maze: %s\n", pacman_maze->toString().c_str());
    }

    if (SDL_Init(SDL_INIT_EVERYTHING) != 0) {
        log_print("error initializing SDL: %s\n", SDL_GetError());
    }

    if ( ( IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG ) != IMG_INIT_PNG ) {
        log_print("error initializing SDL_image: %s\n", SDL_GetError());
    }

    if( 0 != TTF_Init() ) {
        log_print("Font: Error initializing.\n");
    }

    if( enable_vsync ) {
        SDL_SetHint(SDL_HINT_RENDER_VSYNC, "1");
    }

    SDL_Window* win = SDL_CreateWindow("Pacman",
                                       SDL_WINDOWPOS_UNDEFINED,
                                       SDL_WINDOWPOS_UNDEFINED,
                                       win_width,
                                       win_height,
                                       SDL_WINDOW_ALLOW_HIGHDPI | SDL_WINDOW_RESIZABLE);
 
    const Uint32 render_flags = enable_vsync ? SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC :
                                               SDL_RENDERER_ACCELERATED;
    Uint32 fullscreen_flags = 0;
    bool uses_vsync = false;

    SDL_Renderer* rend = SDL_CreateRenderer(win, -1, render_flags);
    {
        SDL_RendererInfo info;
        SDL_GetRendererInfo(rend, &info);
        bool _uses_vsync = 0 != ( info.flags & SDL_RENDERER_PRESENTVSYNC );
        uses_vsync = _uses_vsync | enable_vsync; // FIXME: Assume yes if enforced with enable_vsync, since info.flags is not reliable
        log_print("renderer: name: %s\n", info.name);
        log_print("renderer: accel %d\n", 0 != ( info.flags & SDL_RENDERER_ACCELERATED ));
        log_print("renderer: soft %d\n", 0 != ( info.flags & SDL_RENDERER_SOFTWARE ));
        log_print("renderer: vsync %d -> %d\n", _uses_vsync, uses_vsync);
    }
 
    std::unique_ptr<texture_t> pacman_maze_tex = std::make_unique<texture_t>(rend, "media/"+pacman_maze->get_texture_file());
    {
        int win_pixel_width_=0;
        int win_pixel_height_=0;
        SDL_GetRendererOutputSize(rend, &win_pixel_width_, &win_pixel_height_);
        {
            SDL_DisplayMode mode;
            {
                const int num_displays = SDL_GetNumVideoDisplays();
                for(int i=0; i<num_displays; ++i) {
                    bzero(&mode, sizeof(mode));
                    SDL_GetCurrentDisplayMode(i, &mode);
                    log_print("Display %d: %d x %d @ %d Hz\n", i, mode.w, mode.h, mode.refresh_rate);
                }
            }
            const int win_display_idx = SDL_GetWindowDisplayIndex(win);
            bzero(&mode, sizeof(mode));
            SDL_GetCurrentDisplayMode(win_display_idx, &mode); // SDL_GetWindowDisplayMode(..) fails on some systems (wrong refresh_rate and logical size
            log_print("WindowDisplayMode: %d x %d @ %d Hz @ display %d\n", mode.w, mode.h, mode.refresh_rate, win_display_idx);
            if( 0 < forced_fps ) {
                frames_per_sec = forced_fps;
            } else {
                frames_per_sec = mode.refresh_rate;
            }
        }
        on_window_resized(rend, win_pixel_width_, win_pixel_height_);
    }
    SDL_SetWindowSize(win, pacman_maze->get_pixel_width()*win_pixel_scale,
                           pacman_maze->get_pixel_height()*win_pixel_scale);

    global_tex = std::make_shared<global_tex_t>(rend);
    pacman = std::make_shared<pacman_t>(rend, fields_per_sec, auto_move);
    ghosts.push_back( std::make_shared<ghost_t>(ghost_t::personality_t::BLINKY, rend, fields_per_sec) );
    ghosts.push_back( std::make_shared<ghost_t>(ghost_t::personality_t::PINKY, rend, fields_per_sec) );
    // ghosts.push_back( std::make_shared<ghost_t>(ghost_t::personality_t::CLYDE, rend, fields_per_sec) );
    // ghosts.push_back( std::make_shared<ghost_t>(ghost_t::personality_t::INKY, rend, fields_per_sec) );
    for(ghost_ref g : ghosts) {
        g->set_log_moves(show_ghost_moves);
    }

    bool close = false;
    bool set_dir = false;
    bool pause = true;
    direction_t dir = pacman->get_dir();

    const uint64_t fps_range_ms = 3000;
    uint64_t t0 = getCurrentMilliseconds();
    uint64_t frame_count = 0;
    uint64_t t1 = t0;
    uint64_t td_print_fps = t0;
    uint64_t reset_fps_frame = fps_range_ms;

    uint64_t last_score = pacman->get_score();
    std::shared_ptr<text_texture_t> ttex_score = nullptr;
    std::shared_ptr<text_texture_t> ttex_score_title = nullptr;

    while (!close) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
                case SDL_QUIT:
                    close = true;
                    break;

                case SDL_KEYUP:
                    set_dir = false;
                    break;

                case SDL_WINDOWEVENT:
                    switch (event.window.event) {
                        case SDL_WINDOWEVENT_SHOWN:
                            pause = false;
                            break;
                        case SDL_WINDOWEVENT_HIDDEN:
                            pause = true;
                            break;
                        case SDL_WINDOWEVENT_RESIZED:
                            on_window_resized(rend, event.window.data1, event.window.data2);
                            ttex_score_title = nullptr;
                            ttex_score = nullptr;
                            break;
                        case SDL_WINDOWEVENT_SIZE_CHANGED:
                            // INFO_PRINT("Window SizeChanged: %d x %d\n", event.window.data1, event.window.data2);
                            break;
                    }
                    break;

                case SDL_KEYDOWN:
                    // keyboard API for key pressed
                    switch (event.key.keysym.scancode) {
                        case SDL_SCANCODE_Q:
                            [[fallthrough]];
                        case SDL_SCANCODE_ESCAPE:
                            close = true;
                            break;
                        case SDL_SCANCODE_P:
                            pause = pause ? false : true;
                            break;
                        case SDL_SCANCODE_R:
                            pacman_maze->reset();
                            break;
                        case SDL_SCANCODE_W:
                        case SDL_SCANCODE_UP:
                            dir = direction_t::UP;
                            set_dir = true;
                            break;
                        case SDL_SCANCODE_A:
                        case SDL_SCANCODE_LEFT:
                            dir = direction_t::LEFT;
                            set_dir = true;
                            break;
                        case SDL_SCANCODE_S:
                        case SDL_SCANCODE_DOWN:
                            dir = direction_t::DOWN;
                            set_dir = true;
                            break;
                        case SDL_SCANCODE_D:
                        case SDL_SCANCODE_RIGHT:
                            dir = direction_t::RIGHT;
                            set_dir = true;
                            break;
                        case SDL_SCANCODE_F:
                            fullscreen_flags = 0 == fullscreen_flags ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0;
                            SDL_SetWindowFullscreen(win, fullscreen_flags);
                            break;
                        default:
                            // nop
                            break;
                    }
            }
        }
        if( pause ) {
            SDL_Delay( 1000 / 60 );
            continue;
        }
        if( set_dir ) {
            pacman->set_dir(dir);
        }
        global_tex->tick();
        for(ghost_ref g : ghosts) {
            g->tick();
        }
        pacman->tick();

        SDL_RenderClear(rend);

        pacman_maze_tex->draw(rend, 0, 0, false /* maze_offset */);
        pacman_maze->draw( [&rend](int x, int y, tile_t tile) {
            global_tex->draw_tile(tile, rend, x, y);
        });
        pacman->draw(rend);
        for(ghost_ref ghost : ghosts) {
            ghost->draw(rend);

            if( show_targets ) {
                uint8_t r, g, b, a;
                SDL_GetRenderDrawColor(rend, &r, &g, &b, &a);
                SDL_SetRenderDrawColor(rend, 150, 150, 150, 255);
                const acoord_t& p1 = ghost->get_pos();
                const acoord_t& p2 = ghost->get_target();
                SDL_RenderDrawLine(rend,
                        pacman_maze->x_to_pixel( p1.get_x_i(), win_pixel_scale, false ),
                        pacman_maze->y_to_pixel( p1.get_y_i(), win_pixel_scale, false ),
                        pacman_maze->x_to_pixel( p2.get_x_i(), win_pixel_scale, false ),
                        pacman_maze->y_to_pixel( p2.get_y_i(), win_pixel_scale, false ) );
                SDL_SetRenderDrawColor(rend, r, g, b, a);
            }
        }

        if( nullptr != font_ttf ) {
            if( nullptr == ttex_score_title ) {
                const std::string highscore_s("HIGH SCORE");
                ttex_score_title = draw_text_scaled(rend, font_ttf, highscore_s, 255, 255, 255, [&](const texture_t& tex, int &x, int&y) {
                    x = ( pacman_maze->get_pixel_width()*win_pixel_scale - tex.get_width() ) / 2;
                    y = pacman_maze->x_to_pixel(0, win_pixel_scale, false);
                });
            } else {
                ttex_score_title->redraw(rend);
            }
            if( nullptr != ttex_score && last_score == pacman->get_score() ) {
                ttex_score->redraw(rend);
            } else {
                std::string score_s = std::to_string( pacman->get_score() );
                ttex_score = draw_text_scaled(rend, font_ttf, score_s, 255, 255, 255, [&](const texture_t& tex, int &x, int&y) {
                    x = ( pacman_maze->get_pixel_width()*win_pixel_scale - tex.get_width() ) / 2;
                    y = pacman_maze->x_to_pixel(1, win_pixel_scale, false);
                });
                last_score = pacman->get_score();
            }
        }
 
        // swap double buffer incl. v-sync
        SDL_RenderPresent(rend);
        ++frame_count;
        if( !uses_vsync ) {
            const uint64_t ms_per_frame = (uint64_t)std::round(1000.0 / (float)get_frames_per_sec());
            const uint64_t ms_last_frame = getCurrentMilliseconds() - t1;
            if( ms_per_frame > ms_last_frame + 1 )
            {
                const uint64_t td = ms_per_frame - ms_last_frame;
                #if 1
                    struct timespec ts { (long)(td/1000UL), (long)((td%1000UL)*1000000UL) };
                    nanosleep( &ts, NULL );
                #else
                    SDL_Delay( td );
                #endif
                // INFO_PRINT("soft-sync exp %zd > has %zd, delay %zd (%lds, %ldns)\n", ms_per_frame, ms_last_frame, td, ts.tv_sec, ts.tv_nsec);
            }
        }
        t1 = getCurrentMilliseconds();
        if( show_fps && fps_range_ms <= t1 - td_print_fps ) {
            const float fps = get_fps(t0, t1, frame_count);
            std::string fps_str(64, '\0');
            const int written = std::snprintf(&fps_str[0], fps_str.size(), "fps %6.2f", fps);
            fps_str.resize(written);
            // log_print("%s, td %" PRIu64 "ms, frames %" PRIu64 "\n", fps_str.c_str(), t1-t0, frame_count);
            log_print("%s\n", fps_str.c_str());
            td_print_fps = t1;
        }
        if( 0 == --reset_fps_frame ) {
            t0 = t1;
            frame_count = 0;
            reset_fps_frame = fps_range_ms;
        }
    } // loop

    pacman->destroy();
    ghosts.clear();
    pacman_maze_tex->destroy();

    SDL_DestroyRenderer(rend);
 
    SDL_DestroyWindow(win);

    TTF_CloseFont(font_ttf);

    SDL_Quit();
 
    return 0;
}

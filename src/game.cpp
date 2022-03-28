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
#include <pacman/utils.hpp>
#include <pacman/graphics.hpp>
#include <pacman/audio.hpp>
#include <pacman/maze.hpp>
#include <pacman/game.hpp>
#include <pacman/globals.hpp>

#include <limits>

#include <cstdio>
#include <time.h>

//
// globals across modules 'globals.hpp'
//

int win_pixel_width = 0;
int win_pixel_scale = 1;

static int frames_per_sec = 0;
int get_frames_per_sec() { return frames_per_sec; }

static int current_level = 1;
int get_current_level() { return current_level; }

std::unique_ptr<maze_t> global_maze;
std::shared_ptr<global_tex_t> global_tex;
std::vector<ghost_ref> ghosts;
pacman_ref pacman;

std::vector<audio_sample_ref> audio_samples;

static bool original_pacman_behavior = true;
bool use_original_pacman_behavior() { return original_pacman_behavior; }

static bool enable_debug_gfx = false;
bool show_debug_gfx() { return enable_debug_gfx; }

static bool enable_log_fps = false;
static bool enable_log_moves = false;
static bool enable_log_modes = false;
bool log_fps() { return enable_log_fps; }
bool log_moves() { return enable_log_moves; }
bool log_modes() { return enable_log_modes; }

//
// globals for this module
//
static constexpr const int64_t MilliPerOne = 1000L;
static constexpr const int64_t NanoPerMilli = 1000000L;
static constexpr const int64_t NanoPerOne = NanoPerMilli*MilliPerOne;

static constexpr const bool debug_key_input = false;

//
// score_t
//

score_t tile_to_score(const tile_t tile) noexcept {
    switch(tile) {
        case tile_t::PELLET: return score_t::PELLET;
        case tile_t::PELLET_POWER: return score_t::PELLET_POWER;
        case tile_t::CHERRY: return score_t::CHERRY;
        case tile_t::STRAWBERRY: return score_t::STRAWBERRY;
        case tile_t::PEACH: return score_t::PEACH;
        case tile_t::APPLE: return score_t::APPLE;
        case tile_t::MELON: return score_t::MELON;
        case tile_t::GALAXIAN: return score_t::GALAXIAN;
        case tile_t::BELL: return score_t::BELL;
        case tile_t::KEY: return score_t::KEY;
        default: return score_t::NONE;
    }
}

//
// level_spec_t
//

static ghost_wave_vec_t ghost_waves_1 = { { 7000, 20000 }, { 7000, 20000 }, { 5000,   20000 }, { 5000, -1 } };
static ghost_wave_vec_t ghost_waves_2 = { { 7000, 20000 }, { 7000, 20000 }, { 5000, 1033000 }, {   17, -1 } };
static ghost_wave_vec_t ghost_waves_5 = { { 5000, 20000 }, { 5000, 20000 }, { 5000, 1037000 }, {   17, -1 } };

static std::vector<game_level_spec_t> level_spec_array = {
    //                                                                                            Fright Milliseconds
    //                                                                                      Fright Ghost Speed      |
    //                                                                       Powered Pac-Man Dots Speed      |      |
    //                                                                     Powered Pac-Man Speed      |      |      |
    //                                          Ghost Tunnel Speed                             |      |      |      |
    //                                          Ghost Speed      |                             |      |      |      |
    //                            Pac-Man Dots Speed      |      |                             |      |      |      |
    //                          Pac-Man Speed      |      |      |                             |      |      |      |
    //                     Bonus Points     |      |      |      |                             |      |      |      |
    //                                |     |      |      |      |                             |      |      |      |
    /*  1 */ { tile_t::CHERRY,      100, 0.80f, 0.71f, 0.75f, 0.40f,  20, 0.80f, 10, 0.85f, 0.90f, 0.79f, 0.50f, 6000, 5, ghost_waves_1},
    /*  2 */ { tile_t::STRAWBERRY,  300, 0.90f, 0.79f, 0.85f, 0.45f,  30, 0.90f, 15, 0.95f, 0.95f, 0.83f, 0.55f, 5000, 5, ghost_waves_2},
    /*  3 */ { tile_t::PEACH,       500, 0.90f, 0.79f, 0.85f, 0.45f,  40, 0.90f, 20, 0.95f, 0.95f, 0.83f, 0.55f, 4000, 5, ghost_waves_2},
    /*  4 */ { tile_t::PEACH,       500, 0.90f, 0.79f, 0.85f, 0.45f,  40, 0.90f, 20, 0.95f, 0.95f, 0.83f, 0.55f, 3000, 5, ghost_waves_2},
    /*  5 */ { tile_t::APPLE,       700, 1.00f, 0.87f, 0.95f, 0.50f,  40, 1.00f, 20, 1.05f, 1.00f, 0.87f, 0.60f, 2000, 5, ghost_waves_5},
    /*  6 */ { tile_t::APPLE,       700, 1.00f, 0.87f, 0.95f, 0.50f,  50, 1.00f, 25, 1.05f, 1.00f, 0.87f, 0.60f, 5000, 5, ghost_waves_5},
    /*  7 */ { tile_t::MELON,      1000, 1.00f, 0.87f, 0.95f, 0.50f,  50, 1.00f, 25, 1.05f, 1.00f, 0.87f, 0.60f, 2000, 5, ghost_waves_5},
    /*  8 */ { tile_t::MELON,      1000, 1.00f, 0.87f, 0.95f, 0.50f,  50, 1.00f, 25, 1.05f, 1.00f, 0.87f, 0.60f, 2000, 5, ghost_waves_5},
    /*  9 */ { tile_t::GALAXIAN,   2000, 1.00f, 0.87f, 0.95f, 0.50f,  60, 1.00f, 30, 1.05f, 1.00f, 0.87f, 0.60f, 1000, 3, ghost_waves_5},
    /* 10 */ { tile_t::GALAXIAN,   2000, 1.00f, 0.87f, 0.95f, 0.50f,  60, 1.00f, 30, 1.05f, 1.00f, 0.87f, 0.60f, 5000, 5, ghost_waves_5},
    /* 11 */ { tile_t::BELL,       3000, 1.00f, 0.87f, 0.95f, 0.50f,  60, 1.00f, 30, 1.05f, 1.00f, 0.87f, 0.60f, 2000, 5, ghost_waves_5},
    /* 12 */ { tile_t::BELL,       3000, 1.00f, 0.87f, 0.95f, 0.50f,  80, 1.00f, 40, 1.05f, 1.00f, 0.87f, 0.60f, 1000, 3, ghost_waves_5},
    /* 13 */ { tile_t::KEY,        5000, 1.00f, 0.87f, 0.95f, 0.50f,  80, 1.00f, 40, 1.05f, 1.00f, 0.87f, 0.60f, 1000, 3, ghost_waves_5},
    /* 14 */ { tile_t::KEY,        5000, 1.00f, 0.87f, 0.95f, 0.50f,  80, 1.00f, 40, 1.05f, 1.00f, 0.87f, 0.60f, 3000, 5, ghost_waves_5},
    /* 15 */ { tile_t::KEY,        5000, 1.00f, 0.87f, 0.95f, 0.50f, 100, 1.00f, 50, 1.05f, 1.00f, 0.87f, 0.60f, 1000, 3, ghost_waves_5},
    /* 16 */ { tile_t::KEY,        5000, 1.00f, 0.87f, 0.95f, 0.50f, 100, 1.00f, 50, 1.05f, 1.00f, 0.87f, 0.60f, 1000, 3, ghost_waves_5},
    /* 17 */ { tile_t::KEY,        5000, 1.00f, 0.87f, 0.95f, 0.50f, 100, 1.00f, 50, 1.05f, 1.00f, 0.87f, 0.60f, 1000, 3, ghost_waves_5},
    /* 18 */ { tile_t::KEY,        5000, 1.00f, 0.87f, 0.95f, 0.50f, 100, 1.00f, 50, 1.05f, 1.00f, 0.87f, 0.60f, 1000, 3, ghost_waves_5},
    /* 19 */ { tile_t::KEY,        5000, 1.00f, 0.87f, 0.95f, 0.50f, 120, 1.00f, 60, 1.05f, 1.00f, 0.87f, 0.60f, 1000, 3, ghost_waves_5},
    /* 20 */ { tile_t::KEY,        5000, 1.00f, 0.87f, 0.95f, 0.50f, 120, 1.00f, 60, 1.05f, 1.00f, 0.87f, 0.60f, 1000, 3, ghost_waves_5},
    /* 21 */ { tile_t::KEY,        5000, 0.90f, 0.79f, 0.95f, 0.50f, 120, 1.00f, 60, 1.05f, 1.00f, 0.87f, 0.60f, 1000, 3, ghost_waves_5}
};

static constexpr int level_to_idx(const int level) noexcept {
    return 1 <= level && (size_t)level <= level_spec_array.size() ? level-1 : (int)level_spec_array.size()-1;
}

const game_level_spec_t& game_level_spec(const int level) noexcept {
    return level_spec_array[ level_to_idx( level ) ];
}

const game_level_spec_t& game_level_spec() noexcept {
    return level_spec_array[ level_to_idx( get_current_level() ) ];
}

const ghost_wave_t& get_ghost_wave(const int level, const int phase_idx) noexcept {
    const ghost_wave_vec_t& waves = game_level_spec(level).ghost_waves;
    const int idx = 0 <= phase_idx && (size_t)phase_idx < waves.size() ? phase_idx : (int)waves.size()-1;
    return waves[idx];
}
const ghost_wave_t& get_ghost_wave(const int phase_idx) noexcept {
    return get_ghost_wave( get_current_level(), phase_idx );
}

//
// global_tex_t
//

int global_tex_t::tile_to_texidx(const tile_t tile) const noexcept {
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

int global_tex_t::validate_texidx(const int idx) const noexcept {
    if( 0 <= idx && (size_t)idx < textures.size() ) {
        return idx;
    }
    return -1;
}

global_tex_t::global_tex_t(SDL_Renderer* rend) noexcept
: all_images_( std::make_shared<texture_t>(rend, "media/tiles_all.png") ),
  atex_pellet_power( "PP", rend, 250, all_images_, 0, 0, 14, 14, { { 1*14, 0 }, { -1, -1} })
{
    add_sub_textures(textures, rend, all_images_, 0, 0, 14, 14, {
            {  0*14, 0 }, {  1*14, 0 }, {  2*14, 0 }, {  3*14, 0 }, {  4*14, 0 }, {  5*14, 0 }, {  6*14, 0 },
            {  7*14, 0 }, {  8*14, 0 }, {  9*14, 0 }, { 10*14, 0 }, { 11*14, 0 }, { 12*14, 0 }, { 13*14, 0 },  } );
}

void global_tex_t::destroy() noexcept {
    for(size_t i=0; i<textures.size(); ++i) {
        textures[i]->destroy();
    }
    textures.clear();
    all_images_->destroy();
}

std::shared_ptr<texture_t> global_tex_t::texture(const int idx) noexcept {
    const int idx2 = validate_texidx(idx);
    return 0 <= idx2 ? textures[idx2] : nullptr;
}
std::shared_ptr<const texture_t> global_tex_t::texture(const int idx) const noexcept {
    const int idx2 = validate_texidx(idx);
    return 0 <= idx2 ? textures[idx2] : nullptr;
}

std::shared_ptr<texture_t> global_tex_t::texture(const tile_t tile) noexcept {
    const int idx = tile_to_texidx(tile);
    return 0 <= idx ? textures[idx] : nullptr;
}
std::shared_ptr<const texture_t> global_tex_t::texture(const tile_t tile) const noexcept {
    const int idx = tile_to_texidx(tile);
    return 0 <= idx ? textures[idx] : nullptr;
}

void global_tex_t::draw_tile(const tile_t tile, SDL_Renderer* rend, const int x, const int y) noexcept {
    if( tile_t::PELLET_POWER == tile ) {
        atex_pellet_power.draw(rend, x, y);
    } else {
        std::shared_ptr<texture_t> tex = texture(tile);
        if( nullptr != tex ) {
            tex->draw2_i(rend, x, y);
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

static void on_window_resized(SDL_Renderer* rend, const int win_width_l, const int win_height_l) noexcept {
    int win_pixel_height=0;
    SDL_GetRendererOutputSize(rend, &win_pixel_width, &win_pixel_height);

    float sx = win_pixel_width / global_maze->pixel_width();
    float sy = win_pixel_height / global_maze->pixel_height();
    win_pixel_scale = static_cast<int>( std::round( std::fmin<float>(sx, sy) ) );

    if( nullptr != font_ttf ) {
        TTF_CloseFont(font_ttf);
        font_ttf = nullptr;
    }
    int font_height;
    {
        const std::string fontfilename = "fonts/freefont/FreeSansBold.ttf";
        font_height = global_maze->ppt_y() * win_pixel_scale;
        font_ttf = TTF_OpenFont(fontfilename.c_str(), font_height);
    }
    log_printf("Window Resized: %d x %d pixel ( %d x %d logical ) @ %d hz\n",
            win_pixel_width, win_pixel_height, win_width_l, win_height_l, get_frames_per_sec());
    log_printf("Pixel scale: %f x %f -> %d, font[ok %d, height %d]\n", sx, sy, win_pixel_scale, nullptr!=font_ttf, font_height);
}

static std::string get_usage(const std::string& exename) noexcept {
    // TODO: Keep in sync with README.md
    return "Usage: "+exename+" [-no_vsync] [-fps <int>] [-speed <int>] [-wwidth <int>] [-wheight <int>] "+
              "[-show_fps] [-show_modes] [-show_moves] [-show_targets] [-show_debug_gfx] [-show_all] "+
              "[-no_ghosts] [-bugfix] [-audio] [-level <int>]";
}

//
// FIXME: Consider moving game state types and fields into its own class
//
enum class game_mode_t {
    NEXT_LEVEL,
    LEVEL_START,
    GAME,
    PAUSE
};
static std::string to_string(game_mode_t m) noexcept {
    switch( m ) {
        case game_mode_t::NEXT_LEVEL:
            return "next_level";
        case game_mode_t::LEVEL_START:
            return "level_start";
        case game_mode_t::GAME:
            return "game";
        case game_mode_t::PAUSE:
            return "pause";
        default:
            return "unknown";
    }
}
enum class game_mode_duration_t : int {
    LEVEL_START = 4000 - pacman_t::number(pacman_t::mode_duration_t::HOMESTAY)
};
static constexpr int number(const game_mode_duration_t item) noexcept {
    return static_cast<int>(item);
}
static int game_mode_ms_left = -1;
static game_mode_t game_mode = game_mode_t::PAUSE;
static game_mode_t game_mode_last = game_mode_t::LEVEL_START;

static void set_game_mode(const game_mode_t m, const int caller) noexcept {
    const game_mode_t old_mode = game_mode;
    const int old_level = current_level;
    switch( m ) {
        case game_mode_t::NEXT_LEVEL:
            ++current_level;
            [[fallthrough]];
        case game_mode_t::LEVEL_START:
            pacman->set_mode( pacman_t::mode_t::LEVEL_START );
            global_maze->reset();
            game_mode = game_mode_t::LEVEL_START;
            if( audio_samples[ number( audio_clip_t::INTRO ) ]->is_valid() ) {
                // use_audio == true
                audio_samples[ number( audio_clip_t::INTRO ) ]->play();
                game_mode_ms_left = number( game_mode_duration_t::LEVEL_START );
            } else {
                // use_audio == true, shorten startup time a little
                game_mode_ms_left = 3000;
            }
            break;
        case game_mode_t::GAME:
            [[fallthrough]];
        case game_mode_t::PAUSE:
            [[fallthrough]];
        default:
            game_mode = m;
            game_mode_ms_left = -1;
            break;
    }
    game_mode_last = old_mode;
    log_printf("game set_mode(%d): %s -> %s [%d ms], level %d -> %d\n",
            caller, to_string(old_mode).c_str(), to_string(game_mode).c_str(), game_mode_ms_left, old_level, current_level);
}

int main(int argc, char *argv[])
{
    bool enable_vsync = true;
    int forced_fps = -1;
    float fields_per_sec_total=10;
    int win_width = 640, win_height = 720;
    bool disable_all_ghosts = false;
    bool show_targets = false;
    bool use_audio = false;
    {
        for(int i=1; i<argc; ++i) {
            if( 0 == strcmp("-show_fps", argv[i]) ) {
                enable_log_fps = true;
            } else if( 0 == strcmp("-show_modes", argv[i]) ) {
                enable_log_modes = true;
            } else if( 0 == strcmp("-show_moves", argv[i]) ) {
                enable_log_moves = true;
            } else if( 0 == strcmp("-show_targets", argv[i]) ) {
                show_targets = true;
            } else if( 0 == strcmp("-show_debug_gfx", argv[i]) ) {
                enable_debug_gfx = true;
            } else if( 0 == strcmp("-show_all", argv[i]) ) {
                enable_log_fps = true;
                enable_log_moves = true;
                enable_log_modes = true;
                show_targets = true;
                enable_debug_gfx = true;
            } else if( 0 == strcmp("-no_vsync", argv[i]) ) {
                enable_vsync = false;
            } else if( 0 == strcmp("-fps", argv[i]) && i+1<argc) {
                forced_fps = atoi(argv[i+1]);
                enable_vsync = false;
                ++i;
            } else if( 0 == strcmp("-speed", argv[i]) && i+1<argc) {
                fields_per_sec_total = atof(argv[i+1]);
                ++i;
            } else if( 0 == strcmp("-wwidth", argv[i]) && i+1<argc) {
                win_width = atoi(argv[i+1]);
                ++i;
            } else if( 0 == strcmp("-wheight", argv[i]) && i+1<argc) {
                win_height = atoi(argv[i+1]);
                ++i;
            } else if( 0 == strcmp("-no_ghosts", argv[i]) ) {
                disable_all_ghosts = true;
            } else if( 0 == strcmp("-bugfix", argv[i]) ) {
                original_pacman_behavior = false;
            } else if( 0 == strcmp("-audio", argv[i]) ) {
                use_audio = true;
            } else if( 0 == strcmp("-level", argv[i]) && i+1<argc) {
                current_level = atoi(argv[i+1]);
                ++i;
            }
        }
    }

    global_maze = std::make_unique<maze_t>("media/playfield_pacman.txt");

    if( !global_maze->is_ok() ) {
        log_printf("Maze: Error: %s\n", global_maze->toString().c_str());
        return -1;
    }
    {
        log_printf("--- 8< ---\n");
        const int maze_width = global_maze->width();
        global_maze->draw( [&maze_width](int x, int y, tile_t tile) {
            fprintf(stderr, "%s", to_string(tile).c_str());
            if( x == maze_width-1 ) {
                fprintf(stderr, "\n");
            }
            (void)y;
        });
        log_printf("--- >8 ---\n");
        log_printf("Maze: %s\n", global_maze->toString().c_str());
    }
    {
        log_printf("\n%s\n\n", get_usage(argv[0]).c_str());
        log_printf("- show_fps %d\n", log_fps());
        log_printf("- show_modes %d\n", log_modes());
        log_printf("- show_moves %d\n", log_moves());
        log_printf("- show_targets %d\n", show_targets);
        log_printf("- show_debug_gfx %d\n", show_debug_gfx());
        log_printf("- enable_vsync %d\n", enable_vsync);
        log_printf("- forced_fps %d\n", forced_fps);
        log_printf("- fields_per_sec %5.2f\n", fields_per_sec_total);
        log_printf("- win size %d x %d\n", win_width, win_height);
        log_printf("- use_bugfix_pacman %d\n", !use_original_pacman_behavior());
        log_printf("- use_audio %d\n", use_audio);
        log_printf("- level %d\n", get_current_level());
    }

    if (SDL_Init(SDL_INIT_EVERYTHING) != 0) {
        log_printf("SDL: Error initializing: %s\n", SDL_GetError());
    }

    if ( ( IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG ) != IMG_INIT_PNG ) {
        log_printf("SDL_image: Error initializing: %s\n", SDL_GetError());
    }

    if( 0 != TTF_Init() ) {
        log_printf("SDL_TTF: Error initializing: %s\n", SDL_GetError());
    }

    if( use_audio ) {
        use_audio = audio_open();
    }
    if( use_audio ) {
        for(int i=0; i <= number( audio_clip_t::DEATH ); ++i) {
            audio_samples.push_back( std::make_shared<audio_sample_t>("media/beginning.wav") );
            audio_samples.push_back( std::make_shared<audio_sample_t>("media/chomp.wav") );
            audio_samples.push_back( std::make_shared<audio_sample_t>("media/eatfruit.wav") );
            audio_samples.push_back( std::make_shared<audio_sample_t>("media/eatghost.wav", false /* single_play */) );
            audio_samples.push_back( std::make_shared<audio_sample_t>("media/extrapac.wav") );
            audio_samples.push_back( std::make_shared<audio_sample_t>("media/intermission.wav") );
            audio_samples.push_back( std::make_shared<audio_sample_t>("media/death.wav") );
        }
    } else {
        for(int i=0; i <= number( audio_clip_t::DEATH ); ++i) {
            audio_samples.push_back( std::make_shared<audio_sample_t>() );
        }
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
        log_printf("renderer: name: %s\n", info.name);
        log_printf("renderer: accel %d\n", 0 != ( info.flags & SDL_RENDERER_ACCELERATED ));
        log_printf("renderer: soft %d\n", 0 != ( info.flags & SDL_RENDERER_SOFTWARE ));
        log_printf("renderer: vsync %d -> %d\n", _uses_vsync, uses_vsync);
    }
 
    std::unique_ptr<texture_t> pacman_maze_tex = std::make_unique<texture_t>(rend, "media/"+global_maze->get_texture_file());
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
                    log_printf("Display %d: %d x %d @ %d Hz\n", i, mode.w, mode.h, mode.refresh_rate);
                }
            }
            const int win_display_idx = SDL_GetWindowDisplayIndex(win);
            bzero(&mode, sizeof(mode));
            SDL_GetCurrentDisplayMode(win_display_idx, &mode); // SDL_GetWindowDisplayMode(..) fails on some systems (wrong refresh_rate and logical size
            log_printf("WindowDisplayMode: %d x %d @ %d Hz @ display %d\n", mode.w, mode.h, mode.refresh_rate, win_display_idx);
            if( 0 < forced_fps ) {
                frames_per_sec = forced_fps;
            } else {
                frames_per_sec = mode.refresh_rate;
            }
        }
        on_window_resized(rend, win_pixel_width_, win_pixel_height_);
    }
    SDL_SetWindowSize(win, global_maze->pixel_width()*win_pixel_scale,
                           global_maze->pixel_height()*win_pixel_scale);

    global_tex = std::make_shared<global_tex_t>(rend);
    std::shared_ptr<texture_t> pacman_left2_tex = std::make_shared<texture_t>(global_tex->all_images()->sdl_texture(), 0 + 1*13, 28 + 0, 13, 13, false /* owner*/);

    pacman = std::make_shared<pacman_t>(rend, fields_per_sec_total);
    log_printf("%s\n", pacman->toString().c_str());

    if( !disable_all_ghosts ) {
        ghosts.push_back( std::make_shared<ghost_t>(ghost_t::personality_t::BLINKY, rend, fields_per_sec_total) );
        ghosts.push_back( std::make_shared<ghost_t>(ghost_t::personality_t::PINKY, rend, fields_per_sec_total) );
        ghosts.push_back( std::make_shared<ghost_t>(ghost_t::personality_t::INKY, rend, fields_per_sec_total) );
        ghosts.push_back( std::make_shared<ghost_t>(ghost_t::personality_t::CLYDE, rend, fields_per_sec_total) );
    }
    for(ghost_ref g : ghosts) {
        log_printf("%s\n", g->toString().c_str());
    }

    bool close = false;
    bool set_dir = false;
    bool level_start = true;
    direction_t current_dir = pacman->direction();
    SDL_Scancode current_scancode = SDL_SCANCODE_STOP;

    const uint64_t fps_range_ms = 5000;
    uint64_t t0 = getCurrentMilliseconds();
    uint64_t t1 = t0;
    uint64_t frame_count = 0;

    uint64_t last_score = pacman->score();
    std::shared_ptr<text_texture_t> ttex_score = nullptr;
    std::shared_ptr<text_texture_t> ttex_score_title = nullptr;

    set_game_mode(game_mode_t::PAUSE, 0);

    while (!close) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
                case SDL_QUIT:
                    close = true;
                    break;

                case SDL_KEYUP:
                    /**
                     * The following key sequence is possible, hence we need to validate whether the KEYUP
                     * matches and releases the current active keyscan/direction:
                     * - KEY DOWN: scancode 81 -> 'D', scancode 81, set_dir 1)
                     * - [    3,131] KEY DOWN: scancode 81 -> 'D', scancode 81, set_dir 1)
                     * - [    3,347] KEY DOWN: scancode 80 -> 'L', scancode 80, set_dir 1)
                     * - [    3,394] KEY UP: scancode 81 (ignored) -> 'L', scancode 80, set_dir 1)
                     * - [    4,061] KEY UP: scancode 80 (release) -> 'L', scancode 80, set_dir 0)
                     */
                    if ( event.key.keysym.scancode == current_scancode ) {
                        set_dir = false;
                        if( debug_key_input ) {
                            log_printf("KEY UP: scancode %d (release) -> '%s', scancode %d, set_dir %d)\n", event.key.keysym.scancode, to_string(current_dir).c_str(), current_scancode, set_dir);
                        }
                    } else {
                        if( debug_key_input ) {
                            log_printf("KEY UP: scancode %d (ignored) -> '%s', scancode %d, set_dir %d)\n", event.key.keysym.scancode, to_string(current_dir).c_str(), current_scancode, set_dir);
                        }
                    }
                    break;

                case SDL_WINDOWEVENT:
                    switch (event.window.event) {
                        case SDL_WINDOWEVENT_SHOWN:
                            if( level_start ) {
                                set_game_mode(game_mode_t::LEVEL_START, 10);
                            } else {
                                set_game_mode(game_mode_t::GAME, 11);
                            }
                            break;
                        case SDL_WINDOWEVENT_HIDDEN:
                            set_game_mode(game_mode_t::PAUSE, 12);
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
                            if( game_mode_t::PAUSE == game_mode ) {
                                set_game_mode( game_mode_last, 13 );
                            } else {
                                set_game_mode( game_mode_t::PAUSE, 14 );
                            }
                            break;
                        case SDL_SCANCODE_R:
                            current_level = 1;
                            set_game_mode(game_mode_t::LEVEL_START, 15);
                            break;
                        case SDL_SCANCODE_W:
                        case SDL_SCANCODE_UP:
                            current_dir = direction_t::UP;
                            set_dir = true;
                            break;
                        case SDL_SCANCODE_A:
                        case SDL_SCANCODE_LEFT:
                            current_dir = direction_t::LEFT;
                            set_dir = true;
                            break;
                        case SDL_SCANCODE_S:
                        case SDL_SCANCODE_DOWN:
                            current_dir = direction_t::DOWN;
                            set_dir = true;
                            break;
                        case SDL_SCANCODE_D:
                        case SDL_SCANCODE_RIGHT:
                            current_dir = direction_t::RIGHT;
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
                    if( set_dir ) {
                        current_scancode = event.key.keysym.scancode;
                    }
                    if( debug_key_input ) {
                        log_printf("KEY DOWN: scancode %d -> '%s', scancode %d, set_dir %d)\n", event.key.keysym.scancode, to_string(current_dir).c_str(), current_scancode, set_dir);
                    }
            }
        }

        bool game_active;

        if( 0 < game_mode_ms_left ) {
            game_mode_ms_left = std::max( 0, game_mode_ms_left - get_ms_per_frame() );
        }

        switch( game_mode ) {
            case game_mode_t::LEVEL_START:
                if( 0 == game_mode_ms_left ) {
                    set_game_mode( game_mode_t::GAME, 20 );
                    level_start = false;
                    game_active = true;
                } else {
                    game_active = false;
                }
                break;
            case game_mode_t::PAUSE:
                game_active = false;
                break;
            case game_mode_t::GAME:
                if( 0 == global_maze->count( tile_t::PELLET ) && 0 == global_maze->count( tile_t::PELLET_POWER ) ) {
                    set_game_mode(game_mode_t::NEXT_LEVEL, 21);
                }
            [[fallthrough]];
            default:
                game_active = true;
                break;
        }

        if( game_active ) {
            if( set_dir ) {
                pacman->set_dir(current_dir);
            }
            global_tex->tick();
            ghost_t::global_tick();
            pacman->tick();
        }
        SDL_RenderClear(rend);

        pacman_maze_tex->draw(rend, 0, 0);

        global_maze->draw( [&rend](int x, int y, tile_t tile) {
            global_tex->draw_tile(tile, rend, x, y);
        });

        if( show_debug_gfx() ) {
            uint8_t r, g, b, a;
            SDL_GetRenderDrawColor(rend, &r, &g, &b, &a);
            const int win_pixel_offset = ( win_pixel_width - global_maze->pixel_width()*win_pixel_scale ) / 2;
            {
                SDL_SetRenderDrawColor(rend, 150, 150, 150, 255);
                for(int y = global_maze->height(); y>=0; --y) {
                    SDL_RenderDrawLine(rend,
                            win_pixel_offset + global_maze->x_to_pixel( 0, win_pixel_scale),
                                               global_maze->y_to_pixel( y, win_pixel_scale),
                            win_pixel_offset + global_maze->x_to_pixel( global_maze->width(), win_pixel_scale),
                                               global_maze->y_to_pixel( y, win_pixel_scale) );
                }
                for(int x = global_maze->width(); x>=0; --x) {
                    SDL_RenderDrawLine(rend,
                            win_pixel_offset + global_maze->x_to_pixel( x, win_pixel_scale),
                                               global_maze->y_to_pixel( 0, win_pixel_scale),
                            win_pixel_offset + global_maze->x_to_pixel( x, win_pixel_scale),
                                               global_maze->y_to_pixel( global_maze->height(), win_pixel_scale) );
                }
            }
            {
                SDL_SetRenderDrawColor(rend, 0, 0, 255, 255);
                SDL_Rect bounds = {
                        .x=win_pixel_offset + global_maze->x_to_pixel(0, win_pixel_scale),
                        .y=global_maze->y_to_pixel(0, win_pixel_scale),
                        .w=global_maze->x_to_pixel(1, win_pixel_scale),
                        .h=global_maze->y_to_pixel(1, win_pixel_scale)};
                SDL_RenderDrawRect(rend, &bounds);
                SDL_Rect bounds2 = {
                        .x=win_pixel_offset + global_maze->x_to_pixel(1, win_pixel_scale),
                        .y=global_maze->y_to_pixel(4, win_pixel_scale),
                        .w=global_maze->x_to_pixel(1, win_pixel_scale),
                        .h=global_maze->y_to_pixel(1, win_pixel_scale)};
                SDL_RenderDrawRect(rend, &bounds2);
            }
            SDL_SetRenderDrawColor(rend, r, g, b, a);
        }

        pacman->draw(rend);

        ghost_t::global_draw(rend);

        if( show_targets ) {
            for(ghost_ref ghost : ghosts) {
                uint8_t r, g, b, a;
                SDL_GetRenderDrawColor(rend, &r, &g, &b, &a);
                SDL_SetRenderDrawColor(rend, 255, 255, 255, 255);
                const int win_pixel_offset = ( win_pixel_width - global_maze->pixel_width()*win_pixel_scale ) / 2;
                const acoord_t& p1 = ghost->position();
                const acoord_t& p2 = ghost->target();
                SDL_RenderDrawLine(rend,
                        win_pixel_offset + global_maze->x_to_pixel( p1.x_f(), win_pixel_scale),
                                           global_maze->y_to_pixel( p1.y_f(), win_pixel_scale),
                        win_pixel_offset + global_maze->x_to_pixel( p2.x_f(), win_pixel_scale),
                                           global_maze->y_to_pixel( p2.y_f(), win_pixel_scale) );
                SDL_SetRenderDrawColor(rend, r, g, b, a);
            }
        }

        // top line
        if( nullptr != font_ttf ) {
            if( nullptr == ttex_score_title ) {
                const std::string highscore_s("HIGH SCORE");
                ttex_score_title = draw_text_scaled(rend, font_ttf, highscore_s, 255, 255, 255, [&](const texture_t& tex, int &x, int&y) {
                    x = ( global_maze->pixel_width()*win_pixel_scale - tex.width() ) / 2;
                    y = global_maze->x_to_pixel(0, win_pixel_scale);
                });
            } else {
                ttex_score_title->redraw(rend);
            }
            if( nullptr != ttex_score && last_score == pacman->score() ) {
                ttex_score->redraw(rend);
            } else {
                std::string score_s = std::to_string( pacman->score() );
                ttex_score = draw_text_scaled(rend, font_ttf, score_s, 255, 255, 255, [&](const texture_t& tex, int &x, int&y) {
                    x = ( global_maze->pixel_width()*win_pixel_scale - tex.width() ) / 2;
                    y = global_maze->x_to_pixel(1, win_pixel_scale);
                });
                last_score = pacman->score();
            }
        }

        // bottom line: level
        {
            const float y = 34.0f;
            float x = 24.0f;

            for(int i=1; i <= get_current_level(); ++i, x-=2) {
                const tile_t f = game_level_spec(i).symbol;
                std::shared_ptr<texture_t> f_tex = global_tex->texture(f);
                if( nullptr != f_tex ) {
                    const float dx = ( 16.0f - f_tex->width() ) / 2.0f / 16.0f;
                    const float dy = ( 16.0f - f_tex->height() + 1.0f ) / 16.0f; // FIXME: funny adjustment?
                    f_tex->draw(rend, x+dx, y+dy);
                    // log_printf("XX1 level %d: %s, %.2f / %.2f + %.2f / %.2f = %.2f / %.2f\n", i, to_string(f).c_str(), x, y, dx, dy, x+dx, y+dy);
                }
            }
        }
        // bottom line: lives left
        if( nullptr != pacman_left2_tex ) {
            const float dx = ( 16.0f - pacman_left2_tex->width() ) / 2.0f / 16.0f;
            const float dy = ( 16.0f - pacman_left2_tex->height() + 1.0f ) / 16.0f; // FIXME: funny adjustment?
            const float y = 34.0f;
            float x = 2.0f;
            for(int i=0; i < 2; ++i, x+=2) {
                pacman_left2_tex->draw(rend, x+dx, y+dy);
                // log_printf("XX2 %d: %.2f / %.2f + %.2f / %.2f = %.2f / %.2f\n", i, x, y, dx, dy, x+dx, y+dy);
            }
        }
 
        // swap double buffer incl. v-sync
        SDL_RenderPresent(rend);
        ++frame_count;
        if( !uses_vsync ) {
            const int64_t fudge_ns = NanoPerMilli / 4;
            const uint64_t ms_per_frame = (uint64_t)std::round(1000.0 / (float)get_frames_per_sec());
            const uint64_t ms_last_frame = getCurrentMilliseconds() - t1;
            int64_t td_ns = int64_t( ms_per_frame - ms_last_frame ) * NanoPerMilli;
            if( td_ns > fudge_ns )
            {
                if( true ) {
                    const int64_t td_ns_0 = td_ns%NanoPerOne;
                    struct timespec ts { td_ns/NanoPerOne, td_ns_0 - fudge_ns };
                    nanosleep( &ts, NULL );
                    // log_printf("soft-sync [exp %zd > has %zd]ms, delay %" PRIi64 "ms (%lds, %ldns)\n", ms_per_frame, ms_last_frame, td_ns/NanoPerMilli, ts.tv_sec, ts.tv_nsec);
                } else {
                    SDL_Delay( td_ns / NanoPerMilli );
                    // log_printf("soft-sync [exp %zd > has %zd]ms, delay %" PRIi64 "ms\n", ms_per_frame, ms_last_frame, td_ns/NanoPerMilli);
                }
            }
        }
        t1 = getCurrentMilliseconds();
        if( log_fps() && fps_range_ms <= t1 - t0 ) {
            const float fps = get_fps(t0, t1, frame_count);
            std::string fps_str(64, '\0');
            const int written = std::snprintf(&fps_str[0], fps_str.size(), "fps %6.2f", fps);
            fps_str.resize(written);
            // log_printf("%s, td %" PRIu64 "ms, frames %" PRIu64 "\n", fps_str.c_str(), t1-t0, frame_count);
            log_printf("%s\n", fps_str.c_str());
            t0 = t1;
            frame_count = 0;
        }
    } // loop

    if( use_audio ) {
        audio_samples.clear();
        audio_close();
    }
    ghosts.clear();
    pacman->destroy();
    pacman_left2_tex->destroy();
    global_tex->destroy();
    pacman_maze_tex->destroy();

    SDL_DestroyRenderer(rend);
 
    SDL_DestroyWindow(win);

    TTF_CloseFont(font_ttf);

    SDL_Quit();
 
    return 0;
}

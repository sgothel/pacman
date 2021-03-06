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

static int win_pixel_width_ = 0;
static int win_pixel_height_ = 0;
static int win_pixel_scale_ = 1;
int win_pixel_width() noexcept { return win_pixel_width_; }
int win_pixel_height() noexcept { return win_pixel_height_; }
int win_pixel_scale() noexcept{ return win_pixel_scale_; }

static int frames_per_sec = 0;
int get_frames_per_sec() noexcept { return frames_per_sec; }

static TTF_Font* font_ttf_ = nullptr;
TTF_Font* font_ttf() noexcept { return font_ttf_; }

//
// globals across modules 'game.hpp'
//

static int current_level = 1;
int get_current_level() noexcept { return current_level; }

std::unique_ptr<maze_t> global_maze;
std::shared_ptr<global_tex_t> global_tex;
static std::vector<ghost_ref> ghosts_;
pacman_ref pacman;

std::vector<ghost_ref>& ghosts() noexcept { return ghosts_; }

ghost_ref ghost(const ghost_t::personality_t id) noexcept {
    const int idx = ghost_t::number(id);
    if( 0 <= idx && (size_t)idx < ghosts_.size() ) {
        return ghosts_[ ghost_t::number(id)];
    } else {
        return nullptr;
    }
}
std::vector<audio_sample_ref> audio_samples;

static bool original_pacman_behavior = true;
bool use_original_pacman_behavior() noexcept { return original_pacman_behavior; }

static bool decision_one_field_ahead = true;
bool use_decision_one_field_ahead() noexcept {
    return decision_one_field_ahead;
}
static bool manhatten_distance_enabled = false;
bool use_manhatten_distance() noexcept { return manhatten_distance_enabled; }

static bool enable_debug_gfx = false;
bool show_debug_gfx() noexcept { return enable_debug_gfx; }

static bool enable_log_fps = false;
static bool enable_log_moves = false;
static bool enable_log_modes = false;
bool log_fps() noexcept { return enable_log_fps; }
bool log_moves() noexcept { return enable_log_moves; }
bool log_modes() noexcept { return enable_log_modes; }

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

static ghost_wave_vec_t ghost_waves_1 = { { 7000, 20000 }, { 7000, 20000 }, { 5000,   20000 }, { 5000, std::numeric_limits<int>::max() }, { 0, std::numeric_limits<int>::max() } };
static ghost_wave_vec_t ghost_waves_2 = { { 7000, 20000 }, { 7000, 20000 }, { 5000, 1033000 }, {   17, std::numeric_limits<int>::max() }, { 0, std::numeric_limits<int>::max() } };
static ghost_wave_vec_t ghost_waves_5 = { { 5000, 20000 }, { 5000, 20000 }, { 5000, 1037000 }, {   17, std::numeric_limits<int>::max() }, { 0, std::numeric_limits<int>::max() } };

static ghost_pellet_counter_limit_t pellet_counter_limit_l1 = { 0, 0, 30, 60 };
static ghost_pellet_counter_limit_t pellet_counter_limit_l2 = { 0, 0, 0, 50 };
static ghost_pellet_counter_limit_t pellet_counter_limit_l3 = { 0, 0, 0, 0 };

ghost_pellet_counter_limit_t global_ghost_pellet_counter_limit = { 0, 7, 17, 32 };

static std::vector<game_level_spec_t> level_spec_array = {
    //                                                                                                  Elroy_1,   Elroy_2
    //                                                                                                        |          |
    //                                                                            Fright Milliseconds         |          |
    //                                                                                              |         |          |
    //        Ghost                                                               Fright Speed      |         |          |
    //        Ghost                                                       Tunnel Speed       |      |         |          |
    //        Ghost                                                Normal Speed      |       |      |         |          |
    //                                                                        |      |       |      |         |          |
    //      Pac-Man                                 Powered Dots Speed        |      |       |      |         |          |
    //      Pac-Man                               Powered Speed      |        |      |       |      |         |          |
    //                                                        |      |        |      |       |      |         |          |
    //      Pac-Man                  Normal Dots Speed        |      |        |      |       |      |         |          |
    //      Pac-Man                Normal Speed      |        |      |        |      |       |      |         |          |
    //                                        |      |        |      |        |      |       |      |         |          |
    //                     Bonus Points       |      |        |      |        |      |       |      |         |          |
    //                                |       |      |        |      |        |      |       |      |         |          |
    /*  1 */ { tile_t::CHERRY,      100,   0.80f, 0.71f,   0.90f, 0.79f,   0.75f, 0.40f,  0.50f, 6000, 5,    20, 0.80f, 10, 0.85f, ghost_waves_1, pellet_counter_limit_l1, 4000 },
    /*  2 */ { tile_t::STRAWBERRY,  300,   0.90f, 0.79f,   0.95f, 0.83f,   0.85f, 0.45f,  0.55f, 5000, 5,    30, 0.90f, 15, 0.95f, ghost_waves_2, pellet_counter_limit_l2, 4000 },
    /*  3 */ { tile_t::PEACH,       500,   0.90f, 0.79f,   0.95f, 0.83f,   0.85f, 0.45f,  0.55f, 4000, 5,    40, 0.90f, 20, 0.95f, ghost_waves_2, pellet_counter_limit_l3, 4000 },
    /*  4 */ { tile_t::PEACH,       500,   0.90f, 0.79f,   0.95f, 0.83f,   0.85f, 0.45f,  0.55f, 3000, 5,    40, 0.90f, 20, 0.95f, ghost_waves_2, pellet_counter_limit_l3, 4000 },
    /*  5 */ { tile_t::APPLE,       700,   1.00f, 0.87f,   1.00f, 0.87f,   0.95f, 0.50f,  0.60f, 2000, 5,    40, 1.00f, 20, 1.05f, ghost_waves_5, pellet_counter_limit_l3, 3000 },
    /*  6 */ { tile_t::APPLE,       700,   1.00f, 0.87f,   1.00f, 0.87f,   0.95f, 0.50f,  0.60f, 5000, 5,    50, 1.00f, 25, 1.05f, ghost_waves_5, pellet_counter_limit_l3, 3000 },
    /*  7 */ { tile_t::MELON,      1000,   1.00f, 0.87f,   1.00f, 0.87f,   0.95f, 0.50f,  0.60f, 2000, 5,    50, 1.00f, 25, 1.05f, ghost_waves_5, pellet_counter_limit_l3, 3000 },
    /*  8 */ { tile_t::MELON,      1000,   1.00f, 0.87f,   1.00f, 0.87f,   0.95f, 0.50f,  0.60f, 2000, 5,    50, 1.00f, 25, 1.05f, ghost_waves_5, pellet_counter_limit_l3, 3000 },
    /*  9 */ { tile_t::GALAXIAN,   2000,   1.00f, 0.87f,   1.00f, 0.87f,   0.95f, 0.50f,  0.60f, 1000, 3,    60, 1.00f, 30, 1.05f, ghost_waves_5, pellet_counter_limit_l3, 3000 },
    /* 10 */ { tile_t::GALAXIAN,   2000,   1.00f, 0.87f,   1.00f, 0.87f,   0.95f, 0.50f,  0.60f, 5000, 5,    60, 1.00f, 30, 1.05f, ghost_waves_5, pellet_counter_limit_l3, 3000 },
    /* 11 */ { tile_t::BELL,       3000,   1.00f, 0.87f,   1.00f, 0.87f,   0.95f, 0.50f,  0.60f, 2000, 5,    60, 1.00f, 30, 1.05f, ghost_waves_5, pellet_counter_limit_l3, 3000 },
    /* 12 */ { tile_t::BELL,       3000,   1.00f, 0.87f,   1.00f, 0.87f,   0.95f, 0.50f,  0.60f, 1000, 3,    80, 1.00f, 40, 1.05f, ghost_waves_5, pellet_counter_limit_l3, 3000 },
    /* 13 */ { tile_t::KEY,        5000,   1.00f, 0.87f,   1.00f, 0.87f,   0.95f, 0.50f,  0.60f, 1000, 3,    80, 1.00f, 40, 1.05f, ghost_waves_5, pellet_counter_limit_l3, 3000 },
    /* 14 */ { tile_t::KEY,        5000,   1.00f, 0.87f,   1.00f, 0.87f,   0.95f, 0.50f,  0.60f, 3000, 5,    80, 1.00f, 40, 1.05f, ghost_waves_5, pellet_counter_limit_l3, 3000 },
    /* 15 */ { tile_t::KEY,        5000,   1.00f, 0.87f,   1.00f, 0.87f,   0.95f, 0.50f,  0.60f, 1000, 3,   100, 1.00f, 50, 1.05f, ghost_waves_5, pellet_counter_limit_l3, 3000 },
    /* 16 */ { tile_t::KEY,        5000,   1.00f, 0.87f,   1.00f, 0.87f,   0.95f, 0.50f,  0.60f, 1000, 3,   100, 1.00f, 50, 1.05f, ghost_waves_5, pellet_counter_limit_l3, 3000 },
    /* 17 */ { tile_t::KEY,        5000,   1.00f, 0.87f,   1.00f, 0.87f,   0.95f, 0.50f,  0.60f, 1000, 3,   100, 1.00f, 50, 1.05f, ghost_waves_5, pellet_counter_limit_l3, 3000 },
    /* 18 */ { tile_t::KEY,        5000,   1.00f, 0.87f,   1.00f, 0.87f,   0.95f, 0.50f,  0.60f, 1000, 3,   100, 1.00f, 50, 1.05f, ghost_waves_5, pellet_counter_limit_l3, 3000 },
    /* 19 */ { tile_t::KEY,        5000,   1.00f, 0.87f,   1.00f, 0.87f,   0.95f, 0.50f,  0.60f, 1000, 3,   120, 1.00f, 60, 1.05f, ghost_waves_5, pellet_counter_limit_l3, 3000 },
    /* 20 */ { tile_t::KEY,        5000,   1.00f, 0.87f,   1.00f, 0.87f,   0.95f, 0.50f,  0.60f, 1000, 3,   120, 1.00f, 60, 1.05f, ghost_waves_5, pellet_counter_limit_l3, 3000 },
    /* 21 */ { tile_t::KEY,        5000,   0.90f, 0.79f,   0.90f, 0.79f,   0.95f, 0.50f,  0.60f, 1000, 3,   120, 1.00f, 60, 1.05f, ghost_waves_5, pellet_counter_limit_l3, 3000 }
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

void global_tex_t::draw_tile(const tile_t tile, SDL_Renderer* rend, const float x, const float y) noexcept {
    if( tile_t::PELLET_POWER == tile ) {
        atex_pellet_power.draw2(rend, x, y);
    } else {
        std::shared_ptr<texture_t> tex = texture(tile);
        if( nullptr != tex ) {
            tex->draw2_f(rend, x, y);
        }
    }
}

std::string global_tex_t::toString() const {
    return "tiletex[count "+std::to_string(textures.size())+"]";
}

//
// main
//

static void on_window_resized(SDL_Renderer* rend, const int win_width_l, const int win_height_l) noexcept {
    SDL_GetRendererOutputSize(rend, &win_pixel_width_, &win_pixel_height_);

    float sx = win_pixel_width() / global_maze->pixel_width();
    float sy = win_pixel_height() / global_maze->pixel_height();
    win_pixel_scale_ = static_cast<int>( std::round( std::fmin<float>(sx, sy) ) );

    if( nullptr != font_ttf() ) {
        TTF_CloseFont(font_ttf());
        font_ttf_ = nullptr;
    }
    int font_height;
    {
        const std::string fontfilename = "fonts/freefont/FreeSansBold.ttf";
        font_height = global_maze->ppt_y() * win_pixel_scale();
        font_ttf_ = TTF_OpenFont(fontfilename.c_str(), font_height);
    }
    log_printf("Window Resized: %d x %d pixel ( %d x %d logical ) @ %d hz\n",
            win_pixel_width(), win_pixel_height(), win_width_l, win_height_l, get_frames_per_sec());
    log_printf("Pixel scale: %f x %f -> %d, font[ok %d, height %d]\n", sx, sy, win_pixel_scale(), nullptr!=font_ttf(), font_height);
}

static std::string get_usage(const std::string& exename) noexcept {
    // TODO: Keep in sync with README.md
    return "Usage: "+exename+" [-2p] [-audio] [-pixqual <int>] [-no_vsync] [-fps <int>] [-speed <int>] [-wwidth <int>] [-wheight <int>] "+
              "[-show_fps] [-show_modes] [-show_moves] [-show_targets] [-show_debug_gfx] [-show_all] "+
              "[-no_ghosts] [-invincible] [-bugfix] [-decision_on_spot] [-dist_manhatten] [-level <int>] [-record <basename-of-bmp-files>]";
}

//
// FIXME: Consider moving game state types and fields into its own class
//
enum class game_mode_t {
    NEXT_LEVEL,
    START,
    GAME,
    PAUSE
};
static std::string to_string(game_mode_t m) noexcept {
    switch( m ) {
        case game_mode_t::NEXT_LEVEL:
            return "next_level";
        case game_mode_t::START:
            return "start";
        case game_mode_t::GAME:
            return "game";
        case game_mode_t::PAUSE:
            return "pause";
        default:
            return "unknown";
    }
}
enum class game_mode_duration_t : int {
    LEVEL_START_SOUND = 4000,
    LEVEL_START       = 3000,
    START             = 2000
};
static constexpr int number(const game_mode_duration_t item) noexcept {
    return static_cast<int>(item);
}
static int game_mode_ms_left = -1;
static game_mode_t game_mode = game_mode_t::PAUSE;
static game_mode_t game_mode_last = game_mode_t::PAUSE;

static void set_game_mode(const game_mode_t m, const int caller) noexcept {
    const game_mode_t old_mode = game_mode;
    const int old_level = current_level;
    switch( m ) {
        case game_mode_t::NEXT_LEVEL:
            ++current_level;
            global_maze->reset();
            pacman->set_mode( pacman_t::mode_t::LEVEL_SETUP );
            game_mode = game_mode_t::START;
            if( audio_samples[ number( audio_clip_t::INTRO ) ]->is_valid() ) {
                audio_samples[ number( audio_clip_t::INTRO ) ]->play();
                game_mode_ms_left = number( game_mode_duration_t::LEVEL_START_SOUND );
            } else {
                game_mode_ms_left = number( game_mode_duration_t::LEVEL_START );
            }
            break;
        case game_mode_t::START:
            pacman->set_mode( pacman_t::mode_t::LEVEL_SETUP );
            game_mode = game_mode_t::START;
            game_mode_ms_left = number( game_mode_duration_t::START );
            break;
        case game_mode_t::GAME:
            if( game_mode_t::START == old_mode ) {
                pacman->set_mode( pacman_t::mode_t::START );
            }
            [[fallthrough]];
        case game_mode_t::PAUSE:
            pacman->stop_audio_loops();
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
    bool invincible = false;
    bool show_targets = false;
    bool use_audio = false;
    int pixel_filter_quality = 0;
    int start_level = 1;
    bool human_blinky = false;
    std::string record_bmpseq_basename;
    {
        for(int i=1; i<argc; ++i) {
            if( 0 == strcmp("-2p", argv[i]) ) {
                human_blinky = true;
            } else if( 0 == strcmp("-audio", argv[i]) ) {
                use_audio = true;
            } else if( 0 == strcmp("-pixqual", argv[i]) && i+1<argc) {
                pixel_filter_quality = atoi(argv[i+1]);
                ++i;
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
            } else if( 0 == strcmp("-show_fps", argv[i]) ) {
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
            } else if( 0 == strcmp("-no_ghosts", argv[i]) ) {
                disable_all_ghosts = true;
            } else if( 0 == strcmp("-invincible", argv[i]) ) {
                invincible = true;
            } else if( 0 == strcmp("-bugfix", argv[i]) ) {
                original_pacman_behavior = false;
            } else if( 0 == strcmp("-decision_on_spot", argv[i]) ) {
                decision_one_field_ahead = false;
            } else if( 0 == strcmp("-dist_manhatten", argv[i]) ) {
                manhatten_distance_enabled = true;
            } else if( 0 == strcmp("-level", argv[i]) && i+1<argc) {
                start_level = atoi(argv[i+1]);
                ++i;
            } else if( 0 == strcmp("-record", argv[i]) && i+1<argc) {
                record_bmpseq_basename = argv[i+1];
                ++i;
            }
        }
    }
    const std::string exename(argv[0]);

    global_maze = std::make_unique<maze_t>("media/playfield_pacman.txt");
    current_level = start_level;

    if( !global_maze->is_ok() ) {
        log_printf("Maze: Error: %s\n", global_maze->toString().c_str());
        return -1;
    }
    {
        log_printf("--- 8< ---\n");
        const int maze_width = global_maze->width();
        global_maze->draw( [&maze_width](float x, float y, tile_t tile) {
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
        log_printf("\n%s\n\n", get_usage(exename).c_str());
        log_printf("- 2p %d\n", human_blinky);
        log_printf("- use_audio %d\n", use_audio);
        log_printf("- pixqual %d\n", pixel_filter_quality);
        log_printf("- enable_vsync %d\n", enable_vsync);
        log_printf("- forced_fps %d\n", forced_fps);
        log_printf("- fields_per_sec %5.2f\n", fields_per_sec_total);
        log_printf("- win size %d x %d\n", win_width, win_height);
        log_printf("- show_fps %d\n", log_fps());
        log_printf("- show_modes %d\n", log_modes());
        log_printf("- show_moves %d\n", log_moves());
        log_printf("- show_targets %d\n", show_targets);
        log_printf("- show_debug_gfx %d\n", show_debug_gfx());
        log_printf("- no_ghosts %d\n", disable_all_ghosts);
        log_printf("- invincible %d\n", invincible);
        log_printf("- bugfix %d\n", !use_original_pacman_behavior());
        log_printf("- decision_on_spot %d\n", !use_decision_one_field_ahead());
        log_printf("- distance %s\n", use_manhatten_distance() ? "Manhatten" : "Euclidean");
        log_printf("- level %d\n", get_current_level());
        log_printf("- record %s\n", record_bmpseq_basename.size()==0 ? "disabled" : record_bmpseq_basename.c_str());
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
        audio_samples.push_back( std::make_shared<audio_sample_t>("media/intro.mp3") );
        audio_samples.push_back( std::make_shared<audio_sample_t>("media/munch.wav") );
        audio_samples.push_back( std::make_shared<audio_sample_t>("media/eatfruit.mp3") );
        audio_samples.push_back( std::make_shared<audio_sample_t>("media/eatghost.mp3", false /* single_play */) );
        audio_samples.push_back( std::make_shared<audio_sample_t>("media/death.mp3") );
        // audio_samples.push_back( std::make_shared<audio_sample_t>("media/extrapac.mp3") );
        // audio_samples.push_back( std::make_shared<audio_sample_t>("media/intermission.mp3") );
    } else {
        for(int i=0; i <= number( audio_clip_t::DEATH ); ++i) {
            audio_samples.push_back( std::make_shared<audio_sample_t>() );
        }
    }

    if( enable_vsync ) {
        SDL_SetHint(SDL_HINT_RENDER_VSYNC, "1");
    }
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, std::to_string(pixel_filter_quality).c_str());

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
        int width=0;
        int height=0;
        SDL_GetRendererOutputSize(rend, &width, &height);
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
        on_window_resized(rend, width, height);

        SDL_SetWindowSize(win, global_maze->pixel_width()*win_pixel_scale(),
                               global_maze->pixel_height()*win_pixel_scale());
    }

    global_tex = std::make_shared<global_tex_t>(rend);
    std::shared_ptr<texture_t> pacman_left2_tex = std::make_shared<texture_t>(global_tex->all_images()->sdl_texture(), 0 + 1*13, 28 + 0, 13, 13, false /* owner*/);

    pacman = std::make_shared<pacman_t>(rend, fields_per_sec_total);
    pacman->set_invincible(invincible);
    log_printf("%s\n", pacman->toString().c_str());

    ghost_ref blinky = nullptr;
    if( !disable_all_ghosts ) {
        ghosts_.push_back( std::make_shared<ghost_t>(ghost_t::personality_t::BLINKY, rend, fields_per_sec_total) );
        ghosts_.push_back( std::make_shared<ghost_t>(ghost_t::personality_t::PINKY, rend, fields_per_sec_total) );
        ghosts_.push_back( std::make_shared<ghost_t>(ghost_t::personality_t::INKY, rend, fields_per_sec_total) );
        ghosts_.push_back( std::make_shared<ghost_t>(ghost_t::personality_t::CLYDE, rend, fields_per_sec_total) );
        blinky = ghost( ghost_t::personality_t::BLINKY );
        if( human_blinky ) {
            blinky->set_manual_control(true);
        }
    }
    for(ghost_ref g : ghosts()) {
        log_printf("%s\n", g->toString().c_str());
    }

    bool window_shown = false;
    bool close = false;
    bool set_dir_1 = false;
    bool set_dir_2 = false;
    direction_t pacman_dir = pacman->direction();
    direction_t blinky_dir = direction_t::LEFT;
    SDL_Scancode pacman_scancode = SDL_SCANCODE_STOP;
    SDL_Scancode blinky_scancode = SDL_SCANCODE_STOP;

    const uint64_t fps_range_ms = 5000;
    uint64_t t0 = getCurrentMilliseconds();
    uint64_t t1 = t0;
    uint64_t frame_count = 0;
    uint64_t frame_count_total = 0;
    int snapshot_counter = 0;

    current_level = start_level - 1;
    pacman->reset_score();
    set_game_mode(game_mode_t::NEXT_LEVEL, 1);

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
                    if ( event.key.keysym.scancode == pacman_scancode ) {
                        set_dir_1 = false;
                        if( debug_key_input ) {
                            log_printf("KEY UP: pacman scancode %d (release) -> '%s', scancode %d, set_dir %d)\n", event.key.keysym.scancode, to_string(pacman_dir).c_str(), pacman_scancode, set_dir_1);
                        }
                    } else if ( event.key.keysym.scancode == blinky_scancode ) {
                        set_dir_2 = false;
                        if( debug_key_input ) {
                            log_printf("KEY UP: blinky scancode %d (release) -> '%s', scancode %d, set_dir %d)\n", event.key.keysym.scancode, to_string(pacman_dir).c_str(), pacman_scancode, set_dir_1);
                        }
                    } else {
                        if( debug_key_input ) {
                            log_printf("KEY UP: scancode %d (ignored) -> '%s', scancode %d, set_dir %d)\n", event.key.keysym.scancode, to_string(pacman_dir).c_str(), pacman_scancode, set_dir_1);
                        }
                    }
                    break;

                case SDL_WINDOWEVENT:
                    switch (event.window.event) {
                        case SDL_WINDOWEVENT_SHOWN:
                            // log_printf("Window Shown\n");
                            window_shown = true;
                            break;
                        case SDL_WINDOWEVENT_HIDDEN:
                            // log_printf("Window Hidden\n");
                            window_shown = false;
                            break;
                        case SDL_WINDOWEVENT_RESIZED:
                            // log_printf("Window Resize: %d x %d\n", event.window.data1, event.window.data2);
                            on_window_resized(rend, event.window.data1, event.window.data2);
                            clear_text_texture_cache();
                            break;
                        case SDL_WINDOWEVENT_SIZE_CHANGED:
                            // log_printf("Window SizeChanged: %d x %d\n", event.window.data1, event.window.data2);
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
                            current_level = start_level - 1;
                            pacman->reset_score();
                            set_game_mode(game_mode_t::NEXT_LEVEL, 15);
                            break;
                        case SDL_SCANCODE_W:
                            if( human_blinky ) {
                                blinky_dir = direction_t::UP;
                                set_dir_2 = true;
                                break;
                            }
                            [[fallthrough]];
                        case SDL_SCANCODE_UP:
                            pacman_dir = direction_t::UP;
                            set_dir_1 = true;
                            break;
                        case SDL_SCANCODE_A:
                            if( human_blinky ) {
                                blinky_dir = direction_t::LEFT;
                                set_dir_2 = true;
                                break;
                            }
                            [[fallthrough]];
                        case SDL_SCANCODE_LEFT:
                            pacman_dir = direction_t::LEFT;
                            set_dir_1 = true;
                            break;
                        case SDL_SCANCODE_S:
                            if( human_blinky ) {
                                blinky_dir = direction_t::DOWN;
                                set_dir_2 = true;
                                break;
                            }
                            [[fallthrough]];
                        case SDL_SCANCODE_DOWN:
                            pacman_dir = direction_t::DOWN;
                            set_dir_1 = true;
                            break;
                        case SDL_SCANCODE_D:
                            if( human_blinky ) {
                                blinky_dir = direction_t::RIGHT;
                                set_dir_2 = true;
                                break;
                            }
                            [[fallthrough]];
                        case SDL_SCANCODE_RIGHT:
                            pacman_dir = direction_t::RIGHT;
                            set_dir_1 = true;
                            break;
                        case SDL_SCANCODE_F:
                            fullscreen_flags = 0 == fullscreen_flags ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0;
                            SDL_SetWindowFullscreen(win, fullscreen_flags);
                            break;
                        case SDL_SCANCODE_F12: {
                            std::string snap_fname(128, '\0');
                            const int written = std::snprintf(&snap_fname[0], snap_fname.size(), "puckman-snap-%4.4d.bmp", snapshot_counter);
                            snap_fname.resize(written);
                            save_snapshot(rend, win_pixel_width(), win_pixel_height(), snap_fname);
                            ++snapshot_counter;
                            break;
                        }
                        default:
                            // nop
                            break;
                    }
                    if( set_dir_1 ) {
                        pacman_scancode = event.key.keysym.scancode;
                    } else if( set_dir_2 ) {
                        blinky_scancode = event.key.keysym.scancode;
                    }
                    if( debug_key_input ) {
                        log_printf("KEY DOWN: scancode %d -> '%s', scancode[pacman %d, blinky %d], set_dir[pacman %d, blinky %d])\n",
                                event.key.keysym.scancode, to_string(pacman_dir).c_str(), pacman_scancode, blinky_scancode, set_dir_1, set_dir_2);
                    }
            }
        }

        if( !window_shown ) {
            SDL_Delay( 100 );
            continue;
        }
        bool game_active;

        if( 0 < game_mode_ms_left ) {
            game_mode_ms_left = std::max( 0, game_mode_ms_left - get_ms_per_frame() );
        }

        switch( game_mode ) {
            case game_mode_t::START:
                if( 0 == game_mode_ms_left ) {
                    set_game_mode( game_mode_t::GAME, 20 );
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
            if( set_dir_1 ) {
                pacman->set_dir(pacman_dir);
            }
            if( set_dir_2 && nullptr != blinky ) {
                blinky->set_dir(blinky_dir);
            }
            global_tex->tick();
            ghost_t::global_tick();
            if( !pacman->tick() ) {
                // pacman caught and died .. post dead animation
                set_game_mode( game_mode_t::START, 22 );
            }
        }
        SDL_RenderClear(rend);

        const int win_pixel_offset = ( win_pixel_width() - global_maze->pixel_width()*win_pixel_scale() ) / 2;

        if( show_debug_gfx() ) {
            uint8_t r, g, b, a;
            SDL_GetRenderDrawColor(rend, &r, &g, &b, &a);
            {
                // Red Zones + Tunnel
                const box_t& red_zone1 = global_maze->red_zone1_box();
                const box_t& red_zone2 = global_maze->red_zone2_box();
                const box_t& tunnel1 = global_maze->tunnel1_box();
                const box_t& tunnel2 = global_maze->tunnel2_box();

                SDL_SetRenderDrawColor(rend, 255, 96, 96, 100);
                draw_box(rend, true, win_pixel_offset, 0, red_zone1.x(), red_zone1.y(), red_zone1.width(), red_zone1.height());
                draw_box(rend, true, win_pixel_offset, 0, red_zone2.x(), red_zone2.y(), red_zone2.width(), red_zone2.height());

                SDL_SetRenderDrawColor(rend, 96, 96, 255, 100);
                draw_box(rend, true, win_pixel_offset, 0, tunnel1.x(), tunnel1.y(), tunnel1.width(), tunnel1.height());
                draw_box(rend, true, win_pixel_offset, 0, tunnel2.x(), tunnel2.y(), tunnel2.width(), tunnel2.height());
            }
            {
                // Grey Grid
                SDL_SetRenderDrawColor(rend, 150, 150, 150, 255);
                for(int y = global_maze->height()-1; y>=0; --y) {
                    draw_line(rend, 1, win_pixel_offset, 0, 0, y, global_maze->width(), y);
                }
                for(int x = global_maze->width()-1; x>=0; --x) {
                    draw_line(rend, 1, win_pixel_offset, 0, x, 0, x, global_maze->height());
                }
            }
            {
                // Filled check-boxes at 0/0 and each scatter target tile
                acoord_t blinky_top_right = global_maze->top_right_scatter();
                acoord_t pinky_top_left = global_maze->top_left_scatter();
                acoord_t inky_bottom_right = global_maze->bottom_right_scatter();
                acoord_t clyde_bottom_left = global_maze->bottom_left_scatter();

                SDL_SetRenderDrawColor(rend, pacman_t::rgb_color[0], pacman_t::rgb_color[1], pacman_t::rgb_color[2], 255);
                draw_box(rend, true, win_pixel_offset, 0, 0, 0, 1, 1);

                SDL_SetRenderDrawColor(rend,
                        ghost_t::rgb_color[ ghost_t::number( ghost_t::personality_t::BLINKY ) ][0],
                        ghost_t::rgb_color[ ghost_t::number( ghost_t::personality_t::BLINKY ) ][1],
                        ghost_t::rgb_color[ ghost_t::number( ghost_t::personality_t::BLINKY ) ][2], 255);
                draw_box(rend, true, win_pixel_offset, 0, blinky_top_right.x_i(),  blinky_top_right.y_i(), 1, 1);

                SDL_SetRenderDrawColor(rend,
                        ghost_t::rgb_color[ ghost_t::number( ghost_t::personality_t::PINKY ) ][0],
                        ghost_t::rgb_color[ ghost_t::number( ghost_t::personality_t::PINKY ) ][1],
                        ghost_t::rgb_color[ ghost_t::number( ghost_t::personality_t::PINKY ) ][2], 255);
                draw_box(rend, true, win_pixel_offset, 0, pinky_top_left.x_i(),    pinky_top_left.y_i(),    1, 1);

                SDL_SetRenderDrawColor(rend,
                        ghost_t::rgb_color[ ghost_t::number( ghost_t::personality_t::INKY ) ][0],
                        ghost_t::rgb_color[ ghost_t::number( ghost_t::personality_t::INKY ) ][1],
                        ghost_t::rgb_color[ ghost_t::number( ghost_t::personality_t::INKY ) ][2], 255);
                draw_box(rend, true, win_pixel_offset, 0, inky_bottom_right.x_i(), inky_bottom_right.y_i(), 1, 1);

                SDL_SetRenderDrawColor(rend,
                        ghost_t::rgb_color[ ghost_t::number( ghost_t::personality_t::CLYDE ) ][0],
                        ghost_t::rgb_color[ ghost_t::number( ghost_t::personality_t::CLYDE ) ][1],
                        ghost_t::rgb_color[ ghost_t::number( ghost_t::personality_t::CLYDE ) ][2], 255);
                draw_box(rend, true, win_pixel_offset, 0, clyde_bottom_left.x_i(), clyde_bottom_left.y_i(), 1, 1);
            }
            SDL_SetRenderDrawColor(rend, r, g, b, a);
        }

        pacman_maze_tex->draw(rend, 0, 0);

        global_maze->draw( [&rend](float x, float y, tile_t tile) {
            global_tex->draw_tile(tile, rend, x, y);
        });

        pacman->draw(rend);

        ghost_t::global_draw(rend);

        if( show_targets ) {
            const int pixel_width_scaled = std::max( 1, round_to_int( win_pixel_scale() / 2.0f ) );
            uint8_t r, g, b, a;
            SDL_GetRenderDrawColor(rend, &r, &g, &b, &a);
            for(ghost_ref ghost : ghosts()) {
                if( ghost->is_scattering_or_chasing() ) {
                    const acoord_t& p1 = ghost->position();
                    const acoord_t& p2 = ghost->target();
                    SDL_SetRenderDrawColor(rend,
                            ghost_t::rgb_color[ ghost_t::number( ghost->id() ) ][0],
                            ghost_t::rgb_color[ ghost_t::number( ghost->id() ) ][1],
                            ghost_t::rgb_color[ ghost_t::number( ghost->id() ) ][2], 255);
                    draw_line(rend, pixel_width_scaled, win_pixel_offset, 0, p1.x_f(), p1.y_f(), p2.x_f(), p2.y_f());
                }
            }
            SDL_SetRenderDrawColor(rend, r, g, b, a);
        }

        // top line: title
        draw_text_scaled(rend, font_ttf(), "HIGH SCORE", 255, 255, 255, true /* cache */, [&](const texture_t& tex, int &x, int&y) {
            x = ( global_maze->pixel_width()*win_pixel_scale() - tex.width() ) / 2;
            y = global_maze->x_to_pixel(0, win_pixel_scale());
        });

        // 2nd line - center: score
        draw_text_scaled(rend, font_ttf(), std::to_string( pacman->score() ), 255, 255, 255, false /* cache */, [&](const texture_t& tex, int &x, int&y) {
            x = ( global_maze->pixel_width()*win_pixel_scale() - tex.width() ) / 2;
            y = global_maze->x_to_pixel(1, win_pixel_scale());
        });

        if( show_debug_gfx() ) {
            // 2nd line - right: tiles
            draw_text_scaled(rend, font_ttf(), std::to_string(global_maze->count(tile_t::PELLET))+" / "+std::to_string(global_maze->max(tile_t::PELLET)),
                             255, 255, 255, false /* cache */, [&](const texture_t& tex, int &x, int&y) {
                x = global_maze->pixel_width()*win_pixel_scale() - tex.width();
                y = global_maze->x_to_pixel(1, win_pixel_scale());
            });
        }

        // optional text
        if( game_mode_t::START == game_mode ) {
            const box_t& msg_box = global_maze->message_box();
            draw_text_scaled(rend, font_ttf(), "READY!",
                             pacman_t::rgb_color[0], pacman_t::rgb_color[1], pacman_t::rgb_color[2],
                             true /* cache */, [&](const texture_t& tex, int &x, int&y) {
                x = global_maze->x_to_pixel(msg_box.center_x(), win_pixel_scale()) - tex.width()  / 2;
                y = global_maze->x_to_pixel(msg_box.y(), win_pixel_scale()) - tex.height() / 4;
            });
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
        if( record_bmpseq_basename.size() > 0 ) {
            std::string snap_fname(128, '\0');
            const int written = std::snprintf(&snap_fname[0], snap_fname.size(), "%s-%7.7" PRIu64 ".bmp", record_bmpseq_basename.c_str(), frame_count_total);
            snap_fname.resize(written);
            save_snapshot(rend, win_pixel_width(), win_pixel_height(), snap_fname);
        }
        ++frame_count;
        ++frame_count_total;
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
    ghosts_.clear();
    pacman->destroy();
    pacman_left2_tex->destroy();
    global_tex->destroy();
    pacman_maze_tex->destroy();

    SDL_DestroyRenderer(rend);
 
    SDL_DestroyWindow(win);

    TTF_CloseFont(font_ttf());

    SDL_Quit();
 
    return 0;
}

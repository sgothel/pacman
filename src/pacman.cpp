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
#include <pacman/graphics.hpp>
#include <pacman/audio.hpp>
#include <pacman/game.hpp>
#include <pacman/globals.hpp>

#include <limits>

#include <cstdio>
#include <time.h>

static constexpr const bool DEBUG_GFX_BOUNDS = false;

//
// pacman_t
//

animtex_t& pacman_t::get_tex() {
    switch( mode ) {
        case pacman_t::mode_t::HOME:
            return atex_home;
        case pacman_t::mode_t::NORMAL:
            [[fallthrough]];
        case pacman_t::mode_t::POWERED:
            switch( dir_ ) {
                case direction_t::DOWN:
                    return atex_down;

                case direction_t::RIGHT:
                    return atex_right;

                case direction_t::UP:
                    return atex_up;

                case direction_t::LEFT:
                    [[fallthrough]];
                default:
                    return atex_left;
            }
        case pacman_t::mode_t::DEAD:
            return atex_dead;
        default:
            return atex_left;
    }
}

pacman_t::pacman_t(SDL_Renderer* rend, const float fields_per_sec_total_, bool auto_move_)
: fields_per_sec_total(fields_per_sec_total_),
  current_speed_pct(0.80f),
  keyframei(true /* odd */, get_frames_per_sec(), fields_per_sec_total*current_speed_pct, false /* hint_slower */),
  skip_tick_each_frames( -1 ),
  skip_tick_counter( -1 ),
  auto_move(auto_move_),
  mode( mode_t::HOME ),
  mode_ms_left ( -1 ),
  lives( 3 ),
  dir_( direction_t::LEFT ),
  steps_left( auto_move ? -1 : 0),
  score( 0 ),
  atex_left( "L", rend, ms_per_tex, global_tex->get_all_images(), 0, 28, 13, 13, { { 0*13, 0 }, { 1*13, 0 } }),
  atex_right("R", rend, ms_per_tex, global_tex->get_all_images(), 0, 28, 13, 13, { { 2*13, 0 }, { 3*13, 0 } }),
  atex_up(   "U", rend, ms_per_tex, global_tex->get_all_images(), 0, 28, 13, 13, { { 4*13, 0 }, { 5*13, 0 } }),
  atex_down( "D", rend, ms_per_tex, global_tex->get_all_images(), 0, 28, 13, 13, { { 6*13, 0 }, { 7*13, 0 } }),
  atex_dead( "X", rend, ms_per_tex, global_tex->get_all_images(), 0, 14, 14, 14, {
          { 0*14, 0 }, { 1*14, 0 }, { 2*14, 0 }, { 3*14, 0 }, { 4*14, 0 }, { 5*14, 0 },
          { 6*14, 0 }, { 7*14, 0 }, { 8*14, 0 }, { 9*14, 0 }, { 10*14, 0 }, { 11*14, 0 } }),
  atex_home( "H", ms_per_tex, { atex_dead.get_tex(0) }),
  atex( &get_tex() ),
  pos( global_maze->get_pacman_start_pos() )
{
    set_mode( mode_t::HOME );
}

void pacman_t::destroy() {
    atex_left.destroy();
    atex_right.destroy();
    atex_up.destroy();
    atex_down.destroy();
    atex_dead.destroy();
    atex_home.destroy();
}

void pacman_t::set_mode(const mode_t m) {
    const mode_t old_mode = mode;
    switch( m ) {
        case mode_t::HOME:
            audio_samples[ ::number( audio_clip_t::MUNCH ) ]->stop();
            mode = m;
            mode_ms_left = -1;
            pos = global_maze->get_pacman_start_pos();
            pos.set_centered(keyframei);
            set_dir( direction_t::LEFT );
            atex = &get_tex();
            for(ghost_ref g : ghosts) {
                g->set_mode(ghost_t::mode_t::HOME);
            }
            set_speed(0.80f);
            break;
        case mode_t::NORMAL:
            mode = m;
            mode_ms_left = -1;
            perf_fields_walked_t0 = getCurrentMilliseconds();
            pos.reset_stats();
            set_speed(0.80f);
            break;
        case mode_t::POWERED:
            mode = m;
            mode_ms_left = number( mode_duration_t::INPOWER );
            for(ghost_ref g : ghosts) {
                g->set_mode(ghost_t::mode_t::SCARED);
            }
            set_speed(0.90f);
            break;
        case mode_t::DEAD:
            audio_samples[ ::number( audio_clip_t::MUNCH ) ]->stop();
            mode = m;
            mode_ms_left = number( mode_duration_t::DEADANIM );
            atex_dead.reset();
            for(ghost_ref g : ghosts) {
                g->set_mode(ghost_t::mode_t::AWAY);
            }
            audio_samples[ ::number( audio_clip_t::DEATH ) ]->play();
            break;
        default:
            mode = m;
            mode_ms_left = -1;
            break;
    }
    log_printf("pacman set_mode: %s -> %s [%d ms], %s\n", to_string(old_mode).c_str(), to_string(mode).c_str(), mode_ms_left, pos.toShortString().c_str());
}

void pacman_t::set_speed(const float pct) {
    const float old = current_speed_pct;
    current_speed_pct = pct;
    keyframei.reset(true /* odd */, get_frames_per_sec(), fields_per_sec_total*pct, false /* hint_slower */);
    const int fps_d = trunc_to_int( keyframei.get_frames_per_second_diff() );
    if( fps_d > 0 ) {
        skip_tick_each_frames = get_frames_per_sec() / fps_d;
    } else {
        skip_tick_each_frames = -1;
    }
    skip_tick_counter = skip_tick_each_frames;
    if( log_moves ) {
        log_printf("pacman set_speed: %5.2f -> %5.2f: skip_tick_each_frames %d, %s\n", old, current_speed_pct, skip_tick_each_frames, keyframei.toString().c_str());
    }
}

void pacman_t::set_dir(direction_t dir) {
    const bool collision_maze = !pos.test(dir, keyframei, [](tile_t tile) -> bool {
        return tile_t::WALL == tile || tile_t::GATE == tile;
    });
    if( !collision_maze ) {
        if( !auto_move ) {
            ++steps_left;
        }
        dir_ = dir;
        perf_fields_walked_t0 = getCurrentMilliseconds();
        pos.reset_stats();
    }
}

bool pacman_t::tick() {
    // skip_tick_counter = skip_tick_each_frames;
    if( 0 < skip_tick_each_frames ) {
        if( 0 >= --skip_tick_counter ) {
            skip_tick_counter = skip_tick_each_frames; // reload
            return true; // skip
        }
    }

    atex = &get_tex();
    atex->tick();

    bool collision_maze = false;
    bool collision_enemies = false;

    if( 0 < mode_ms_left ) {
        mode_ms_left = std::max( 0, mode_ms_left - get_ms_per_frame() );
    }

    if( mode_t::HOME == mode ) {
        set_mode( mode_t::NORMAL );
    } else if( mode_t::DEAD == mode ) {
        if( 0 == mode_ms_left ) {
            set_mode( mode_t::HOME );
        }
    } else {
        // NORMAL and POWERED

        if( mode_t::POWERED == mode ) {
            if( 0 == mode_ms_left ) {
                set_mode( mode_t::NORMAL );
            }
        }

        /**
         * Pacman's position depends on:
         * - its direction
         * - speed (change_per_tick)
         * - environment
         */
         if( auto_move || 0 < steps_left ) {
             if( !auto_move ) {
                 --steps_left;
             }
             collision_maze = !pos.step(dir_, keyframei, [](tile_t tile) -> bool {
                 return tile_t::WALL == tile || tile_t::GATE == tile ;
             });
             if( collision_maze ) {
                 audio_samples[ ::number( audio_clip_t::MUNCH ) ]->stop();
                 uint64_t t1 = getCurrentMilliseconds();
                 if( pos.get_fields_walked_i() > 0 ) {
                     const float fps = get_fps(perf_fields_walked_t0, t1, pos.get_fields_walked_f());
                     log_printf("pacman: fields %.2f/s, td %" PRIu64 " ms, %s, %s\n", fps, t1-perf_fields_walked_t0, pos.toString().c_str(), keyframei.toString().c_str());
                 }
                 pos.reset_stats();
                 perf_fields_walked_t0 = getCurrentMilliseconds();
             } else {
                 const int x_i = pos.get_x_i();
                 const int y_i = pos.get_y_i();
                 const tile_t tile = global_maze->get_tile(x_i, y_i);
                 // log_printf("tick: %s, %s c%d e%d, tile %s\n", to_string(dir_).c_str(), pos.toString().c_str(), pos.is_center_dir(keyframei), pos.entered_tile(keyframei), to_string(tile).c_str());
                 if( tile_t::PELLET <= tile && tile <= tile_t::KEY ) {
                     score += ::number( tile_to_score(tile) );
                     global_maze->set_tile(x_i, y_i, tile_t::EMPTY);
                     if( tile_t::PELLET == tile ) {
                         audio_samples[ ::number( audio_clip_t::MUNCH ) ]->play(0);
                         set_speed(0.71f);
                         next_field_frame_cntr = keyframei.get_frames_per_field();
                     } else if( tile_t::PELLET_POWER == tile ) {
                         set_mode( mode_t::POWERED );
                         audio_samples[ ::number( audio_clip_t::MUNCH ) ]->play(0);
                         set_speed(0.90f);
                         next_field_frame_cntr = keyframei.get_frames_per_field();
                     }
                 } else if( tile_t::EMPTY == tile ) {
                     if( 0 < next_field_frame_cntr ) {
                         // log_printf("next_field_frame_cntr: %d\n", next_field_frame_cntr);
                         if( 0 == --next_field_frame_cntr ) {
                             set_speed(0.80f);
                             audio_samples[ ::number( audio_clip_t::MUNCH ) ]->stop();
                         }
                     }
                 }
             }
         }
         // Collision test with ghosts
         for(ghost_ref g : ghosts) {
             if( pos.intersects(g->get_pos()) ) {
                 const ghost_t::mode_t g_mode = g->get_mode();
                 if( ghost_t::mode_t::CHASE <= g_mode && g_mode <= ghost_t::mode_t::SCATTER ) {
                     collision_enemies = true;
                 } else if( ghost_t::mode_t::SCARED == g_mode ) {
                     score += ::number( score_t::GHOST_1 ); // FIXME
                     g->set_mode( ghost_t::mode_t::PHANTOM );
                     audio_samples[ ::number( audio_clip_t::EAT_GHOST ) ]->play();
                 }
             }
         }

         if( collision_enemies ) {
             set_mode( mode_t::DEAD );
         }
    }

    if( DEBUG_GFX_BOUNDS ) {
        log_printf("tick: %s, %s, crash[maze %d, ghosts %d], textures %s\n",
                to_string(dir_).c_str(), pos.toString().c_str(), collision_maze, collision_enemies, atex->toString().c_str());
    }
    return !collision_enemies;
}

void pacman_t::draw(SDL_Renderer* rend) {
    atex->draw(rend, pos.get_x_f()-keyframei.get_center(), pos.get_y_f()-keyframei.get_center());

    if( show_all_debug_gfx() || DEBUG_GFX_BOUNDS ) {
        uint8_t r, g, b, a;
        SDL_GetRenderDrawColor(rend, &r, &g, &b, &a);
        SDL_SetRenderDrawColor(rend, 255, 255, 0, 255);
        const int win_pixel_offset = ( win_pixel_width - global_maze->get_pixel_width()*win_pixel_scale ) / 2;
        // pos is on player center position
        SDL_Rect bounds = { .x=win_pixel_offset + round_to_int( pos.get_x_f() * global_maze->get_ppt_y() * win_pixel_scale ) - ( atex->get_width()  * win_pixel_scale ) / 2,
                            .y=                   round_to_int( pos.get_y_f() * global_maze->get_ppt_y() * win_pixel_scale ) - ( atex->get_height() * win_pixel_scale ) / 2,
                            .w=atex->get_width()*win_pixel_scale, .h=atex->get_height()*win_pixel_scale };
        SDL_RenderDrawRect(rend, &bounds);
        SDL_SetRenderDrawColor(rend, r, g, b, a);
    }
}

std::string pacman_t::toString() const {
    return "pacman["+to_string(mode)+"["+std::to_string(mode_ms_left)+" ms], "+to_string(dir_)+", "+pos.toString()+", "+atex->toString()+", "+keyframei.toString()+"]";
}


std::string to_string(pacman_t::mode_t m) {
    switch( m ) {
        case pacman_t::mode_t::HOME:
            return "home";
        case pacman_t::mode_t::NORMAL:
            return "normal";
        case pacman_t::mode_t::POWERED:
            return "powered";
        case pacman_t::mode_t::DEAD:
            return "dead";
        default:
            return "unknown";
    }
}


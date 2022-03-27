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

animtex_t& pacman_t::get_tex() noexcept {
    switch( mode ) {
        case pacman_t::mode_t::HOME:
            return atex_home;
        case pacman_t::mode_t::NORMAL:
            [[fallthrough]];
        case pacman_t::mode_t::POWERED:
            switch( current_dir ) {
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

pacman_t::pacman_t(SDL_Renderer* rend, const float fields_per_sec_total_) noexcept
: fields_per_sec_total(fields_per_sec_total_),
  current_speed_pct(0.80f),
  keyframei_(get_frames_per_sec(), fields_per_sec_total*current_speed_pct, true /* nearest */),
  sync_next_frame_cntr( keyframei_.sync_frame_count(), true /* auto_reload */),
  // next_field_frame_cntr(0, false /* auto_reload */),
  mode( mode_t::HOME ),
  mode_ms_left ( -1 ),
  lives( 3 ),
  current_dir( direction_t::LEFT ),
  score_( 0 ),
  atex_left( "L", rend, ms_per_tex, global_tex->all_images(), 0, 28, 13, 13, { { 0*13, 0 }, { 1*13, 0 } }),
  atex_right("R", rend, ms_per_tex, global_tex->all_images(), 0, 28, 13, 13, { { 2*13, 0 }, { 3*13, 0 } }),
  atex_up(   "U", rend, ms_per_tex, global_tex->all_images(), 0, 28, 13, 13, { { 4*13, 0 }, { 5*13, 0 } }),
  atex_down( "D", rend, ms_per_tex, global_tex->all_images(), 0, 28, 13, 13, { { 6*13, 0 }, { 7*13, 0 } }),
  atex_dead( "X", rend, ms_per_tex, global_tex->all_images(), 0, 14, 14, 14, {
          { 0*14, 0 }, { 1*14, 0 }, { 2*14, 0 }, { 3*14, 0 }, { 4*14, 0 }, { 5*14, 0 },
          { 6*14, 0 }, { 7*14, 0 }, { 8*14, 0 }, { 9*14, 0 }, { 10*14, 0 }, { 11*14, 0 } }),
  atex_home( "H", ms_per_tex, { atex_dead.texture(0) }),
  atex( &get_tex() ),
  pos_( global_maze->pacman_start_pos() )
{
    set_mode( mode_t::HOME );
}

void pacman_t::destroy() noexcept {
    atex_left.destroy();
    atex_right.destroy();
    atex_up.destroy();
    atex_down.destroy();
    atex_dead.destroy();
    atex_home.destroy();
}

void pacman_t::set_mode(const mode_t m) noexcept {
    const mode_t old_mode = mode;
    switch( m ) {
        case mode_t::HOME:
            audio_samples[ ::number( audio_clip_t::MUNCH ) ]->stop();
            mode = m;
            mode_ms_left = -1;
            pos_ = global_maze->pacman_start_pos();
            pos_.set_aligned_dir( direction_t::LEFT, keyframei_ );
            set_dir( direction_t::LEFT );
            reset_stats(); // always, even if speed is unchanged
            atex = &get_tex();
            for(ghost_ref g : ghosts) {
                g->set_mode(ghost_t::mode_t::HOME);
            }
            set_speed(0.80f);
            break;
        case mode_t::NORMAL:
            mode = m;
            mode_ms_left = -1;
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
    log_printf("pacman set_mode: %s -> %s [%d ms], %s\n", to_string(old_mode).c_str(), to_string(mode).c_str(), mode_ms_left, pos_.toShortString().c_str());
}

void pacman_t::set_speed(const float pct) noexcept {
    if( std::abs( current_speed_pct - pct ) <= std::numeric_limits<float>::epsilon() ) {
        return;
    }
    const float old = current_speed_pct;
    current_speed_pct = pct;
    keyframei_.reset(get_frames_per_sec(), fields_per_sec_total*pct, true /* nearest */);
    pos_.set_aligned_dir(keyframei_);
    reset_stats();
    if( log_moves ) {
        log_printf("pacman set_speed: %5.2f -> %5.2f: sync_each_frames %s, %s\n", old, current_speed_pct, sync_next_frame_cntr.toString().c_str(), keyframei_.toString().c_str());
    }
}

void pacman_t::print_stats() noexcept {
    const acoord_t::stats_t &stats = pos_.get_stats();
    if( stats.fields_walked_i > 0 && perf_frame_count_walked > 0) {
        const uint64_t t1 = getCurrentMilliseconds();
        const float fields_per_seconds = get_fps(perf_fields_walked_t0, t1, stats.fields_walked_f);
        const float fps_draw = get_fps(perf_fields_walked_t0, t1, perf_frame_count_walked);
        const float fps_tick = get_fps(perf_fields_walked_t0, t1, perf_frame_count_walked - sync_next_frame_cntr.events());
        log_printf("pacman stats: fields[%.2f walked, %.2f/s], fps[draw %.2f/s, tick %.2f/s], frames[draw %" PRIu64 ", synced %d], td %" PRIu64 " ms, req_speed[%f\%, fields %f/s], %s, %s\n",
                stats.fields_walked_f, fields_per_seconds,
                fps_draw, fps_tick, perf_frame_count_walked, sync_next_frame_cntr.events(),
                t1-perf_fields_walked_t0,
                current_speed_pct, fields_per_sec_total*current_speed_pct,
                keyframei_.toString().c_str(), pos_.toString().c_str());
    }
}

void pacman_t::reset_stats() noexcept {
    print_stats();
    perf_fields_walked_t0 = getCurrentMilliseconds();
    perf_frame_count_walked = 0;
    pos_.reset_stats();
    sync_next_frame_cntr.reset( keyframei_.sync_frame_count(), true /* auto_reload */);
}

bool pacman_t::set_dir(const direction_t new_dir) noexcept {
    if( current_dir == new_dir ) {
        return true;
    }
    const bool collision_maze = !pos_.test(new_dir, keyframei_, [](tile_t tile) -> bool {
        return tile_t::WALL == tile || tile_t::GATE == tile;
    });
    // log_printf("pacman set_dir: %s -> %s, collision %d, %s\n", to_string(current_dir).c_str(), to_string(new_dir).c_str(), collision_maze, pos.toString().c_str());
    if( !collision_maze ) {
        const direction_t old_dir = current_dir;
        current_dir = new_dir;
        reset_stats();
        if( log_moves ) {
            log_printf("pacman set_dir: %s -> %s, %s c%d e%d\n",
                    to_string(old_dir).c_str(), to_string(current_dir).c_str(), pos_.toString().c_str(), pos_.is_center(keyframei_), pos_.entered_tile(keyframei_));
        }
        return true;
    } else {
        return false;
    }
}

bool pacman_t::tick() noexcept {
    if( sync_next_frame_cntr.count_down() ) {
        return true; // skip tick, just repaint
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
         {
             collision_maze = !pos_.step(current_dir, keyframei_, [](tile_t tile) -> bool {
                 return tile_t::WALL == tile || tile_t::GATE == tile ;
             });
             const int x_i = pos_.x_i();
             const int y_i = pos_.y_i();
             const tile_t tile = global_maze->tile(x_i, y_i);
             const bool entered_tile = pos_.entered_tile(keyframei_);
             if( DEBUG_GFX_BOUNDS ) {
                 log_printf("pacman tick: %s, %s c%d e%d '%s', crash[maze %d, ghosts %d], textures %s\n",
                         to_string(current_dir).c_str(), pos_.toString().c_str(), pos_.is_center(keyframei_), entered_tile,
                         to_string(tile).c_str(),
                         collision_maze, collision_enemies, atex->toString().c_str());
             }
             if( collision_maze ) {
                 audio_samples[ ::number( audio_clip_t::MUNCH ) ]->stop();
                 reset_stats();
             } else if( entered_tile ) {
                 if( tile_t::PELLET <= tile && tile <= tile_t::KEY ) {
                     score_ += ::number( tile_to_score(tile) );
                     global_maze->set_tile(x_i, y_i, tile_t::EMPTY);
                     if( tile_t::PELLET == tile ) {
                         audio_samples[ ::number( audio_clip_t::MUNCH ) ]->play(0);
                         set_speed(0.71f);
                     } else if( tile_t::PELLET_POWER == tile ) {
                         set_mode( mode_t::POWERED );
                         audio_samples[ ::number( audio_clip_t::MUNCH ) ]->play(0);
                         set_speed(0.90f);
                     }
                 } else if( tile_t::EMPTY == tile ) {
                     set_speed(0.80f);
                     audio_samples[ ::number( audio_clip_t::MUNCH ) ]->stop();
                 }
             }
         }
         // Collision test with ghosts
         for(ghost_ref g : ghosts) {
             if( pos_.intersects(g->position()) ) {
                 const ghost_t::mode_t g_mode = g->mode();
                 if( ghost_t::mode_t::CHASE <= g_mode && g_mode <= ghost_t::mode_t::SCATTER ) {
                     collision_enemies = true;
                 } else if( ghost_t::mode_t::SCARED == g_mode ) {
                     score_ += ::number( score_t::GHOST_1 ); // FIXME
                     g->set_mode( ghost_t::mode_t::PHANTOM );
                     audio_samples[ ::number( audio_clip_t::EAT_GHOST ) ]->play();
                 }
             }
         }

         if( collision_enemies ) {
             set_mode( mode_t::DEAD );
         }
    }
    return !collision_enemies;
}

void pacman_t::draw(SDL_Renderer* rend) noexcept {
    ++perf_frame_count_walked;
    atex->draw(rend, pos_.x_f()-keyframei_.center(), pos_.y_f()-keyframei_.center());

    if( show_all_debug_gfx() || DEBUG_GFX_BOUNDS ) {
        uint8_t r, g, b, a;
        SDL_GetRenderDrawColor(rend, &r, &g, &b, &a);
        SDL_SetRenderDrawColor(rend, 255, 255, 0, 255);
        const int win_pixel_offset = ( win_pixel_width - global_maze->pixel_width()*win_pixel_scale ) / 2;
        // pos is on player center position
        SDL_Rect bounds = { .x=win_pixel_offset + round_to_int( pos_.x_f() * global_maze->ppt_y() * win_pixel_scale ) - ( atex->width()  * win_pixel_scale ) / 2,
                            .y=                   round_to_int( pos_.y_f() * global_maze->ppt_y() * win_pixel_scale ) - ( atex->height() * win_pixel_scale ) / 2,
                            .w=atex->width()*win_pixel_scale, .h=atex->height()*win_pixel_scale };
        SDL_RenderDrawRect(rend, &bounds);
        SDL_SetRenderDrawColor(rend, r, g, b, a);
    }
}

std::string pacman_t::toString() const noexcept {
    return "pacman["+to_string(mode)+"["+std::to_string(mode_ms_left)+" ms], "+to_string(current_dir)+", "+pos_.toString()+", "+atex->toString()+", "+keyframei_.toString()+"]";
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


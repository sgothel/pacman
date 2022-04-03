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

std::vector<int> pacman_t::rgb_color = { 0xff, 0xff, 0x00 };

random_engine_t<random_engine_mode_t::STD_RNG> pacman_t::rng_hw;

animtex_t& pacman_t::get_tex() noexcept {
    switch( mode_ ) {
        case mode_t::FREEZE:
            return *atex;
        case mode_t::LEVEL_SETUP:
            return atex_home;
        case mode_t::START:
            return atex_home;
        case mode_t::NORMAL:
            [[fallthrough]];
        case mode_t::POWERED:
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
        case mode_t::DEAD:
            return atex_dead;
        default:
            return *atex;
    }
}

pacman_t::pacman_t(SDL_Renderer* rend, const float fields_per_sec_total_) noexcept
: fields_per_sec_total(fields_per_sec_total_),
  current_speed_pct(0.0f),
  keyframei_(get_frames_per_sec(), fields_per_sec_total*current_speed_pct, true /* nearest */),
  sync_next_frame_cntr( keyframei_.sync_frame_count(), true /* auto_reload */),
  next_empty_field_frame_cntr(0, false /* auto_reload */),
  invincible( false ),
  mode_( mode_t::FREEZE ),
  mode_last( mode_t::NORMAL ),
  mode_ms_left ( -1 ),
  mode_last_ms_left( -1 ),
  fruit_ms_left( -1 ),
  lives( 3 ),
  ghosts_eaten_powered( 0 ),
  current_dir( direction_t::LEFT ),
  score_( 0 ),
  freeze_score( -1 ),
  freeze_frame_count(0),
  atex_left( "L", rend, ms_per_tex, global_tex->all_images(), 0, 28, 13, 13, { { 0*13, 0 }, { 1*13, 0 } }),
  atex_right("R", rend, ms_per_tex, global_tex->all_images(), 0, 28, 13, 13, { { 2*13, 0 }, { 3*13, 0 } }),
  atex_up(   "U", rend, ms_per_tex, global_tex->all_images(), 0, 28, 13, 13, { { 4*13, 0 }, { 5*13, 0 } }),
  atex_down( "D", rend, ms_per_tex, global_tex->all_images(), 0, 28, 13, 13, { { 6*13, 0 }, { 7*13, 0 } }),
  atex_dead( "X", rend, ms_per_tex, global_tex->all_images(), 0, 14, 14, 14, {
          { 0*14, 0 }, { 1*14, 0 }, { 2*14, 0 }, { 3*14, 0 }, { 4*14, 0 }, { 5*14, 0 },
          { 6*14, 0 }, { 7*14, 0 }, { 8*14, 0 }, { 9*14, 0 }, { 10*14, 0 }, { 11*14, 0 } }),
  atex_home( "H", ms_per_tex, { atex_dead.texture(0) }),
  atex( &atex_home ),
  pos_( global_maze->pacman_start_pos() )
{
    set_mode( mode_t::FREEZE );
}

void pacman_t::destroy() noexcept {
    atex_left.destroy();
    atex_right.destroy();
    atex_up.destroy();
    atex_down.destroy();
    atex_dead.destroy();
    atex_home.destroy();
}

void pacman_t::set_mode(const mode_t m, const int mode_ms) noexcept {
    if( m              != mode_ && // only earmark last other mode, avoid B -> [curr = B] [last = A] to become [curr = B] [last = B]
        mode_t::FREEZE != mode_ )  // and avoid FREEZE to be earmarked at all
    {
        mode_last = mode_;
        mode_last_ms_left = mode_ms_left;
    }
    const mode_t old_mode = mode_;
    const int old_mode_ms_left = mode_ms_left;
    mode_ = m;
    mode_ms_left = mode_ms;
    switch( m ) {
        case mode_t::FREEZE:
            stop_audio_loops();
            break;
        case mode_t::LEVEL_SETUP: {
            stop_audio_loops();
            atex = &get_tex();
            ghost_t::set_global_mode(ghost_t::mode_t::LEVEL_SETUP);
            pos_ = global_maze->pacman_start_pos();
            pos_.set_aligned_dir( direction_t::LEFT, keyframei_ );
            set_dir( direction_t::LEFT );
            const acoord_t& f_p = global_maze->fruit_pos();
            global_maze->set_tile(f_p.x_i(), f_p.y_i(), tile_t::EMPTY);
            fruit_ms_left = 0;
            freeze_frame_count = 0;
            break;
        }
        case mode_t::START:
            stop_audio_loops();
            ghost_t::set_global_mode(ghost_t::mode_t::START);
            pos_ = global_maze->pacman_start_pos();
            pos_.set_aligned_dir( direction_t::LEFT, keyframei_ );
            set_dir( direction_t::LEFT );
            set_speed(game_level_spec().pacman_speed);
            break;
        case mode_t::NORMAL:
            set_speed(game_level_spec().pacman_speed);
            break;
        case mode_t::POWERED:
            ghost_t::set_global_mode( ghost_t::mode_t::SCARED, mode_ms_left );
            set_speed(game_level_spec().pacman_powered_speed);
            break;
        case mode_t::DEAD:
            stop_audio_loops();
            atex_dead.reset();
            ghost_t::set_global_mode(ghost_t::mode_t::PACMAN_DIED);
            audio_samples[ ::number( audio_clip_t::DEATH ) ]->play();
            break;
        default:
            break;
    }
    if( log_modes() ) {
        log_printf("pacman set_mode: %s* / %s -> %s [%d* / %d -> %d ms], speed %5.2f, pos %s\n",
                to_string(mode_last).c_str(), to_string(old_mode).c_str(), to_string(mode_).c_str(),
                mode_last_ms_left, old_mode_ms_left, mode_ms_left,
                current_speed_pct, pos_.toShortString().c_str());
    }
}

void pacman_t::stop_audio_loops() noexcept {
    audio_samples[ ::number( audio_clip_t::MUNCH ) ]->stop();
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
    if( log_modes() ) {
        log_printf("pacman set_speed: %5.2f -> %5.2f: sync_each_frames %zd, %s\n", old, current_speed_pct, sync_next_frame_cntr.counter(), keyframei_.toString().c_str());
    }
}

void pacman_t::print_stats() noexcept {
    const acoord_t::stats_t &stats = pos_.get_stats();
    if( perf_frame_count_walked >= (uint64_t) keyframei_.frames_per_second() ) {
        const uint64_t t1 = getCurrentMilliseconds();
        const uint64_t td = t1-perf_fields_walked_t0;
        const float fields_per_seconds = get_fps(perf_fields_walked_t0, t1, stats.fields_walked_f);
        const float fields_per_seconds_req = keyframei_.fields_per_second_requested();
        const float fields_per_seconds_diff_pct = ( std::abs(fields_per_seconds_req - fields_per_seconds ) / fields_per_seconds_req ) * 100.0f;
        const float fps_draw = get_fps(perf_fields_walked_t0, t1, perf_frame_count_walked);
        const float fps_tick = get_fps(perf_fields_walked_t0, t1, perf_frame_count_walked - sync_next_frame_cntr.events());
        log_printf("pacman stats: speed %.2f%, td %" PRIu64 "ms, fields[%.2f walked, actual %.3f/s, requested %.3f/s, diff %.4f%], fps[draw %.2f/s, tick %.2f/s], frames[draw %" PRIu64 ", synced %d], %s, %s\n",
                current_speed_pct, td,
                stats.fields_walked_f, fields_per_seconds, fields_per_seconds_req, fields_per_seconds_diff_pct,
                fps_draw, fps_tick, perf_frame_count_walked, sync_next_frame_cntr.events(),
                keyframei_.toString().c_str(), pos_.toString().c_str());
    }
}

void pacman_t::reset_stats() noexcept {
    if( log_moves() || log_fps() ) {
        print_stats();
    }
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
    if( false ) {
        log_printf("pacman set_dir: %s -> %s, collision %d, %s\n", to_string(current_dir).c_str(), to_string(new_dir).c_str(), collision_maze, pos_.toString().c_str());
    }
    if( !collision_maze ) {
        const direction_t old_dir = current_dir;
        current_dir = new_dir;
        reset_stats();
        if( log_moves() ) {
            log_printf("pacman set_dir: %s -> %s, %s c%d e%d\n",
                    to_string(old_dir).c_str(), to_string(current_dir).c_str(), pos_.toString().c_str(), pos_.is_center(keyframei_), pos_.entered_tile(keyframei_));
        }
        return true;
    } else {
        return false;
    }
}

bool pacman_t::tick() noexcept {
    atex = &get_tex();
    atex->tick();

    if( sync_next_frame_cntr.count_down() ) {
        return true; // skip tick, just repaint
    }
    if( 0 < freeze_frame_count ) {
        --freeze_frame_count;
        return true;
    }

    bool collision_maze = false;
    bool collision_enemies = false;

    if( 0 < mode_ms_left ) {
        mode_ms_left = std::max( 0, mode_ms_left - get_ms_per_frame() );
    }

    switch( mode_ ) {
        case mode_t::FREEZE:
            if( 0 >= mode_ms_left ) {
                freeze_score = -1;
                freeze_box_.set(-1, -1, -1, -1);
                set_mode( mode_last, mode_last_ms_left );
                break;
            } else {
                return true;
            }
        case mode_t::LEVEL_SETUP:
            return true;
        case mode_t::START:
            set_mode( mode_t::NORMAL );
            break;
        case mode_t::NORMAL:
            break;
        case mode_t::POWERED:
            if( 0 >= mode_ms_left ) {
                set_mode( mode_t::NORMAL );
            }
            break;
        case mode_t::DEAD:
            if( 0 >= mode_ms_left ) {
                return false; // main look shall handle restart: set_mode( mode_t::START );
            }
            return true;
        default:
            break;
    }
    // NORMAL and POWERED

    if( 0 < fruit_ms_left ) {
        fruit_ms_left = std::max( 0, fruit_ms_left - get_ms_per_frame() );
        if( 0 == fruit_ms_left ) {
            const acoord_t& f_p = global_maze->fruit_pos();
            global_maze->set_tile(f_p.x_i(), f_p.y_i(), tile_t::EMPTY);
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
        const bool is_center = pos_.is_center(keyframei_);
        if( log_moves() || DEBUG_GFX_BOUNDS ) {
            log_printf("pacman tick: %s, %s c%d e%d '%s', coll[maze %d, ghosts %d], textures %s\n",
                    to_string(current_dir).c_str(), pos_.toString().c_str(), is_center, entered_tile,
                    to_string(tile).c_str(),
                    collision_maze, collision_enemies, atex->toString().c_str());
        }
        if( collision_maze ) {
            audio_samples[ ::number( audio_clip_t::MUNCH ) ]->stop();
            reset_stats();
        } else { // if( entered_tile ) {
            if( tile_t::PELLET <= tile && tile <= tile_t::KEY ) {
                if( tile_t::PELLET == tile ) {
                    global_maze->set_tile(x_i, y_i, tile_t::EMPTY);
                    score_ += ::number( tile_to_score(tile) );
                    audio_samples[ ::number( audio_clip_t::MUNCH ) ]->play(0);
                    if( mode_t::POWERED == mode_ ) {
                        set_speed(game_level_spec().pacman_powered_speed_dots);
                    } else {
                        set_speed(game_level_spec().pacman_speed_dots);
                    }
                    next_empty_field_frame_cntr.load( keyframei_.frames_per_field() + 1 );
                    ghost_t::notify_pellet_eaten();

                    if( fruit_1_eaten == global_maze->taken(tile_t::PELLET) ||
                        fruit_2_eaten == global_maze->taken(tile_t::PELLET) )
                    {
                        const acoord_t& f_p = global_maze->fruit_pos();
                        const tile_t fruit = game_level_spec().symbol;
                        global_maze->set_tile(f_p.x_i(), f_p.y_i(), fruit);
                        fruit_ms_left = fruit_duration_min + ( rng_hw() % ( fruit_duration_max - fruit_duration_min + 1 ) );
                        if( log_modes() ) {
                            log_printf("fruit appears: tile %s, dur %dms\n", to_string( fruit ).c_str(), fruit_ms_left);
                        }
                    }
                } else if( tile_t::PELLET_POWER == tile ) {
                    global_maze->set_tile(x_i, y_i, tile_t::EMPTY);
                    score_ += ::number( tile_to_score(tile) );
                    set_mode( mode_t::POWERED, game_level_spec().fright_time_ms );
                    audio_samples[ ::number( audio_clip_t::MUNCH ) ]->play(0);
                    next_empty_field_frame_cntr.load( keyframei_.frames_per_field() + 1 );
                    ghosts_eaten_powered = 0;
                    freeze_frame_count = 3;
                }
            } else if( tile_t::EMPTY == tile ) {
                if( next_empty_field_frame_cntr.count_down() ) {
                    if( mode_t::POWERED == mode_ ) {
                        set_speed(game_level_spec().pacman_powered_speed);
                    } else {
                        set_speed(game_level_spec().pacman_speed);
                    }
                    audio_samples[ ::number( audio_clip_t::MUNCH ) ]->stop();
                }
            }
            if( is_center ) {
                // bonus fruit test on float position, since it crosses two tiles
                const acoord_t& f_p = global_maze->fruit_pos();
                const tile_t fruit_tile = global_maze->tile(f_p.x_i(), f_p.y_i());
                if( tile_t::CHERRY <= fruit_tile && fruit_tile <= tile_t::KEY && pos_.intersects_f(f_p) ) {
                    global_maze->set_tile(f_p.x_i(), f_p.y_i(), tile_t::EMPTY);
                    freeze_score = game_level_spec().bonus_points;
                    score_ += freeze_score;
                    freeze_box_.set(f_p.x_i()-1, f_p.y_i()-1, 2, 2);
                    set_mode(mode_t::FREEZE, number( mode_duration_t::FREEZE ));
                    audio_samples[ ::number( audio_clip_t::EAT_FRUIT) ]->play();
                    if( log_modes() ) {
                        log_printf("pacman eats: a fruit: score %d, tile [pos %s, fpos %s], left %dms, pos[self %s, fruit %s]\n",
                                freeze_score, to_string(tile).c_str(), to_string(fruit_tile).c_str(), fruit_ms_left,
                                pos_.toShortString().c_str(), f_p.toShortString().c_str());
                    }
                }
            }
        }
    }
    // Collision test with ghosts
    int i=0;
    for(ghost_ref g : ghosts()) {
        if( pos_.intersects_f(g->position()) ) {
            const ghost_t::mode_t g_mode = g->mode();
            if( ghost_t::mode_t::CHASE <= g_mode && g_mode <= ghost_t::mode_t::SCATTER ) {
                if( !invincible ) {
                    collision_enemies = true;
                }
            } else if( ghost_t::mode_t::SCARED == g_mode ) {
                switch( ghosts_eaten_powered ) {
                    case 0: freeze_score = ::number( score_t::GHOST_1 ); break;
                    case 1: freeze_score = ::number( score_t::GHOST_2 ); break;
                    case 2: freeze_score = ::number( score_t::GHOST_3 ); break;
                    default: freeze_score = ::number( score_t::GHOST_4 ); break;
                }
                score_ += freeze_score;
                ++ghosts_eaten_powered;
                g->set_mode( ghost_t::mode_t::PHANTOM );
                audio_samples[ ::number( audio_clip_t::MUNCH ) ]->stop();
                audio_samples[ ::number( audio_clip_t::EAT_GHOST ) ]->play();
                freeze_box_.set(pos_.x_i()-1, pos_.y_i()-1, 2, 2);
                set_mode(mode_t::FREEZE, number( mode_duration_t::FREEZE ));
                if( log_modes() ) {
                    log_printf("pacman eats: ghost# %d, score %d, ghost %s\n", ghosts_eaten_powered, freeze_score, g->toString().c_str());
                }
            }
        }
        ++i;
    }

    if( collision_enemies ) {
        set_mode( mode_t::DEAD, number( mode_duration_t::DEADANIM ) );
    }
    return true; // return false after DEAD animation, not yet: !collision_enemies;
}

void pacman_t::draw(SDL_Renderer* rend) noexcept {
    ++perf_frame_count_walked;

    if( mode_t::FREEZE == mode_ ) {
        if( 0 <= freeze_score ) {
            draw_text_scaled(rend, font_ttf(), std::to_string( freeze_score ), 255, 255, 255, true /* cache */, [&](const texture_t& tex, int &x, int&y) {
                x = round_to_int( pos_.x_f() * global_maze->ppt_y() * win_pixel_scale() ) - tex.width()  / 2;
                y = round_to_int( pos_.y_f() * global_maze->ppt_y() * win_pixel_scale() ) - tex.height() / 2;
            });
        }
    } else {
        atex->draw2(rend, pos_.x_f()-keyframei_.center(), pos_.y_f()-keyframei_.center());
    }

    if( show_debug_gfx() || DEBUG_GFX_BOUNDS ) {
        uint8_t r, g, b, a;
        SDL_GetRenderDrawColor(rend, &r, &g, &b, &a);
        SDL_SetRenderDrawColor(rend, rgb_color[0], rgb_color[1], rgb_color[2], 255);
        const int win_pixel_offset = ( win_pixel_width() - global_maze->pixel_width()*win_pixel_scale() ) / 2;
        // pos is on player center position
        SDL_Rect bounds = { .x=win_pixel_offset + round_to_int( pos_.x_f() * global_maze->ppt_y() * win_pixel_scale() ) - ( atex->width()  * win_pixel_scale() ) / 2,
                            .y=                   round_to_int( pos_.y_f() * global_maze->ppt_y() * win_pixel_scale() ) - ( atex->height() * win_pixel_scale() ) / 2,
                            .w=atex->width()*win_pixel_scale(), .h=atex->height()*win_pixel_scale() };
        SDL_RenderDrawRect(rend, &bounds);
        SDL_SetRenderDrawColor(rend, r, g, b, a);
    }
}

std::string pacman_t::toString() const noexcept {
    return "pacman["+to_string(mode_)+"["+std::to_string(mode_ms_left)+" ms], "+to_string(current_dir)+", "+pos_.toString()+", "+atex->toString()+", "+keyframei_.toString()+"]";
}


std::string to_string(pacman_t::mode_t m) {
    switch( m ) {
        case pacman_t::mode_t::FREEZE:
            return "freeze";
        case pacman_t::mode_t::LEVEL_SETUP:
            return "level_setup";
        case pacman_t::mode_t::START:
            return "start";
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


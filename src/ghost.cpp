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
#include <pacman/game.hpp>
#include <pacman/globals.hpp>

#include <limits>

#include <cstdio>
#include <time.h>

static constexpr const bool DEBUG_GFX_BOUNDS = false;

//
// ghost_t
//

int ghost_t::id_to_yoff(ghost_t::personality_t id) {
    switch( id ) {
        case ghost_t::personality_t::BLINKY:
            return 41 + 0*14;
        case ghost_t::personality_t::CLYDE:
            return 41 + 1*14;
        case ghost_t::personality_t::INKY:
            return 41 + 2*14;
        case ghost_t::personality_t::PINKY:
            return 41 + 3*14;
        default:
            return 0;
    }
}

animtex_t& ghost_t::get_tex() {
    switch( mode ) {
        case mode_t::SCARED:
            return atex_scared;

        case mode_t::PHANTOM:
            return atex_phantom;

        default:
            return atex_normal;
    }
}

ghost_t::ghost_t(const personality_t id_, SDL_Renderer* rend, const float fields_per_sec_total_)
: fields_per_sec_total(fields_per_sec_total_),
  current_speed_pct(0.75f),
  keyframei(true /* odd */, get_frames_per_sec(), fields_per_sec_total*current_speed_pct, false /* hint_slower */),
  skip_tick_each_frames( -1 ),
  skip_tick_counter( -1 ),
  id( id_ ),
  mode( mode_t::HOME ),
  mode_ms_left ( number( mode_duration_t::HOMESTAY ) ),
  dir_( direction_t::LEFT ),
  atex_normal( "N", rend, ms_per_atex, global_tex->get_all_images(), 0, id_to_yoff(id), 14, 14, { { 0*14, 0 }, { 1*14, 0 }, { 2*14, 0 }, { 3*14, 0 } }),
  atex_scared( "S", rend, ms_per_atex, global_tex->get_all_images(), 0, 0, 14, 14, { { 10*14, 0 } }),
  atex_phantom( "P", rend, ms_per_atex, global_tex->get_all_images(), 0, 41 + 4*14, 14, 14, { { 0*14, 0 }, { 1*14, 0 }, { 2*14, 0 }, { 3*14, 0 } }),
  atex( &get_tex() ),
  pos( global_maze->get_ghost_home_pos() ),
  target( global_maze->get_ghost_home_pos() )
{
    set_mode( mode_t::HOME );
}


void ghost_t::destroy() {
    atex_normal.destroy();
    atex_scared.destroy();
    atex_phantom.destroy();
}

void ghost_t::set_next_target() {
    switch( mode ) {
        case mode_t::HOME:
            if( ghost_t::personality_t::BLINKY == id ) {
                target = pacman->get_pos();
                break;
            } else {
                target = global_maze->get_ghost_home_pos();
                target.set_centered(keyframei);
                break;
            }

        case mode_t::LEAVE_HOME:
            target = global_maze->get_ghost_start_pos();
            target.set_centered(keyframei);
            break;

        case mode_t::CHASE:
            switch( id ) {
                case ghost_t::personality_t::BLINKY:
                    target = pacman->get_pos();
                    break;
                case ghost_t::personality_t::CLYDE:
                    target = global_maze->get_top_left_corner();
                    target.set_centered(keyframei);
                    break;
                case ghost_t::personality_t::INKY: {
                    /**
                     * Selecting the position two tiles in front of Pac-Man in his current direction of travel.
                     * From there, imagine drawing a vector from Blinky's position to this tile,
                     * and then doubling the length of the vector.
                     * The tile that this new, extended vector ends on will be Inky's actual target.
                     */
                    acoord_t p = pacman->get_pos();
                    acoord_t b = ghosts[ number( ghost_t::personality_t::BLINKY ) ]->get_pos();
                    p.incr_fwd(keyframei, 2);
                    float p_[] = { p.get_x_f(), p.get_y_f() };
                    float b_[] = { b.get_x_f(), b.get_y_f() };
                    float bp_[] = { (p_[0] - b_[0])*2, (p_[1] - b_[1])*2 }; // vec_bp * 2
                    p.set_pos_clipped( keyframei.get_centered( bp_[0] + b_[0] ),
                                       keyframei.get_centered( bp_[1] + b_[1] ) ); // add back to blinky
                    target =  p;
                    break;
                }
                case ghost_t::personality_t::PINKY: {
                    acoord_t p = pacman->get_pos();
                    if( use_original_pacman_behavior() && direction_t::UP == pacman->get_dir() ) {
                        // See http://donhodges.com/pacman_pinky_explanation.htm
                        // See https://gameinternals.com/understanding-pac-man-ghost-behavior
                        p.incr_fwd(keyframei, 4);
                        p.incr_left(keyframei, 4);
                    } else {
                        p.incr_fwd(keyframei, 4);
                    }
                    target =  p;
                    break;
                }
                default:
                    target = pacman->get_pos();
                    break;
            }
            break;

        case mode_t::SCATTER:
            // FIXME ???
            switch( id ) {
                case ghost_t::personality_t::BLINKY:
                    target = global_maze->get_top_right_corner();
                    break;
                case ghost_t::personality_t::CLYDE:
                    target = global_maze->get_top_left_corner();
                    break;
                case ghost_t::personality_t::INKY:
                    target = global_maze->get_bottom_left_corner();
                    break;
                case ghost_t::personality_t::PINKY:
                    [[fallthrough]];
                default:
                    target = global_maze->get_bottom_right_corner();
                    break;
            }
            target.set_centered(keyframei);
            break;

        case mode_t::PHANTOM:
            target = global_maze->get_ghost_home_pos();
            target.set_centered(keyframei);
            break;

        case mode_t::SCARED:
            [[fallthrough]];
            // dummy, since an RNG is being used
        default:
            target = global_maze->get_ghost_start_pos();
            target.set_centered(keyframei);
            break;
    }
}

void ghost_t::set_mode(const mode_t m) {
    const mode_t old_mode = mode;
    switch( m ) {
        case mode_t::HOME:
            if( ghost_t::personality_t::BLINKY == id ) {
                // positioned outside of the box and chasing right away
                mode = mode_t::CHASE;
                mode_ms_left = number( mode_duration_t::CHASING );
                dir_ = direction_t::LEFT;
                pos = global_maze->get_ghost_start_pos();
                pos.set_centered(keyframei);
            } else {
                mode = m;
                mode_ms_left = number( mode_duration_t::HOMESTAY );
                pos = global_maze->get_ghost_home_pos();
                pos.set_centered(keyframei);
            }
            set_speed(0.75f);
            break;
        case mode_t::LEAVE_HOME:
            mode = m;
            mode_ms_left = number( mode_duration_t::HOMESTAY );
            break;
        case mode_t::CHASE:
            mode = m;
            mode_ms_left = number( mode_duration_t::CHASING );
            if( mode_t::LEAVE_HOME != old_mode ) {
                dir_ = direction_t::LEFT;
            } else if( mode_t::SCARED != old_mode ) {
                dir_ = inverse(dir_);
            }
            set_speed(0.75f);
            break;
        case mode_t::SCATTER:
            mode = m;
            mode_ms_left = number( mode_duration_t::SCATTERING );
            if( mode_t::HOME == old_mode || mode_t::LEAVE_HOME == old_mode ) {
                // NOP
            } else if( mode_t::SCARED != old_mode ) {
                dir_ = inverse(dir_);
                set_speed(0.75f);
            }
            break;
        case mode_t::SCARED:
            if( mode_t::HOME == old_mode || mode_t::LEAVE_HOME == old_mode ) {
                // NOP
            } else {
                mode = m;
                mode_ms_left = number( mode_duration_t::SCARED );
                dir_ = inverse(dir_);
                set_speed(0.50f);
            }
            break;
        case mode_t::PHANTOM:
            mode = m;
            mode_ms_left = number( mode_duration_t::PHANTOM );
            set_speed(0.75f);
            break;
        default:
            mode = m;
            mode_ms_left = -1;
            break;
    }
    set_next_target();
    log_printf("%s set_mode: %s -> %s [%d ms], pos %s -> %s\n", to_string(id).c_str(), to_string(old_mode).c_str(), to_string(mode).c_str(), mode_ms_left,
            pos.toShortString().c_str(), target.toShortString().c_str());
}

void ghost_t::set_speed(const float pct) {
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
        log_printf("%s set_speed: %5.2f -> %5.2f: skip_tick_each_frames %d, %s\n", to_string(id).c_str(), old, current_speed_pct, skip_tick_each_frames, keyframei.toString().c_str());
    }
}

void ghost_t::set_next_dir() {
    if( !pos.is_center_dir(keyframei) ) {
        return; // NOP
    }
    /**
     * Original Puckman direction encoding, see http://donhodges.com/pacman_pinky_explanation.htm
     */
    constexpr const int R = 0;
    constexpr const int D = 1;
    constexpr const int L = 2;
    constexpr const int U = 3;

    const direction_t cur_dir = dir_;
    const direction_t inv_dir = inverse(cur_dir);
    const direction_t left_dir = rot_left(cur_dir);
    const direction_t right_dir = rot_right(cur_dir);
    direction_t new_dir;
    int choice = 0;
    const float d_inf = global_maze->get_width() * global_maze->get_height() * 10; // std::numeric_limits<float>::max();

    acoord_t right = pos;   // 0
    acoord_t down = pos;    // 1
    acoord_t left = pos;    // 2
    acoord_t up = pos;      // 3
    acoord_t::collisiontest_simple_t collisiontest = [&](tile_t tile) -> bool {
        return ( mode_t::LEAVE_HOME == mode || mode_t::PHANTOM == mode ) ?
               tile_t::WALL == tile : ( tile_t::WALL == tile || tile_t::GATE == tile );
    };

    const bool dir_coll[4] = {
            !right.step(direction_t::RIGHT, keyframei, collisiontest),
            !down.step(direction_t::DOWN, keyframei, collisiontest),
            !left.step(direction_t::LEFT, keyframei, collisiontest),
            !up.step(direction_t::UP, keyframei, collisiontest) };

    if( dir_coll[ ::number(left_dir) ] && dir_coll[ ::number(right_dir) ] ) {
        // walls left and right
        if( dir_coll[ ::number(cur_dir) ] ) {
            // dead-end, can only return .. unusual (not in orig map)
            new_dir = inv_dir;
            choice = 10;
        } else {
            // straight ahead and walls left and right
            new_dir = cur_dir;
            choice = 20;
        }
    } else {
        // find shortest path
        float dir_dist[4] = {
                dir_coll[R] ? d_inf : right.sq_distance(target),
                dir_coll[D] ? d_inf : down.sq_distance(target),
                dir_coll[L] ? d_inf : left.sq_distance(target),
                dir_coll[U] ? d_inf : up.sq_distance(target) };

        // penalty for reversal
        dir_dist[ ::number(inv_dir) ] += 2*2;

        if( log_moves ) {
            log_printf(std::string(to_string(id)+": collisions r "+std::to_string(dir_coll[R])+", d "+std::to_string(dir_coll[D])+", l "+std::to_string(dir_coll[L])+", u "+std::to_string(dir_coll[U])+"\n").c_str());
            log_printf(std::string(to_string(id)+": distances r "+std::to_string(dir_dist[R])+", d "+std::to_string(dir_dist[D])+", l "+std::to_string(dir_dist[L])+", u "+std::to_string(dir_dist[U])+"\n").c_str());
        }
        // Check for a clear short path, reversal has been punished
        if( !dir_coll[R] && dir_dist[R] < dir_dist[U] && dir_dist[R] < dir_dist[D] && dir_dist[R] < dir_dist[L] ) {
            new_dir = direction_t::RIGHT;
            choice = 30;
        } else if( !dir_coll[D] && dir_dist[D] < dir_dist[U] && dir_dist[D] < dir_dist[L] && dir_dist[D] < dir_dist[R] ) {
            new_dir = direction_t::DOWN;
            choice = 31;
        } else if( !dir_coll[L] && dir_dist[L] < dir_dist[U] && dir_dist[L] < dir_dist[D] && dir_dist[L] < dir_dist[R] ) {
            new_dir = direction_t::LEFT;
            choice = 32;
        } else if( !dir_coll[U] && dir_dist[U] < dir_dist[D] && dir_dist[U] < dir_dist[L] && dir_dist[U] < dir_dist[R] ) {
            new_dir = direction_t::UP;
            choice = 33;
        } else {
            if( !dir_coll[ ::number(cur_dir) ] ) {
                // straight ahead .. no better choice
                new_dir = cur_dir;
                choice = 40;
            } else if( !dir_coll[R] && dir_dist[R] <= dir_dist[U] && dir_dist[R] <= dir_dist[D] && dir_dist[R] <= dir_dist[L] ) {
                new_dir = direction_t::RIGHT;
                choice = 50;
            } else if( !dir_coll[D] && dir_dist[D] <= dir_dist[U] && dir_dist[D] <= dir_dist[L] && dir_dist[D] <= dir_dist[R] ) {
                new_dir = direction_t::DOWN;
                choice = 51;
            } else if( !dir_coll[L] && dir_dist[L] <= dir_dist[U] && dir_dist[L] <= dir_dist[D] && dir_dist[L] <= dir_dist[R] ) {
                new_dir = direction_t::LEFT;
                choice = 52;
            } else if( !dir_coll[U] /* && dir_dist[U] <= dir_dist[D] && dir_dist[U] <= dir_dist[L] && dir_dist[U] <= dir_dist[R] */ ) {
                new_dir = direction_t::UP;
                choice = 53;
            } else {
                new_dir = direction_t::LEFT;
                choice = 60;
            }
        }
    }
    dir_ = new_dir;
    if( log_moves ) {
        log_printf("%s set_next_dir: %s -> %s (%d), %s [%d ms], pos %s -> %s\n", to_string(id).c_str(), to_string(cur_dir).c_str(), to_string(new_dir).c_str(),
                choice, to_string(mode).c_str(), mode_ms_left, pos.toShortString().c_str(), target.toShortString().c_str());
    }
}

bool ghost_t::tick() {
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

    if( 0 < mode_ms_left ) {
        mode_ms_left = std::max( 0, mode_ms_left - get_ms_per_frame() );
    }

    if( mode_t::AWAY == mode ) {
        return true; // NOP
    } else if( mode_t::HOME == mode ) {
        if( 0 == mode_ms_left ) {
            set_mode( mode_t::LEAVE_HOME );
        }
    } else if( mode_t::LEAVE_HOME == mode ) {
        if( pos.intersects(target) ) {
            set_mode( mode_t::CHASE );
        } else if( 0 == mode_ms_left ) { // ooops
            pos = global_maze->get_ghost_start_pos();
            pos.set_centered(keyframei);
            set_mode( mode_t::CHASE );
        }
    } else if( mode_t::CHASE == mode ) {
        if( !pos.intersects_f( global_maze->get_ghost_home_box() ) ) {
            set_next_target(); // update ...
        }
        if( 0 == mode_ms_left ) {
            set_mode( mode_t::SCATTER );
        }
    } else if( mode_t::SCATTER == mode ) {
        if( 0 == mode_ms_left ) {
            set_mode( mode_t::CHASE );
        }
    } else if( mode_t::SCARED == mode ) {
        if( 0 == mode_ms_left ) {
            set_mode( mode_t::SCATTER );
        }
    } else if( mode_t::PHANTOM == mode ) {
        if( pos.intersects(target) || 0 == mode_ms_left ) {
            set_mode( mode_t::HOME );
        }
    }

    collision_maze = !pos.step(dir_, keyframei, [&](tile_t tile) -> bool {
        return ( mode_t::LEAVE_HOME == mode || mode_t::PHANTOM == mode ) ?
               tile_t::WALL == tile : ( tile_t::WALL == tile || tile_t::GATE == tile );
    });
    set_next_dir();

    if( DEBUG_GFX_BOUNDS ) {
        log_printf("tick[%s]: %s, pos %s -> %s, crash[maze %d], textures %s\n",
                to_string(id).c_str(), to_string(dir_).c_str(), pos.toShortString().c_str(), target.toShortString().c_str(), collision_maze, atex->toString().c_str());
    }
    return true;
}

void ghost_t::draw(SDL_Renderer* rend) {
    if( mode_t::AWAY != mode ) {
        atex->draw(rend, pos.get_x_f()-keyframei.get_center(), pos.get_y_f()-keyframei.get_center());
    }
    if( show_all_debug_gfx() ) {
        if( show_all_debug_gfx() || DEBUG_GFX_BOUNDS ) {
            uint8_t r, g, b, a;
            SDL_GetRenderDrawColor(rend, &r, &g, &b, &a);
            SDL_SetRenderDrawColor(rend, 255, 0, 0, 255);
            const int win_pixel_offset = ( win_pixel_width - global_maze->get_pixel_width()*win_pixel_scale ) / 2;
            // pos is on player center position
            SDL_Rect bounds = { .x=win_pixel_offset + round_to_int( pos.get_x_f() * global_maze->get_ppt_y() * win_pixel_scale ) - ( atex->get_width()  * win_pixel_scale ) / 2,
                                .y=                   round_to_int( pos.get_y_f() * global_maze->get_ppt_y() * win_pixel_scale ) - ( atex->get_height() * win_pixel_scale ) / 2,
                                .w=atex->get_width()*win_pixel_scale, .h=atex->get_height()*win_pixel_scale };
            SDL_RenderDrawRect(rend, &bounds);
            SDL_SetRenderDrawColor(rend, r, g, b, a);
        }
    }
}

std::string ghost_t::toString() const {
    return to_string(id)+"["+to_string(mode)+"["+std::to_string(mode_ms_left)+" ms], "+to_string(dir_)+", "+pos.toString()+" -> "+target.toShortString()+", "+atex->toString()+", "+keyframei.toString()+"]";
}

std::string to_string(ghost_t::personality_t id) {
    switch( id ) {
        case ghost_t::personality_t::BLINKY:
            return "blinky";
        case ghost_t::personality_t::CLYDE:
            return "clyde";
        case ghost_t::personality_t::INKY:
            return "inky";
        case ghost_t::personality_t::PINKY:
            return "pinky";
        default:
            return "unknown";
    }
}

std::string to_string(ghost_t::mode_t m) {
    switch( m ) {
        case ghost_t::mode_t::AWAY:
            return "away";
        case ghost_t::mode_t::HOME:
            return "home";
        case ghost_t::mode_t::LEAVE_HOME:
            return "leave_home";
        case ghost_t::mode_t::CHASE:
            return "chase";
        case ghost_t::mode_t::SCATTER:
            return "scatter";
        case ghost_t::mode_t::SCARED:
            return "scared";
        case ghost_t::mode_t::PHANTOM:
            return "phantom";
        default:
            return "unknown";
    }
}


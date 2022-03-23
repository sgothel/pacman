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

int ghost_t::id_to_yoff(ghost_t::personality_t id) noexcept {
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

animtex_t& ghost_t::get_tex() noexcept {
    switch( mode_ ) {
        case mode_t::SCARED:
            return atex_scared;

        case mode_t::PHANTOM:
            return atex_phantom;

        default:
            return atex_normal;
    }
}

ghost_t::ghost_t(const personality_t id_, SDL_Renderer* rend, const float fields_per_sec_total_) noexcept
: fields_per_sec_total(fields_per_sec_total_),
  current_speed_pct(0.75f),
  keyframei_(true /* odd */, get_frames_per_sec(), fields_per_sec_total*current_speed_pct, false /* hint_slower */),
  sync_next_frame_cntr( keyframei_.sync_frame_count() ),
  synced_frame_count( 0 ),
  id( id_ ),
  mode_( mode_t::HOME ),
  mode_ms_left ( number( mode_duration_t::HOMESTAY ) ),
  dir_( direction_t::LEFT ),
  atex_normal( "N", rend, ms_per_atex, global_tex->all_images(), 0, id_to_yoff(id), 14, 14, { { 0*14, 0 }, { 1*14, 0 }, { 2*14, 0 }, { 3*14, 0 } }),
  atex_scared( "S", rend, ms_per_atex, global_tex->all_images(), 0, 0, 14, 14, { { 10*14, 0 } }),
  atex_phantom( "P", rend, ms_per_atex, global_tex->all_images(), 0, 41 + 4*14, 14, 14, { { 0*14, 0 }, { 1*14, 0 }, { 2*14, 0 }, { 3*14, 0 } }),
  atex( &get_tex() ),
  pos_( global_maze->ghost_home_pos() ),
  target_( global_maze->ghost_home_pos() )
{
    set_mode( mode_t::HOME );
}


void ghost_t::destroy() noexcept {
    atex_normal.destroy();
    atex_scared.destroy();
    atex_phantom.destroy();
}

void ghost_t::set_next_target() noexcept {
    switch( mode_ ) {
        case mode_t::HOME:
            if( ghost_t::personality_t::BLINKY == id ) {
                target_ = pacman->position();
                break;
            } else {
                target_ = global_maze->ghost_home_pos();
                target_.set_centered(keyframei_);
                break;
            }

        case mode_t::LEAVE_HOME:
            target_ = global_maze->ghost_start_pos();
            target_.set_centered(keyframei_);
            break;

        case mode_t::CHASE:
            switch( id ) {
                case ghost_t::personality_t::BLINKY:
                    target_ = pacman->position();
                    break;
                case ghost_t::personality_t::CLYDE:
                    target_ = global_maze->top_left_corner();
                    target_.set_centered(keyframei_);
                    break;
                case ghost_t::personality_t::INKY: {
                    /**
                     * Selecting the position two tiles in front of Pac-Man in his current direction of travel.
                     * From there, imagine drawing a vector from Blinky's position to this tile,
                     * and then doubling the length of the vector.
                     * The tile that this new, extended vector ends on will be Inky's actual target.
                     */
                    acoord_t p = pacman->position();
                    acoord_t b = ghosts[ number( ghost_t::personality_t::BLINKY ) ]->position();
                    p.incr_fwd(keyframei_, 2);
                    float p_[] = { p.x_f(), p.y_f() };
                    float b_[] = { b.x_f(), b.y_f() };
                    float bp_[] = { (p_[0] - b_[0])*2, (p_[1] - b_[1])*2 }; // vec_bp * 2
                    p.set_pos_clipped( keyframei_.center_valued( bp_[0] + b_[0] ),
                                       keyframei_.center_valued( bp_[1] + b_[1] ) ); // add back to blinky
                    target_ =  p;
                    break;
                }
                case ghost_t::personality_t::PINKY: {
                    acoord_t p = pacman->position();
                    if( use_original_pacman_behavior() && direction_t::UP == pacman->direction() ) {
                        // See http://donhodges.com/pacman_pinky_explanation.htm
                        // See https://gameinternals.com/understanding-pac-man-ghost-behavior
                        p.incr_fwd(keyframei_, 4);
                        p.incr_left(keyframei_, 4);
                    } else {
                        p.incr_fwd(keyframei_, 4);
                    }
                    target_ =  p;
                    break;
                }
                default:
                    target_ = pacman->position();
                    break;
            }
            break;

        case mode_t::SCATTER:
            // FIXME ???
            switch( id ) {
                case ghost_t::personality_t::BLINKY:
                    target_ = global_maze->top_right_corner();
                    break;
                case ghost_t::personality_t::CLYDE:
                    target_ = global_maze->top_left_corner();
                    break;
                case ghost_t::personality_t::INKY:
                    target_ = global_maze->bottom_left_corner();
                    break;
                case ghost_t::personality_t::PINKY:
                    [[fallthrough]];
                default:
                    target_ = global_maze->bottom_right_corner();
                    break;
            }
            target_.set_centered(keyframei_);
            break;

        case mode_t::PHANTOM:
            target_ = global_maze->ghost_home_pos();
            target_.set_centered(keyframei_);
            break;

        case mode_t::SCARED:
            [[fallthrough]];
            // dummy, since an RNG is being used
        default:
            target_ = global_maze->ghost_start_pos();
            target_.set_centered(keyframei_);
            break;
    }
}

void ghost_t::set_mode(const mode_t m) noexcept {
    const mode_t old_mode = mode_;
    switch( m ) {
        case mode_t::HOME:
            if( ghost_t::personality_t::BLINKY == id ) {
                // positioned outside of the box and chasing right away
                mode_ = mode_t::CHASE;
                mode_ms_left = number( mode_duration_t::CHASING );
                dir_ = direction_t::LEFT;
                pos_ = global_maze->ghost_start_pos();
                pos_.set_centered(keyframei_);
            } else {
                mode_ = m;
                mode_ms_left = number( mode_duration_t::HOMESTAY );
                pos_ = global_maze->ghost_home_pos();
                pos_.set_centered(keyframei_);
            }
            set_speed(0.75f);
            break;
        case mode_t::LEAVE_HOME:
            mode_ = m;
            mode_ms_left = number( mode_duration_t::HOMESTAY );
            break;
        case mode_t::CHASE:
            mode_ = m;
            mode_ms_left = number( mode_duration_t::CHASING );
            if( mode_t::LEAVE_HOME != old_mode ) {
                dir_ = direction_t::LEFT;
            } else if( mode_t::SCARED != old_mode ) {
                dir_ = inverse(dir_);
            }
            set_speed(0.75f);
            break;
        case mode_t::SCATTER:
            mode_ = m;
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
                mode_ = m;
                mode_ms_left = number( mode_duration_t::SCARED );
                dir_ = inverse(dir_);
                set_speed(0.50f);
            }
            break;
        case mode_t::PHANTOM:
            mode_ = m;
            mode_ms_left = number( mode_duration_t::PHANTOM );
            set_speed(0.75f);
            break;
        default:
            mode_ = m;
            mode_ms_left = -1;
            break;
    }
    set_next_target();
    log_printf("%s set_mode: %s -> %s [%d ms], pos %s -> %s\n", to_string(id).c_str(), to_string(old_mode).c_str(), to_string(mode_).c_str(), mode_ms_left,
            pos_.toShortString().c_str(), target_.toShortString().c_str());
}

void ghost_t::set_speed(const float pct) noexcept {
    if( std::abs( current_speed_pct - pct ) <= std::numeric_limits<float>::epsilon() ) {
        return;
    }
    const float old = current_speed_pct;
    current_speed_pct = pct;
    keyframei_.reset(true /* odd */, get_frames_per_sec(), fields_per_sec_total*pct, false /* hint_slower */);
    sync_next_frame_cntr = keyframei_.sync_frame_count();
    synced_frame_count = 0;
    if( log_moves ) {
        log_printf("%s set_speed: %5.2f -> %5.2f: sync_each_frames %d, %s\n", to_string(id).c_str(), old, current_speed_pct, sync_next_frame_cntr, keyframei_.toString().c_str());
    }
}

void ghost_t::set_next_dir(const bool collision, const bool is_center) noexcept {
    if( !is_center && !collision ) {
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
    const float d_inf = global_maze->width() * global_maze->height() * 10; // std::numeric_limits<float>::max();

    acoord_t right = pos_;   // 0
    acoord_t down = pos_;    // 1
    acoord_t left = pos_;    // 2
    acoord_t up = pos_;      // 3
    acoord_t::collisiontest_simple_t collisiontest = [&](tile_t tile) -> bool {
        return ( mode_t::LEAVE_HOME == mode_ || mode_t::PHANTOM == mode_ ) ?
               tile_t::WALL == tile : ( tile_t::WALL == tile || tile_t::GATE == tile );
    };

    const bool dir_coll[4] = {
            !right.step(direction_t::RIGHT, keyframei_, collisiontest),
            !down.step(direction_t::DOWN, keyframei_, collisiontest),
            !left.step(direction_t::LEFT, keyframei_, collisiontest),
            !up.step(direction_t::UP, keyframei_, collisiontest) };

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
                dir_coll[R] ? d_inf : right.sq_distance(target_),
                dir_coll[D] ? d_inf : down.sq_distance(target_),
                dir_coll[L] ? d_inf : left.sq_distance(target_),
                dir_coll[U] ? d_inf : up.sq_distance(target_) };

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
        log_printf("%s set_next_dir: %s -> %s (%d), %s [%d ms], pos %s c%d e%d -> %s\n", to_string(id).c_str(), to_string(cur_dir).c_str(), to_string(new_dir).c_str(),
                choice, to_string(mode_).c_str(), mode_ms_left, pos_.toShortString().c_str(), pos_.is_center_dir(keyframei_), pos_.entered_tile(keyframei_), target_.toShortString().c_str());
    }
}

bool ghost_t::tick() noexcept {
    if( 0 < sync_next_frame_cntr ) {
        if( 0 >= --sync_next_frame_cntr ) {
            sync_next_frame_cntr = keyframei_.sync_frame_count();
            synced_frame_count++;
            return true; // skip tick, just repaint
        }
    }
    atex = &get_tex();
    atex->tick();

    bool collision_maze = false;

    if( 0 < mode_ms_left ) {
        mode_ms_left = std::max( 0, mode_ms_left - get_ms_per_frame() );
    }

    if( mode_t::AWAY == mode_ ) {
        return true; // NOP
    } else if( mode_t::HOME == mode_ ) {
        if( 0 == mode_ms_left ) {
            set_mode( mode_t::LEAVE_HOME );
        }
    } else if( mode_t::LEAVE_HOME == mode_ ) {
        if( pos_.intersects(target_) ) {
            set_mode( mode_t::CHASE );
        } else if( 0 == mode_ms_left ) { // ooops
            pos_ = global_maze->ghost_start_pos();
            pos_.set_centered(keyframei_);
            set_mode( mode_t::CHASE );
        }
    } else if( mode_t::CHASE == mode_ ) {
        if( !pos_.intersects_f( global_maze->ghost_home_box() ) ) {
            set_next_target(); // update ...
        }
        if( 0 == mode_ms_left ) {
            set_mode( mode_t::SCATTER );
        }
    } else if( mode_t::SCATTER == mode_ ) {
        if( 0 == mode_ms_left ) {
            set_mode( mode_t::CHASE );
        }
    } else if( mode_t::SCARED == mode_ ) {
        if( 0 == mode_ms_left ) {
            set_mode( mode_t::SCATTER );
        }
    } else if( mode_t::PHANTOM == mode_ ) {
        if( pos_.intersects(target_) || 0 == mode_ms_left ) {
            set_mode( mode_t::HOME );
        }
    }

    if( 0 >= next_field_frame_cntr ) {
        next_field_frame_cntr = keyframei_.frames_per_field();
    } else {
        --next_field_frame_cntr;
    }

    collision_maze = !pos_.step(dir_, keyframei_, [&](tile_t tile) -> bool {
        return ( mode_t::LEAVE_HOME == mode_ || mode_t::PHANTOM == mode_ ) ?
               tile_t::WALL == tile : ( tile_t::WALL == tile || tile_t::GATE == tile );
    });

    if( log_moves || DEBUG_GFX_BOUNDS ) {
        log_printf("%s tick: %s, %s [%d ms], pos %s c%d e%d crash %d -> %s, textures %s\n",
                to_string(id).c_str(), to_string(dir_).c_str(), to_string(mode_).c_str(), mode_ms_left,
                pos_.toShortString().c_str(), pos_.is_center_dir(keyframei_), pos_.entered_tile(keyframei_), collision_maze,
                target_.toShortString().c_str(), atex->toString().c_str());
    }

    set_next_dir(collision_maze, pos_.is_center_dir(keyframei_));

    return true;
}

void ghost_t::draw(SDL_Renderer* rend) noexcept {
    if( mode_t::AWAY != mode_ ) {
        atex->draw(rend, pos_.x_f()-keyframei_.center(), pos_.y_f()-keyframei_.center());
    }
    if( show_all_debug_gfx() ) {
        if( show_all_debug_gfx() || DEBUG_GFX_BOUNDS ) {
            uint8_t r, g, b, a;
            SDL_GetRenderDrawColor(rend, &r, &g, &b, &a);
            SDL_SetRenderDrawColor(rend, 255, 0, 0, 255);
            const int win_pixel_offset = ( win_pixel_width - global_maze->pixel_width()*win_pixel_scale ) / 2;
            // pos is on player center position
            SDL_Rect bounds = { .x=win_pixel_offset + round_to_int( pos_.x_f() * global_maze->ppt_y() * win_pixel_scale ) - ( atex->width()  * win_pixel_scale ) / 2,
                                .y=                   round_to_int( pos_.y_f() * global_maze->ppt_y() * win_pixel_scale ) - ( atex->height() * win_pixel_scale ) / 2,
                                .w=atex->width()*win_pixel_scale, .h=atex->height()*win_pixel_scale };
            SDL_RenderDrawRect(rend, &bounds);
            SDL_SetRenderDrawColor(rend, r, g, b, a);
        }
    }
}

std::string ghost_t::toString() const noexcept {
    return to_string(id)+"["+to_string(mode_)+"["+std::to_string(mode_ms_left)+" ms], "+to_string(dir_)+", "+pos_.toString()+" -> "+target_.toShortString()+", "+atex->toString()+", "+keyframei_.toString()+"]";
}

std::string to_string(ghost_t::personality_t id) noexcept {
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

std::string to_string(ghost_t::mode_t m) noexcept {
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


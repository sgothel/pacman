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
#include <random>

#include <cstdio>
#include <time.h>

static constexpr const bool DEBUG_PELLET_COUNTER = false;

//
// ghost_t
//

ghost_t::mode_t ghost_t::global_mode = mode_t::AWAY;
ghost_t::mode_t ghost_t::global_mode_last = mode_t::AWAY;
int ghost_t::global_mode_ms_left = 0; // for SCATTER, CHASE or SCARED
int ghost_t::global_scatter_mode_count = 0;

bool ghost_t::global_pellet_counter_active = false;
int ghost_t::global_pellet_counter = 0;
int ghost_t::global_pellet_time_left = 0;

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
    if( at_home() && mode_t::SCARED == global_mode ) {
        return atex_scared;
    }
    switch( mode_ ) {
        case mode_t::SCARED:
            if( mode_ms_left <= ms_per_fright_flash * game_level_spec().fright_flash_count ) {
                return atex_scared_flash;
            } else {
                return atex_scared;
            }
        case mode_t::PHANTOM:
            return atex_phantom;

        default:
            return atex_normal;
    }
}

ghost_t::ghost_t(const personality_t id__, SDL_Renderer* rend, const float fields_per_sec_total_) noexcept
: fields_per_sec_total(fields_per_sec_total_),
  current_speed_pct(0.0f),
  keyframei_(get_frames_per_sec(), fields_per_sec_total*current_speed_pct, true /* nearest */),
  sync_next_frame_cntr( keyframei_.sync_frame_count(), true /* auto_reload */),
  id_( id__ ),
  live_counter_during_pacman_live( 0 ),
  mode_( mode_t::AWAY ),
  mode_last( mode_t::AWAY ),
  mode_ms_left ( 0 ),
  dir_( direction_t::LEFT ),
  pellet_counter_active_( false ),
  pellet_counter_( 0 ),
  atex_normal( "N", rend, ms_per_atex, global_tex->all_images(), 0, id_to_yoff(id_), 14, 14, { { 0*14, 0 }, { 1*14, 0 }, { 2*14, 0 }, { 3*14, 0 } }),
  atex_scared( "S", rend, ms_per_atex, global_tex->all_images(), 0, 0, 14, 14, { { 10*14, 0 } }),
  atex_scared_flash( "S+", rend, ms_per_fright_flash/2, global_tex->all_images(), 0, 0, 14, 14, { { 10*14, 0 }, { 11*14, 0 } }),
  atex_phantom( "P", rend, ms_per_atex, global_tex->all_images(), 0, 41 + 4*14, 14, 14, { { 0*14, 0 }, { 1*14, 0 }, { 2*14, 0 }, { 3*14, 0 } }),
  atex( &get_tex() ),
  dir_next( dir_ ),
  pos_next(-1, -1)
{
    if( ghost_t::personality_t::BLINKY == id_ ) {
        // positioned outside of the box at start
        home_pos = acoord_t( global_maze->ghost_start_box().center_x()-0.0f, global_maze->ghost_start_box().y()-0.0f );
        // home_pos = acoord_t( global_maze->ghost_home_int_box().center_x(), global_maze->ghost_home_int_box().center_y() );
    } else if( ghost_t::personality_t::PINKY == id_ ) {
        home_pos = acoord_t( global_maze->ghost_home_int_box().center_x()-0.0f, global_maze->ghost_home_int_box().center_y()-0.0f );
    } else if( ghost_t::personality_t::INKY == id_ ) {
        home_pos = acoord_t( global_maze->ghost_home_int_box().center_x()-2.0f, global_maze->ghost_home_int_box().center_y()-0.0f );
    } else {
        home_pos = acoord_t( global_maze->ghost_home_int_box().center_x()+2.0f, global_maze->ghost_home_int_box().center_y()-0.0f );
    }
    pos_ = home_pos;
    target_ = home_pos;
}


void ghost_t::destroy() noexcept {
    atex_normal.destroy();
    atex_scared.destroy();
    atex_phantom.destroy();
}

bool ghost_t::set_speed(const float pct) noexcept {
    if( std::abs( current_speed_pct - pct ) <= std::numeric_limits<float>::epsilon() ) {
        return false;
    }
    const float old = current_speed_pct;
    current_speed_pct = pct;
    keyframei_.reset(get_frames_per_sec(), fields_per_sec_total*pct, true /* nearest */);
    pos_.set_aligned_dir(keyframei_);
    sync_next_frame_cntr.reset( keyframei_.sync_frame_count(), true /* auto_reload */);
    if( log_modes() ) {
        log_printf("%s set_speed: %5.2f -> %5.2f: sync_each_frames %zd, %s\n", to_string(id_).c_str(), old, current_speed_pct, sync_next_frame_cntr.counter(), keyframei_.toString().c_str());
    }
    return true;
}

void ghost_t::set_next_target() noexcept {
    switch( mode_ ) {
        case mode_t::HOME:
            target_ = home_pos;
            target_.set_centered(keyframei_);
            break;

        case mode_t::LEAVE_HOME:
            target_ = acoord_t( global_maze->ghost_start_box().x(), global_maze->ghost_start_box().y() );
            target_.set_centered(keyframei_);
            break;

        case mode_t::CHASE:
            switch( id_ ) {
                case ghost_t::personality_t::BLINKY:
                    target_ = pacman->position();
                    break;
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
                    target_ = p;
                    break;
                }
                case ghost_t::personality_t::INKY: {
                    /**
                     * Selecting the position two tiles in front of Pac-Man in his current direction of travel.
                     * From there, imagine drawing a vector from Blinky's position to this tile,
                     * and then doubling the length of the vector.
                     * The tile that this new, extended vector ends on will be Inky's actual target.
                     */
                    acoord_t p = pacman->position();
                    acoord_t b = ghost( ghost_t::personality_t::BLINKY )->position();
                    p.incr_fwd(keyframei_, 2);
                    float p_[] = { p.x_f(), p.y_f() };
                    float b_[] = { b.x_f(), b.y_f() };
                    float bp_[] = { (p_[0] - b_[0])*2, (p_[1] - b_[1])*2 }; // vec_bp * 2
                    p.set_pos( keyframei_.center_value( bp_[0] + b_[0] ),
                               keyframei_.center_value( bp_[1] + b_[1] ) ); // add back to blinky
                    target_ = p;
                    break;
                }
                case ghost_t::personality_t::CLYDE: {
                    acoord_t p = pacman->position();
                    const float d_p = pos_.sq_distance(p);
                    // farther than eight tiles away, his targeting is identical to Blinky
                    if( d_p > 8*8 ) {
                        target_ = pacman->position();
                    } else {
                        target_ = global_maze->bottom_left_scatter();
                        target_.set_centered(keyframei_);
                    }
                    break;
                }
                default:
                    target_ = pacman->position();
                    break;
            }
            break;

        case mode_t::SCATTER:
            switch( id_ ) {
                case ghost_t::personality_t::BLINKY:
                    target_ = global_maze->top_right_scatter();
                    break;
                case ghost_t::personality_t::PINKY:
                    target_ = global_maze->top_left_scatter();
                    break;
                case ghost_t::personality_t::INKY:
                    target_ = global_maze->bottom_right_scatter();
                    break;
                case ghost_t::personality_t::CLYDE:
                    [[fallthrough]];
                default:
                    target_ = global_maze->bottom_left_scatter();
                    break;
            }
            target_.set_centered(keyframei_);
            break;

        case mode_t::PHANTOM:
            if( ghost_t::personality_t::BLINKY == id_ ) {
                target_ = acoord_t( global_maze->ghost_home_int_box().center_x(), global_maze->ghost_home_int_box().center_y() );
            } else {
                target_ = home_pos;
            }
            target_.set_centered(keyframei_);
            break;

        case mode_t::SCARED:
            [[fallthrough]];
            // dummy, since an RNG is being used
        default:
            target_ = pos_; // dummy self target
            break;
    }
}

std::vector<std::vector<int>> ghost_t::rgb_color = {
        { 0xff, 0x00, 0x00 }, // blinky color
        { 0xff, 0xb7, 0xff }, // pinky color
        { 0x00, 0xff, 0xff }, // inky color
        { 0xff, 0xb7, 0x51 }  // clyde color
};

random_engine_t<random_engine_mode_t::STD_RNG> ghost_t::rng_hw;
random_engine_t<random_engine_mode_t::STD_PRNG_0> ghost_t::rng_prng;
random_engine_t<random_engine_mode_t::PUCKMAN> ghost_t::rng_pm;
std::uniform_int_distribution<int> ghost_t::rng_dist(::number(direction_t::RIGHT), ::number(direction_t::UP));

direction_t ghost_t::get_random_dir() noexcept {
    if( true ) {
        return static_cast<direction_t>( rng_dist(rng_pm) );
    } else if( false ) {
        // return static_cast<direction_t>( rng_prng() % ( 3 + 1 ) );
        return static_cast<direction_t>( rng_dist(rng_prng) );
    } else {
        return static_cast<direction_t>( rng_dist(rng_hw) );
    }
}

void ghost_t::reset_random() noexcept {
    rng_pm.seed(0);
    rng_prng.seed(0);
    // rng_hw.seed(0);
    rng_dist.reset();
}

void ghost_t::set_next_dir(const bool collision, const bool is_center) noexcept {
    if( !is_center && !collision ) {
        return; // NOP
    }

    // Perform one full step to next tile as look-ahead
    static const keyframei_t one_step(1.0f, 1.0f, true /* nearest */);

    /**
     * [The Pac-Man Dossier](https://www.gamedeveloper.com/design/the-pac-man-dossier)
     * - ghost prefers directions in this order: up, left, down, right
     *
     * [Original Puckman direction encoding](http://donhodges.com/pacman_pinky_explanation.htm).
     */

    acoord_t::collisiontest_simple_t collisiontest = [&](tile_t tile) -> bool {
        return ( mode_t::LEAVE_HOME == mode_ || mode_t::PHANTOM == mode_ ) ?
               tile_t::WALL == tile : ( tile_t::WALL == tile || tile_t::GATE == tile );
    };

    const direction_t cur_dir = dir_;
    const direction_t inv_dir = inverse(cur_dir);
    direction_t new_dir;
    int choice;

    // the position to be tested
    acoord_t  test_pos = pos_;

    bool ahead_coll;
    if( use_decision_one_field_ahead() ) {
        // If ahead_coll -> true (in corner),
        // needs to set actual dir_ instead of dir_next and shall not set pos_next!
        test_pos.set_centered(one_step);
        ahead_coll = !test_pos.step(cur_dir, one_step, collisiontest);
        if( ahead_coll ) {
            pos_next.set_pos(-1, -1);
            test_pos = pos_;
        } else {
            pos_next = test_pos;
            pos_next.set_centered(keyframei_);
        }
    } else {
        ahead_coll = false;
    }

    if( mode_t::SCARED == mode_ ) {
        const direction_t rdir = get_random_dir();
        if( rdir != inv_dir && test_pos.test(rdir, one_step, collisiontest) ) {
            new_dir = rdir;
            choice = 1;
        } else if( rdir != direction_t::UP && inv_dir != direction_t::UP && test_pos.test(direction_t::UP, one_step, collisiontest) ) {
            new_dir = direction_t::UP;
            choice = 2;
        } else if( rdir != direction_t::LEFT && inv_dir != direction_t::LEFT && test_pos.test(direction_t::LEFT, one_step, collisiontest) ) {
            new_dir = direction_t::LEFT;
            choice = 3;
        } else if( rdir != direction_t::DOWN && inv_dir != direction_t::DOWN && test_pos.test(direction_t::DOWN, one_step, collisiontest) ) {
            new_dir = direction_t::DOWN;
            choice = 4;
        } else if( rdir != direction_t::RIGHT && inv_dir != direction_t::RIGHT && test_pos.test(direction_t::RIGHT, one_step, collisiontest) ) {
            new_dir = direction_t::RIGHT;
            choice = 5;
        } else {
            new_dir = cur_dir;
            choice = 6;
        }
    } else {
        const constexpr int R = 0;
        const constexpr int D = 1;
        const constexpr int L = 2;
        const constexpr int U = 3;

        // not_up on red_zones acts as collision, also assume it as a wall when deciding whether we have a decision point or not!
        const bool not_up = is_scattering_or_chasing() &&
                            ( test_pos.intersects_i( global_maze->red_zone1_box() ) || test_pos.intersects_i( global_maze->red_zone2_box() ) );

        const direction_t left_dir = rot_left(cur_dir);
        const direction_t right_dir = rot_right(cur_dir);

        acoord_t dir_pos[4] = { test_pos, test_pos, test_pos, test_pos }; // R D L U

        const bool dir_coll[4] = {
                !dir_pos[R].step(direction_t::RIGHT, one_step, collisiontest),
                !dir_pos[D].step(direction_t::DOWN, one_step, collisiontest),
                !dir_pos[L].step(direction_t::LEFT, one_step, collisiontest),
                !dir_pos[U].step(direction_t::UP, one_step, collisiontest) || not_up };

        if( log_moves() ) {
            log_printf(std::string(to_string(id_)+" set_next_dir: curr "+to_string(cur_dir)+" -> "+to_string(dir_next)+"\n").c_str());
            log_printf(std::string(to_string(id_)+": p "+pos_.toShortString()+" -> "+test_pos.toShortString()+" (pos_next "+pos_next.toShortString()+")\n").c_str());
            log_printf(std::string(to_string(id_)+": u "+dir_pos[U].toIntString()+", l "+dir_pos[L].toIntString()+", d "+dir_pos[D].toIntString()+", r "+dir_pos[R].toIntString()+", target "+target_.toShortString()+"\n").c_str());
            log_printf(std::string(to_string(id_)+": collisions not_up "+std::to_string(not_up)+", a "+std::to_string(ahead_coll)+", u "+std::to_string(dir_coll[U])+", l "+std::to_string(dir_coll[L])+", d "+std::to_string(dir_coll[D])+", r "+std::to_string(dir_coll[R])+"\n").c_str());
        }

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
            // decision: find shortest path

            const float d_inf =  ( global_maze->width() * global_maze->height() ) * 10; // infinity :)

            const float d_half = use_manhatten_distance() ?
                                 ( global_maze->width() + global_maze->height() ) / 2 : // Manhatten half game diagonal
                                 ( global_maze->width() * global_maze->height() ) / 2;  // Euclidean half game diagonal squared

            float dir_dist[4];

            if( use_manhatten_distance() ) {
                // not default
                dir_dist[R] = dir_coll[R] ? d_inf : dir_pos[R].distance_manhatten_i(target_);
                dir_dist[D] = dir_coll[D] ? d_inf : dir_pos[D].distance_manhatten_i(target_);
                dir_dist[L] = dir_coll[L] ? d_inf : dir_pos[L].distance_manhatten_i(target_);
                dir_dist[U] = dir_coll[U] ? d_inf : dir_pos[U].distance_manhatten_i(target_);
            } else {
                // default
                dir_dist[R] = dir_coll[R] ? d_inf : dir_pos[R].sq_distance_i(target_);
                dir_dist[D] = dir_coll[D] ? d_inf : dir_pos[D].sq_distance_i(target_);
                dir_dist[L] = dir_coll[L] ? d_inf : dir_pos[L].sq_distance_i(target_);
                dir_dist[U] = dir_coll[U] ? d_inf : dir_pos[U].sq_distance_i(target_);
            }

            // penalty for inverse direction
            dir_dist[ ::number(inv_dir) ] += d_half;

            if( log_moves() ) {
                log_printf(std::string(to_string(id_)+": distances u "+std::to_string(dir_dist[U])+", l "+std::to_string(dir_dist[L])+", d "+std::to_string(dir_dist[D])+", r "+std::to_string(dir_dist[R])+"\n").c_str());
            }

            // Check for a clear short path: Reversal has been punished and collision dir_dist set to 'max * 10'
            //
            // A: dir_dist[d] <= dir_dist[other] (inverse direction is punished, collision set to 'max * 10')
            if( dir_dist[U] <= dir_dist[D] && dir_dist[U] <= dir_dist[L] && dir_dist[U] <= dir_dist[R] ) {
                new_dir = direction_t::UP;
                choice = 30;
            } else if( dir_dist[L] <= dir_dist[U] && dir_dist[L] <= dir_dist[D] && dir_dist[L] <= dir_dist[R] ) {
                new_dir = direction_t::LEFT;
                choice = 31;
            } else if( dir_dist[D] <= dir_dist[U] && dir_dist[D] <= dir_dist[L] && dir_dist[D] <= dir_dist[R] ) {
                new_dir = direction_t::DOWN;
                choice = 32;
            } else if( dir_dist[R] <= dir_dist[U] && dir_dist[R] <= dir_dist[D] && dir_dist[R] <= dir_dist[L] ) {
                new_dir = direction_t::RIGHT;
                choice = 33;
            } else {
                // B: !dir_coll[d] && not inverse direction
                if( !dir_coll[U] && inv_dir != direction_t::UP ) {
                    new_dir = direction_t::UP;
                    choice = 50;
                } else if( !dir_coll[L] && inv_dir != direction_t::LEFT ) {
                    new_dir = direction_t::LEFT;
                    choice = 51;
                } else if( !dir_coll[D] && inv_dir != direction_t::DOWN ) {
                    new_dir = direction_t::DOWN;
                    choice = 52;
                } else if( !dir_coll[R] && inv_dir != direction_t::RIGHT ) {
                    new_dir = direction_t::RIGHT;
                    choice = 53;
                } else {
                    // C: !dir_coll[d]
                    if( !dir_coll[U] ) {
                        new_dir = direction_t::UP;
                        choice = 60;
                    } else if( !dir_coll[L] ) {
                        new_dir = direction_t::LEFT;
                        choice = 61;
                    } else if( !dir_coll[D] ) {
                        new_dir = direction_t::DOWN;
                        choice = 62;
                    } else if( !dir_coll[R] ) {
                        new_dir = direction_t::RIGHT;
                        choice = 63;
                    } else {
                        // D: Desperate UP ..
                        new_dir = direction_t::UP;
                        choice = 70;
                    }
                } // C - D
            } // B - D
        } // A - D
    } // SCARED or not
    if( use_decision_one_field_ahead() && !ahead_coll ) {
        dir_next = new_dir;
    } else {
        dir_ = new_dir;
    }
    if( log_moves() ) {
        log_printf("%s set_next_dir: %s -> %s (%d), %s [%d ms], pos %s c%d e%d -> %s\n", to_string(id_).c_str(), to_string(cur_dir).c_str(), to_string(new_dir).c_str(),
                choice, to_string(mode_).c_str(), mode_ms_left, test_pos.toShortString().c_str(), test_pos.is_center(keyframei_), test_pos.entered_tile(keyframei_), target_.toShortString().c_str());
    }
}

void ghost_t::set_global_mode(const mode_t m, const int mode_ms) noexcept {
    if( m != global_mode ) { // only earmark last other mode
        global_mode_last = global_mode;
    }
    global_mode = m;
    global_mode_ms_left = mode_ms;
    bool propagate = true;
    switch( m ) {
        case mode_t::PACMAN_DIED:
            global_pellet_counter_active = true;
            global_pellet_counter = 0;
            reset_random();
            break;
        case mode_t::AWAY:
            break;
        case mode_t::LEVEL_SETUP:
            global_pellet_counter_active = false;
            global_pellet_counter = 0;
            global_pellet_time_left = game_level_spec().ghost_max_home_time_ms;
            global_scatter_mode_count = 0;
            reset_random();
            break;
        case mode_t::START:
            break;
        case mode_t::HOME:
            break;
        case mode_t::LEAVE_HOME:
            break;
        case mode_t::CHASE:
            propagate = false;
            for(ghost_ref g : ghosts()) {
                if( g->is_scattering_or_chasing() ) {
                    g->set_mode( global_mode, global_mode_ms_left );
                }
            }
            break;
        case mode_t::SCATTER:
            propagate = false;
            for(ghost_ref g : ghosts()) {
                if( g->is_scattering_or_chasing() ) {
                    g->set_mode( global_mode, global_mode_ms_left );
                }
            }
            ++global_scatter_mode_count;
            break;
        case mode_t::SCARED:
            propagate = false;
            for(ghost_ref g : ghosts()) {
                if( !g->in_house() ) {
                    g->set_mode( global_mode, global_mode_ms_left );
                }
            }
            break;
        case mode_t::PHANTOM:
            [[fallthrough]];
        default:
            log_printf("Error: set_global_mode: %s -> %s -> %s [%s -> %d ms]\n",
                    to_string(global_mode_last).c_str(), to_string(m).c_str(), to_string(global_mode).c_str(), mode_ms, global_mode_ms_left);
            return;
    }
    if( propagate ) {
        for(ghost_ref g : ghosts()) {
            g->set_mode( global_mode, global_mode_ms_left );
        }
    }
    if( log_modes() ) {
        log_printf("ghosts set_global_mode: %s -> %s -> %s [%d -> %d ms]\n",
                to_string(global_mode_last).c_str(), to_string(m).c_str(), to_string(global_mode).c_str(), mode_ms, global_mode_ms_left);
    }
}

void ghost_t::global_tick() noexcept {
    if( pacman_t::mode_t::FREEZE != pacman->mode() ) {
        if( 0 < global_mode_ms_left ) {
            global_mode_ms_left = std::max( 0, global_mode_ms_left - get_ms_per_frame() );
        }
        if( 0 < global_pellet_time_left ) {
            global_pellet_time_left = std::max( 0, global_pellet_time_left - get_ms_per_frame() );
        }

        switch( global_mode ) {
            case mode_t::PACMAN_DIED:
                set_global_mode( mode_t::AWAY );
                break;
            case mode_t::AWAY:
                break;
            case mode_t::LEVEL_SETUP:
                break;
            case mode_t::START:
                set_global_mode( mode_t::SCATTER, get_ghost_wave(global_scatter_mode_count).scatter_ms );
                break;
            case mode_t::HOME:
                break;
            case mode_t::LEAVE_HOME:
                break;
            case mode_t::CHASE:
                if( 0 == global_mode_ms_left ) {
                    set_global_mode( mode_t::SCATTER, get_ghost_wave(global_scatter_mode_count).scatter_ms );
                }
                break;
            case mode_t::SCATTER:
                if( 0 == global_mode_ms_left ) {
                    set_global_mode( mode_t::CHASE, get_ghost_wave(global_scatter_mode_count).chase_ms );
                }
                break;
            case mode_t::SCARED:
                if( 0 == global_mode_ms_left ) {
                    if( mode_t::CHASE == global_mode_last ) {
                        set_global_mode( mode_t::CHASE, get_ghost_wave(global_scatter_mode_count).chase_ms );
                    } else if( mode_t::SCATTER == global_mode_last ) {
                        set_global_mode( mode_t::SCATTER, get_ghost_wave(global_scatter_mode_count).scatter_ms );
                    } else {
                        set_global_mode( global_mode_last );
                    }
                }
                break;
            case mode_t::PHANTOM:
                break;
            default:
                break;
        }
    }
    for(ghost_ref g : ghosts()) {
        g->tick();
    }
}

void ghost_t::global_draw(SDL_Renderer* rend) noexcept {
    for(ghost_ref g : ghosts()) {
        g->draw(rend);
    }
}

bool ghost_t::set_mode_speed() noexcept {
    switch( mode_ ) {
        case mode_t::PACMAN_DIED:
            break;
        case mode_t::AWAY:
            break;
        case mode_t::LEVEL_SETUP:
            break;
        case mode_t::START:
            [[fallthrough]];
        case mode_t::HOME:
            [[fallthrough]];
        case mode_t::LEAVE_HOME:
            [[fallthrough]];
        case mode_t::CHASE:
            [[fallthrough]];
        case mode_t::SCATTER: {
            if( ghost_t::personality_t::BLINKY == id_ ) {
                const int pellets_left = global_maze->count(tile_t::PELLET);
                if( pellets_left <= game_level_spec().elroy2_dots_left ) { // elroy2_dots < elroy1_dots
                    return set_speed(game_level_spec().elroy2_speed);
                } else if( pellets_left <= game_level_spec().elroy1_dots_left ) {
                    return set_speed(game_level_spec().elroy1_speed);
                }
            }
            return set_speed(game_level_spec().ghost_speed);
            break;
        }
        case mode_t::SCARED:
            return set_speed(game_level_spec().ghost_fright_speed);
            break;
        case mode_t::PHANTOM:
            return set_speed(2.00f);
            break;
        default:
            break;
    }
    return false;
}

void ghost_t::set_mode(const mode_t m, const int mode_ms) noexcept {
    if( m != mode_last ) { // only earmark last other mode
        mode_last = mode_;
    }
    const mode_t old_mode = mode_;
    const int old_mode_ms_left = mode_ms_left;
    mode_ = m;
    mode_ms_left = mode_ms;
    switch( m ) {
        case mode_t::PACMAN_DIED:
            // use global counter
            pellet_counter_active_ = false;
            live_counter_during_pacman_live = 0;
            break;
        case mode_t::AWAY:
            break;
        case mode_t::LEVEL_SETUP:
            pos_ = home_pos;
            pos_.set_aligned_dir(keyframei_);
            dir_ = direction_t::LEFT;
            break;
        case mode_t::START:
            pellet_counter_active_ = true;
            pellet_counter_ = 0;
            live_counter_during_pacman_live = 0;
            break;
        case mode_t::HOME: {
            pos_ = home_pos;
            pos_.set_aligned_dir(keyframei_);
            dir_ = direction_t::LEFT;
            break;
        }
        case mode_t::LEAVE_HOME:
            pellet_counter_active_ = false;
            dir_ = direction_t::LEFT;
            pos_.set_aligned_dir(keyframei_);
            break;
        case mode_t::CHASE:
            // set_global_mode blocked if !is_scattering_or_chasing()
            if( mode_t::LEAVE_HOME == old_mode ) {
                dir_ = direction_t::LEFT;
            } else if( mode_t::SCARED != old_mode ) {
                dir_ = inverse(dir_);
            }
            break;
        case mode_t::SCATTER:
            // set_global_mode blocked if !is_scattering_or_chasing()
            if( mode_t::LEAVE_HOME == old_mode ) {
                dir_ = direction_t::LEFT;
            } else if( mode_t::SCARED != old_mode ) {
                dir_ = inverse(dir_);
            }
            break;
        case mode_t::SCARED: {
            // set_global_mode blocked if !in_house()
            if( mode_t::LEAVE_HOME == old_mode ) {
                // From own tick, reached start pos and using remaining global_mode_ms_left
                dir_ = direction_t::LEFT;
            } else {
                dir_ = inverse(dir_);
            }
            break;
        }
        case mode_t::PHANTOM:
            ++live_counter_during_pacman_live;
            break;
        default:
            log_printf("%s set_global_mode: Error: %s -> %s [%d -> %d ms]; Global_mode %s -> %s [%d ms]\n", to_string(id_).c_str(),
                    to_string(old_mode).c_str(), to_string(m).c_str(), old_mode_ms_left, mode_ms_left,
                    to_string(global_mode_last).c_str(), to_string(global_mode).c_str(), global_mode_ms_left);
            break;
    }
    set_mode_speed();
    set_next_target();
    dir_next = dir_;
    pos_next.set_pos(-1, -1);
    if( log_modes() ) {
        log_printf("%s set_mode: %s -> %s -> %s [%d -> %d ms], speed %5.2f, pos %s -> %s\n", to_string(id_).c_str(),
                to_string(old_mode).c_str(), to_string(m).c_str(), to_string(mode_).c_str(), mode_ms, mode_ms_left,
                current_speed_pct, pos_.toShortString().c_str(), target_.toShortString().c_str());
    }
}

void ghost_t::tick() noexcept {
    atex = &get_tex();
    atex->tick();

    if( sync_next_frame_cntr.count_down() ) {
        return; // skip tick, just repaint
    }

    if( pacman_t::mode_t::FREEZE == pacman->mode() ) {
        return; // NOP
    }

    bool collision_maze = false;

    if( 0 < mode_ms_left ) {
        mode_ms_left = std::max( 0, mode_ms_left - get_ms_per_frame() );
    }

    switch( mode_ ) {
        case mode_t::PACMAN_DIED:
            return; // wait
        case mode_t::AWAY:
            return; // wait
        case mode_t::LEVEL_SETUP:
            return; // wait
        case mode_t::START:
            set_mode( mode_t::HOME );
            return; // wait
        case mode_t::HOME: {
            if( can_leave_home() ) {
                set_mode( mode_t::LEAVE_HOME );
                // move for exit
            } else {
                return; // wait
            }
            break;
        }
        case mode_t::LEAVE_HOME: {
            if( pos_.intersects_f( target_ ) ) {
                if( mode_t::PHANTOM == mode_last && mode_t::SCARED == global_mode ) {
                    // avoid re-materialized ghost to restart SCARED again
                    set_mode( global_mode_last );
                } else {
                    // global_mode_ms_left in case for SCARED time left
                    set_mode( global_mode, global_mode_ms_left );
                }
            } // else move for exit
            break;
        }
        case mode_t::CHASE:
            set_next_target(); // update ...
            break;
        case mode_t::SCATTER:
            break;
        case mode_t::SCARED: {
            if( 0 == mode_ms_left ) {
                set_mode( global_mode );
            } else {
                set_next_target(); // update dummy
            }
            break;
        }
        case mode_t::PHANTOM: {
            if( pos_.intersects_f( target_) ) { // too fuzzy: global_maze->ghost_home_int_box()
                set_mode( mode_t::LEAVE_HOME );
            }
            break;
        }
        default:
            log_printf("Error: tick: %s -> %s [%d ms]; Global_mode %s -> %s [%d ms]\n",
                    to_string(mode_last).c_str(), to_string(mode_).c_str(), mode_ms_left, to_string(global_mode_last).c_str(), to_string(global_mode).c_str(), global_mode_ms_left);
            return;
    }

    collision_maze = !pos_.step(dir_, keyframei_, [&](tile_t tile) -> bool {
        return ( mode_t::LEAVE_HOME == mode_ || mode_t::PHANTOM == mode_ ) ?
               tile_t::WALL == tile : ( tile_t::WALL == tile || tile_t::GATE == tile );
    });
    if( pos_.intersects_i( global_maze->tunnel1_box() ) || pos_.intersects_i( global_maze->tunnel2_box() ) ) {
        set_speed(game_level_spec().ghost_speed_tunnel);
    } else {
        set_mode_speed();
    }
    if( use_decision_one_field_ahead() ) {
        if( pos_.is_center(keyframei_) && pos_.intersects_i(pos_next) ) {
            if( log_moves() ) {
                log_printf("%s tick dir_next: %s -> %s, pos %s, reached %s, coll %d\n",
                        to_string(id_).c_str(), to_string(dir_).c_str(), to_string(dir_next).c_str(),
                        pos_.toShortString().c_str(), pos_next.toShortString().c_str(), collision_maze);
            }
            dir_ = dir_next;
            pos_next.set_pos(-1, -1);
            set_next_dir(collision_maze, true /* center */);
        } else if( collision_maze ) {
            // safeguard against move deadlock, should not happen - could if not centered (was a tunnel issue)
            if( log_moves() ) {
                log_printf("%s tick dir_next: %s -> %s, pos %s, skipped %s, coll %d - collision\n",
                        to_string(id_).c_str(), to_string(dir_).c_str(), to_string(dir_next).c_str(),
                        pos_.toShortString().c_str(), pos_next.toShortString().c_str(), collision_maze);
            }
            pos_next.set_pos(-1, -1);
            set_next_dir(collision_maze, pos_.is_center(keyframei_));
        } else if( pos_next.intersects_i(-1, -1) ) {
            set_next_dir(collision_maze, pos_.is_center(keyframei_));
        }
    }
    if( log_moves() ) {
        if( use_decision_one_field_ahead() ) {
            log_printf("%s tick: %s -> %s, %s [%d ms], pos %s c%d e%d coll %d -> %s -> %s, textures %s\n",
                    to_string(id_).c_str(), to_string(dir_).c_str(), to_string(dir_next).c_str(), to_string(mode_).c_str(), mode_ms_left,
                    pos_.toShortString().c_str(), pos_.is_center(keyframei_), pos_.entered_tile(keyframei_),
                    collision_maze, pos_next.toShortString().c_str(), target_.toShortString().c_str(), atex->toString().c_str());
        } else {
            log_printf("%s tick: %s, %s [%d ms], pos %s c%d e%d, coll %d -> %s, textures %s\n",
                    to_string(id_).c_str(), to_string(dir_).c_str(), to_string(mode_).c_str(), mode_ms_left,
                    pos_.toShortString().c_str(), pos_.is_center(keyframei_), pos_.entered_tile(keyframei_),
                    collision_maze, target_.toShortString().c_str(), atex->toString().c_str());
        }
    }
    if( !use_decision_one_field_ahead() ) {
        set_next_dir(collision_maze, pos_.is_center(keyframei_));
    }
}

void ghost_t::draw(SDL_Renderer* rend) noexcept {
    if( mode_t::AWAY == mode_ ) {
        return;
    }
    if( pos_.intersects_i( pacman->freeze_box() ) ) {
        return;
    }

    atex->draw2(rend, pos_.x_f()-keyframei_.center(), pos_.y_f()-keyframei_.center());

    if( show_debug_gfx() ) {
        if( show_debug_gfx() ) {
            uint8_t r, g, b, a;
            SDL_GetRenderDrawColor(rend, &r, &g, &b, &a);
            const int win_pixel_offset = ( win_pixel_width() - global_maze->pixel_width()*win_pixel_scale() ) / 2;
            SDL_SetRenderDrawColor(rend,
                                   rgb_color[ number( id() ) ][0],
                                   rgb_color[ number( id() ) ][1],
                                   rgb_color[ number( id() ) ][2], 255);
            // pos is on player center position
            SDL_Rect bounds = { .x=win_pixel_offset + round_to_int( pos_.x_f() * global_maze->ppt_y() * win_pixel_scale() ) - ( atex->width()  * win_pixel_scale() ) / 2,
                                .y=                   round_to_int( pos_.y_f() * global_maze->ppt_y() * win_pixel_scale() ) - ( atex->height() * win_pixel_scale() ) / 2,
                                .w=atex->width()*win_pixel_scale(), .h=atex->height()*win_pixel_scale() };
            SDL_RenderDrawRect(rend, &bounds);
            SDL_SetRenderDrawColor(rend, r, g, b, a);
        }
    }
}

//
// ghost_t pellet counter
//
std::string ghost_t::pellet_counter_string() noexcept {
    std::string str = "global_pellet[on "+std::to_string(global_pellet_counter_active)+", ctr "+std::to_string(global_pellet_counter)+"], pellet[";
    for(ghost_ref g : ghosts()) {
        str += to_string(g->id_)+"[on "+std::to_string(g->pellet_counter_active_)+", ctr "+std::to_string(g->pellet_counter_)+"], ";
    }
    str += "]";
    return str;
}

void ghost_t::notify_pellet_eaten() noexcept {
    if( global_pellet_counter_active ) {
        ++global_pellet_counter;
    } else {
        ghost_ref blinky = ghost( personality_t::BLINKY );
        ghost_ref pinky = ghost( personality_t::PINKY );
        ghost_ref inky = ghost( personality_t::INKY );
        ghost_ref clyde = ghost( personality_t::CLYDE );

        // Blinky is always out
        if( nullptr != pinky && pinky->at_home() && pinky->pellet_counter_active_ ) {
            pinky->pellet_counter_++;
        } else if( nullptr != inky && inky->at_home() && inky->pellet_counter_active_ ) {
            inky->pellet_counter_++;
        } else if( nullptr != clyde && clyde->at_home() && clyde->pellet_counter_active_ ) {
            clyde->pellet_counter_++;
        }
        if( nullptr != blinky ) {
            blinky->set_mode_speed(); // in case he shall become Elroy
        }
    }
    global_pellet_time_left = game_level_spec().ghost_max_home_time_ms; // reset
    if( DEBUG_PELLET_COUNTER ) {
        log_printf("%s\n", pellet_counter_string().c_str());
    }
}

int ghost_t::pellet_counter() const noexcept {
    if( pellet_counter_active_ ) {
        return pellet_counter_;
    }
    if( global_pellet_counter_active ) {
        return global_pellet_counter;
    }
    return -1;
}

int ghost_t::pellet_counter_limit() const noexcept {
    if( pellet_counter_active_ ) {
        return game_level_spec().ghost_pellet_counter_limit[ number(id_) ];
    }
    // global
    return global_ghost_pellet_counter_limit[ number(id_) ];
}

bool ghost_t::can_leave_home() noexcept {
    if( at_home() ) {
        if( 0 == global_pellet_time_left ) {
            global_pellet_time_left = game_level_spec().ghost_max_home_time_ms; // reset
            return true;
        }
        if( 0 < live_counter_during_pacman_live ) {
            return true;
        }
        const int counter = pellet_counter();
        const int limit = pellet_counter_limit();
        if( counter >= limit ) {
            if( global_pellet_counter_active && ghost_t::personality_t::CLYDE == id_ ) {
                // re-enable local counter
                global_pellet_counter_active = false;
                global_pellet_counter = 0;
                for(ghost_ref g : ghosts()) {
                    g->pellet_counter_active_ = true;
                }
            }
            return true;
        }
    }
    return false;
}

//
// ghost_t strings
//

std::string ghost_t::toString() const noexcept {
    return to_string(id_)+"["+to_string(mode_)+"["+std::to_string(mode_ms_left)+" ms], "+to_string(dir_)+", "+pos_.toString()+" -> "+target_.toShortString()+", "+atex->toString()+", "+keyframei_.toString()+"]";
}

std::string to_string(ghost_t::personality_t id) noexcept {
    switch( id ) {
        case ghost_t::personality_t::BLINKY:
            return "blinky";
        case ghost_t::personality_t::PINKY:
            return "pinky";
        case ghost_t::personality_t::INKY:
            return "inky";
        case ghost_t::personality_t::CLYDE:
            return "clyde";
        default:
            return "unknown";
    }
}

std::string to_string(ghost_t::mode_t m) noexcept {
    switch( m ) {
        case ghost_t::mode_t::PACMAN_DIED:
            return "pacman_died";
        case ghost_t::mode_t::AWAY:
            return "away";
        case ghost_t::mode_t::LEVEL_SETUP:
            return "level_setup";
        case ghost_t::mode_t::START:
            return "start";
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


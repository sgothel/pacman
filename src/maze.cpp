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
#include <pacman/maze.hpp>
#include <pacman/globals.hpp>

#include <iostream>
#include <fstream>

#include <strings.h>

//
// direction_t
//
std::string to_string(direction_t dir) {
    switch(dir) {
        case direction_t::RIGHT: return "R";
        case direction_t::DOWN: return "D";
        case direction_t::LEFT: return "L";
        case direction_t::UP: return "U";
        default: return "?";
    }
}

direction_t inverse(direction_t dir) {
    switch(dir) {
        case direction_t::RIGHT: return direction_t::LEFT;
        case direction_t::DOWN: return direction_t::UP;
        case direction_t::LEFT: return direction_t::RIGHT;
        case direction_t::UP: return direction_t::DOWN;
        default: return direction_t::DOWN;
    }
}

direction_t rot_left(direction_t dir) {
    switch(dir) {
        case direction_t::RIGHT: return direction_t::UP;
        case direction_t::DOWN: return direction_t::RIGHT;
        case direction_t::LEFT: return direction_t::DOWN;
        case direction_t::UP: return direction_t::LEFT;
        default: return direction_t::LEFT;
    }
}

direction_t rot_right(direction_t dir) {
    switch(dir) {
        case direction_t::RIGHT: return direction_t::DOWN;
        case direction_t::DOWN: return direction_t::LEFT;
        case direction_t::LEFT: return direction_t::UP;
        case direction_t::UP: return direction_t::RIGHT;
        default: return direction_t::RIGHT;
    }
}

//
// tile_t
//
std::string to_string(tile_t tile) {
    switch(tile) {
        case tile_t::EMPTY: return " ";
        case tile_t::PELLET: return ".";
        case tile_t::PELLET_POWER: return "*";
        case tile_t::WALL: return "X";
        case tile_t::GATE: return "-";
        default: return "?";
    }
}

//
// keyframei_t
//
int keyframei_t::calc_odd_frames_per_field(const float frames_per_second, const float fields_per_second, const bool hint_slower) {
    const float v0 = frames_per_second / fields_per_second;
    const int v0_floor = floor_to_int( v0 );
    const int v0_ceil = ceil_to_int( v0 );
    if( 0 != v0_floor % 2 ) {
        // Use faster floor, i.e. resulting in higher fields per second.
        // Difference is 'only' < 1
        return v0_floor;
    } else if( 0 != v0_ceil % 2 ) {
        // Use slower ceil, i.e. resulting in lower fields per second.
        // Difference is 'only' < 1
        return v0_ceil;
    } else {
        // v0's mantissa == 0, difference of one full frame.
        if( hint_slower ) {
            return v0_floor + 1;
        } else {
            return v0_ceil - 1;
        }
    }
}

int keyframei_t::calc_nearest_frames_per_field(const float frames_per_second, const float fields_per_second) {
    const float v0 = frames_per_second / fields_per_second;
    const int v0_floor = floor_to_int( v0 );
    const int v0_ceil = ceil_to_int( v0 );
    if( v0 - v0_floor <= v0_ceil - v0 ) {
        return v0_floor;
    } else {
        return v0_ceil;
    }
}

float keyframei_t::get_sync_delay() const {
    const float frame_per_seconds_diff = 1.0f / get_fields_per_second_diff() * get_frames_per_field();
    return ( 1000.0f / ( frames_per_second_const - frame_per_seconds_diff ) ) - ( 1000.0f / frames_per_second_const ) ;
}

bool keyframei_t::intersects_center(const float x, const float y) const {
    // use epsilon delta to avoid matching direct neighbors
    const float fields_per_frame = get_fields_per_frame();
    const float cx = std::trunc(x) + center;
    const float cy = std::trunc(y) + center;
#if 1
    // uniform dimension
    return std::abs( cx - x ) < fields_per_frame - std::numeric_limits<float>::epsilon() &&
           std::abs( cy - y ) < fields_per_frame - std::numeric_limits<float>::epsilon();
#else
    return !( cx + fields_per_frame - std::numeric_limits<float>::epsilon() < x || x + fields_per_frame - std::numeric_limits<float>::epsilon() < cx ||
              cy + fields_per_frame - std::numeric_limits<float>::epsilon() < y || y + fields_per_frame - std::numeric_limits<float>::epsilon() < cy );
#endif
}

bool keyframei_t::is_center(const float x, const float y) const {
    // use epsilon delta to have tolerance
    const float cx = std::trunc(x) + center;
    const float cy = std::trunc(y) + center;

    return std::abs( cx - x ) <= std::numeric_limits<float>::epsilon() &&
           std::abs( cy - y ) <= std::numeric_limits<float>::epsilon();
}

bool keyframei_t::is_center_xy(const float xy) const {
    const float cx = std::trunc(xy) + center;
    return std::abs( cx - xy ) <= std::numeric_limits<float>::epsilon();
}

bool keyframei_t::is_center_dir(const direction_t dir, const float x, const float y) const {
    // use epsilon delta to have tolerance
    switch( dir ) {
        case direction_t::RIGHT:
            [[fallthrough]];
        case direction_t::LEFT: {
            const float cx = std::trunc(x) + center;
            return std::abs( cx - x ) <= std::numeric_limits<float>::epsilon();
        }
        case direction_t::DOWN:
            [[fallthrough]];
        case direction_t::UP:
            [[fallthrough]];
        default: {
            const float cy = std::trunc(y) + center;
            return std::abs( cy - y ) <= std::numeric_limits<float>::epsilon();
        }
    }
}

bool keyframei_t::entered_tile(const direction_t dir, const float x, const float y) const {
    // FIXME: Wrong code, need to act solely on center tile
    // use epsilon delta to have tolerance
    const float fields_per_frame = get_fields_per_frame();
    switch( dir ) {
        case direction_t::RIGHT: {
            const float v0_trunc = std::trunc(x);
            const float v0_m = x - v0_trunc;
            return v0_m <= std::numeric_limits<float>::epsilon();
        }
        case direction_t::LEFT: {
            const float v0_trunc = std::trunc(x);
            const float v0_m = x - v0_trunc;
            // at the tile edge
            return 1.0f - v0_m <= fields_per_frame + std::numeric_limits<float>::epsilon();
            // return v0_m <= std::numeric_limits<float>::epsilon();
        }
        case direction_t::DOWN: {
            const float v0_trunc = std::trunc(y);
            const float v0_m = y - v0_trunc;
            return v0_m <= std::numeric_limits<float>::epsilon();
        }
        case direction_t::UP:
            [[fallthrough]];
        default: {
            const float v0_trunc = std::trunc(y);
            const float v0_m = y - v0_trunc;
            return 1.0f - v0_m <= fields_per_frame + std::numeric_limits<float>::epsilon();
            // return v0_m <= std::numeric_limits<float>::epsilon();
        }
    }
}

float keyframei_t::get_aligned(const float v) const {
    const float fields_per_frame = get_fields_per_frame();
    const float v0_trunc = std::trunc(v);
    const float v0_m = v - v0_trunc;
    const int n = round_to_int(v0_m / fields_per_frame);
    return v0_trunc + n*fields_per_frame;
}

float keyframei_t::get_centered(const float v) const {
    return std::trunc(v) + center;
}

std::string keyframei_t::toString() const {
    return "[fps "+std::to_string(frames_per_second_const)+", frames_per_field "+std::to_string(frames_per_field)+
            ", fields_per_frame "+std::to_string(get_fields_per_frame())+", fields_per_second "+std::to_string(get_fields_per_second())+
            " (diff "+std::to_string(fields_per_second_diff)+", delay "+std::to_string(get_sync_delay())+"ms), center "+std::to_string(center)+"]";
}

//
// box_t
//
std::string box_t::toString() const {
    return "["+std::to_string(x_pos)+"/"+std::to_string(y_pos)+" "+std::to_string(width)+"x"+std::to_string(height)+"]";
}

//
// acoord_t
//

acoord_t::acoord_t(const int x, const int y)
: x_pos_i(x), y_pos_i(y),
  x_pos_f(x), y_pos_f(y),
  last_dir(direction_t::LEFT),
  last_collided(false),
  fields_walked_i(0),
  fields_walked_f(0)
{}

void acoord_t::reset_stats() {
    fields_walked_i = 0;
    fields_walked_f = 0;
}

void acoord_t::set_pos(const int x, const int y) {
    x_pos_i = x;
    y_pos_i = y;
    x_pos_f = x;
    y_pos_f = y;
    last_dir = direction_t::LEFT;
    last_collided = false;
}

void acoord_t::set_pos_clipped(const float x, const float y) {
    maze_t& maze = *global_maze;
    x_pos_f = maze.clip_pos_x( x );
    y_pos_f = maze.clip_pos_y( y );
    x_pos_i = maze.clip_pos_x( round_to_int(x_pos_f) );
    y_pos_i = maze.clip_pos_y( round_to_int(y_pos_f) );
    last_dir = direction_t::LEFT;
    last_collided = false;
}

bool acoord_t::intersects_f(const acoord_t& other) const {
    // use machine epsilon delta to avoid matching direct neighbors
#if 1
    // uniform dimension
    return std::abs( x_pos_f - other.get_x_f() ) < 1.0f - std::numeric_limits<float>::epsilon() &&
           std::abs( y_pos_f - other.get_y_f() ) < 1.0f - std::numeric_limits<float>::epsilon();
#else
    return !(       x_pos_f + 1.0f - std::numeric_limits<float>::epsilon() < other.x_pos_f ||
              other.x_pos_f + 1.0f - std::numeric_limits<float>::epsilon() <       x_pos_f ||
                    y_pos_f + 1.0f - std::numeric_limits<float>::epsilon() < other.y_pos_f ||
              other.y_pos_f + 1.0f - std::numeric_limits<float>::epsilon() <       y_pos_f );
#endif
}

bool acoord_t::intersects_i(const acoord_t& other) const {
    return x_pos_i == other.x_pos_i && y_pos_i == other.y_pos_i;
}

bool acoord_t::intersects(const acoord_t& other) const {
    return use_original_pacman_behavior() ? intersects_i(other) : intersects_f(other);
}

bool acoord_t::intersects_f(const box_t& other) const {
    return !( x_pos_f + 1.0f - std::numeric_limits<float>::epsilon() < other.get_x() || other.get_x() + other.get_width() < x_pos_f ||
              y_pos_f + 1.0f - std::numeric_limits<float>::epsilon() < other.get_y() || other.get_y() + other.get_height() < y_pos_f );
}

float acoord_t::distance(const float x, const float y) const {
    const float x_d = x - x_pos_f;
    const float y_d = y - y_pos_f;
    return std::sqrt(x_d * x_d + y_d * y_d);
}

float acoord_t::sq_distance(const float x, const float y) const {
    const float x_d = x - x_pos_f;
    const float y_d = y - y_pos_f;
    return x_d * x_d + y_d * y_d;
}

void acoord_t::incr_fwd(const direction_t dir, const keyframei_t& keyframei, const int tile_count) {
    const float fields_per_frame = tile_count;
    maze_t& maze = *global_maze;

    switch( dir ) {
        case direction_t::DOWN:
            if( round_to_int(y_pos_f + fields_per_frame) < maze.get_height() ) {
                y_pos_f = keyframei.get_aligned( y_pos_f + fields_per_frame );
            } else {
                y_pos_f = maze.get_height() - 1; // clip only, no overflow to other side
            }
            y_pos_i = trunc_to_int(y_pos_f);
            x_pos_f = keyframei.get_centered(x_pos_i);
            break;

        case direction_t::RIGHT:
            if( round_to_int(x_pos_f + fields_per_frame) < maze.get_width() ) {
                x_pos_f = keyframei.get_aligned( x_pos_f + fields_per_frame );
            } else {
                x_pos_f = maze.get_width() - 1; // clip only, no overflow to other side
            }
            x_pos_i = trunc_to_int(x_pos_f);
            y_pos_f = keyframei.get_centered(y_pos_i);
            break;

        case direction_t::UP:
            if( round_to_int(y_pos_f - fields_per_frame) >= 0 ) {
                y_pos_f = keyframei.get_aligned( y_pos_f - fields_per_frame );
            } else {
                y_pos_f = 0; // clip only, no overflow to other side
            }
            y_pos_i = trunc_to_int(y_pos_f);
            x_pos_f = keyframei.get_centered(x_pos_i);
            break;

        case direction_t::LEFT:
            [[fallthrough]];
        default:
            if( round_to_int(x_pos_f - fields_per_frame) >= 0 ) {
                x_pos_f = keyframei.get_aligned( x_pos_f - fields_per_frame );
            } else {
                x_pos_f = 0; // clip only, no overflow to other side
            }
            x_pos_i = trunc_to_int(x_pos_f);
            y_pos_f = keyframei.get_centered(y_pos_i);
            break;
    }
}

bool acoord_t::step_impl(direction_t dir, const bool test_only, const keyframei_t& keyframei, collisiontest_simple_t ct0, collisiontest_t ct1) {
    const float fields_per_frame = keyframei.get_fields_per_frame();
    const float center = keyframei.get_center();

    maze_t& maze = *global_maze;

    /**
     * The new float position, pixel accurate.
     */
    float new_x_pos_f;
    float new_y_pos_f;

    /**
     * The new int position.
     */
    int new_x_pos_i;
    int new_y_pos_i;

    /**
     * The forward look-ahead int position for wall collision tests,
     * also avoid overstepping center position if wall is following.
     *
     * Depending on the direction, it adds a whole tile position
     * to the pixel accurate position only to test for wall collisions.
     *
     * This way, it avoids 'overstepping' from its path!
     */
    int fwd_x_pos_i;
    int fwd_y_pos_i;

    /**
     * This step walking distance will be accumulated for statistics.
     */
    float fields_stepped_f = 0;

    /**
     * Resulting int position is be weighted
     * as the original pacman game.
     *
     * Note: This also allows 'cutting the corner' for pacman to speed up.
     */
    switch( dir ) {
        case direction_t::DOWN:
            if( round_to_int(y_pos_f + fields_per_frame) < maze.get_height() ) {
                // next
                new_y_pos_f = keyframei.get_aligned( y_pos_f + fields_per_frame );
                fields_stepped_f = fields_per_frame;

                if( new_y_pos_f - std::trunc(new_y_pos_f) >= center - fields_per_frame/2 ) {
                    new_y_pos_i = trunc_to_int(new_y_pos_f);
                } else {
                    new_y_pos_i = std::min(maze.get_height()-1, trunc_to_int(new_y_pos_f) - 1);
                }
                // forward
                if( new_y_pos_f - std::trunc(new_y_pos_f) > center + fields_per_frame/2 ) {
                    fwd_y_pos_i = std::min(maze.get_height()-1, trunc_to_int(new_y_pos_f) + 1);
                } else {
                    fwd_y_pos_i = trunc_to_int(new_y_pos_f);
                }
            } else {
                new_y_pos_f = 0;
                fwd_y_pos_i = 0;
                new_y_pos_i = 0;
            }
            new_x_pos_f = keyframei.get_centered(x_pos_i);
            new_x_pos_i = x_pos_i;
            fwd_x_pos_i = x_pos_i;
            break;

        case direction_t::RIGHT:
            if( round_to_int(x_pos_f + fields_per_frame) < maze.get_width() ) {
                // next
                new_x_pos_f = keyframei.get_aligned( x_pos_f + fields_per_frame );
                fields_stepped_f = fields_per_frame;
                if( new_x_pos_f - std::trunc(new_x_pos_f) >= center - fields_per_frame/2 ) {
                    new_x_pos_i = trunc_to_int(new_x_pos_f);
                } else {
                    new_x_pos_i = std::min(maze.get_width()-1, trunc_to_int(new_x_pos_f) - 1);
                }
                // forward
                if( new_x_pos_f - std::trunc(new_x_pos_f) > center + fields_per_frame/2 ) {
                    fwd_x_pos_i = std::min(maze.get_width()-1, trunc_to_int(new_x_pos_f) + 1);
                } else {
                    fwd_x_pos_i = trunc_to_int(new_x_pos_f);
                }
            } else {
                new_x_pos_f = 0;
                fwd_x_pos_i = 0;
                new_x_pos_i = 0;
            }
            new_y_pos_f = keyframei.get_centered(y_pos_i);
            new_y_pos_i = y_pos_i;
            fwd_y_pos_i = y_pos_i;
            break;

        case direction_t::UP:
            if( trunc_to_int(y_pos_f - fields_per_frame) >= 0 ) {
                // next
                new_y_pos_f = keyframei.get_aligned( y_pos_f - fields_per_frame );
                fields_stepped_f = fields_per_frame;
                if( new_y_pos_f - std::trunc(new_y_pos_f) < center - fields_per_frame/2 ) {
                    new_y_pos_i = std::max(0, trunc_to_int(new_y_pos_f) - 1);
                } else {
                    new_y_pos_i = trunc_to_int(new_y_pos_f);
                }
                // forward is same
                fwd_y_pos_i = new_y_pos_i;
            } else {
                new_y_pos_f = maze.get_height() - 1;
                fwd_y_pos_i = ceil_to_int(new_y_pos_f);
                new_y_pos_i = fwd_y_pos_i;
            }
            new_x_pos_f = keyframei.get_centered(x_pos_i);
            new_x_pos_i = x_pos_i;
            fwd_x_pos_i = x_pos_i;
            break;

        case direction_t::LEFT:
            [[fallthrough]];
        default:
            if( trunc_to_int(x_pos_f - fields_per_frame) >= 0 ) {
                // next
                new_x_pos_f = keyframei.get_aligned( x_pos_f - fields_per_frame );
                fields_stepped_f = fields_per_frame;
                if( new_x_pos_f - std::trunc(new_x_pos_f) < center - fields_per_frame/2 ) {
                    new_x_pos_i = std::max(0, trunc_to_int(new_x_pos_f) - 1);
                } else {
                    new_x_pos_i = trunc_to_int(new_x_pos_f);
                }
                // forward is same
                fwd_x_pos_i = new_x_pos_i;
            } else {
                new_x_pos_f = maze.get_width() - 1;
                fwd_x_pos_i = ceil_to_int(new_x_pos_f);
                new_x_pos_i = fwd_x_pos_i;
            }
            new_y_pos_f = keyframei.get_centered(y_pos_i);
            new_y_pos_i = y_pos_i;
            fwd_y_pos_i = y_pos_i;
            break;

    }
    // Collision test with walls
    const tile_t fwd_tile = maze.get_tile(fwd_x_pos_i, fwd_y_pos_i);
    const bool new_pos_is_centered = keyframei.is_center_dir(dir, new_x_pos_f, new_y_pos_f);
    bool collision;
    if( nullptr != ct0 ) {
        collision = ct0(fwd_tile);
    } else if( nullptr != ct1 ) {
        collision = ct1(dir, new_x_pos_f, new_y_pos_f, new_pos_is_centered,
                             fwd_x_pos_i, fwd_y_pos_i, fwd_tile);
    } else {
        collision = false;
    }
    if( !test_only && DEBUG_BOUNDS ) {
        log_printf("%s: %s -> %s: %9.6f/%9.6f %2.2d/%2.2d c%d e%d -> new %9.6f/%9.6f %2.2d/%2.2d c%d e%d -> fwd %2.2d/%2.2d, tile '%s', collision %d\n",
                test_only ? "test" : "step",
                to_string(last_dir).c_str(), to_string(dir).c_str(),
                x_pos_f, y_pos_f, x_pos_i, y_pos_i, keyframei.is_center_dir(last_dir, x_pos_f, y_pos_f), keyframei.entered_tile(last_dir, x_pos_f, y_pos_f),
                new_x_pos_f, new_y_pos_f, new_x_pos_i, new_y_pos_i, new_pos_is_centered, keyframei.entered_tile(dir, new_x_pos_f, new_y_pos_f),
                fwd_x_pos_i, fwd_y_pos_i,
                to_string(fwd_tile).c_str(), collision);
    }
    if( !test_only ) {
        if( !collision ) {
            last_collided = false;
            x_pos_f = new_x_pos_f;
            y_pos_f = new_y_pos_f;
            const int x_pos_i_old = x_pos_i;
            const int y_pos_i_old = y_pos_i;
            x_pos_i = new_x_pos_i;
            y_pos_i = new_y_pos_i;
            last_dir = dir;
            fields_walked_i += std::abs(x_pos_i - x_pos_i_old) + std::abs(y_pos_i - y_pos_i_old);
            fields_walked_f += fields_stepped_f;
        } else {
            last_collided = true;
        }
    }
    return !collision;
}

std::string acoord_t::toString() const {
    return "["+std::to_string(x_pos_f)+"/"+std::to_string(y_pos_f)+" "+std::to_string(x_pos_i)+"/"+std::to_string(y_pos_i)+
            ", last[dir "+to_string(last_dir)+", collided "+std::to_string(last_collided)+"], walked["+std::to_string(fields_walked_f)+", "+std::to_string(fields_walked_i)+"]]";
}

std::string acoord_t::toShortString() const {
    return "["+std::to_string(x_pos_f)+"/"+std::to_string(y_pos_f)+" "+std::to_string(x_pos_i)+"/"+std::to_string(y_pos_i)+"]";
}

//
// maze_t::field_t
//

maze_t::field_t::field_t()
: width(0), height(0)
{
    bzero(&count, sizeof(count));
}

void maze_t::field_t::clear() {
    width = 0; height = 0;
    tiles.clear();
    bzero(&count, sizeof(count));
}

tile_t maze_t::field_t::get_tile(const int x, const int y) const {
    if( 0 <= x && x < width && 0 <= y && y < height ) {
        return tiles[y*width+x];
    }
    return tile_t::EMPTY;
}

void maze_t::field_t::add_tile(const tile_t tile) {
    tiles.push_back(tile);
    ++count[number(tile)];
}

void maze_t::field_t::set_tile(const int x, const int y, tile_t tile) {
    if( 0 <= x && x < width && 0 <= y && y < height ) {
        const tile_t old_tile = tiles[y*width+x];
        tiles[y*width+x] = tile;
        --count[number(old_tile)];
        ++count[number(tile)];
    }
}

std::string maze_t::field_t::toString() const {
    return "field["+std::to_string(width)+"x"+std::to_string(height)+", pellets["+std::to_string(get_count(tile_t::PELLET))+", power "+std::to_string(get_count(tile_t::PELLET_POWER))+"]]";
}

//
// maze_t
//

bool maze_t::digest_position_line(const std::string& name, acoord_t& dest, const std::string& line) {
    if( -1 == dest.get_x_i() || -1 == dest.get_y_i() ) {
        int x_pos = 0, y_pos = 0;
        sscanf(line.c_str(), "%d %d", &x_pos, &y_pos);
        dest.set_pos(x_pos, y_pos);
        if( DEBUG ) {
            log_printf("maze: read %s position: %s\n", name.c_str(), dest.toString().c_str());
        }
        return true;
    } else {
        return false;
    }
}

bool maze_t::digest_box_line(const std::string& name, box_t& dest, const std::string& line) {
    if( -1 == dest.get_x() || -1 == dest.get_y() ) {
        int x_pos = 0, y_pos = 0, w = 0, h = 0;
        sscanf(line.c_str(), "%d %d %d %d", &x_pos, &y_pos, &w, &h);
        dest.set_dim(x_pos, y_pos, w, h);
        if( DEBUG ) {
            log_printf("maze: read %s box: %s\n", name.c_str(), dest.toString().c_str());
        }
        return true;
    } else {
        return false;
    }
}

maze_t::maze_t(const std::string& fname)
: filename(fname),
  top_left_pos(-1, -1),
  bottom_left_pos(-1, -1),
  bottom_right_pos(-1, -1),
  top_right_pos(-1, -1),
  pacman_start_pos(-1, -1),
  ghost_home(-1, -1, -1, -1),
  ghost_home_pos(-1, -1),
  ghost_start_pos(-1, -1),
  ppt_x( -1 ),
  ppt_y( -1 )
{
    int field_line_iter = 0;
    std::fstream file;
    file.open(fname, std::ios::in);
    if( file.is_open() ) {
        std::string line;
        while( std::getline(file, line) ) {
            if( 0 == original.get_width() || 0 == original.get_height() ) {
                int w=-1, h=-1;
                int visual_width=-1, visual_height=-1;
                sscanf(line.c_str(), "%d %d %d %d", &w, &h, &visual_width, &visual_height);
                original.set_dim(w, h);
                ppt_x = visual_width / original.get_width();
                ppt_y = visual_height / original.get_height();
                if( DEBUG ) {
                    log_printf("maze: read dimension: %s\n", toString().c_str());
                }
            } else if( digest_position_line("top_left_pos", top_left_pos, line) ) {
            } else if( digest_position_line("bottom_left_pos", bottom_left_pos, line) ) {
            } else if( digest_position_line("bottom_right_pos", bottom_right_pos, line) ) {
            } else if( digest_position_line("top_right_pos", top_right_pos, line) ) {
            } else if( digest_position_line("pacman", pacman_start_pos, line) ) {
            } else if( digest_box_line("ghost_home", ghost_home, line) ) {
            } else if( digest_position_line("ghost_home", ghost_home_pos, line) ) {
            } else if( digest_position_line("ghost_start", ghost_start_pos, line) ) {
            } else if( 0 == texture_file.length() ) {
                texture_file = line;
            } else if( field_line_iter < original.get_height() ) {
                if( DEBUG ) {
                    log_printf("maze: read line y = %d, len = %zd: %s\n", field_line_iter, line.length(), line.c_str());
                }
                if( line.length() == (size_t)original.get_width() ) {
                    for(int x=0; x<original.get_width(); ++x) {
                        const char c = line[x];
                        switch( c ) {
                            case '_':
                                original.add_tile(tile_t::EMPTY);
                                break;
                            case '|':
                                original.add_tile(tile_t::WALL);
                                break;
                            case '-':
                                original.add_tile(tile_t::GATE);
                                break;
                            case '.':
                                original.add_tile(tile_t::PELLET);
                                break;
                            case '*':
                                original.add_tile(tile_t::PELLET_POWER);
                                break;
                            default:
                                log_printf("maze error: unknown tile @ %d / %d: '%c'\n", x, field_line_iter, c);
                                break;
                        }
                    }
                }
                ++field_line_iter;
            }
        }
        file.close();
        if( original.validate_size() ) {
            reset();
            return; // OK
        }
    } else {
        log_printf("Could not open maze file: %s\n", filename.c_str());
    }
    original.clear();
    pacman_start_pos.set_pos(0, 0);
    ghost_home_pos.set_pos(0, 0);
    ghost_start_pos.set_pos(0, 0);
    ppt_x = 0;
    ppt_y = 0;
}

void maze_t::draw(std::function<void(const int x, const int y, tile_t tile)> draw_pixel) {
    for(int y=0; y<get_height(); ++y) {
        for(int x=0; x<get_width(); ++x) {
            draw_pixel(x, y, active.get_tile_nc(x, y));
        }
    }
}

void maze_t::reset() {
    active = original;
}

std::string maze_t::toString() const {
    std::string errstr = is_ok() ? "ok" : "error";
    return filename+"["+errstr+", "+active.toString()+
                    ", pacman "+pacman_start_pos.toShortString()+
                    ", ghost[box "+ghost_home.toString()+", home "+ghost_home_pos.toShortString()+
                    ", start "+ghost_start_pos.toShortString()+
                    "], tex "+texture_file+
                    ", ppt "+std::to_string(ppt_x)+"x"+std::to_string(ppt_y)+
                    "]";
}

/*
 * Author: Sven Gothel <sgothel@jausoft.com>
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

#include <string>
#include <cstdio>
#include <cinttypes>
#include <cmath>

#include <ctime>

static constexpr const uint64_t NanoPerMilli = 1000000UL;
static constexpr const uint64_t MilliPerOne = 1000UL;

//
// Cut from jaulib
//
#include <type_traits>

template <typename T>
constexpr ssize_t sign(const T x) noexcept
{
    return (T(0) < x) - (x < T(0));
}

template <typename T>
constexpr T invert_sign(const T x) noexcept
{
    return std::numeric_limits<T>::min() == x ? std::numeric_limits<T>::max() : -x;
}

template<typename T>
constexpr size_t digits10(const T x, const ssize_t x_sign, const bool sign_is_digit=true) noexcept
{
    if( x_sign == 0 ) {
        return 1;
    }
    if( x_sign < 0 ) {
        return 1 + static_cast<size_t>( std::log10<T>( invert_sign<T>( x ) ) ) + ( sign_is_digit ? 1 : 0 );
    } else {
        return 1 + static_cast<size_t>( std::log10<T>(                 x   ) );
    }
}

template< class value_type,
          std::enable_if_t< std::is_integral_v<value_type>,
                            bool> = true>
std::string to_decstring(const value_type& v, const char separator=',', const size_t width=0) noexcept {
    const ssize_t v_sign = sign<value_type>(v);
    const size_t digit10_count1 = digits10<value_type>(v, v_sign, true /* sign_is_digit */);
    const size_t digit10_count2 = v_sign < 0 ? digit10_count1 - 1 : digit10_count1; // less sign

    const size_t comma_count = 0 == separator ? 0 : ( digit10_count1 - 1 ) / 3;
    const size_t net_chars = digit10_count1 + comma_count;
    const size_t total_chars = std::max<size_t>(width, net_chars);
    std::string res(total_chars, ' ');

    value_type n = v;
    size_t char_iter = 0;

    for(size_t digit10_iter = 0; digit10_iter < digit10_count2 /* && char_iter < total_chars */; digit10_iter++ ) {
        const int digit = v_sign < 0 ? invert_sign( n % 10 ) : n % 10;
        n /= 10;
        if( 0 < digit10_iter && 0 == digit10_iter % 3 ) {
            res[total_chars-1-(char_iter++)] = separator;
        }
        res[total_chars-1-(char_iter++)] = '0' + digit;
    }
    if( v_sign < 0 /* && char_iter < total_chars */ ) {
        res[total_chars-1-(char_iter++)] = '-';
    }
    return res;
}


//
// utils.hpp
//

/**
 * See <http://man7.org/linux/man-pages/man2/clock_gettime.2.html>
 * <p>
 * Regarding avoiding kernel via VDSO,
 * see <http://man7.org/linux/man-pages/man7/vdso.7.html>,
 * clock_gettime seems to be well supported at least on kernel >= 4.4.
 * Only bfin and sh are missing, while ia64 seems to be complicated.
 */
uint64_t getCurrentMilliseconds() noexcept {
    struct timespec t;
    clock_gettime(CLOCK_MONOTONIC, &t);
    return static_cast<uint64_t>( t.tv_sec ) * MilliPerOne +
           static_cast<uint64_t>( t.tv_nsec ) / NanoPerMilli;
}

static uint64_t _exe_start_time = getCurrentMilliseconds();

uint64_t getElapsedMillisecond() noexcept {
    return getCurrentMilliseconds() - _exe_start_time;
}

float get_fps(const uint64_t t0, const uint64_t t1, const float event_count) {
    const uint64_t td_ms = t1 - t0;
    float fps;
    if( td_ms > 0 ) {
        fps = ( event_count / (float)td_ms ) * 1000.0;
    } else {
        fps = 0;
    }
    return fps;
}

void log_printf(const char * format, ...) noexcept {
    fprintf(stderr, "[%s] ", to_decstring(getElapsedMillisecond(), ',', 9).c_str());
    va_list args;
    va_start (args, format);
    vfprintf(stderr, format, args);
    va_end (args);
}

//
// direction_t
//
std::string to_string(direction_t dir) noexcept {
    switch(dir) {
        case direction_t::RIGHT: return "R";
        case direction_t::DOWN: return "D";
        case direction_t::LEFT: return "L";
        case direction_t::UP: return "U";
        default: return "?";
    }
}

direction_t inverse(direction_t dir) noexcept {
    switch(dir) {
        case direction_t::RIGHT: return direction_t::LEFT;
        case direction_t::DOWN: return direction_t::UP;
        case direction_t::LEFT: return direction_t::RIGHT;
        case direction_t::UP: return direction_t::DOWN;
        default: return direction_t::DOWN;
    }
}

direction_t rot_left(direction_t dir) noexcept {
    switch(dir) {
        case direction_t::RIGHT: return direction_t::UP;
        case direction_t::DOWN: return direction_t::RIGHT;
        case direction_t::LEFT: return direction_t::DOWN;
        case direction_t::UP: return direction_t::LEFT;
        default: return direction_t::LEFT;
    }
}

direction_t rot_right(direction_t dir) noexcept {
    switch(dir) {
        case direction_t::RIGHT: return direction_t::DOWN;
        case direction_t::DOWN: return direction_t::LEFT;
        case direction_t::LEFT: return direction_t::UP;
        case direction_t::UP: return direction_t::RIGHT;
        default: return direction_t::RIGHT;
    }
}

//
// keyframei_t
//
int keyframei_t::calc_odd_frames_per_field(const float frames_per_second, const float fields_per_second) noexcept {
    const float v0 = frames_per_second / fields_per_second;
    const int v0_floor = floor_to_int( v0 );
    if( 0 != v0_floor % 2 ) {
        // Use faster floor, i.e. resulting in higher fields per second.
        // Difference is 'only' < 1
        return v0_floor;
    } else {
        // Avoid slower ceiling, i.e. would result in lower fields per second.
        return v0_floor - 1; // faster
    }
}

int keyframei_t::calc_nearest_frames_per_field(const float frames_per_second, const float fields_per_second) noexcept {
    const float v0 = frames_per_second / fields_per_second;
    const int v0_floor = floor_to_int( v0 );
    return v0_floor;
}

int keyframei_t::sync_frame_count() const noexcept {
    const float fps_d = frames_per_second_diff();
    if( fps_d > std::numeric_limits<float>::epsilon() ) {
        return round_to_int( frames_per_second() / fps_d );
    } else {
        return -1;
    }
}

float keyframei_t::sync_delay() const noexcept {
    return ( 1000.0f / ( frames_per_second() - frames_per_second_diff() ) ) - ( 1000.0f / frames_per_second() ) ;
}

bool keyframei_t::intersects_center(const float x, const float y) const noexcept {
    // use epsilon delta to avoid matching direct neighbors
    const float fields_per_frame_ = fields_per_frame();
    const float cx = std::trunc(x) + center_;
    const float cy = std::trunc(y) + center_;
#if 1
    // uniform dimension
    return std::abs( cx - x ) < fields_per_frame_ - std::numeric_limits<float>::epsilon() &&
           std::abs( cy - y ) < fields_per_frame_ - std::numeric_limits<float>::epsilon();
#else
    return !( cx + fields_per_frame_ - std::numeric_limits<float>::epsilon() < x || x + fields_per_frame - std::numeric_limits<float>::epsilon() < cx ||
              cy + fields_per_frame_ - std::numeric_limits<float>::epsilon() < y || y + fields_per_frame - std::numeric_limits<float>::epsilon() < cy );
#endif
}

bool keyframei_t::is_center(const float x, const float y) const noexcept {
    // use epsilon delta to have tolerance
    const float cx = std::trunc(x) + center_;
    const float cy = std::trunc(y) + center_;

    return std::abs( cx - x ) <= std::numeric_limits<float>::epsilon() &&
           std::abs( cy - y ) <= std::numeric_limits<float>::epsilon();
}

bool keyframei_t::is_center_xy(const float xy) const noexcept {
    const float cx = std::trunc(xy) + center_;
    return std::abs( cx - xy ) <= std::numeric_limits<float>::epsilon();
}

bool keyframei_t::is_center_dir(const direction_t dir, const float x, const float y) const noexcept {
    // use epsilon delta to have tolerance
    switch( dir ) {
        case direction_t::RIGHT:
            [[fallthrough]];
        case direction_t::LEFT: {
            const float cx = std::trunc(x) + center_;
            return std::abs( cx - x ) <= std::numeric_limits<float>::epsilon();
        }
        case direction_t::DOWN:
            [[fallthrough]];
        case direction_t::UP:
            [[fallthrough]];
        default: {
            const float cy = std::trunc(y) + center_;
            return std::abs( cy - y ) <= std::numeric_limits<float>::epsilon();
        }
    }
}

bool keyframei_t::field_entered(const direction_t dir, const float x, const float y) const noexcept {
    // FIXME: Wrong code, need to act solely on center tile
    // use epsilon delta to have tolerance
    const float fields_per_frame_ = fields_per_frame();
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
            return 1.0f - v0_m <= fields_per_frame_ + std::numeric_limits<float>::epsilon();
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
            return 1.0f - v0_m <= fields_per_frame_ + std::numeric_limits<float>::epsilon();
            // return v0_m <= std::numeric_limits<float>::epsilon();
        }
    }
}

float keyframei_t::align_value(const float v) const noexcept {
    const float fields_per_frame_ = fields_per_frame();
    const float v0_trunc = std::trunc(v);
    const float v0_m = v - v0_trunc;
    const int n = round_to_int(v0_m / fields_per_frame_);
    return v0_trunc + n*fields_per_frame_;
}

float keyframei_t::center_value(const float v) const noexcept {
    return std::trunc(v) + center_;
}

std::string keyframei_t::toString() const noexcept {
    return "[fps "+std::to_string(frames_per_second_const_)+", frames "+std::to_string(frames_per_field_)+
            "/field, fields "+std::to_string(fields_per_second())+
            "/s (diff "+std::to_string(fields_per_second_diff_)+", "+std::to_string(frames_per_second_diff())+
            "f/s, "+std::to_string(sync_delay())+"ms, sync "+std::to_string(sync_frame_count())+"/f), center "+std::to_string(center_)+"]";
}

//
// box_t
//
std::string box_t::toString() const noexcept {
    return "["+std::to_string(x_)+"/"+std::to_string(y_)+" "+std::to_string(w_)+"x"+std::to_string(h_)+"]";
}

//
// countdown_t
//
bool countdown_t::count_down() noexcept {
    if( 0 == counter_ ) {
        return false;
    }
    const bool r = 0 == --counter_;
    if( r ) {
        ++events_;
        if( 0 < reload_value_ ) {
            counter_ = reload_value_;
        }
    }
    return r;
}

std::string countdown_t::toString() const noexcept {
    return "["+std::to_string(counter_)+"/"+std::to_string(reload_value_)+", events "+std::to_string(events_)+"]";
}



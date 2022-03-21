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




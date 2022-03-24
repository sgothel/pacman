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
#ifndef PACMAN_UTILS_HPP_
#define PACMAN_UTILS_HPP_

#include <string>
#include <cstdint>
#include <cstdarg>
#include <cmath>

/**
 * See <http://man7.org/linux/man-pages/man2/clock_gettime.2.html>
 * <p>
 * Regarding avoiding kernel via VDSO,
 * see <http://man7.org/linux/man-pages/man7/vdso.7.html>,
 * clock_gettime seems to be well supported at least on kernel >= 4.4.
 * Only bfin and sh are missing, while ia64 seems to be complicated.
 */
uint64_t getCurrentMilliseconds() noexcept;

uint64_t getElapsedMillisecond() noexcept;

float get_fps(const uint64_t t0, const uint64_t t1, const float event_count);

void log_printf(const char * format, ...) noexcept;

//
// misc math
//

inline constexpr int trunc_to_int(const float f) {
    return (int)std::trunc(f);
}
inline constexpr int round_to_int(const float f) {
    return (int)std::round(f);
}
inline constexpr int floor_to_int(const float f) {
    return (int)std::floor(f);
}
inline constexpr int ceil_to_int(const float f) {
    return (int)std::ceil(f);
}
inline constexpr bool is_equal(const float a, const float b) {
    return std::abs(a - b) < std::numeric_limits<float>::epsilon();
}

//
// direction_t
//

/**
 * Generic relative direction.
 */
enum class direction_t : int {
    RIGHT = 0,
    DOWN = 1,
    LEFT = 2,
    UP = 3
};
constexpr int number(const direction_t d) noexcept {
    return static_cast<int>(d);
}
std::string to_string(direction_t dir) noexcept;
direction_t inverse(direction_t dir) noexcept;
direction_t rot_left(direction_t dir) noexcept;
direction_t rot_right(direction_t dir) noexcept;

//
// keyframei_t
//

/**
 * Animation key frame interval.
 */
class keyframei_t {
    private:
        float frames_per_second_const_;

        /** frames_per_field, dividing the field in sub-fields. */
        int frames_per_field_;

        /**
         * Center x/y sub-field position of square field, i.e. fields_per_frame * frames_per_field/2
         *
         * Odd frames_per_field int-value gets truncated by div_2, matching the 0-based sub-field index.
         */
        float center_;

        /** fields_per_second difference requested - has */
        float fields_per_second_diff_;

        /**
         * Return an odd frames_per_field, which divides the field in sub-fields with a deterministic single center position.
         */
        static int calc_odd_frames_per_field(const float frames_per_second, const float fields_per_second, const bool hint_slower) noexcept;

        /**
         * Returns the closest frames_per_field, which can be odd or even.
         * If even, it can't be used for a single center position!
         */
        static int calc_nearest_frames_per_field(const float frames_per_second, const float fields_per_second) noexcept;

    public:

        /**
         * Constructs a keyframei_t instance.
         * @param force_odd if true enforces an odd frames_per_field, which enables a single center position, otherwise the nearest is chosen.
         * @param frames_per_second renderer frames per seconds
         * @param fields_per_second_req desired fields per second 'moving' speed
         * @param hint_slower if true (default) and force_odd is also true, the higher fields_per_second value is chosen for slower speed, otherwise the opposite.
         */
        keyframei_t(const bool force_odd, const float frames_per_second, float fields_per_second_req, const bool hint_slower=true) noexcept
        : frames_per_second_const_(frames_per_second),
          frames_per_field_( force_odd ? calc_odd_frames_per_field(frames_per_second_const_, fields_per_second_req, hint_slower) :
                                        calc_nearest_frames_per_field(frames_per_second_const_, fields_per_second_req) ),
          center_( fields_per_frame() * ( frames_per_field_ / 2 ) ),
          fields_per_second_diff_( fields_per_second() - fields_per_second_req )
        { }

        /**
         * Recalculates this keyframei_t instance's values
         * @param force_odd if true enforces an odd frames_per_field, which enables a single center position, otherwise the nearest is chosen.
         * @param frames_per_second renderer frames per seconds
         * @param fields_per_second_req desired fields per second 'moving' speed
         * @param hint_slower if true (default) and force_odd is also true, the higher fields_per_second value is chosen for slower speed, otherwise the opposite.
         */
        void reset(const bool force_odd, const float frames_per_second, const float fields_per_second_req, const bool hint_slower=true) noexcept {
            frames_per_second_const_ = frames_per_second;
            frames_per_field_ = force_odd ? calc_odd_frames_per_field(frames_per_second_const_, fields_per_second_req, hint_slower) :
                                           calc_nearest_frames_per_field(frames_per_second_const_, fields_per_second_req);
            center_ = fields_per_frame() * ( frames_per_field_ / 2 );
            fields_per_second_diff_ = fields_per_second() - fields_per_second_req;
        }

        /** Return the renderer frames per second constant */
        constexpr int frames_per_second() const noexcept { return frames_per_second_const_; }

        /** Return the number of frames per field, dividing the field in sub-fields. */
        constexpr int  frames_per_field() const noexcept { return frames_per_field_; }

        /** Return count of fields per frame, also the sub-field width or height, 1 / get_frames_per_field(). */
        constexpr float fields_per_frame() const noexcept { return 1.0f / frames_per_field_; }

        /** Returns the fields per second actual 'moving' speed, i.e. get_frames_per_second() / get_frames_per_field() */
        constexpr float fields_per_second() const noexcept { return frames_per_second_const_ / frames_per_field_; }

        /**
         * Return fields_per_second difference `actual - requested`, i.e. negative if slower - and positive if faster frame rendering (fps) speed than animation.
         *
         * @see get_frames_per_second_diff()
         * @see get_sync_frame_count()
         */
        constexpr float fields_per_second_diff() const noexcept { return fields_per_second_diff_; }

        /**
         * Return delay in frames to sync for get_fields_per_second_diff() compensation, i.e. via repainting same frame.
         *
         * If positive, frame rendering (fps) is faster than animation and caller may sync via animation delay, see get_sync_frame_count().
         * Otherwise value is negative, indicating slower frame rendeinf (fps) speed than animation.
         *
         * @see get_fields_per_second_diff()
         * @see get_sync_frame_count()
         */
        constexpr float frames_per_second_diff() const noexcept {
            return fields_per_second_diff() * frames_per_field();
        }

        /**
         * Returns a positive frame_count to sync animation if frame rendering (fps) is faster than desired animation speed.
         * Caller may insert one repaint w/o animation-tick every returned sync frame_count.
         *
         * Otherwise returns -1 if frame rendering (fps) is slower than desired animation speed.
         *
         * @see get_fields_per_second_diff()
         * @see get_frames_per_second_diff()
         */
        int sync_frame_count() const noexcept;

        /**
         * Return required delay in milliseconds to sync for get_fields_per_second_diff() compensation.
         *
         * If positive, get_fields_per_second_diff() was positive hence actual faster stepping speed and the value should be used to sync delay.
         * Otherwise value is negative, indicating slower stepping speed.
         */
        float sync_delay() const noexcept;

        /**
         * If true, this instances uses odd frames_per_field, hence allowing a single center sub-field.
         * Otherwise the center sub-field might be 'walked' twice.
         */
        constexpr bool uses_odd_frames_per_field() const noexcept { return 0 != frames_per_field_ / 2; }

        /**
         * Return center x/y sub-field position of square field, i.e. fields_per_frame * frames_per_field/2.
         *
         * Odd frames_per_field int-value gets truncated by div_2, matching the 0-based center sub-field index.
         *
         * The center sub-field is defined by `{ .x = center, .y = center, .width = fields_per_frame, .height = fields_per_frame }`.
         **/
        constexpr float center() const noexcept { return center_; }

        /**
         * Returns true if position lies within the center sub-field
         * as defined by `{ .x = center, .y = center, .width = fields_per_frame, .height = fields_per_frame }`.
         */
        bool intersects_center(const float x, const float y) const noexcept;

        /**
         * Returns true if position is exactly on the center sub-field within machine epsilon.
         */
        bool is_center(const float x, const float y) const noexcept;

        /**
         * Returns true if position component is exactly on the center sub-field within machine epsilon.
         */
        bool is_center_xy(const float xy) const noexcept;

        /**
         * Returns true if position in direction is exactly on the center sub-field within machine epsilon.
         */
        bool is_center_dir(const direction_t dir, const float x, const float y) const noexcept;

        bool field_entered(const direction_t dir, const float x, const float y) const noexcept;

        float align_value(const float v) const noexcept;
        float center_valued(const float v) const noexcept;

        std::string toString() const noexcept;
};

//
// box_t
//
class box_t {
    private:
        int x_;
        int y_;
        int w_;
        int h_;

    public:
        box_t(const int x, const int y, const int w, const int h) noexcept
        : x_(x), y_(y), w_(w), h_(h) {}

        constexpr void set_dim(const int x, const int y, const int w, const int h) noexcept {
            x_ = x; y_ = y; w_ = w; h_ = h;
        }
        constexpr int x() const noexcept { return x_; }
        constexpr int y() const noexcept { return y_; }
        constexpr int width() const noexcept { return w_; }
        constexpr int height() const noexcept { return h_; }

        std::string toString() const noexcept;
};

/**
 * A non thread safe latch-type counter to count down.
 */
class countdown_t {
    private:
        size_t reload_value_;
        size_t counter_;
        size_t events_;

    public:
        countdown_t(const size_t value, const bool auto_reload) noexcept
        : reload_value_( auto_reload ? value : 0 ), counter_( value ), events_(0) { }

        /**
         * Reset this instance as done via constructor.
         *
         * If clear_events := true (default), the events() count is also set to zero.
         */
        void reset(const size_t value, const bool auto_reload, const bool clear_events=true) noexcept {
            reload_value_ = auto_reload ? value : 0;
            counter_ = value;
            if( clear_events ) {
                events_ = 0;
            }
        }

        constexpr size_t counter() const noexcept { return counter_; }

        constexpr size_t events() const noexcept { return events_; }

        void clear_events() noexcept { events_ = 0; }

        /**
         * Count down counter and increase events() count by one.
         * Also reload the counter if auto_reload := true has been passed in constructor.
         *
         * If counter() is zero, do nothing and return false.
         *
         * @return true if an event has been reached, i.e. counter has reached zero,
         * otherwise return false.
         */
        bool count_down() noexcept;

        /**
         * Manually reload counter with given value.
         * This is intended if passing auto_reload := false in constructor.
         */
        void load(const size_t value) { counter_ = value; }

        std::string toString() const noexcept;
};

#endif /* PACMAN_UTILS_HPP_ */

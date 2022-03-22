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
std::string to_string(direction_t dir);
direction_t inverse(direction_t dir);
direction_t rot_left(direction_t dir);
direction_t rot_right(direction_t dir);

//
// keyframei_t
//

/**
 * Animation key frame interval.
 */
class keyframei_t {
    private:
        float frames_per_second_const;

        /** frames_per_field, dividing the field in sub-fields. */
        int frames_per_field;

        /**
         * Center x/y sub-field position of square field, i.e. fields_per_frame * frames_per_field/2
         *
         * Odd frames_per_field int-value gets truncated by div_2, matching the 0-based sub-field index.
         */
        float center;

        /** fields_per_second difference requested - has */
        float fields_per_second_diff;

        /**
         * Return an odd frames_per_field, which divides the field in sub-fields with a deterministic single center position.
         */
        static int calc_odd_frames_per_field(const float frames_per_second, const float fields_per_second, const bool hint_slower);

        /**
         * Returns the closest frames_per_field, which can be odd or even.
         * If even, it can't be used for a single center position!
         */
        static int calc_nearest_frames_per_field(const float frames_per_second, const float fields_per_second);

    public:

        /**
         * Constructs a keyframei_t instance.
         * @param force_odd if true enforces an odd frames_per_field, which enables a single center position, otherwise the nearest is chosen.
         * @param frames_per_second renderer frames per seconds
         * @param fields_per_second_req desired fields per second 'moving' speed
         * @param hint_slower if true (default) and force_odd is also true, the higher fields_per_second value is chosen for slower speed, otherwise the opposite.
         */
        keyframei_t(const bool force_odd, const float frames_per_second, float fields_per_second_req, const bool hint_slower=true)
        : frames_per_second_const(frames_per_second),
          frames_per_field( force_odd ? calc_odd_frames_per_field(frames_per_second_const, fields_per_second_req, hint_slower) :
                                        calc_nearest_frames_per_field(frames_per_second_const, fields_per_second_req) ),
          center( get_fields_per_frame() * ( frames_per_field / 2 ) ),
          fields_per_second_diff( get_fields_per_second() - fields_per_second_req )
        { }

        /**
         * Recalculates this keyframei_t instance's values
         * @param force_odd if true enforces an odd frames_per_field, which enables a single center position, otherwise the nearest is chosen.
         * @param frames_per_second renderer frames per seconds
         * @param fields_per_second_req desired fields per second 'moving' speed
         * @param hint_slower if true (default) and force_odd is also true, the higher fields_per_second value is chosen for slower speed, otherwise the opposite.
         */
        void reset(const bool force_odd, const float frames_per_second, const float fields_per_second_req, const bool hint_slower=true) {
            frames_per_second_const = frames_per_second;
            frames_per_field = force_odd ? calc_odd_frames_per_field(frames_per_second_const, fields_per_second_req, hint_slower) :
                                           calc_nearest_frames_per_field(frames_per_second_const, fields_per_second_req);
            center = get_fields_per_frame() * ( frames_per_field / 2 );
            fields_per_second_diff = get_fields_per_second() - fields_per_second_req;
        }

        /** Return the renderer frames per second constant */
        int get_frames_per_second() const { return frames_per_second_const; }

        /** Return the number of frames per field, dividing the field in sub-fields. */
        int  get_frames_per_field() const { return frames_per_field; }

        /** Return count of fields per frame, also the sub-field width or height, 1 / get_frames_per_field(). */
        float get_fields_per_frame() const { return 1.0f / frames_per_field; }

        /** Returns the fields per second actual 'moving' speed, i.e. get_frames_per_second() / get_frames_per_field() */
        float get_fields_per_second() const { return frames_per_second_const / frames_per_field; }

        /**
         * Return fields_per_second difference `actual - requested`, i.e. negative if slower - and positive if faster frame rendering (fps) speed than animation.
         *
         * @see get_frames_per_second_diff()
         * @see get_sync_frame_count()
         */
        float get_fields_per_second_diff() const { return fields_per_second_diff; }

        /**
         * Return delay in frames to sync for get_fields_per_second_diff() compensation, i.e. via repainting same frame.
         *
         * If positive, frame rendering (fps) is faster than animation and caller may sync via animation delay, see get_sync_frame_count().
         * Otherwise value is negative, indicating slower frame rendeinf (fps) speed than animation.
         *
         * @see get_fields_per_second_diff()
         * @see get_sync_frame_count()
         */
        float get_frames_per_second_diff() const;

        /**
         * Returns a positive frame_count to sync animation if frame rendering (fps) is faster than desired animation speed.
         * Caller may insert one repaint w/o animation-tick every returned sync frame_count.
         *
         * Otherwise returns -1 if frame rendering (fps) is slower than desired animation speed.
         *
         * @see get_fields_per_second_diff()
         * @see get_frames_per_second_diff()
         */
        int get_sync_frame_count() const;

        /**
         * Return required delay in milliseconds to sync for get_fields_per_second_diff() compensation.
         *
         * If positive, get_fields_per_second_diff() was positive hence actual faster stepping speed and the value should be used to sync delay.
         * Otherwise value is negative, indicating slower stepping speed.
         */
        float get_sync_delay() const;

        /**
         * If true, this instances uses odd frames_per_field, hence allowing a single center sub-field.
         * Otherwise the center sub-field might be 'walked' twice.
         */
        bool uses_odd_frames_per_field() const { return 0 != frames_per_field / 2; }

        /**
         * Return center x/y sub-field position of square field, i.e. fields_per_frame * frames_per_field/2.
         *
         * Odd frames_per_field int-value gets truncated by div_2, matching the 0-based center sub-field index.
         *
         * The center sub-field is defined by `{ .x = center, .y = center, .width = fields_per_frame, .height = fields_per_frame }`.
         **/
        float get_center() const { return center; }

        /**
         * Returns true if position lies within the center sub-field
         * as defined by `{ .x = center, .y = center, .width = fields_per_frame, .height = fields_per_frame }`.
         */
        bool intersects_center(const float x, const float y) const;

        /**
         * Returns true if position is exactly on the center sub-field within machine epsilon.
         */
        bool is_center(const float x, const float y) const;

        /**
         * Returns true if position component is exactly on the center sub-field within machine epsilon.
         */
        bool is_center_xy(const float xy) const;

        /**
         * Returns true if position in direction is exactly on the center sub-field within machine epsilon.
         */
        bool is_center_dir(const direction_t dir, const float x, const float y) const;

        bool entered_tile(const direction_t dir, const float x, const float y) const;

        float get_aligned(const float v) const;
        float get_centered(const float v) const;

        std::string toString() const;
};

//
// box_t
//
class box_t {
    private:
        int x_pos;
        int y_pos;
        int width;
        int height;

    public:
        box_t(const int x, const int y, const int w, const int h)
        : x_pos(x), y_pos(y), width(w), height(h) {}

        void set_dim(const int x, const int y, const int w, const int h) {
            x_pos = x; y_pos = y; width = w; height = h;
        }
        int get_x() const { return x_pos; }
        int get_y() const { return y_pos; }
        int get_width() const { return width; }
        int get_height() const { return height; }

        std::string toString() const;
};


#endif /* PACMAN_UTILS_HPP_ */

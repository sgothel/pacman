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
#ifndef PACMAN_MAZE_HPP_
#define PACMAN_MAZE_HPP_

#include <pacman/utils.hpp>

#include <string>
#include <vector>
#include <functional>
#include <cmath>

//
// misc
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
 * Direction of the players.
 *
 * Original Puckman direction encoding, see http://donhodges.com/pacman_pinky_explanation.htm
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
// tile_t
//
enum class tile_t : int {
    EMPTY = 0,
    WALL = 1,
    GATE = 2,
    PELLET = 3,
    PELLET_POWER = 4,
    CHERRY = 5,
    STRAWBERRY = 6,
    ORANGE = 7,
    APPLE = 8,
    MELON = 9,
    GALAXIAN = 10,
    BELL = 11,
    KEY = 12
};
constexpr int number(const tile_t item) noexcept {
    return static_cast<int>(item);
}
std::string to_string(tile_t tile);

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

        /** Return fields_per_second difference has - requested, i.e. negative if slower and positive if faster. */
        float get_fields_per_second_diff() const { return fields_per_second_diff; }

        /**
         * Return required delay in frames to sync for get_fields_per_second_diff() compensation, i.e. via repainting same frame.
         *
         * If positive, get_fields_per_second_diff() was positive hence actual faster stepping speed and the value should be used to sync delay.
         * Otherwise value is negative, indicating slower stepping speed.
         */
        float get_frames_per_second_diff() const;

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

//
// acoord_t
//
class maze_t; // fwd

class acoord_t {
    public:
        typedef std::function<bool(tile_t)> collisiontest_simple_t;
        typedef std::function<bool(direction_t d, float x_pos_f, float y_pos_f, bool center, int x_pos, int y_pos, tile_t)> collisiontest_t;

    private:
        static constexpr const bool DEBUG_BOUNDS = false;

        int x_pos_i, y_pos_i;
        float x_pos_f, y_pos_f;
        direction_t last_dir;
        bool last_collided;
        int fields_walked_i;
        float fields_walked_f;

        bool step_impl(direction_t dir, const bool test_only, const keyframei_t& keyframei, collisiontest_simple_t ct0, collisiontest_t ct1);

    public:
        acoord_t(const int x, const int y);

        void reset_stats();

        void set_pos(const int x, const int y);
        void set_pos_clipped(const float x, const float y);
        void set_centered(const keyframei_t& keyframei) { x_pos_f = keyframei.get_centered(x_pos_f); y_pos_f = keyframei.get_centered(y_pos_f); }

        int get_x_i() const { return x_pos_i; }
        int get_y_i() const { return y_pos_i; }
        int get_fields_walked_i() const { return fields_walked_i; }
        float get_x_f() const { return x_pos_f; }
        float get_y_f() const { return y_pos_f; }
        float get_fields_walked_f() const { return fields_walked_f; }

        /**
         * Almost pixel accurate collision test.
         *
         * Note: This is not used in orig pacman game,
         * since it uses tile weighted (rounded) tile position test only.
         */
        bool intersects_f(const acoord_t& other) const;

        /**
         * Weighted tile (rounded) test, i.e. simply comparing the tile position.
         *
         * This is used in orig pacman game.
         *
         * The weighted tile position is determined in step(..) implementation.
         */
        bool intersects_i(const acoord_t& other) const;

        /**
         * Intersection test using either the pixel accurate float method
         * or the original pacman game weighted int method
         * depending on use_original_pacman_behavior().
         */
        bool intersects(const acoord_t& other) const;

        /**
         * Pixel accurate position test for intersection.
         */
        bool intersects_f(const box_t& other) const;

        float distance(const float x, const float y) const;

        float distance(const acoord_t& other) const {
            return distance(other.x_pos_f, other.y_pos_f);
        }

        float sq_distance(const float x, const float y) const;

        float sq_distance(const acoord_t& other) const {
            return sq_distance(other.x_pos_f, other.y_pos_f);
        }

        void incr_fwd(const direction_t dir, const keyframei_t& keyframei, const int tile_count);
        void incr_fwd(const keyframei_t& keyframei, const int tile_count) {
            incr_fwd(last_dir, keyframei, tile_count);
        }
        void incr_left(const keyframei_t& keyframei, const int tile_count) {
            incr_fwd(rot_left(last_dir), keyframei, tile_count);
        }
        void incr_right(const keyframei_t& keyframei, const int tile_count) {
            incr_fwd(rot_right(last_dir), keyframei, tile_count);
        }

        void step(direction_t dir, const keyframei_t& keyframe) {
            step_impl(dir, false, keyframe, nullptr, nullptr);
        }

        /**
         *
         * @param maze
         * @param dir
         * @param fields_per_sec
         * @param frames_per_sec
         * @param ct
         * @return true if successful, otherwise false for collision
         */
        bool step(direction_t dir, const keyframei_t& keyframei, collisiontest_simple_t ct) {
            return step_impl(dir, false, keyframei, ct, nullptr);
        }

        bool step(direction_t dir, const keyframei_t& keyframei, collisiontest_t ct) {
            return step_impl(dir, false, keyframei, nullptr, ct);
        }

        bool test(direction_t dir, const keyframei_t& keyframei, collisiontest_simple_t ct) {
            return step_impl(dir, true, keyframei, ct, nullptr);
        }

        bool test(direction_t dir, const keyframei_t& keyframei, collisiontest_t ct) {
            return step_impl(dir, true, keyframei, nullptr, ct);
        }

        bool is_center(const keyframei_t& keyframei) const {
            return keyframei.is_center(x_pos_f, y_pos_f);
        }

        bool is_center_dir(const keyframei_t& keyframei) const {
            return keyframei.is_center_dir(last_dir, x_pos_f, y_pos_f);
        }

        bool entered_tile(const keyframei_t& keyframei) const {
            return keyframei.entered_tile(last_dir, x_pos_f, y_pos_f);
        }

        /**
         * Returns whether the last step has collided according to the given collistiontest_t or not.
         */
        bool has_collided() const { return last_collided; }

        direction_t get_last_dir() const { return last_dir; }

        std::string toString() const;
        std::string toShortString() const;
};

//
// maze_t
//

class maze_t {
    public:
        class field_t {
            private:
                int width, height;
                std::vector<tile_t> tiles;
                int count[13];

            public:
                field_t();

                void set_dim(const int w, const int h) { width=w; height=h; }
                void add_tile(const tile_t tile);

                void clear();
                bool validate_size() const { return tiles.size() == (size_t)width * (size_t)height; }

                int get_width() const { return width; }
                int get_height() const { return height; }

                int get_count(const tile_t tile) const { return count[number(tile)]; }

                tile_t get_tile(const int x, const int y) const;
                tile_t get_tile_nc(const int x, const int y) const { return tiles[y*width+x]; }
                void set_tile(const int x, const int y, tile_t tile);

                std::string toString() const;
        };
    private:
        static constexpr const bool DEBUG = false;
        std::string filename;
        acoord_t top_left_pos;
        acoord_t bottom_left_pos;
        acoord_t bottom_right_pos;
        acoord_t top_right_pos;
        acoord_t pacman_start_pos;
        box_t ghost_home;
        acoord_t ghost_home_pos;
        acoord_t ghost_start_pos;
        int ppt_x, ppt_y;
        std::string texture_file;
        field_t active;
        field_t original;

        bool digest_position_line(const std::string& name, acoord_t& dest, const std::string& line);
        bool digest_box_line(const std::string& name, box_t& dest, const std::string& line);

    public:
        maze_t(const std::string& fname);

        bool is_ok() const { return active.get_width() > 0 && active.get_height() > 0; };

        int get_width() const { return active.get_width(); }
        int get_height() const { return active.get_height(); }
        const acoord_t& get_top_left_corner() const { return top_left_pos; }
        const acoord_t& get_bottom_left_corner() const { return bottom_left_pos; }
        const acoord_t& get_bottom_right_corner() const { return bottom_right_pos; }
        const acoord_t& get_top_right_corner() const { return top_right_pos; }
        const acoord_t& get_pacman_start_pos() const { return pacman_start_pos; }
        const box_t&  get_ghost_home_box() const { return ghost_home; }
        const acoord_t& get_ghost_home_pos() const { return ghost_home_pos; }
        const acoord_t& get_ghost_start_pos() const { return ghost_start_pos; }

        int get_ppt_x() const { return ppt_x; }
        int get_ppt_y() const { return ppt_y; }
        int x_to_pixel(const int x, const int win_scale) const {
            return x * ppt_x * win_scale;
        }
        int y_to_pixel(const int y, const int win_scale) const {
            return y * ppt_y * win_scale;
        }
        int x_to_pixel(const float x, const int win_scale) const {
            return round_to_int(x * ppt_x * win_scale);
        }
        int y_to_pixel(const float y, const int win_scale) const {
            return round_to_int(y * ppt_y * win_scale);
        }
        std::string get_texture_file() const { return texture_file; }
        int get_pixel_width() const { return get_width() * ppt_x; }
        int get_pixel_height() const { return get_height() * ppt_x; }

        int clip_pos_x(const int x) const {
            return std::max(0, std::min(get_width()-1, x));
        }
        int clip_pos_y(const int y) const {
            return std::max(0, std::min(get_height()-1, y));
        }
        float clip_pos_x(const float x) const {
            return std::max(0.0f, std::min<float>(get_width()-1.0f, x));
        }
        float clip_pos_y(const float y) const {
            return std::max(0.0f, std::min<float>(get_height()-1.0f, y));
        }

        int get_count(const tile_t tile) const { return active.get_count(tile); }
        tile_t get_tile(const int x, const int y) const { return active.get_tile(x, y); }
        void set_tile(const int x, const int y, tile_t tile) { active.set_tile(x, y, tile); }

        void draw(std::function<void(const int x_pos, const int y_pos, tile_t tile)> draw_pixel);

        void reset();

        std::string toString() const;
};

#endif /* PACMAN_MAZE_HPP_ */

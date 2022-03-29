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
    PEACH = 7,
    APPLE = 8,
    MELON = 9,
    GALAXIAN = 10,
    BELL = 11,
    KEY = 12
};
constexpr int number(const tile_t item) noexcept {
    return static_cast<int>(item);
}
std::string to_string(tile_t tile) noexcept;

//
// acoord_t
//

/**
 * @anchor position_semantics
 * ### Positioning
 * The int position represents the tile index.
 *
 * The float position represents the center of the object moving across tiles.
 * It is positioned on the edge of keyframei_t sub-tiles in its moving direction_t with a centered opposite component.
 */
class acoord_t {
    public:
        typedef std::function<bool(tile_t)> collisiontest_simple_t;
        typedef std::function<bool(direction_t d, float x_pos_f, float y_pos_f, bool center, int x_pos, int y_pos, tile_t)> collisiontest_t;

        struct stats_t {
            int fields_walked_i;
            float fields_walked_f;

            int field_center_count;
            int field_entered_count;

            stats_t() noexcept { reset(); }

            void reset() noexcept {
                fields_walked_i = 0;
                fields_walked_f = 0.0f;
                field_center_count = 0;
                field_entered_count = 0;
            }
            std::string toString() const noexcept;
        };

    private:
        // tile int position
        int x_pos_i, y_pos_i;

        // see @ref position_semantics
        float x_pos_f, y_pos_f;

        direction_t last_dir_;
        bool last_collided;

        stats_t stats_;

        bool step_impl(direction_t dir, const bool test_only, const keyframei_t& keyframei, collisiontest_simple_t ct0, collisiontest_t ct1) noexcept;

    public:
        acoord_t(const int x, const int y) noexcept;

        acoord_t(const float x, const float y) noexcept;

        void reset_stats() noexcept { stats_.reset(); }

        void set_pos(const int x, const int y) noexcept;

        void set_pos(const float x, const float y) noexcept;

        void set_pos_clipped(const float x, const float y) noexcept;

        void set_centered(const keyframei_t& keyframei) noexcept;

        void set_aligned_dir(const direction_t dir, const keyframei_t& keyframei) noexcept;
        void set_aligned_dir(const keyframei_t& keyframei) noexcept { set_aligned_dir(last_dir_, keyframei); }

        constexpr direction_t last_dir() const noexcept { return last_dir_; }

        /** Return tile int position, x component. */
        constexpr int x_i() const noexcept { return x_pos_i; }

        /** Return tile int position, y component. */
        constexpr int y_i() const noexcept { return y_pos_i; }

        /**
         * Return tile float position, center of object, x component.
         *
         * @see @ref position_semantics
         */
        constexpr float x_f() const noexcept { return x_pos_f; }

        /**
         * Return tile float position, center of object, y component.
         *
         * @see @ref position_semantics
         */
        constexpr float y_f() const noexcept { return y_pos_f; }

        constexpr const stats_t& get_stats() const noexcept { return stats_; }

        /**
         * Almost pixel accurate collision test.
         *
         * Note: This is not used in orig pacman game,
         * since it uses tile weighted (rounded) tile position test only.
         */
        bool intersects_f(const acoord_t& other) const noexcept;

        /**
         * Weighted tile (rounded) test, i.e. simply comparing the tile position.
         *
         * This is used in orig pacman game.
         *
         * The weighted tile position is determined in step(..) implementation.
         */
        bool intersects_i(const acoord_t& other) const noexcept;

        /**
         * Intersection test using either the pixel accurate float method
         * or the original pacman game weighted int method
         * depending on use_original_pacman_behavior().
         */
        bool intersects(const acoord_t& other) const noexcept;

        /**
         * Pixel accurate position test for intersection.
         */
        bool intersects_f(const box_t& other) const noexcept;

        /**
         * Tile position test for intersection.
         */
        bool intersects_i(const box_t& other) const noexcept;

        /** Returns Euclidian distance */
        float distance(const float x, const float y) const noexcept;

        /** Returns Euclidian distance */
        float distance(const acoord_t& other) const noexcept {
            return distance(other.x_pos_f, other.y_pos_f);
        }

        /** Returns squared Euclidian distance */
        float sq_distance(const float x, const float y) const noexcept;

        /** Returns squared Euclidian distance */
        float sq_distance(const acoord_t& other) const noexcept {
            return sq_distance(other.x_pos_f, other.y_pos_f);
        }

        /** Returns Manhatten distance */
        float distance_manhatten(const float x, const float y) const noexcept;

        /** Returns Manhatten distance */
        float distance_manhatten(const acoord_t& other) const noexcept {
            return distance_manhatten(other.x_pos_f, other.y_pos_f);
        }

        void incr_fwd(const direction_t dir, const keyframei_t& keyframei, const int tile_count) noexcept;
        void incr_fwd(const keyframei_t& keyframei, const int tile_count) noexcept {
            incr_fwd(last_dir_, keyframei, tile_count);
        }
        void incr_left(const keyframei_t& keyframei, const int tile_count) noexcept {
            incr_fwd(rot_left(last_dir_), keyframei, tile_count);
        }
        void incr_right(const keyframei_t& keyframei, const int tile_count) noexcept {
            incr_fwd(rot_right(last_dir_), keyframei, tile_count);
        }

        void step(direction_t dir, const keyframei_t& keyframe) noexcept {
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
        bool step(direction_t dir, const keyframei_t& keyframei, collisiontest_simple_t ct) noexcept {
            return step_impl(dir, false, keyframei, ct, nullptr);
        }

        bool step(direction_t dir, const keyframei_t& keyframei, collisiontest_t ct) noexcept {
            return step_impl(dir, false, keyframei, nullptr, ct);
        }

        bool test(direction_t dir, const keyframei_t& keyframei, collisiontest_simple_t ct) noexcept {
            return step_impl(dir, true, keyframei, ct, nullptr);
        }

        bool test(direction_t dir, const keyframei_t& keyframei, collisiontest_t ct) noexcept {
            return step_impl(dir, true, keyframei, nullptr, ct);
        }

        bool is_center(const keyframei_t& keyframei) const noexcept {
            return keyframei.is_center(x_pos_f, y_pos_f);
        }

        static bool entered_tile(const keyframei_t& keyframei, const direction_t dir, const float x, const float y) noexcept;

        bool entered_tile(const keyframei_t& keyframei) const noexcept {
            return entered_tile(keyframei, last_dir_, x_pos_f, y_pos_f);
        }

        /**
         * Returns whether the last step has collided according to the given collistiontest_t or not.
         */
        bool has_collided() const noexcept { return last_collided; }

        direction_t get_last_dir() const noexcept { return last_dir_; }

        std::string toString() const noexcept;
        std::string toShortString() const noexcept;
        std::string toIntString() const noexcept;
};

//
// maze_t
//

class maze_t {
    public:
        class field_t {
            private:
                int width_, height_;
                std::vector<tile_t> tiles;
                int count_[13];

            public:
                field_t() noexcept;

                void set_dim(const int w, const int h) noexcept { width_=w; height_=h; }
                void add_tile(const tile_t tile) noexcept;

                void clear() noexcept;
                bool validate_size() const noexcept { return tiles.size() == (size_t)width_ * (size_t)height_; }

                constexpr int width() const noexcept { return width_; }
                constexpr int height() const noexcept { return height_; }

                constexpr int count(const tile_t tile) const noexcept { return count_[number(tile)]; }

                tile_t tile(const int x, const int y) const noexcept;
                tile_t tile_nc(const int x, const int y) const noexcept { return tiles[y*width_+x]; }
                void set_tile(const int x, const int y, tile_t tile) noexcept;

                std::string toString() const noexcept;
        };
    private:
        static constexpr const bool DEBUG = false;
        std::string filename;
        acoord_t top_left_pos;
        acoord_t bottom_left_pos;
        acoord_t bottom_right_pos;
        acoord_t top_right_pos;
        box_t tunnel1;
        box_t tunnel2;
        box_t red_zone1;
        box_t red_zone2;
        acoord_t pacman_start_pos_;
        box_t ghost_home_ext;
        box_t ghost_home_int;
        box_t ghost_start;
        int ppt_x_, ppt_y_;
        std::string texture_file;
        field_t active;
        field_t original;

        bool digest_iposition_line(const std::string& name, acoord_t& dest, const std::string& line) noexcept;
        bool digest_fposition_line(const std::string& name, acoord_t& dest, const std::string& line) noexcept;
        bool digest_ibox_line(const std::string& name, box_t& dest, const std::string& line) noexcept;

    public:
        maze_t(const std::string& fname) noexcept;

        constexpr bool is_ok() const noexcept { return active.width() > 0 && active.height() > 0; };

        constexpr int width() const noexcept { return active.width(); }
        constexpr int height() const noexcept { return active.height(); }
        constexpr const acoord_t& top_left_corner() const noexcept { return top_left_pos; }
        constexpr const acoord_t& bottom_left_corner() const noexcept { return bottom_left_pos; }
        constexpr const acoord_t& bottom_right_corner() const noexcept { return bottom_right_pos; }
        constexpr const acoord_t& top_right_corner() const noexcept { return top_right_pos; }
        constexpr const box_t&    tunnel1_box() const noexcept { return tunnel1; }
        constexpr const box_t&    tunnel2_box() const noexcept { return tunnel2; }
        constexpr const box_t&    red_zone1_box() const noexcept { return red_zone1; }
        constexpr const box_t&    red_zone2_box() const noexcept { return red_zone2; }
        constexpr const acoord_t& pacman_start_pos() const noexcept { return pacman_start_pos_; }
        constexpr const box_t&    ghost_home_ext_box() const noexcept { return ghost_home_ext; }
        constexpr const box_t&    ghost_home_int_box() const noexcept { return ghost_home_int; }
        constexpr const box_t&    ghost_start_box() const noexcept { return ghost_start; }

        constexpr int ppt_x() const noexcept { return ppt_x_; }
        constexpr int ppt_y() const noexcept { return ppt_y_; }
        constexpr int x_to_pixel(const int x, const int win_scale) const noexcept {
            return x * ppt_x_ * win_scale;
        }
        constexpr int y_to_pixel(const int y, const int win_scale) const noexcept {
            return y * ppt_y_ * win_scale;
        }
        constexpr int x_to_pixel(const float x, const int win_scale) const noexcept {
            return round_to_int(x * ppt_x_ * win_scale);
        }
        constexpr int y_to_pixel(const float y, const int win_scale) const noexcept {
            return round_to_int(y * ppt_y_ * win_scale);
        }
        std::string get_texture_file() const noexcept { return texture_file; }
        constexpr int pixel_width() const noexcept { return width() * ppt_x_; }
        constexpr int pixel_height() const noexcept { return height() * ppt_x_; }

        constexpr int clip_pos_x(const int x) const noexcept {
            return std::max(0, std::min(width()-1, x));
        }
        constexpr int clip_pos_y(const int y) const noexcept {
            return std::max(0, std::min(height()-1, y));
        }
        constexpr float clip_pos_x(const float x) const noexcept {
            return std::max(0.0f, std::min<float>(width()-1.0f, x));
        }
        constexpr float clip_pos_y(const float y) const noexcept {
            return std::max(0.0f, std::min<float>(height()-1.0f, y));
        }

        constexpr int count(const tile_t tile) const noexcept { return active.count(tile); }
        tile_t tile(const int x, const int y) const noexcept { return active.tile(x, y); }
        void set_tile(const int x, const int y, tile_t tile) noexcept { active.set_tile(x, y, tile); }

        void draw(std::function<void(const int x_pos, const int y_pos, tile_t tile)> draw_pixel) noexcept;

        void reset() noexcept;

        std::string toString() const noexcept;
};

#endif /* PACMAN_MAZE_HPP_ */

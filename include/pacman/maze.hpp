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
enum class direction_t : int {
    UP = 0,
    LEFT = 1,
    DOWN = 2,
    RIGHT = 3
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
        typedef std::function<bool(direction_t d, float x_pos_f, float y_pos_f, bool inbetween, int x_pos, int y_pos, tile_t)> collisiontest_t;

    private:
        static constexpr const bool DEBUG_BOUNDS = false;

        int x_pos_i, y_pos_i;
        float x_pos_f, y_pos_f;
        direction_t last_dir;
        bool last_collided;
        int fields_walked_i;
        float fields_walked_f;

        bool step_impl(const maze_t& maze, direction_t dir, const bool test_only, const float fields_per_frame, collisiontest_t ct);

    public:
        acoord_t(const int x, const int y);

        void reset_stats();

        void set_pos(const int x, const int y);

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

        void incr_fwd(const maze_t& maze, const direction_t dir, const int tile_count);
        void incr_fwd(const maze_t& maze, const int tile_count) {
            incr_fwd(maze, last_dir, tile_count);
        }
        void incr_left(const maze_t& maze, const int tile_count) {
            incr_fwd(maze, rot_left(last_dir), tile_count);
        }
        void incr_right(const maze_t& maze, const int tile_count) {
            incr_fwd(maze, rot_right(last_dir), tile_count);
        }

        void step(const maze_t& maze, direction_t dir, const float fields_per_sec, const int frames_per_sec) {
            step_impl(maze, dir, false, fields_per_sec / frames_per_sec, nullptr);
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
        bool step(const maze_t& maze, direction_t dir, const float fields_per_sec, const int frames_per_sec, collisiontest_t ct) {
            return step_impl(maze, dir, false, fields_per_sec / frames_per_sec, ct);
        }

        bool test(const maze_t& maze, direction_t dir, collisiontest_t ct) {
            return step_impl(maze, dir, true, 0.50, ct);
        }

        static bool is_inbetween(const float fields_per_frame, const float x, const float y);
        bool is_inbetween(const float fields_per_sec, const int frames_per_sec) const {
            return is_inbetween(fields_per_sec / frames_per_sec, x_pos_f, y_pos_f);
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
        int x_to_pixel(const int x, const int win_scale, const bool maze_offset) const {
            return x * ppt_x * win_scale - ( maze_offset ? ppt_x/4 * win_scale : 0 );
        }
        int y_to_pixel(const int y, const int win_scale, const bool maze_offset) const {
            return y * ppt_y * win_scale - ( maze_offset ? ppt_y/4 * win_scale : 0 );
        }
        int x_to_pixel(const float x, const int win_scale, const bool maze_offset) const {
            return round_to_int(x * ppt_x * win_scale) - ( maze_offset ? ( ppt_x/4 * win_scale ) : 0 );
        }
        int y_to_pixel(const float y, const int win_scale, const bool maze_offset) const {
            return round_to_int(y * ppt_y * win_scale) - ( maze_offset ? ( ppt_y/4 * win_scale ) : 0 );
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

        int get_count(const tile_t tile) const { return active.get_count(tile); }
        tile_t get_tile(const int x, const int y) const { return active.get_tile(x, y); }
        void set_tile(const int x, const int y, tile_t tile) { active.set_tile(x, y, tile); }

        void draw(std::function<void(const int x_pos, const int y_pos, tile_t tile)> draw_pixel);

        void reset();

        std::string toString() const;
};

#endif /* PACMAN_MAZE_HPP_ */

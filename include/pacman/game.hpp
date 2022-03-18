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
#ifndef PACMAN_GAME_HPP_
#define PACMAN_GAME_HPP_

#include <pacman/maze.hpp>

//
// score_t
//

enum class score_t : int {
    NONE = 0,
    PELLET = 10,
    PELLET_POWER = 50,
    GHOST_1 =  200,
    GHOST_2 =  400,
    GHOST_3 =  800,
    GHOST_4 = 1600,
    CHERRY = 100,
    STRAWBERRY = 300,
    ORANGE = 500,
    APPLE = 700,
    MELON = 1000,
    GALAXIAN = 2000,
    BELL = 300,
    KEY = 5000
};
constexpr int number(const score_t item) noexcept {
    return static_cast<int>(item);
}
score_t tile_to_score(const tile_t tile);

//
// global_tex_t
//

class global_tex_t {
    private:
        std::shared_ptr<texture_t> all_images;
        std::vector<std::shared_ptr<texture_t>> textures;
        animtex_t atex_pellet_power;

        /**
         * @param tile
         * @return -1 if tile not handled, otherwise a valid textures index
         */
        int tile_to_texidx(const tile_t tile) const;

        /**
         * @param idx
         * @return -1 if wrong idx, otherwise a valid textures index
         */
        int validate_texidx(const int idx) const;

    public:
        enum class special_idx : int {
            GHOST_SCARED_BLUE = 10,
            GHOST_SCARED_PINK = 11
        };
        static constexpr int number(const special_idx item) noexcept {
            return static_cast<int>(item);
        }

        global_tex_t(SDL_Renderer* rend);

        ~global_tex_t() {
            destroy();
        }

        void destroy();

        std::shared_ptr<texture_t> get_all_images() { return all_images; }

        std::shared_ptr<texture_t> get_tex(const tile_t tile);
        std::shared_ptr<const texture_t> get_tex(const tile_t tile) const;

        std::shared_ptr<texture_t> get_tex(const int idx);
        std::shared_ptr<const texture_t> get_tex(const int idx) const;

        bool tick() {
            atex_pellet_power.tick();
            return true;
        }

        void draw_tile(const tile_t tile, SDL_Renderer* rend, const int x, const int y);

        std::string toString() const;
};

//
// ghost_t
//

/**
 * See https://www.gamedeveloper.com/design/the-pac-man-dossier
 */
class ghost_t {
    public:
        enum class personality_t : int {
            /** Red */
            BLINKY = 0,
            /** Orange */
            CLYDE = 1,
            /** Cyan or blue */
            INKY = 2,
            /** Pink or mangenta */
            PINKY = 3
        };
        static constexpr int number(const personality_t item) noexcept {
            return static_cast<int>(item);
        }

        enum class mode_t {
            AWAY,
            HOME,
            LEAVE_HOME,
            CHASE,
            SCATTER,
            SCARED,
            PHANTOM
        };

        /** mode durations in ms */
        enum class mode_duration_t : int {
            HOMESTAY = 4000,
            CHASING = 20000,
            SCATTERING = 7000,
            SCARED = 10000,
            PHANTOM = 20000
        };
        static constexpr int number(const mode_duration_t item) noexcept {
            return static_cast<int>(item);
        }
    private:
        const int ms_per_atex = 500;

        const float fields_per_sec;

        personality_t id; // not necessarily unique
        mode_t mode;
        int mode_ms_left;
        direction_t dir_, last_dir;
        int frame_count;

        animtex_t atex_normal;
        animtex_t atex_scared;
        animtex_t atex_phantom;
        animtex_t * atex;

        acoord_t pos;
        acoord_t target;

        bool log_moves = false;

        static int id_to_yoff(ghost_t::personality_t id);

        animtex_t& get_tex();

        animtex_t& get_phantom_tex() {
            return atex_phantom;
        }

        void set_next_target();
        void set_next_dir();

    public:
        ghost_t(const personality_t id_, SDL_Renderer* rend, const float fields_per_sec_=8);

        ~ghost_t() {
            destroy();
        }

        void destroy();

        void set_log_moves(const bool v) { log_moves = v; }

        mode_t get_mode() const { return mode; }
        void set_mode(const mode_t m);

        direction_t get_dir() const { return dir_; }

        const acoord_t& get_pos() const { return pos; }
        const acoord_t& get_target() const { return target; }

        /**
         * A game engine tick needs to:
         * - adjust speed (acceleration?)
         * - adjust position, taking direction, speed and collision into account.
         *
         * @return true if object is still alive, otherwise false
         */
        bool tick();

        void draw(SDL_Renderer* rend);

        std::string toString() const;
};

std::string to_string(ghost_t::personality_t id);
std::string to_string(ghost_t::mode_t m);

//
// pacman_t
//

class pacman_t {
    public:
        enum class mode_t {
            HOME,
            NORMAL,
            POWERED,
            DEAD
        };

    private:
        /** mode durations in ms */
        enum class mode_duration_t : int {
            INPOWER = ghost_t::number(ghost_t::mode_duration_t::SCARED),
            DEADANIM = 2000
        };
        static constexpr int number(const mode_duration_t item) noexcept {
            return static_cast<int>(item);
        }
        const int ms_per_tex = 167;

        const float fields_per_sec;

        const bool auto_move;

        mode_t mode;
        int mode_ms_left;
        int lives;
        direction_t dir_, last_dir;
        int frame_count;
        int steps_left;
        uint64_t score;
        int audio_nopellet_cntr = 0;

        animtex_t atex_left;
        animtex_t atex_right;
        animtex_t atex_up;
        animtex_t atex_down;
        animtex_t atex_dead;
        animtex_t atex_home;
        animtex_t * atex;

        acoord_t pos;

        uint64_t perf_fields_walked_t0 =0;

        animtex_t& get_tex();

    public:
        pacman_t(SDL_Renderer* rend, const float fields_per_sec_=8, bool auto_move_=true);

        ~pacman_t() {
            destroy();
        }

        void destroy();

        void set_mode(const mode_t m);

        /**
         * Set direction
         */
        void set_dir(direction_t dir);

        direction_t get_dir() const { return dir_; }

        const acoord_t& get_pos() const { return pos; }

        uint64_t get_score() const { return score; }

        /**
         * A game engine tick needs to:
         * - adjust speed (acceleration?)
         * - adjust position, taking direction, speed and collision into account.
         *
         * @return true if object is still alive, otherwise false
         */
        bool tick();

        void draw(SDL_Renderer* rend);

        std::string toString() const;
};

std::string to_string(pacman_t::mode_t m);

#endif /* PACMAN_GAME_HPP_ */

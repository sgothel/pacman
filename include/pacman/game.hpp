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
score_t tile_to_score(const tile_t tile) noexcept;

//
// global_tex_t
//

class global_tex_t {
    private:
        std::shared_ptr<texture_t> all_images_;
        std::vector<std::shared_ptr<texture_t>> textures;
        animtex_t atex_pellet_power;

        /**
         * @param tile
         * @return -1 if tile not handled, otherwise a valid textures index
         */
        int tile_to_texidx(const tile_t tile) const noexcept;

        /**
         * @param idx
         * @return -1 if wrong idx, otherwise a valid textures index
         */
        int validate_texidx(const int idx) const noexcept;

    public:
        enum class special_idx : int {
            GHOST_SCARED_BLUE = 10,
            GHOST_SCARED_PINK = 11
        };
        static constexpr int number(const special_idx item) noexcept {
            return static_cast<int>(item);
        }

        global_tex_t(SDL_Renderer* rend) noexcept;

        ~global_tex_t() noexcept {
            destroy();
        }

        void destroy() noexcept;

        std::shared_ptr<texture_t> all_images() noexcept { return all_images_; }

        std::shared_ptr<texture_t> texture(const tile_t tile) noexcept;
        std::shared_ptr<const texture_t> texture(const tile_t tile) const noexcept;

        std::shared_ptr<texture_t> texture(const int idx) noexcept;
        std::shared_ptr<const texture_t> texture(const int idx) const noexcept;

        bool tick() noexcept {
            atex_pellet_power.tick();
            return true;
        }

        void draw_tile(const tile_t tile, SDL_Renderer* rend, const int x, const int y) noexcept;

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
            /** Pink or mangenta */
            PINKY = 1,
            /** Cyan or blue */
            INKY = 2,
            /** Orange */
            CLYDE = 3
        };
        static constexpr int number(const personality_t item) noexcept {
            return static_cast<int>(item);
        }

        enum class mode_t {
            /** Transitions to HOME right away */
            LEVEL_START,
            /** Turned off ghosts while pacman's death animation */
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
            CHASING = 20000,
            SCATTERING = 7000,
            SCARED = 10000
        };
        static constexpr int number(const mode_duration_t item) noexcept {
            return static_cast<int>(item);
        }
    private:
        const int ms_per_atex = 500;

        const float fields_per_sec_total;
        float current_speed_pct;
        keyframei_t keyframei_;
        countdown_t sync_next_frame_cntr;

        personality_t id;
        int live_counter_during_pacman_live;
        mode_t mode_;
        int mode_ms_left;
        direction_t dir_;
        bool pellet_counter_active_;
        int pellet_counter_;
        static bool global_pellet_counter_active;
        static int global_pellet_counter;

        animtex_t atex_normal;
        animtex_t atex_scared;
        animtex_t atex_phantom;
        animtex_t * atex;

        acoord_t pos_;
        acoord_t target_;

        bool log_moves = false;

        static int id_to_yoff(ghost_t::personality_t id) noexcept;

        animtex_t& get_tex() noexcept;

        constexpr animtex_t& get_phantom_tex() noexcept { return atex_phantom; }

        void set_next_target() noexcept;
        void set_next_dir(const bool collision, const bool is_center) noexcept;

    public:
        ghost_t(const personality_t id_, SDL_Renderer* rend, const float fields_per_sec_total_) noexcept;

        ~ghost_t() noexcept {
            destroy();
        }

        void destroy() noexcept;

        void set_log_moves(const bool v) noexcept { log_moves = v; }

        constexpr const keyframei_t& keyframei() const noexcept { return keyframei_; }

        constexpr mode_t mode() const noexcept { return mode_; }
        constexpr bool at_home() const noexcept { return mode_t::HOME == mode_; }
        constexpr bool in_house() const noexcept { return mode_t::HOME == mode_ || mode_t::LEAVE_HOME == mode_; }
        void set_mode(const mode_t m, const int mode_ms=-1) noexcept;

        void set_speed(const float pct) noexcept;

        static std::string pellet_counter_string() noexcept;
        static void notify_pellet_eaten() noexcept;
        int pellet_counter() const noexcept;
        int pellet_counter_limit() const noexcept;
        bool can_leave_home() noexcept;

        constexpr direction_t direction() const noexcept { return dir_; }
        constexpr const acoord_t& position() const noexcept { return pos_; }
        constexpr const acoord_t& target() const noexcept { return target_; }

        /**
         * A game engine tick needs to:
         * - adjust speed (acceleration?)
         * - adjust position, taking direction, speed and collision into account.
         *
         * @return true if object is still alive, otherwise false
         */
        bool tick() noexcept;

        void draw(SDL_Renderer* rend) noexcept;

        std::string toString() const noexcept;
};

std::string to_string(ghost_t::personality_t id) noexcept;
std::string to_string(ghost_t::mode_t m) noexcept;

//
// pacman_t
//

class pacman_t {
    public:
        enum class mode_t {
            /** Transitions to HOME right away */
            LEVEL_START,
            HOME,
            NORMAL,
            POWERED,
            DEAD
        };

        /** mode durations in ms */
        enum class mode_duration_t : int {
            HOMESTAY = 2000,
            INPOWER = ghost_t::number(ghost_t::mode_duration_t::SCARED),
            DEADANIM = 2000
        };
        static constexpr int number(const mode_duration_t item) noexcept {
            return static_cast<int>(item);
        }
    private:
        const int ms_per_tex = 167;

        const float fields_per_sec_total;
        float current_speed_pct;
        keyframei_t keyframei_;
        countdown_t sync_next_frame_cntr;
        countdown_t next_empty_field_frame_cntr;

        mode_t mode;
        int mode_ms_left;
        int lives;
        direction_t current_dir;
        uint64_t score_;

        animtex_t atex_left;
        animtex_t atex_right;
        animtex_t atex_up;
        animtex_t atex_down;
        animtex_t atex_dead;
        animtex_t atex_home;
        animtex_t * atex;

        acoord_t pos_;

        bool log_moves = false;

        uint64_t perf_fields_walked_t0 = 0;
        uint64_t perf_frame_count_walked = 0;

        animtex_t& get_tex() noexcept;

        void print_stats() noexcept;
        void reset_stats() noexcept;

    public:
        pacman_t(SDL_Renderer* rend, const float fields_per_sec_total_) noexcept;

        ~pacman_t() noexcept {
            destroy();
        }

        void destroy() noexcept;

        void set_log_moves(const bool v) noexcept { log_moves = v; }

        void set_mode(const mode_t m) noexcept;
        void set_speed(const float pct) noexcept;
        const keyframei_t& get_keyframei() const noexcept { return keyframei_; }

        /**
         * Set direction
         */
        bool set_dir(const direction_t new_dir) noexcept;

        constexpr direction_t direction() const noexcept { return current_dir; }

        constexpr const acoord_t& position() const noexcept { return pos_; }

        constexpr uint64_t score() const noexcept { return score_; }

        /**
         * A game engine tick needs to:
         * - adjust speed (acceleration?)
         * - adjust position, taking direction, speed and collision into account.
         *
         * @return true if object is still alive, otherwise false
         */
        bool tick() noexcept;

        void draw(SDL_Renderer* rend) noexcept;

        std::string toString() const noexcept;
};

std::string to_string(pacman_t::mode_t m);

#endif /* PACMAN_GAME_HPP_ */

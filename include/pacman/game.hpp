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

#include <pacman/utils.hpp>
#include <pacman/audio.hpp>
#include <pacman/maze.hpp>

//
// Note: Game globals are added on the bottom of this file
//

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
    PEACH = 500,
    APPLE = 700,
    MELON = 1000,
    GALAXIAN = 2000,
    BELL = 3000,
    KEY = 5000
};
constexpr int number(const score_t item) noexcept {
    return static_cast<int>(item);
}
score_t tile_to_score(const tile_t tile) noexcept;

//
// level_spec_t
//

/**
 * Ghost wave timings for each scatter and chase duration per phase and per level
 * from [The Pac-Man Dossier](https://www.gamedeveloper.com/design/the-pac-man-dossier).
 */
struct ghost_wave_t {
    int scatter_ms;
    int chase_ms;
};
typedef std::vector<ghost_wave_t> ghost_wave_vec_t;

// ghost_t::ghost_count size - one for each
typedef std::vector<int> ghost_pellet_counter_limit_t;

extern ghost_pellet_counter_limit_t global_ghost_pellet_counter_limit;

/**
 * `Level Specification` from [The Pac-Man Dossier](https://www.gamedeveloper.com/design/the-pac-man-dossier),
 * Appendix A: Reference Tables.
 */
struct game_level_spec_t {
    tile_t symbol;
    int bonus_points;
    float pacman_speed;
    float pacman_speed_dots;
    float pacman_powered_speed;
    float pacman_powered_speed_dots;
    float ghost_speed;
    float ghost_speed_tunnel;
    float ghost_fright_speed;
    int fright_time_ms;
    int fright_flash_count;
    int elroy1_dots_left;
    float elroy1_speed;
    int elroy2_dots_left;
    float elroy2_speed;
    ghost_wave_vec_t ghost_waves;
    ghost_pellet_counter_limit_t ghost_pellet_counter_limit;
    int ghost_max_home_time_ms;
};

const game_level_spec_t& game_level_spec(const int level) noexcept;
const game_level_spec_t& game_level_spec() noexcept;

const ghost_wave_t& get_ghost_wave(const int level, const int phase_idx) noexcept;
const ghost_wave_t& get_ghost_wave(const int phase_idx) noexcept;

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

        void draw_tile(const tile_t tile, SDL_Renderer* rend, const float x, const float y) noexcept;

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
        static constexpr const int ghost_count = 4;

        enum class mode_t {
            PACMAN_DIED,
            AWAY,
            LEVEL_SETUP,
            START,
            HOME,
            LEAVE_HOME,
            CHASE,
            SCATTER,
            SCARED,
            PHANTOM
        };

        /** RGB color of the ghosts in number(personality_t) order. */
        static std::vector<std::vector<int>> rgb_color;

    private:
        static random_engine_t<random_engine_mode_t::STD_RNG> rng_hw;
        static random_engine_t<random_engine_mode_t::STD_PRNG_0> rng_prng;
        static random_engine_t<random_engine_mode_t::PUCKMAN> rng_pm;
        static std::uniform_int_distribution<int> rng_dist;

        static mode_t global_mode; // SCATTER, CHASE or SCARED
        static mode_t global_mode_last; // SCATTER, CHASE or SCARED
        static int global_mode_ms_left; // for SCATTER, CHASE or SCARED
        static int global_scatter_mode_count; // for SCATTER, CHASE or SCARED

        const int ms_per_atex = 500;
        const int ms_per_fright_flash = 334;

        const float fields_per_sec_total;
        float current_speed_pct;
        keyframei_t keyframei_;
        countdown_t sync_next_frame_cntr;

        personality_t id_;
        int live_counter_during_pacman_live;
        mode_t mode_;
        mode_t mode_last;
        int mode_ms_left;
        direction_t current_dir;
        bool pellet_counter_active_;
        int pellet_counter_;
        static bool global_pellet_counter_active;
        static int global_pellet_counter;
        static int global_pellet_time_left;

        animtex_t atex_normal;
        animtex_t atex_scared;
        animtex_t atex_scared_flash;
        animtex_t atex_phantom;
        animtex_t * atex;

        acoord_t home_pos;
        acoord_t pos_;
        acoord_t target_;

        /** if use_decision_one_field_ahead(), the look-ahead next direction when reaching pos_next */
        direction_t dir_next;
        /** if use_decision_one_field_ahead(), the look-ahead next pos_next. Set to -1/-1 to trigger next lookup. */
        acoord_t pos_next;

        static int id_to_yoff(ghost_t::personality_t id) noexcept;

        animtex_t& get_tex() noexcept;

        constexpr animtex_t& get_phantom_tex() noexcept { return atex_phantom; }

        /**
         * Return a `random` direction_t from the used random engine.
         *
         * Depending on global settings, this could be
         * - a puckman inspired predictable PRNG
         * - the std default predictable PRNG
         * - a hardware unpredictable RNG, unable to reset_random()
         */
        static direction_t get_random_dir() noexcept;

        /**
         * The random engine should be called when pacman dies, a level starts and when a level ends.
         *
         * This method is a NOP for a hardware unpredictable RNG.
         */
        static void reset_random() noexcept;

        void set_next_target() noexcept;
        void set_next_dir(const bool collision, const bool is_center) noexcept;

        /** Return true if speed changed, otherwise false */
        bool set_mode_speed() noexcept;
        void tick() noexcept;
        void draw(SDL_Renderer* rend) noexcept;

    public:
        ghost_t(const personality_t id_, SDL_Renderer* rend, const float fields_per_sec_total_) noexcept;

        ~ghost_t() noexcept {
            destroy();
        }

        void destroy() noexcept;

        constexpr personality_t id() const noexcept { return id_; }

        constexpr const keyframei_t& keyframei() const noexcept { return keyframei_; }

        constexpr mode_t mode() const noexcept { return mode_; }
        constexpr bool at_home() const noexcept { return mode_t::HOME == mode_; }
        constexpr bool in_house() const noexcept { return mode_t::HOME == mode_ || mode_t::LEAVE_HOME == mode_; }
        constexpr bool is_scattering_or_chasing() const noexcept { return mode_t::SCATTER == mode_ || mode_t::CHASE == mode_; }

        constexpr direction_t direction() const noexcept { return current_dir; }
        constexpr const acoord_t& position() const noexcept { return pos_; }
        constexpr const acoord_t& target() const noexcept { return target_; }

        /** Return true if speed changed, otherwise false */
        bool set_speed(const float pct) noexcept;

        /** For global SCATTER, CHASE or SCARED mode switch, etc. */
        static void set_global_mode(const mode_t m, const int mode_ms=-1) noexcept;

        void set_mode(const mode_t m, const int mode_ms=-1) noexcept;

        /** For global SCATTER, CHASE or SCARED mode switch, etc. */
        static void global_tick() noexcept;

        static void global_draw(SDL_Renderer* rend) noexcept;

        static std::string pellet_counter_string() noexcept;
        static void notify_pellet_eaten() noexcept;
        int pellet_counter() const noexcept;
        int pellet_counter_limit() const noexcept;
        bool can_leave_home() noexcept;

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
            FREEZE,
            LEVEL_SETUP,
            START,
            NORMAL,
            POWERED,
            DEAD
        };

        /** mode durations in ms */
        enum class mode_duration_t : int {
            FREEZE = 900,
            DEADANIM = 2000
        };
        static constexpr int number(const mode_duration_t item) noexcept {
            return static_cast<int>(item);
        }

        /** RGB color of pacman . */
        static std::vector<int> rgb_color;

    private:
        static random_engine_t<random_engine_mode_t::STD_RNG> rng_hw;
        const int ms_per_tex = 167;

        /**
         * Notes about bonus fruits:
         *
         * - Fruits appear twice on each board.
         *   - The first fruit appears after Puckman has eaten 70 dots
         *   - The second fruit appears once there are only 70 dots remaining in the maze.
         * - Duration is [9-10] seconds, is variable and not predictable with the use of patterns.
         */
        const int fruit_1_eaten =  70;
        const int fruit_2_eaten = 170;
        const int fruit_duration_min =  9000;
        const int fruit_duration_max = 10000;

        const float fields_per_sec_total;
        float current_speed_pct;
        keyframei_t keyframei_;
        countdown_t sync_next_frame_cntr;
        countdown_t next_empty_field_frame_cntr;

        mode_t mode_;
        mode_t mode_last;
        int mode_ms_left;
        int fruit_ms_left;
        int lives;
        int ghosts_eaten_powered;
        direction_t current_dir;
        uint64_t score_;
        int freeze_score;
        box_t freeze_box_;
        int freeze_frame_count;

        animtex_t atex_left;
        animtex_t atex_right;
        animtex_t atex_up;
        animtex_t atex_down;
        animtex_t atex_dead;
        animtex_t atex_home;
        animtex_t * atex;

        acoord_t pos_;

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

        constexpr mode_t mode() const noexcept { return mode_; }

        constexpr direction_t direction() const noexcept { return current_dir; }
        constexpr const acoord_t& position() const noexcept { return pos_; }
        constexpr uint64_t score() const noexcept { return score_; }
        const box_t& freeze_box() const noexcept { return freeze_box_; }

        void reset_score() noexcept { score_ = 0; }
        void set_mode(const mode_t m, const int mode_ms=-1) noexcept;
        void stop_audio_loops() noexcept;
        void set_speed(const float pct) noexcept;
        const keyframei_t& get_keyframei() const noexcept { return keyframei_; }

        /**
         * Set direction
         */
        bool set_dir(const direction_t new_dir) noexcept;

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

//
// globals for game
//

/** Returns current game level, range [1..255] */
int get_current_level() noexcept;

extern std::unique_ptr<maze_t> global_maze;

extern std::shared_ptr<global_tex_t> global_tex;

typedef std::shared_ptr<ghost_t> ghost_ref;
/**
 * ghosts are in proper ghost_t enum order for array access,
 * i.e. BLINKY, PINKY, INKY and CLYDE.
 */
std::vector<ghost_ref>& ghosts() noexcept;

ghost_ref ghost(const ghost_t::personality_t id) noexcept;

typedef std::shared_ptr<pacman_t> pacman_ref;
extern pacman_ref pacman;

enum class audio_clip_t : int {
    INTRO = 0,
    MUNCH = 1,
    EAT_FRUIT = 2,
    EAT_GHOST = 3,
    DEATH = 4,
    EXTRA = 5,
    INTERMISSION = 6
};
constexpr int number(const audio_clip_t item) noexcept {
    return static_cast<int>(item);
}

typedef std::shared_ptr<audio_sample_t> audio_sample_ref;
extern std::vector<audio_sample_ref> audio_samples;

/**
 * By default the original pacman behavior is being implemented:
 * - weighted (round) tile position for collision tests
 * - pinky's up-target not 4 ahead, but 4 ahead and 4 to the left
 * - ...
 *
 * If false, a more accurate implementation, the pacman bugfix, is used:
 * - pixel accurate tile position for collision tests
 * - pinky's up-traget to be 4 ahead as intended
 * - ...
 *
 * TODO: Keep in sync with README.md
 */
bool use_original_pacman_behavior() noexcept;

/**
 * As stated in `The Pac-Man Dossier`,
 * the ghosts select their next direction *one tile ahead of an intersection*.
 *
 * If this is true, the decision point is
 * one step ahead of a potential intersection befor reaching it, the default.
 *
 * Otherwise each field will be tested when reached
 * with a more current pacman position.
 */
bool use_decision_one_field_ahead() noexcept;

/**
 * Use manhatten distance instead of (squared) Euclidean.
 *
 * Default is to return false, i.e. use Euclidean distance.
 */
bool use_manhatten_distance() noexcept;

bool show_debug_gfx() noexcept;

bool log_fps() noexcept;
bool log_moves() noexcept;
bool log_modes() noexcept;


#endif /* PACMAN_GAME_HPP_ */

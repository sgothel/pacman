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
#ifndef PACMAN_GRAPHICS_HPP_
#define PACMAN_GRAPHICS_HPP_

#include <atomic>
#include <memory>
#include <vector>
#include <string>
#include <inttypes.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_timer.h>
#include <SDL2/SDL_ttf.h>

class texture_t {
    private:
        static std::atomic<int> counter;
        int id;
        SDL_Texture* tex;
        int x, y, width, height;
        bool owner;

    public:
        /**
         * Empty texture
         */
        texture_t()
        : id(counter++), tex(nullptr), x(0), y(0), width(0), height(0), owner(false) {}

        texture_t(SDL_Renderer* rend, const std::string& fname);

        texture_t(SDL_Renderer* rend, SDL_Surface* surface);

        texture_t(SDL_Texture* t, int x_, int y_, int w_, int h_, bool owner=true)
        : id(counter++), tex(t), x(x_), y(y_), width(w_), height(h_), owner(owner) {}

        texture_t(SDL_Texture* t, int w_, int h_, bool owner=true)
        : id(counter++), tex(t), x(0), y(0), width(w_), height(h_), owner(owner) {}

        texture_t(const texture_t&) = delete;
        void operator=(const texture_t&) = delete;

        ~texture_t() {
            destroy();
        }

        void destroy();

        bool is_owner() const { return owner; }
        void disown() { owner = false; }

        int get_id() const { return id; }
        int get_x() const { return x; }
        int get_y() const { return y; }
        int get_width() const { return width; }
        int get_height() const { return height; }
        SDL_Texture* get_sdltex() { return tex; }

        void draw_scaled_dimpos(SDL_Renderer* rend, const int x_pos, const int y_pos);
        void draw_scaled_dim(SDL_Renderer* rend, const int x_pos, const int y_pos);
        void draw(SDL_Renderer* rend, const int x_pos, const int y_pos, const bool maze_offset);

        void draw(SDL_Renderer* rend, const float x_pos, const float y_pos, const bool maze_offset);

        std::string toString() const;
};

struct tex_sub_coord_t {
    int x;
    int y;
};

/**
 * Add sub-textures to the storage list of texture_t from file,
 * where the last one is the sole owner of the common SDL_Texture instance.
 * @param storage
 * @param rend
 * @param filename
 * @param w
 * @param h
 * @param x_off
 * @return number of added sub-textures, last one is owner of the SDL_Texture instance
 */
int add_sub_textures(std::vector<std::shared_ptr<texture_t>>& storage, SDL_Renderer* rend,
                     const std::string& filename, int w, int h, int x_off);

/**
 * Add sub-textures to the storage list of texture_t from given global texture owner.
 *
 * None of the sub-textures is the owner of the common SDL_Texture instance.
 * @param storage
 * @param rend
 * @param global_texture
 * @param x_off
 * @param y_off
 * @param w
 * @param h
 * @param tex_positions
 * @return number of added sub-textures
 */
int add_sub_textures(std::vector<std::shared_ptr<texture_t>>& storage, SDL_Renderer* rend,
                     const std::shared_ptr<texture_t>& global_texture, int x_off, int y_off, int w, int h,
                     const std::vector<tex_sub_coord_t>& tex_positions);

class animtex_t {
    private:
        std::string name;
        std::vector<std::shared_ptr<texture_t>> textures;

        int ms_per_atex;
        int atex_ms_left;
        size_t animation_index;
        bool paused;

    public:
        animtex_t(std::string name_, int ms_per_atex_, const std::vector<std::shared_ptr<texture_t>>& textures_);

        animtex_t(std::string name_, SDL_Renderer* rend, int ms_per_atex_, const std::vector<const char*>& filenames);

        animtex_t(std::string name_, SDL_Renderer* rend, int ms_per_atex_, const std::string& filename, int w, int h, int x_off);

        animtex_t(std::string name_, SDL_Renderer* rend, int ms_per_atex_, const std::shared_ptr<texture_t>& global_texture,
                 int x_off, int y_off, int w, int h, const std::vector<tex_sub_coord_t>& tex_positions);

        ~animtex_t() {
            destroy();
        }

        void destroy();

        std::shared_ptr<texture_t> get_tex(const size_t idx) { return idx < textures.size() ? textures[idx] : nullptr; }
        std::shared_ptr<const texture_t> get_tex(const size_t idx) const { return idx < textures.size() ? textures[idx] : nullptr; }

        std::shared_ptr<texture_t> get_tex() { return animation_index < textures.size() ? textures[animation_index] : nullptr; }
        std::shared_ptr<const texture_t> get_tex() const { return animation_index < textures.size() ? textures[animation_index] : nullptr; }

        int get_width() { std::shared_ptr<texture_t> tex = get_tex(); return nullptr!=tex ? tex->get_width() : 0; }
        int get_height() { std::shared_ptr<texture_t> tex = get_tex(); return nullptr!=tex ? tex->get_height() : 0; }

        void reset();
        void pause(bool enable);
        void tick();

        void draw(SDL_Renderer* rend, const float x, const float y, const bool maze_offset) {
            std::shared_ptr<texture_t> tex = get_tex();
            if( nullptr != tex ) {
                tex->draw(rend, x, y, maze_offset);
            }
        }

        std::string toString() const;
};

/**
 * Storage for a rendered text allowing caching for performance.
 */
struct text_texture_t {
    std::string text;
    texture_t texture;
    bool scaled_pos;
    int x_pos;
    int y_pos;

    text_texture_t(const std::string text_, SDL_Renderer* rend, SDL_Surface* surface, bool scaled_pos_, int x_, int y_)
    : text(text_), texture(rend, surface), scaled_pos(scaled_pos_), x_pos(x_), y_pos(y_) {}

    void redraw(SDL_Renderer* rend) {
        if( scaled_pos ) {
            texture.draw_scaled_dimpos(rend, x_pos, y_pos);
        } else {
            texture.draw_scaled_dim(rend, x_pos, y_pos);
        }
    }
};

/**
 *
 * @param rend
 * @param font
 * @param text
 * @param x
 * @param y
 * @param r
 * @param g
 * @param b
 */
std::shared_ptr<text_texture_t> draw_text(SDL_Renderer* rend, TTF_Font* font, const std::string& text, int x, int y, uint8_t r, uint8_t g, uint8_t b);

std::shared_ptr<text_texture_t> draw_text_scaled(SDL_Renderer* rend, TTF_Font* font, const std::string& text, uint8_t r, uint8_t g, uint8_t b, std::function<void(const texture_t& texture, int &x, int&y)> scaled_coord);

#endif /* PACMAN_GRAPHICS_HPP_ */

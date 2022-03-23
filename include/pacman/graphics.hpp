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
#include <functional>

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_timer.h>
#include <SDL2/SDL_ttf.h>

class texture_t {
    private:
        static std::atomic<int> counter;
        int id_;
        SDL_Texture* tex_;
        int x_, y_, width_, height_;
        bool owner_;

    public:
        /**
         * Empty texture
         */
        texture_t() noexcept
        : id_(counter++), tex_(nullptr), x_(0), y_(0), width_(0), height_(0), owner_(false) {}

        texture_t(SDL_Renderer* rend, const std::string& fname) noexcept;

        texture_t(SDL_Renderer* rend, SDL_Surface* surface) noexcept;

        texture_t(SDL_Texture* t, int x_, int y_, int w_, int h_, bool owner=true) noexcept
        : id_(counter++), tex_(t), x_(x_), y_(y_), width_(w_), height_(h_), owner_(owner) {}

        texture_t(SDL_Texture* t, int w_, int h_, bool owner=true) noexcept
        : id_(counter++), tex_(t), x_(0), y_(0), width_(w_), height_(h_), owner_(owner) {}

        texture_t(const texture_t&) = delete;
        void operator=(const texture_t&) = delete;

        ~texture_t() noexcept {
            destroy();
        }

        void destroy() noexcept;

        constexpr bool is_owner() const noexcept { return owner_; }
        void disown() noexcept { owner_ = false; }

        constexpr int id() const noexcept { return id_; }
        constexpr int x() const noexcept { return x_; }
        constexpr int y() const noexcept { return y_; }
        constexpr int width() const noexcept { return width_; }
        constexpr int height() const noexcept { return height_; }
        constexpr SDL_Texture* sdl_texture() noexcept { return tex_; }

        void draw_scaled_dimpos(SDL_Renderer* rend, const int x_pos, const int y_pos) noexcept;
        void draw_scaled_dim(SDL_Renderer* rend, const int x_pos, const int y_pos) noexcept;
        void draw(SDL_Renderer* rend, const int x_pos, const int y_pos) noexcept;
        void draw2_i(SDL_Renderer* rend, const int x_pos, const int y_pos) noexcept;

        void draw(SDL_Renderer* rend, const float x_pos, const float y_pos) noexcept;
        void draw2_f(SDL_Renderer* rend, const float x_pos, const float y_pos) noexcept;

        std::string toString() const noexcept;
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
                     const std::string& filename, int w, int h, int x_off) noexcept;

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
                     const std::vector<tex_sub_coord_t>& tex_positions) noexcept;

class animtex_t {
    private:
        std::string name_;
        std::vector<std::shared_ptr<texture_t>> textures_;

        int ms_per_atex_;
        int atex_ms_left_;
        size_t animation_index_;
        bool paused_;

    public:
        animtex_t(std::string name, int ms_per_atex, const std::vector<std::shared_ptr<texture_t>>& textures) noexcept;

        animtex_t(std::string name, SDL_Renderer* rend, int ms_per_atex, const std::vector<const char*>& filenames) noexcept;

        animtex_t(std::string name, SDL_Renderer* rend, int ms_per_atex, const std::string& filename, int w, int h, int x_off) noexcept;

        animtex_t(std::string name, SDL_Renderer* rend, int ms_per_atex, const std::shared_ptr<texture_t>& global_texture,
                 int x_off, int y_off, int w, int h, const std::vector<tex_sub_coord_t>& tex_positions) noexcept;

        ~animtex_t() noexcept {
            destroy();
        }

        void destroy() noexcept;

        std::shared_ptr<texture_t> texture(const size_t idx) noexcept { return idx < textures_.size() ? textures_[idx] : nullptr; }
        std::shared_ptr<const texture_t> texture(const size_t idx) const noexcept { return idx < textures_.size() ? textures_[idx] : nullptr; }

        std::shared_ptr<texture_t> texture() noexcept { return animation_index_ < textures_.size() ? textures_[animation_index_] : nullptr; }
        std::shared_ptr<const texture_t> texture() const noexcept { return animation_index_ < textures_.size() ? textures_[animation_index_] : nullptr; }

        int width() noexcept { std::shared_ptr<texture_t> tex = texture(); return nullptr!=tex ? tex->width() : 0; }
        int height() noexcept { std::shared_ptr<texture_t> tex = texture(); return nullptr!=tex ? tex->height() : 0; }

        void reset() noexcept;
        void pause(bool enable) noexcept;
        void tick() noexcept;

        void draw(SDL_Renderer* rend, const float x, const float y) noexcept;

        std::string toString() const noexcept;
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

    text_texture_t(const std::string text_, SDL_Renderer* rend, SDL_Surface* surface, bool scaled_pos_, int x_, int y_) noexcept
    : text(text_), texture(rend, surface), scaled_pos(scaled_pos_), x_pos(x_), y_pos(y_) {}

    void redraw(SDL_Renderer* rend) noexcept {
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
std::shared_ptr<text_texture_t> draw_text(SDL_Renderer* rend, TTF_Font* font, const std::string& text, int x, int y, uint8_t r, uint8_t g, uint8_t b) noexcept;

std::shared_ptr<text_texture_t> draw_text_scaled(SDL_Renderer* rend, TTF_Font* font, const std::string& text, uint8_t r, uint8_t g, uint8_t b, std::function<void(const texture_t& texture, int &x_, int&y_)> scaled_coord) noexcept;

#endif /* PACMAN_GRAPHICS_HPP_ */

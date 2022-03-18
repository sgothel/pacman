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
#include <pacman/maze.hpp>
#include <pacman/graphics.hpp>
#include <pacman/globals.hpp>

#include <cstdio>

static constexpr const bool DEBUG_LOG = false;

//
// texture_t
//

std::atomic<int> texture_t::counter = 0;

texture_t::texture_t(SDL_Renderer* rend, const std::string& fname)
: id(counter++)
{
    SDL_Surface* surface = IMG_Load(fname.c_str());
    if( nullptr != surface ) {
        if( DEBUG_LOG ) {
            log_printf("texture_t::surface: fmt %u 0x%X, %d x %d pitch %d\n", surface->format->format, surface->format->format, surface->w, surface->h, surface->pitch);
        }
        tex = SDL_CreateTextureFromSurface(rend, surface);
        SDL_FreeSurface(surface);
        if( nullptr == tex ) {
            log_printf("texture_t: Error loading %s: %s\n", fname.c_str(), SDL_GetError());
        }
    } else {
        tex = nullptr;
        log_printf("texture_t::surface: Error loading %s: %s\n", fname.c_str(), SDL_GetError());
    }
    x = 0;
    y = 0;
    width = 0;
    height = 0;
    Uint32 format = 0;
    if( nullptr != tex ) {
        SDL_QueryTexture(tex, &format, NULL, &width, &height);
        if( DEBUG_LOG ) {
            log_printf("texture_t: fmt %u 0x%X, %d x %d\n", format, format, width, height);
        }
    }
    owner = true;
}

texture_t::texture_t(SDL_Renderer* rend, SDL_Surface* surface)
: id(counter++)
{
    tex = SDL_CreateTextureFromSurface(rend, surface);
    x = 0;
    y = 0;
    SDL_QueryTexture(tex, NULL, NULL, &width, &height);
    owner = true;
}

void texture_t::destroy() {
    if( owner && nullptr != tex ) {
        SDL_DestroyTexture(tex);
    }
    tex = nullptr;
}

void texture_t::draw_scaled_dimpos(SDL_Renderer* rend, const int x_pos, const int y_pos) {
    if( nullptr != tex ) {
        SDL_Rect src = { .x=x, .y=y, .w=width, .h=height};
        const int win_pixel_offset = ( win_pixel_width - pacman_maze->get_pixel_width()*win_pixel_scale ) / 2;
        SDL_Rect dest = { .x=win_pixel_offset + x_pos,
                          .y=y_pos,
                          .w=width, .h=height };
        SDL_RenderCopy(rend, tex, &src, &dest);
    }
}
void texture_t::draw_scaled_dim(SDL_Renderer* rend, const int x_pos, const int y_pos) {
    if( nullptr != tex ) {
        SDL_Rect src = { .x=x, .y=y, .w=width, .h=height};
        const int win_pixel_offset = ( win_pixel_width - pacman_maze->get_pixel_width()*win_pixel_scale ) / 2;
        SDL_Rect dest = { .x=win_pixel_offset + ( pacman_maze->x_to_pixel(x_pos, win_pixel_scale, false) ),
                          .y=pacman_maze->y_to_pixel(y_pos, win_pixel_scale, false),
                          .w=width, .h=height };
        SDL_RenderCopy(rend, tex, &src, &dest);
    }
}
void texture_t::draw(SDL_Renderer* rend, const int x_pos, const int y_pos, const bool maze_offset) {
    if( nullptr != tex ) {
        SDL_Rect src = { .x=x, .y=y, .w=width, .h=height};
        const int win_pixel_offset = ( win_pixel_width - pacman_maze->get_pixel_width()*win_pixel_scale ) / 2;
        SDL_Rect dest = { .x=win_pixel_offset + ( pacman_maze->x_to_pixel(x_pos, win_pixel_scale, maze_offset) ),
                          .y=pacman_maze->y_to_pixel(y_pos, win_pixel_scale, maze_offset),
                          .w=width*win_pixel_scale, .h=height*win_pixel_scale };
        SDL_RenderCopy(rend, tex, &src, &dest);
    }
}
void texture_t::draw(SDL_Renderer* rend, const float x_pos, const float y_pos, const bool maze_offset) {
    if( nullptr != tex ) {
        SDL_Rect src = { .x=x, .y=y, .w=width, .h=height};
        const int win_pixel_offset = ( win_pixel_width - pacman_maze->get_pixel_width()*win_pixel_scale ) / 2;
        SDL_Rect dest = { .x=win_pixel_offset + pacman_maze->x_to_pixel(x_pos, win_pixel_scale, maze_offset),
                          .y=pacman_maze->y_to_pixel(y_pos, win_pixel_scale, maze_offset),
                          .w=width*win_pixel_scale, .h=height*win_pixel_scale };
        SDL_RenderCopy(rend, tex, &src, &dest);
    }
}

std::string texture_t::toString() const {
    return "id "+std::to_string(id) + " " + std::to_string(x)+"/"+std::to_string(y) + " " + std::to_string(width)+"x"+std::to_string(height) + ", owner " + std::to_string(owner);
}

//
// add_sub_textures(..)
//

int add_sub_textures(std::vector<std::shared_ptr<texture_t>>& storage, SDL_Renderer* rend, const std::string& filename, int w, int h, int x_off)
{
    std::unique_ptr<texture_t> all = std::make_unique<texture_t>(rend, filename);
    all->disown();
    if( DEBUG_LOG ) {
        log_printf("add_sub_textures: each ( %d + %d ) x %d, all: %s\n", w, x_off, h, all->toString().c_str());
    }
    const size_t size_start = storage.size();

    for(int y=0; y<all->get_height(); y+=h) {
        for(int x=0; x<all->get_width(); x+=w+x_off) {
            if( storage.size() > size_start ) {
                storage[ storage.size() - 1 ]->disown(); // only last entry is owner of the SDL_Texture
            }
            storage.push_back( std::make_shared<texture_t>(all->get_sdltex(), x, y, w, h, true /* owner*/) );
            if( DEBUG_LOG ) {
                log_printf("add_sub_textures: tex %zd [%d][%d]: %s\n", storage.size()-1, x, y, storage[ storage.size() - 1 ]->toString().c_str());
            }
        }
    }
    return storage.size() - size_start;
}

int add_sub_textures(std::vector<std::shared_ptr<texture_t>>& storage, SDL_Renderer* rend, const std::shared_ptr<texture_t>& global_texture,
                     int x_off, int y_off, int w, int h, const std::vector<tex_sub_coord_t>& tex_positions)
{
    if( DEBUG_LOG ) {
        log_printf("add_sub_textures: each %d x %d, all: %s\n", w, h, global_texture->toString().c_str());
    }
    const size_t size_start = storage.size();

    for(tex_sub_coord_t p : tex_positions) {
        const int x = x_off+p.x;
        const int y = y_off+p.y;
        if( 0 <= x && 0 <= y && x+w <= global_texture->get_width() && y+h <= global_texture->get_height() ) {
            storage.push_back( std::make_shared<texture_t>(global_texture->get_sdltex(), x, y, w, h, false /* owner*/) );
        } else {
            storage.push_back( std::make_shared<texture_t>() );
        }
        if( DEBUG_LOG ) {
            log_printf("add_sub_textures: tex %zd [%d][%d]: %s\n", storage.size()-1, x, y, storage[ storage.size() - 1 ]->toString().c_str());
        }
    }
    return storage.size() - size_start;
}

//
// animtex_t
//

animtex_t::animtex_t(std::string name_, int ms_per_atex_, const std::vector<std::shared_ptr<texture_t>>& textures_) {
    for(size_t i=0; i<textures_.size(); ++i) {
        texture_t& o = *textures_[i];
        textures.push_back( std::make_shared<texture_t>(o.get_sdltex(), o.get_x(), o.get_y(), o.get_width(), o.get_height(), false /* owner */) );
    }
    ms_per_atex = ms_per_atex_;
    atex_ms_left = 0;
    animation_index = 0;
    paused = false;
}
animtex_t::animtex_t(std::string name_, SDL_Renderer* rend, int ms_per_atex_, const std::vector<const char*>& filenames)
: name(name_)
{
    for(size_t i=0; i<filenames.size(); ++i) {
        textures.push_back( std::make_shared<texture_t>(rend, filenames[i]) );
    }
    ms_per_atex = ms_per_atex_;
    atex_ms_left = 0;
    animation_index = 0;
    paused = false;
}

animtex_t::animtex_t(std::string name_, SDL_Renderer* rend, int ms_per_atex_, const std::string& filename, int w, int h, int x_off)
: name(name_)
{
    add_sub_textures(textures, rend, filename, w, h, x_off);
    ms_per_atex = ms_per_atex_;
    atex_ms_left = 0;
    animation_index = 0;
    paused = false;
}

animtex_t::animtex_t(std::string name_, SDL_Renderer* rend, int ms_per_atex_, const std::shared_ptr<texture_t>& global_texture,
                     int x_off, int y_off, int w, int h, const std::vector<tex_sub_coord_t>& tex_positions)
: name(name_)
{
    add_sub_textures(textures, rend, global_texture, x_off, y_off, w, h, tex_positions);
    ms_per_atex = ms_per_atex_;
    atex_ms_left = 0;
    animation_index = 0;
    paused = false;
}

void animtex_t::destroy() {
    for(size_t i=0; i<textures.size(); ++i) {
        textures[i]->destroy();
    }
    textures.clear();
}

void animtex_t::pause(bool enable) {
    paused = enable;
    if( enable ) {
        animation_index = 0;
    }
}

void animtex_t::reset() {
    animation_index = 0;
    atex_ms_left = ms_per_atex;
}

void animtex_t::tick() {
    if( !paused ) {
        if( 0 < atex_ms_left ) {
            atex_ms_left = std::max( 0, atex_ms_left - get_ms_per_frame() );
        }
        if( 0 == atex_ms_left ) {
            atex_ms_left = ms_per_atex;
            if( textures.size() > 0 ) {
                animation_index = ( animation_index + 1 ) % textures.size();
            } else {
                animation_index = 0;
            }
        }
    }
}

std::string animtex_t::toString() const {
    std::shared_ptr<const texture_t> tex = get_tex();
    std::string tex_s = nullptr != tex ? tex->toString() : "null";
    return name+"[anim "+std::to_string(atex_ms_left)+"/"+std::to_string(ms_per_atex)+
            " ms, paused "+std::to_string(paused)+", idx "+std::to_string(animation_index)+"/"+std::to_string(textures.size())+
            ", textures["+tex_s+"]]";
}

std::shared_ptr<text_texture_t> draw_text(SDL_Renderer* rend, TTF_Font* font, const std::string& text, int x, int y, uint8_t r, uint8_t g, uint8_t b) {
    SDL_Color foregroundColor = { r, g, b, 255 };

    SDL_Surface* textSurface = TTF_RenderText_Solid(font, text.c_str(), foregroundColor);
    if( nullptr != textSurface ) {
        std::shared_ptr<text_texture_t> ttex = std::make_shared<text_texture_t>(text, rend, textSurface, false, x, y);
        SDL_FreeSurface(textSurface);
        ttex->texture.draw_scaled_dim(rend, x, y);
        // log_print("draw_text: '%s', tex %s\n", text.c_str(), tex.toString().c_str());
        return ttex;
    } else {
        log_printf("draw_text: Null texture for '%s': %s\n", text.c_str(), SDL_GetError());
        return nullptr;
    }
}

std::shared_ptr<text_texture_t> draw_text_scaled(SDL_Renderer* rend, TTF_Font* font, const std::string& text, uint8_t r, uint8_t g, uint8_t b, std::function<void(const texture_t& tex, int &x, int&y)> scaled_coord) {
    SDL_Color foregroundColor = { r, g, b, 255 };

    SDL_Surface* textSurface = TTF_RenderText_Solid(font, text.c_str(), foregroundColor);
    if( nullptr != textSurface ) {
        std::shared_ptr<text_texture_t> ttex = std::make_shared<text_texture_t>(text, rend, textSurface, true, 0, 0);
        SDL_FreeSurface(textSurface);
        scaled_coord(ttex->texture, ttex->x_pos, ttex->y_pos);
        ttex->texture.draw_scaled_dimpos(rend, ttex->x_pos, ttex->y_pos);
        // log_print("draw_text: '%s', tex %s\n", text.c_str(), tex.toString().c_str());
        return ttex;
    } else {
        log_printf("draw_text: Null texture for '%s': %s\n", text.c_str(), SDL_GetError());
        return nullptr;
    }
}

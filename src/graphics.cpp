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

texture_t::texture_t(SDL_Renderer* rend, const std::string& fname) noexcept
: id_(counter++)
{
    SDL_Surface* surface = IMG_Load(fname.c_str());
    if( nullptr != surface ) {
        if( DEBUG_LOG ) {
            log_printf("texture_t::surface: fmt %u 0x%X, %d x %d pitch %d\n", surface->format->format, surface->format->format, surface->w, surface->h, surface->pitch);
        }
        tex_ = SDL_CreateTextureFromSurface(rend, surface);
        SDL_FreeSurface(surface);
        if( nullptr == tex_ ) {
            log_printf("texture_t: Error loading %s: %s\n", fname.c_str(), SDL_GetError());
        }
    } else {
        tex_ = nullptr;
        log_printf("texture_t::surface: Error loading %s: %s\n", fname.c_str(), SDL_GetError());
    }
    x_ = 0;
    y_ = 0;
    width_ = 0;
    height_ = 0;
    Uint32 format = 0;
    if( nullptr != tex_ ) {
        SDL_QueryTexture(tex_, &format, NULL, &width_, &height_);
        if( DEBUG_LOG ) {
            log_printf("texture_t: fmt %u 0x%X, %d x %d\n", format, format, width_, height_);
        }
    }
    owner_ = true;
}

texture_t::texture_t(SDL_Renderer* rend, SDL_Surface* surface) noexcept
: id_(counter++)
{
    tex_ = SDL_CreateTextureFromSurface(rend, surface);
    x_ = 0;
    y_ = 0;
    SDL_QueryTexture(tex_, NULL, NULL, &width_, &height_);
    owner_ = true;
}

void texture_t::destroy() noexcept {
    if( owner_ && nullptr != tex_ ) {
        SDL_DestroyTexture(tex_);
    }
    tex_ = nullptr;
}

void texture_t::draw_scaled_dimpos(SDL_Renderer* rend, const int x_pos, const int y_pos) noexcept {
    if( nullptr != tex_ ) {
        SDL_Rect src = { .x=x_, .y=y_, .w=width_, .h=height_};
        const int win_pixel_offset = ( win_pixel_width - global_maze->pixel_width()*win_pixel_scale ) / 2;
        SDL_Rect dest = { .x=win_pixel_offset + x_pos,
                          .y=y_pos,
                          .w=width_, .h=height_ };
        SDL_RenderCopy(rend, tex_, &src, &dest);
    }
}
void texture_t::draw_scaled_dim(SDL_Renderer* rend, const int x_pos, const int y_pos) noexcept {
    if( nullptr != tex_ ) {
        SDL_Rect src = { .x=x_, .y=y_, .w=width_, .h=height_};
        const int win_pixel_offset = ( win_pixel_width - global_maze->pixel_width()*win_pixel_scale ) / 2;
        SDL_Rect dest = { .x=win_pixel_offset + ( global_maze->x_to_pixel(x_pos, win_pixel_scale) ),
                          .y=global_maze->y_to_pixel(y_pos, win_pixel_scale),
                          .w=width_, .h=height_ };
        SDL_RenderCopy(rend, tex_, &src, &dest);
    }
}
void texture_t::draw(SDL_Renderer* rend, const int x_pos, const int y_pos) noexcept {
    if( nullptr != tex_ ) {
        SDL_Rect src = { .x=x_, .y=y_, .w=width_, .h=height_};
        const int win_pixel_offset = ( win_pixel_width - global_maze->pixel_width()*win_pixel_scale ) / 2;
        SDL_Rect dest = { .x=win_pixel_offset + ( global_maze->x_to_pixel(x_pos, win_pixel_scale) ),
                          .y=global_maze->y_to_pixel(y_pos, win_pixel_scale),
                          .w=width_*win_pixel_scale, .h=height_*win_pixel_scale };
        SDL_RenderCopy(rend, tex_, &src, &dest);
    }
}
void texture_t::draw2_i(SDL_Renderer* rend, const int x_pos, const int y_pos) noexcept {
    if( nullptr != tex_ ) {
        SDL_Rect src = { .x=x_, .y=y_, .w=width_, .h=height_};
        const int win_pixel_offset = ( win_pixel_width - global_maze->pixel_width()*win_pixel_scale ) / 2;
        const int dxy = ( ( global_maze->ppt_y() * win_pixel_scale ) / 3 );
        SDL_Rect dest = { .x=win_pixel_offset + ( ( x_pos * global_maze->ppt_y() * win_pixel_scale ) - dxy ),
                          .y= ( ( y_pos * global_maze->ppt_y() * win_pixel_scale ) - dxy ),
                          .w=width_*win_pixel_scale, .h=height_*win_pixel_scale };
        SDL_RenderCopy(rend, tex_, &src, &dest);
    }
}
void texture_t::draw(SDL_Renderer* rend, const float x_pos, const float y_pos) noexcept {
    if( nullptr != tex_ ) {
        SDL_Rect src = { .x=x_, .y=y_, .w=width_, .h=height_};
        const int win_pixel_offset = ( win_pixel_width - global_maze->pixel_width()*win_pixel_scale ) / 2;
        SDL_Rect dest = { .x=win_pixel_offset + global_maze->x_to_pixel(x_pos, win_pixel_scale),
                          .y=global_maze->y_to_pixel(y_pos, win_pixel_scale),
                          .w=width_*win_pixel_scale, .h=height_*win_pixel_scale };
        SDL_RenderCopy(rend, tex_, &src, &dest);
    }
}
void texture_t::draw2_f(SDL_Renderer* rend, const float x_pos, const float y_pos) noexcept {
    if( nullptr != tex_ ) {
        SDL_Rect src = { .x=x_, .y=y_, .w=width_, .h=height_};
        const int win_pixel_offset = ( win_pixel_width - global_maze->pixel_width()*win_pixel_scale ) / 2;
        const int dxy = ( ( global_maze->ppt_y() * win_pixel_scale ) / 3 );
        SDL_Rect dest = { .x=win_pixel_offset + ( round_to_int( x_pos * global_maze->ppt_y() * win_pixel_scale ) - dxy ),
                          .y= ( round_to_int( y_pos * global_maze->ppt_y() * win_pixel_scale ) - dxy ),
                          .w=width_*win_pixel_scale, .h=height_*win_pixel_scale };
        SDL_RenderCopy(rend, tex_, &src, &dest);
    }
}

std::string texture_t::toString() const noexcept {
    return "id "+std::to_string(id_) + " " + std::to_string(x_)+"/"+std::to_string(y_) + " " + std::to_string(width_)+"x"+std::to_string(height_) + ", owner " + std::to_string(owner_);
}

//
// add_sub_textures(..)
//

int add_sub_textures(std::vector<std::shared_ptr<texture_t>>& storage, SDL_Renderer* rend, const std::string& filename, int w, int h, int x_off) noexcept
{
    std::unique_ptr<texture_t> all = std::make_unique<texture_t>(rend, filename);
    all->disown();
    if( DEBUG_LOG ) {
        log_printf("add_sub_textures: each ( %d + %d ) x %d, all: %s\n", w, x_off, h, all->toString().c_str());
    }
    const size_t size_start = storage.size();

    for(int y=0; y<all->height(); y+=h) {
        for(int x=0; x<all->width(); x+=w+x_off) {
            if( storage.size() > size_start ) {
                storage[ storage.size() - 1 ]->disown(); // only last entry is owner of the SDL_Texture
            }
            storage.push_back( std::make_shared<texture_t>(all->sdl_texture(), x, y, w, h, true /* owner*/) );
            if( DEBUG_LOG ) {
                log_printf("add_sub_textures: tex %zd [%d][%d]: %s\n", storage.size()-1, x, y, storage[ storage.size() - 1 ]->toString().c_str());
            }
        }
    }
    return storage.size() - size_start;
}

int add_sub_textures(std::vector<std::shared_ptr<texture_t>>& storage, SDL_Renderer* rend, const std::shared_ptr<texture_t>& global_texture,
                     int x_off, int y_off, int w, int h, const std::vector<tex_sub_coord_t>& tex_positions) noexcept
{
    if( DEBUG_LOG ) {
        log_printf("add_sub_textures: each %d x %d, all: %s\n", w, h, global_texture->toString().c_str());
    }
    const size_t size_start = storage.size();

    for(tex_sub_coord_t p : tex_positions) {
        const int x = x_off+p.x;
        const int y = y_off+p.y;
        if( 0 <= x && 0 <= y && x+w <= global_texture->width() && y+h <= global_texture->height() ) {
            storage.push_back( std::make_shared<texture_t>(global_texture->sdl_texture(), x, y, w, h, false /* owner*/) );
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

animtex_t::animtex_t(std::string name, int ms_per_atex, const std::vector<std::shared_ptr<texture_t>>& textures) noexcept
: name_( name )
{
    for(size_t i=0; i<textures.size(); ++i) {
        texture_t& o = *textures[i];
        textures_.push_back( std::make_shared<texture_t>(o.sdl_texture(), o.x(), o.y(), o.width(), o.height(), false /* owner */) );
    }
    ms_per_atex_ = ms_per_atex;
    atex_ms_left_ = 0;
    animation_index_ = 0;
    paused_ = false;
}

animtex_t::animtex_t(std::string name, SDL_Renderer* rend, int ms_per_atex, const std::vector<const char*>& filenames) noexcept
: name_( name )
{
    for(size_t i=0; i<filenames.size(); ++i) {
        textures_.push_back( std::make_shared<texture_t>(rend, filenames[i]) );
    }
    ms_per_atex_ = ms_per_atex;
    atex_ms_left_ = 0;
    animation_index_ = 0;
    paused_ = false;
}

animtex_t::animtex_t(std::string name, SDL_Renderer* rend, int ms_per_atex, const std::string& filename, int w, int h, int x_off) noexcept
: name_( name )
{
    add_sub_textures(textures_, rend, filename, w, h, x_off);
    ms_per_atex_ = ms_per_atex;
    atex_ms_left_ = 0;
    animation_index_ = 0;
    paused_ = false;
}

animtex_t::animtex_t(std::string name, SDL_Renderer* rend, int ms_per_atex, const std::shared_ptr<texture_t>& global_texture,
                     int x_off, int y_off, int w, int h, const std::vector<tex_sub_coord_t>& tex_positions) noexcept
: name_(name)
{
    add_sub_textures(textures_, rend, global_texture, x_off, y_off, w, h, tex_positions);
    ms_per_atex_ = ms_per_atex;
    atex_ms_left_ = 0;
    animation_index_ = 0;
    paused_ = false;
}

void animtex_t::destroy() noexcept {
    for(size_t i=0; i<textures_.size(); ++i) {
        textures_[i]->destroy();
    }
    textures_.clear();
}

void animtex_t::pause(bool enable) noexcept {
    paused_ = enable;
    if( enable ) {
        animation_index_ = 0;
    }
}

void animtex_t::reset() noexcept {
    animation_index_ = 0;
    atex_ms_left_ = ms_per_atex_;
}

void animtex_t::tick() noexcept {
    if( !paused_ ) {
        if( 0 < atex_ms_left_ ) {
            atex_ms_left_ = std::max( 0, atex_ms_left_ - get_ms_per_frame() );
        }
        if( 0 == atex_ms_left_ ) {
            atex_ms_left_ = ms_per_atex_;
            if( textures_.size() > 0 ) {
                animation_index_ = ( animation_index_ + 1 ) % textures_.size();
            } else {
                animation_index_ = 0;
            }
        }
    }
}

void animtex_t::draw(SDL_Renderer* rend, const float x, const float y) noexcept {
    std::shared_ptr<texture_t> tex = texture();
    if( nullptr != tex ) {
        tex->draw2_f(rend, x, y);
    }
}

std::string animtex_t::toString() const noexcept {
    std::shared_ptr<const texture_t> tex = texture();
    std::string tex_s = nullptr != tex ? tex->toString() : "null";
    return name_+"[anim "+std::to_string(atex_ms_left_)+"/"+std::to_string(ms_per_atex_)+
            " ms, paused "+std::to_string(paused_)+", idx "+std::to_string(animation_index_)+"/"+std::to_string(textures_.size())+
            ", textures["+tex_s+"]]";
}

std::shared_ptr<text_texture_t> draw_text(SDL_Renderer* rend, TTF_Font* font, const std::string& text, int x, int y, uint8_t r, uint8_t g, uint8_t b) noexcept {
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

std::shared_ptr<text_texture_t> draw_text_scaled(SDL_Renderer* rend, TTF_Font* font, const std::string& text, uint8_t r, uint8_t g, uint8_t b,
                                                 std::function<void(const texture_t& tex_, int &x_, int&y_)> scaled_coord) noexcept {
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

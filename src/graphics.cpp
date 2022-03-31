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

#include <thread>
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
        const int win_pixel_offset = ( win_pixel_width() - global_maze->pixel_width()*win_pixel_scale() ) / 2;
        SDL_Rect dest = { .x=win_pixel_offset + x_pos,
                          .y=y_pos,
                          .w=width_, .h=height_ };
        SDL_RenderCopy(rend, tex_, &src, &dest);
    }
}
void texture_t::draw_scaled_dim(SDL_Renderer* rend, const int x_pos, const int y_pos) noexcept {
    if( nullptr != tex_ ) {
        SDL_Rect src = { .x=x_, .y=y_, .w=width_, .h=height_};
        const int win_pixel_offset = ( win_pixel_width() - global_maze->pixel_width()*win_pixel_scale() ) / 2;
        SDL_Rect dest = { .x=win_pixel_offset + ( global_maze->x_to_pixel(x_pos, win_pixel_scale()) ),
                          .y=global_maze->y_to_pixel(y_pos, win_pixel_scale()),
                          .w=width_, .h=height_ };
        SDL_RenderCopy(rend, tex_, &src, &dest);
    }
}
void texture_t::draw(SDL_Renderer* rend, const int x_pos, const int y_pos) noexcept {
    if( nullptr != tex_ ) {
        SDL_Rect src = { .x=x_, .y=y_, .w=width_, .h=height_};
        const int win_pixel_offset = ( win_pixel_width() - global_maze->pixel_width()*win_pixel_scale() ) / 2;
        SDL_Rect dest = { .x=win_pixel_offset + ( global_maze->x_to_pixel(x_pos, win_pixel_scale()) ),
                          .y=global_maze->y_to_pixel(y_pos, win_pixel_scale()),
                          .w=width_*win_pixel_scale(), .h=height_*win_pixel_scale() };
        SDL_RenderCopy(rend, tex_, &src, &dest);
    }
}
void texture_t::draw2_i(SDL_Renderer* rend, const int x_pos, const int y_pos) noexcept {
    if( nullptr != tex_ ) {
        SDL_Rect src = { .x=x_, .y=y_, .w=width_, .h=height_};
        const int win_pixel_offset = ( win_pixel_width() - global_maze->pixel_width()*win_pixel_scale() ) / 2;
        const int dxy = ( ( global_maze->ppt_y() * win_pixel_scale() ) / 3 );
        SDL_Rect dest = { .x=win_pixel_offset + ( ( x_pos * global_maze->ppt_x() * win_pixel_scale() ) - dxy ),
                          .y= ( ( y_pos * global_maze->ppt_y() * win_pixel_scale() ) - dxy ),
                          .w=width_*win_pixel_scale(), .h=height_*win_pixel_scale() };
        SDL_RenderCopy(rend, tex_, &src, &dest);
    }
}
void texture_t::draw(SDL_Renderer* rend, const float x_pos, const float y_pos) noexcept {
    if( nullptr != tex_ ) {
        SDL_Rect src = { .x=x_, .y=y_, .w=width_, .h=height_};
        const int win_pixel_offset = ( win_pixel_width() - global_maze->pixel_width()*win_pixel_scale() ) / 2;
        SDL_Rect dest = { .x=win_pixel_offset + global_maze->x_to_pixel(x_pos, win_pixel_scale()),
                          .y=global_maze->y_to_pixel(y_pos, win_pixel_scale()),
                          .w=width_*win_pixel_scale(), .h=height_*win_pixel_scale() };
        SDL_RenderCopy(rend, tex_, &src, &dest);
    }
}
void texture_t::draw2_f(SDL_Renderer* rend, const float x_pos, const float y_pos) noexcept {
    if( nullptr != tex_ ) {
        SDL_Rect src = { .x=x_, .y=y_, .w=width_, .h=height_};
        const int win_pixel_offset = ( win_pixel_width() - global_maze->pixel_width()*win_pixel_scale() ) / 2;
        const int dxy = ( ( global_maze->ppt_y() * win_pixel_scale() ) / 3 );
        SDL_Rect dest = { .x=win_pixel_offset + ( round_to_int( x_pos * global_maze->ppt_x() * win_pixel_scale() ) - dxy ),
                          .y= ( round_to_int( y_pos * global_maze->ppt_y() * win_pixel_scale() ) - dxy ),
                          .w=width_*win_pixel_scale(), .h=height_*win_pixel_scale() };
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
        tex->draw(rend, x, y);
    }
}

void animtex_t::draw2(SDL_Renderer* rend, const float x, const float y) noexcept {
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

void text_texture_t::draw(SDL_Renderer* rend) noexcept {
    if( scaled_pos ) {
        tex.draw_scaled_dimpos(rend, x_pos, y_pos);
    } else {
        tex.draw_scaled_dim(rend, x_pos, y_pos);
    }
}

void text_texture_t::draw(SDL_Renderer* rend, bool scaled_pos_, int x_, int y_) noexcept {
    if( scaled_pos_ ) {
        tex.draw_scaled_dimpos(rend, x_, y_);
    } else {
        tex.draw_scaled_dim(rend, x_, y_);
    }
}

std::string text_texture_t::toString() const noexcept {
    return "ttext['"+text+"', "+std::to_string(x_pos)+"/"+std::to_string(y_pos)+", scaled "+std::to_string(scaled_pos)+": "+tex.toString()+"]";
}

static text_texture_cache_t text_texture_cache;

text_texture_ref get_text_texture_cache(const std::string& key) noexcept {
    if( true ) {
        return text_texture_cache[ key ];
    } else {
        text_texture_ref ttex = text_texture_cache[ key ];
        if( nullptr == ttex ) {
            log_printf("text_texture_cache::get: Missing '%s'\n", key.c_str());
        } else {
            log_printf("text_texture_cache::get: Has '%s' -> %s\n", key.c_str(), ttex->toString().c_str());
        }
        return ttex;
    }
}

void put_text_texture_cache(const std::string& key, text_texture_ref ttex) noexcept {
    text_texture_cache[ key ] = ttex;
    if( false ) {
        if( nullptr == ttex ) {
            log_printf("text_texture_cache::put: Empty '%s'\n", key.c_str());
        } else {
            log_printf("text_texture_cache::put: Filld '%s' <- %s\n", key.c_str(), ttex->toString().c_str());
        }
    }
}

void clear_text_texture_cache() noexcept {
    text_texture_cache.clear();
}

text_texture_ref draw_text(SDL_Renderer* rend, TTF_Font* font, const std::string& text, int x, int y, uint8_t r, uint8_t g, uint8_t b, const bool use_cache) noexcept
{
    if( nullptr == rend || nullptr == font ) {
        return nullptr;
    }
    text_texture_ref ttex = use_cache ? get_text_texture_cache(text) : nullptr;
    if( nullptr == ttex ) {
        SDL_Color foregroundColor = { r, g, b, 255 };

        SDL_Surface* textSurface = TTF_RenderText_Solid(font, text.c_str(), foregroundColor);
        if( nullptr != textSurface ) {
            ttex = std::make_shared<text_texture_t>(text, rend, textSurface, false, x, y);
            SDL_FreeSurface(textSurface);
            ttex->tex.draw_scaled_dim(rend, x, y);
            // log_print("draw_text: '%s', tex %s\n", text.c_str(), tex.toString().c_str());
            return ttex;
        } else {
            log_printf("draw_text: Null texture for '%s': %s\n", text.c_str(), SDL_GetError());
            return nullptr;
        }
    } else {
        ttex->draw(rend, false /* scaled_pos_ */, x, y);
        return ttex;
    }
}

text_texture_ref draw_text_scaled(SDL_Renderer* rend, TTF_Font* font, const std::string& text, uint8_t r, uint8_t g, uint8_t b,
                                  const bool use_cache,
                                  std::function<void(const texture_t& tex_, int &x_, int&y_)> scaled_coord) noexcept
{
    if( nullptr == rend || nullptr == font ) {
        return nullptr;
    }
    text_texture_ref ttex = use_cache ? get_text_texture_cache(text) : nullptr;
    if( nullptr == ttex ) {
        SDL_Color foregroundColor = { r, g, b, 255 };

        SDL_Surface* textSurface = TTF_RenderText_Solid(font, text.c_str(), foregroundColor);
        if( nullptr != textSurface ) {
            ttex = std::make_shared<text_texture_t>(text, rend, textSurface, true, 0, 0);
            SDL_FreeSurface(textSurface);
            scaled_coord(ttex->tex, ttex->x_pos, ttex->y_pos);
            ttex->draw(rend);
            // log_print("draw_text: '%s', tex %s\n", text.c_str(), tex.toString().c_str());
            if( use_cache ) {
                put_text_texture_cache(text, ttex);
            }
            return ttex;
        } else {
            log_printf("draw_text: Null texture for '%s': %s\n", text.c_str(), SDL_GetError());
            return nullptr;
        }
    } else {
        scaled_coord(ttex->tex, ttex->x_pos, ttex->y_pos);
        ttex->draw(rend);
        return ttex;
    }
}

void draw_box(SDL_Renderer* rend, bool filled, int x_pixel_offset, int y_pixel_offset,
              float x, float y, float width, float height) noexcept
{
    SDL_Rect bounds = {
            .x=x_pixel_offset + global_maze->x_to_pixel(x, win_pixel_scale()),
            .y=y_pixel_offset + global_maze->y_to_pixel(y, win_pixel_scale()),
            .w=global_maze->x_to_pixel(width, win_pixel_scale()),
            .h=global_maze->y_to_pixel(height, win_pixel_scale())};
    if( filled ) {
        SDL_RenderFillRect(rend, &bounds);
    } else {
        SDL_RenderDrawRect(rend, &bounds);
    }
}

void draw_line(SDL_Renderer* rend, int pixel_width_scaled, int x_pixel_offset, int y_pixel_offset,
               float x1, float y1, float x2, float y2) noexcept
{
    if( 0 >= pixel_width_scaled ) {
        return;
    }
    const int x1_i = x_pixel_offset + global_maze->x_to_pixel( x1, win_pixel_scale());
    const int y1_i = y_pixel_offset + global_maze->y_to_pixel( y1, win_pixel_scale());
    const int x2_i = x_pixel_offset + global_maze->x_to_pixel( x2, win_pixel_scale());
    const int y2_i = y_pixel_offset + global_maze->y_to_pixel( y2, win_pixel_scale());
    const int d_extra = pixel_width_scaled - 1;
    if( 0 == d_extra ) {
        SDL_RenderDrawLine(rend, x1_i, y1_i, x2_i, y2_i);
        return;
    }
    const float d_x = std::abs( x1_i - x2_i );
    const float d_y = std::abs( y1_i - y2_i );

    // Create a polygon of connected dots, aka poly-line
    const int c_l = -1 * ( d_extra / 2 + d_extra % 2 ); // gets the remainder
    const int c_r =        d_extra / 2;
    std::vector<SDL_Point> points;
    bool first_of_two = true;
    for(int i=c_l; i<=c_r; ++i) {
        int ix, iy; // rough and simply thickness delta picking
        if( d_y > d_x ) {
            ix = i;
            iy = 0;
        } else {
            ix = 0;
            iy = i;
        }
        if( first_of_two ) {
            points.push_back( SDL_Point { x1_i+ix, y1_i+iy } );
            points.push_back( SDL_Point { x2_i+ix, y2_i+iy } );
            first_of_two = false;
        } else {
            points.push_back( SDL_Point { x2_i+ix, y2_i+iy } );
            points.push_back( SDL_Point { x1_i+ix, y1_i+iy } );
            first_of_two = true;
        }
    }
    const int err = SDL_RenderDrawLines(rend, points.data(), points.size());
    if( false ) {
        log_printf("Poly-Line thick_p %d, %d lines, d %.4f/%.4f, err %d, %s\n", pixel_width_scaled, points.size()/2, d_x, d_y, err, SDL_GetError());
    }
}

static std::atomic<int> active_threads = 0;

static void store_surface(SDL_Surface *sshot, char* fname) noexcept {
    active_threads++;
    if( false ) {
        fprintf(stderr, "XXX: %d: %s\n", active_threads.load(), fname);
    }
    SDL_SaveBMP(sshot, fname);
    free(fname);
    SDL_UnlockSurface(sshot);
    SDL_FreeSurface(sshot);
    active_threads--;
}

void save_snapshot(SDL_Renderer* rend, const int width, const int height, const std::string& fname) noexcept {
    SDL_Surface *sshot = SDL_CreateRGBSurface(0, width, height, 32, 0x00ff0000, 0x0000ff00, 0x000000ff, 0xff000000);
    SDL_LockSurface(sshot);
    SDL_RenderReadPixels(rend, NULL, SDL_PIXELFORMAT_ARGB8888, sshot->pixels, sshot->pitch);
    char * fname2 = strdup(fname.c_str());
    std::thread t(&store_surface, sshot, fname2);
    t.detach();
}

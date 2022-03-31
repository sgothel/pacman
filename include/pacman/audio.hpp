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
#ifndef PACMAN_AUDIO_HPP_
#define PACMAN_AUDIO_HPP_

#include <atomic>
#include <memory>
#include <vector>
#include <string>
#include <inttypes.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_mixer.h>
#include <SDL2/SDL_timer.h>

/**
 * Open the audio system.
 *
 * @param mix_channels
 * @param out_channel
 * @param out_frequency
 * @param out_sample_format
 * @param out_chunksize
 * @return
 */
bool audio_open(int mix_channels=16, int out_channel=2, int out_frequency=MIX_DEFAULT_FREQUENCY, Uint16 out_sample_format=AUDIO_S16SYS, int out_chunksize=1024);

/**
 * Close the audio system.
 */
void audio_close();

class audio_sample_t {
    private:
        std::unique_ptr<Mix_Chunk, void (*)(Mix_Chunk *)> chunk;
        int channel_playing;
        bool singly;

    public:
        audio_sample_t()
        : chunk(nullptr, nullptr), channel_playing(-1), singly(true) {}

        /**
         * Create a new instance.
         * @param fname
         * @param volume value from 0 to MIX_MAX_VOLUME
         */
        audio_sample_t(const std::string &fname, const bool single_play, const int volume=MIX_MAX_VOLUME/2);

        /**
         * Create an instance for single play use, see set_single_play().
         * @param fname
         * @param volume value from 0 to MIX_MAX_VOLUME
         */
        audio_sample_t(const std::string &fname, const int volume=MIX_MAX_VOLUME/2)
        : audio_sample_t(fname, true, volume) {}

        /**
         * Play this sample
         * @param loops pass 0 for infinity, otherwise the number of loops. Defaults to 1.
         * @see set_single_play()
         */
        void play(int loops=1);

        /**
         * Stop playing this sample.
         */
        void stop();

        /**
         *
         * @param volume value from 0 to MIX_MAX_VOLUME
         */
        void set_volume(int volume);

        /**
         * If enabled, only allow to play this sample at one at a time, otherwise it may be played in parallel multiple times.
         * @param enable
         */
        void set_single_play(bool enable) { singly = enable; }

        bool is_valid() const { return nullptr != chunk.get(); }
};

#endif /* PACMAN_AUDIO_HPP_ */

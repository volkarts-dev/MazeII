// Copyright 2026, Daniel Volk <mail@volkarts.com>
// SPDX-License-Identifier: MIT

#pragma once

#include "Macros.hpp"
#include <AL/al.h>

namespace ngn {

class AudioBuffer;

class Sound
{
public:
    Sound();
    Sound(uint32_t bufferId);
    Sound(AudioBuffer* buffer);
    ~Sound();

    void setBuffer(uint32_t bufferId);
    void setBuffer(AudioBuffer* buffer);

    void play() const;
    void stop() const;

    bool isPlaying() const;

private:
    ALuint source_;

    NGN_DISABLE_COPY_MOVE(Sound)
};

} // namespace ngn

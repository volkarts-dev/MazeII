// Copyright 2026, Daniel Volk <mail@volkarts.com>
// SPDX-License-Identifier: MIT

#pragma once

#include "Macros.hpp"
#include <AL/al.h>

namespace ngn {

class Audio;
class AudioFileResult;

class AudioBuffer
{
public:
    ~AudioBuffer();

    ALuint handle() const { return buffer_; }

private:
    AudioBuffer(const AudioFileResult& result);

    ALuint buffer_;

    NGN_DISABLE_COPY_MOVE(AudioBuffer)

    friend Audio;
};

} // namespace ngn

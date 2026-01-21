// Copyright 2026, Daniel Volk <mail@volkarts.com>
// SPDX-License-Identifier: MIT

#include "AudioBuffer.hpp"

#include "Audio.hpp"

namespace ngn {

AudioBuffer::AudioBuffer(const AudioFileResult& result) :
    buffer_{}
{
    if (!alCall(alGenBuffers, 1, &buffer_))
        return;

    if (!alCall(alBufferData, buffer_, result.format, result.data.data(),
                static_cast<int32_t>(result.data.size()), static_cast<int32_t>(result.sampleRate)))
    {
        alCall(alDeleteBuffers, 1, &buffer_);
        buffer_ = 0;
    }
}

AudioBuffer::~AudioBuffer()
{
    alCall(alDeleteBuffers, 1, &buffer_);
}

} // namespace ngn

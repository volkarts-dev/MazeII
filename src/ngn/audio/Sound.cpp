// Copyright 2026, Daniel Volk <mail@volkarts.com>
// SPDX-License-Identifier: MIT

#include "Sound.hpp"

#include "Audio.hpp"
#include "AudioBuffer.hpp"

namespace ngn {

Sound::Sound()
{
    alCall(alGenSources, 1, &source_);
    alCall(alSourcef, source_, AL_PITCH, 1.0f);
    alCall(alSourcef, source_, AL_GAIN, 1.0f);
    alCall(alSourcei, source_, AL_LOOPING, AL_FALSE);
}

Sound::Sound(uint32_t bufferId) :
    Sound{}
{
    setBuffer(bufferId);
}

Sound::Sound(AudioBuffer* buffer) :
    Sound{buffer->handle()}
{
}

Sound::~Sound()
{
    alCall(alDeleteSources, 1, &source_);
}

void Sound::setBuffer(uint32_t bufferId)
{
    alCall(alSourcei, source_, AL_BUFFER, static_cast<ALint>(bufferId));
}

void Sound::setBuffer(AudioBuffer* buffer)
{
    setBuffer(buffer->handle());
}

void Sound::play() const
{
    alCall(alSourcePlay, source_);
}

void Sound::stop() const
{
    alCall(alSourceStop, source_);
}

bool Sound::isPlaying() const
{
    ALint state{};
    alCall(alGetSourcei, source_, AL_SOURCE_STATE, &state);
    return state == AL_PLAYING;
}

} // namespace ngn

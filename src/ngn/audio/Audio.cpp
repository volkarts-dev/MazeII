// Copyright 2026, Daniel Volk <mail@volkarts.com>
// SPDX-License-Identifier: MIT

#include "Audio.hpp"

#include "AudioBuffer.hpp"
#include "StbVorbis.hpp"
#include <AL/al.h>

namespace ngn {

bool Audio::alCheckErrors()
{
    ALenum error = alGetError();
    if (error == AL_NO_ERROR)
        return true;

    switch(error)
    {
    case AL_INVALID_NAME:
        log::error("AL ERROR: AL_INVALID_NAME: a bad name (ID) was passed to an OpenAL function");
        break;
    case AL_INVALID_ENUM:
        log::error("AL ERROR: AL_INVALID_ENUM: an invalid enum value was passed to an OpenAL function");
        break;
    case AL_INVALID_VALUE:
        log::error("AL ERROR: AL_INVALID_VALUE: an invalid value was passed to an OpenAL function");
        break;
    case AL_INVALID_OPERATION:
        log::error("AL ERROR: AL_INVALID_OPERATION: the requested operation is not valid");
        break;
    case AL_OUT_OF_MEMORY:
        log::error("AL ERROR: AL_OUT_OF_MEMORY: the requested operation resulted in OpenAL running out of memory");
        break;
    default:
        log::error("AL ERROR: UNKNOWN AL ERROR: {}", error);
        break;
    }

    return false;
}

bool Audio::alcCeckErrors(ALCdevice* device)
{
    ALCenum error = alcGetError(device);
    if (error == ALC_NO_ERROR)
        return true;

    switch(error)
    {
    case ALC_INVALID_VALUE:
        log::error("AL ERROR: ALC_INVALID_VALUE: an invalid value was passed to an OpenAL function");
        break;
    case ALC_INVALID_DEVICE:
        log::error("AL ERROR: ALC_INVALID_DEVICE: a bad device was passed to an OpenAL function");
        break;
    case ALC_INVALID_CONTEXT:
        log::error("AL ERROR: ALC_INVALID_CONTEXT: a bad context was passed to an OpenAL function");
        break;
    case ALC_INVALID_ENUM:
        log::error("AL ERROR: ALC_INVALID_ENUM: an unknown enum value was passed to an OpenAL function");
        break;
    case ALC_OUT_OF_MEMORY:
        log::error("AL ERROR: ALC_OUT_OF_MEMORY: an unknown enum value was passed to an OpenAL function");
        break;
    default:
        log::error("AL ERROR: UNKNOWN ALC ERROR: {}", error);
    }

    return false;
}

// *********************************************************************************************************************

Audio::Audio()
{
    device_ = alcOpenDevice(nullptr);
    if (!device_)
        throw std::runtime_error("Could not open audio device");

    if (!alcCall(alcCreateContext, context_, device_, device_, nullptr) || !context_)
        throw std::runtime_error("Could not create audio context");

    ALCboolean contextMadeCurrent = false;
    if (!alcCall(alcMakeContextCurrent, contextMadeCurrent, device_, context_) || contextMadeCurrent != ALC_TRUE)
        throw std::runtime_error("Could not make audio context current");
}

Audio::~Audio()
{
    for (auto* buffer : audioBuffers_)
    {
        delete buffer;
    }

    ALCboolean contextMadeCurrent = false;
    alcCall(alcMakeContextCurrent, contextMadeCurrent, device_, nullptr);
    alcCall(alcDestroyContext, device_, context_);

    ALCboolean closed{};
    alcCall(alcCloseDevice, closed, device_, device_);
}

AudioBuffer* Audio::loadOGG(const BufferView& data)
{
    int channels{};
    int sampleRate{};
    short* buffer{};
    const auto samples = stb_vorbis_decode_memory(data.data(), static_cast<int>(data.size()),
                                                  &channels, &sampleRate, &buffer);
    
    // Assume stb_vorbis_decode_memory() returns a consistant state,
    // so we can rely on correct value in all the vars
    if (samples < 0)
        throw std::runtime_error("Invalid ogg file or decoder failure");
    const auto bufferSize = static_cast<std::size_t>(samples * channels) * sizeof(short);

    AudioFileResult result;
    result.data = BufferView{reinterpret_cast<unsigned char*>(buffer), bufferSize};
    if (channels == 1)
        result.format = AL_FORMAT_MONO16;
    else if (channels == 2)
        result.format = AL_FORMAT_STEREO16;
    result.sampleRate = static_cast<uint32_t>(sampleRate);

    auto* audioBuffer = addAudioBuffer(new AudioBuffer{result});

    std::free(buffer);

    return audioBuffer;
}

AudioBuffer* Audio::addAudioBuffer(AudioBuffer* buffer)
{
    audioBuffers_.emplace_back(buffer);
    return buffer;
}

} // namespace ngn

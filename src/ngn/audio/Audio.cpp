// Copyright 2026, Daniel Volk <mail@volkarts.com>
// SPDX-License-Identifier: MIT

#include "Audio.hpp"

#include "AudioBuffer.hpp"
#include "StbVorbis.hpp"
#include <AL/al.h>

namespace ngn {

namespace {

class WavFileInput
{
public:
    BufferView data{};
    std::size_t readPos{};

    bool eof() const
    {
        return readPos >= data.size();
    }

    std::size_t remain() const
    {
        return data.size() - readPos;
    }

    BufferView read(std::size_t bytes)
    {
        BufferView out{};
        if (readPos + bytes >= data.size())
            return {};
        out = {data.data() + readPos, bytes};
        readPos += bytes;
        return out;
    }
};

template<std::size_t N>
inline bool operator==(const BufferView& view, const char (&txt)[N])
{
    if (view.size() != N - 1)
        return false;
    return std::memcmp(view.data(), txt, view.size()) == 0;
}

template<std::size_t N>
inline bool operator!=(const BufferView& view, const char (&txt)[N])
{
    return !(view == txt);
}

template<typename T = uint32_t>
T toInt(const BufferView& buffer)
{
    assert(buffer.size() <= sizeof(T));
    T a = 0;
    if constexpr (std::endian::native == std::endian::little)
    {
        std::memcpy(&a, buffer.data(), buffer.size());
    }
    else
    {
        for (std::size_t i = 0; i < buffer.size(); ++i)
        {
            reinterpret_cast<uint8_t*>(&a)[3 - i] = buffer[i];
        }
    }
    return a;
}

AudioFileResult wavFileLoad(WavFileInput& input)
{
    AudioFileResult result;

    BufferView view;

    // the RIFF
    view = input.read(4);
    if (view != "RIFF")
        return result;

    // the size of the file
    view = input.read(4);
    if (view.empty())
        return result;

    // the WAVE
    view = input.read(4);
    if (view != "WAVE")
        return result;

    // "fmt/0"
    view = input.read(4);
    if (view.empty())
        return result;

    // this is always 16, the size of the fmt data chunk
    view = input.read(4);
    if (view.empty())
        return result;

    // PCM should be 1?
    view = input.read(2);
    if (view.empty())
        return result;

    // the number of channels
    view = input.read(2);
    if (view.empty())
        return result;
    const auto channels = toInt(view);

    // sample rate
    view = input.read(4);
    if (view.empty())
        return result;
    result.sampleRate = toInt(view);

    // (sampleRate * bitsPerSample * channels) / 8
    view = input.read(4);
    if (view.empty())
        return result;

    // ?? dafaq
    view = input.read(2);
    if (view.empty())
        return result;

    // bitsPerSample
    view = input.read(2);
    if (view.empty())
        return result;
    const auto bitsPerSample = toInt(view);

    // data chunk header "data"
    view = input.read(4);
    if (view != "data")
        return result;

    // size of data
    view = input.read(4);
    if (view.empty())
        return result;
    const auto size = toInt(view);

    /* cannot be at the end of file */
    if (input.remain() < size)
        return result;

    if (channels == 1 && bitsPerSample == 8)
        result.format = AL_FORMAT_MONO8;
    else if (channels == 1 && bitsPerSample == 16)
        result.format = AL_FORMAT_MONO16;
    else if (channels == 2 && bitsPerSample == 8)
        result.format = AL_FORMAT_STEREO8;
    else if (channels == 2 && bitsPerSample == 16)
        result.format = AL_FORMAT_STEREO16;
    else
        throw std::runtime_error("Invalid wave format");

    result.data = {input.data.data() + input.readPos, size};

    return result;
}

} // namespace

// *********************************************************************************************************************

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

AudioBuffer* Audio::loadWAV(const BufferView& data)
{
    WavFileInput input{data};
    const auto result = wavFileLoad(input);
    return addAudioBuffer(new AudioBuffer{result});
}

AudioBuffer* Audio::loadOGG(const BufferView& data)
{
    int channels{};
    int sampleRate{};
    short* buffer{};
    const auto samples = stb_vorbis_decode_memory(data.data(), static_cast<int>(data.size()), &channels, &sampleRate, &buffer);
    if (samples < 0)
        throw std::runtime_error("Invalid ogg file");
    const auto bufferSize = static_cast<std::size_t>(samples * channels) * sizeof(short);

    AudioFileResult result;
    result.data = BufferView{reinterpret_cast<unsigned char*>(buffer), bufferSize};
    if (channels == 1)
        result.format = AL_FORMAT_MONO16;
    else if (channels == 2)
        result.format = AL_FORMAT_STEREO16;
    result.sampleRate = static_cast<uint32_t>(sampleRate);

    return addAudioBuffer(new AudioBuffer{result});
}

AudioBuffer* Audio::addAudioBuffer(AudioBuffer* buffer)
{
    audioBuffers_.emplace_back(buffer);
    return buffer;
}

} // namespace ngn

// Copyright 2026, Daniel Volk <mail@volkarts.com>
// SPDX-License-Identifier: MIT

#pragma once

#include "Macros.hpp"
#include "Types.hpp"
#include <AL/al.h>
#include <AL/alc.h>

namespace ngn {

class AudioBuffer;

class AudioFileResult
{
public:
    ALenum format{};
    uint32_t sampleRate{};
    BufferView data{};
};

class Audio
{
public:
    static bool alCheckErrors();
    static bool alcCeckErrors(ALCdevice* device);

public:
    Audio();
    ~Audio();

    AudioBuffer* loadWAV(const BufferView& data);
    AudioBuffer* loadOGG(const BufferView& data);

private:
    AudioBuffer* addAudioBuffer(AudioBuffer* buffer);

private:
    ALCdevice* device_;
    ALCcontext* context_;
    std::vector<AudioBuffer*> audioBuffers_;

    NGN_DISABLE_COPY_MOVE(Audio)
};

template<typename Func, typename... Args> requires ReturnsNonVoid<Func, Args...>
inline auto alCall(Func function, Args... args) -> decltype(function(args...))
{
    auto ret = function(std::forward<Args>(args)...);
    Audio::alCheckErrors();
    return ret;
}

template<typename Func, typename... Args> requires ReturnsVoid<Func, Args...>
inline auto alCall(Func function, Args... args) -> bool
{
    function(std::forward<Args>(args)...);
    return Audio::alCheckErrors();
}

template<typename Func, typename... Args> requires ReturnsVoid<Func, Args...>
inline bool alcCall(Func function,  ALCdevice* device,  Args... args)
{
  function(std::forward<Args>(args)...);
  return Audio::alcCeckErrors(device);
}

template<typename Func, typename ReturnType, typename... Args> requires ReturnsNonVoid<Func, Args...>
inline bool alcCall(Func function, ReturnType& returnValue, ALCdevice* device, Args... args)
{
  returnValue = function(std::forward<Args>(args)...);
  return Audio::alcCeckErrors(device);
}

} // namespace ngn

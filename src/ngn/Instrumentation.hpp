// Copyright 2025, Daniel Volk <mail@volkarts.com>
// SPDX-License-Identifier: <LICENSE>

#pragma once

#if defined(NGN_ENABLE_INSTRUMENTATION)
#include "Macros.hpp"
#include <iostream>
#include <span>
//#include <source_location>
#endif

#if _WIN32

#include <intrin.h>
#include <windows.h>

namespace ngn::instrumentation {

inline uint64_t osTimerFreq()
{
    LARGE_INTEGER Freq;
    QueryPerformanceFrequency(&Freq);
    return Freq.QuadPart;
}

inline uint64_t osTimer()
{
    LARGE_INTEGER Value;
    QueryPerformanceCounter(&Value);
    return Value.QuadPart;
}

} // namespace ngn::instrumentation

#else

#include <cstdint>
#include <ctime>
#include <x86intrin.h>
#include <sys/time.h>

namespace ngn::instrumentation {

inline uint64_t osTimerFreq()
{
    // clock_getres return 1ns for CLOCK_MONOTONIC_RAW, so it's 1GHz
    return 1000000000;
}

inline uint64_t osTimer()
{
    timeval tv;
    gettimeofday(&tv, nullptr);
    timespec ts;
    clock_gettime(CLOCK_MONOTONIC_RAW, &ts);

    uint64_t result = osTimerFreq() * static_cast<uint64_t>(ts.tv_sec) + static_cast<uint64_t>(ts.tv_nsec);
    return result;
}

} // namespace ngn::instrumentation

#endif

namespace ngn::instrumentation {

inline uint64_t cpuTimer()
{
    return __rdtsc();
}

uint64_t calcCpuTimerFreq();

#if defined(NGN_ENABLE_INSTRUMENTATION)

struct TimerInfo
{
    uint64_t timeInclusive{};
    uint64_t timeExclusive{};
    uint64_t hitCount{};
    uint64_t processedBytes{};
    const char* name{};
};

extern TimerInfo* gActualTimerInfo;

struct TimerInfoChain
{
    TimerInfoChain(const char* n, std::span<TimerInfo> ti);

    TimerInfoChain* parent;
    const char* name;
    std::span<TimerInfo> timerInfos;
};

extern TimerInfoChain* gTimerInfoChain;

class ScopeTimer
{
public:
    ScopeTimer(TimerInfo* timerInfo, const char* name, uint64_t processedBytes) :
        timerInfo_{timerInfo},
        name_{name},
        processedBytes_{processedBytes},
        startTime_{cpuTimer()},
        startElapsedTime_{timerInfo_->timeInclusive},
        parentTimerInfo_(gActualTimerInfo)
    {
        gActualTimerInfo = timerInfo;
    }

    ~ScopeTimer()
    {
        stop();
    }

    void stop()
    {
        if (!timerInfo_)
            return;
        const auto elapsed = cpuTimer() - startTime_;
        timerInfo_->timeInclusive = startElapsedTime_ + elapsed;
        timerInfo_->timeExclusive += elapsed;
        timerInfo_->hitCount++;
        timerInfo_->processedBytes += processedBytes_;
        timerInfo_->name = name_;

        parentTimerInfo_->timeExclusive -= elapsed;

        gActualTimerInfo = parentTimerInfo_;

        timerInfo_ = nullptr;
    }

    void patchProcessedBytes(uint64_t processedBytes)
    {
        processedBytes_ = processedBytes;
    }

private:
    TimerInfo* timerInfo_;
    const char* name_;
    uint64_t processedBytes_;
    uint64_t startTime_;
    uint64_t startElapsedTime_;
    TimerInfo* parentTimerInfo_;

    NGN_DISABLE_COPY_MOVE(ScopeTimer)
};

void start();
void stop();
void dumpTimerInfos(std::ostream &output);

static TimerInfo* timerInfos(uint64_t id);

#define NGN_INSTRUMENTATION_EPILOG(name) \
    namespace ngn::instrumentation { \
    static constexpr uint64_t _timerInfos_##name##_size_ = __COUNTER__; \
    static TimerInfo _timerInfos_##name##_data_[_timerInfos_##name##_size_ + 1]; \
    static inline TimerInfo* timerInfos(uint64_t id) { return &_timerInfos_##name##_data_[id]; } \
    TimerInfoChain _timerInfos_##name##_{#name, std::span<TimerInfo>{_timerInfos_##name##_data_}}; \
    }

#define NGN_INSTRUMENTATION_MAIN_START() ngn::instrumentation::start()
#define NGN_INSTRUMENTATION_MAIN_STOP() ngn::instrumentation::stop()

#define NGN_INSTRUMENTENTATION_TIMER(var, name, id, bytes) \
    ::ngn::instrumentation::ScopeTimer _scopeTimer_##var{::ngn::instrumentation::timerInfos(id), name, bytes}; \
    ((void)_scopeTimer_##var)

#define NGN_INSTRUMENT_BLOCK(name) \
    EVAL(DEFER(NGN_INSTRUMENTENTATION_TIMER) (__LINE__, name, __COUNTER__, 0))

#define NGN_INSTRUMENT_BLOCK_VAR(var, name) \
    EVAL(DEFER(NGN_INSTRUMENTENTATION_TIMER) (var, name, __COUNTER__, 0))

#define NGN_INSTRUMENT_BLOCK_BANDWIDTH(name, bytes) \
    EVAL(DEFER(NGN_INSTRUMENTENTATION_TIMER) (__LINE__, name, __COUNTER__, bytes))

#define NGN_INSTRUMENT_BLOCK_BANDWIDTH_VAR(var, name, bytes) \
    EVAL(DEFER(NGN_INSTRUMENTENTATION_TIMER) (var, name, __COUNTER__, bytes))

#define NGN_INSTRUMENT_FUNCTION() \
    NGN_INSTRUMENT_BLOCK(__func__)

#define NGN_INSTRUMENT_FUNCTION_BANDWIDTH(bytes) \
    NGN_INSTRUMENT_BLOCK_BANDWIDTH(__func__, bytes)

#define NGN_SCOPETIMER_STOP(var) \
    _scopeTimer_##var.stop();

#else

#define NGN_INSTRUMENTATION_EPILOG(name)

#define NGN_INSTRUMENTATION_MAIN_START()
#define NGN_INSTRUMENTATION_MAIN_STOP()

#define NGN_INSTRUMENTENTATION_TIMER(name, id)
#define NGN_INSTRUMENT_BLOCK(name)
#define NGN_INSTRUMENT_BLOCK_VAR(var, name)
#define NGN_INSTRUMENT_BLOCK_BANDWIDTH(name, bytes)
#define NGN_INSTRUMENT_BLOCK_BANDWIDTH_VAR(var, name, bytes)
#define NGN_INSTRUMENT_FUNCTION()
#define NGN_INSTRUMENT_FUNCTION_BANDWIDTH(bytes)

#define NGN_SCOPETIMER_STOP(var)

#endif

} // namespace ngn::instrumentation

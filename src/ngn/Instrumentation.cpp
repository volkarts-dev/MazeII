// Copyright 2025, Daniel Volk <mail@volkarts.com>
// SPDX-License-Identifier: <LICENSE>

#include "Instrumentation.hpp"

#include "Logging.hpp"

namespace ngn::instrumentation {

uint64_t calcCpuTimerFreq()
{
    const uint64_t timerFreq = osTimerFreq();

    const uint64_t waitTime = timerFreq / 10;
    const uint64_t cpuStart = cpuTimer();
    const uint64_t osStart = osTimer();
    uint64_t osEnd = 0;
    uint64_t osElapsed = 0;

    while (osElapsed < waitTime)
    {
        osEnd = osTimer();
        osElapsed = osEnd - osStart;
    }

    const uint64_t cpuEnd = cpuTimer();
    const uint64_t cpuElapsed = cpuEnd - cpuStart;

    uint64_t cpuTimerFreq = 0;
    if (cpuElapsed)
    {
        cpuTimerFreq = timerFreq * cpuElapsed / osElapsed;
    }

    return cpuTimerFreq;
}

#if defined(NGN_ENABLE_INSTRUMENTATION)

namespace {

struct DoubleFormatter
{
    int width;
    int precision;
};

DoubleFormatter dbl(int width, int precision)
{
    return {.width = width, .precision = precision};
}

std::ostream& operator<<(std::ostream& out, const DoubleFormatter& info)
{
    out << std::fixed << std::setw(info.width) << std::setprecision(info.precision);
    return out;
}

} // namespace

TimerInfo* gActualTimerInfo{};
TimerInfo* gGlobalTimerInfo{};
uint64_t gGlobalStartTime{};

TimerInfoChain* gTimerInfoChain{};

TimerInfoChain::TimerInfoChain(const char* n, std::span<TimerInfo> ti) :
    parent{gTimerInfoChain},
    name{n},
    timerInfos{ti}
{
    gTimerInfoChain = this;
}

void start()
{
    gGlobalTimerInfo = timerInfos(__COUNTER__);
    gActualTimerInfo = gGlobalTimerInfo;
    gGlobalStartTime = cpuTimer();
}

void stop()
{
    const auto elapsed = cpuTimer() - gGlobalStartTime;

    assert(gActualTimerInfo == gGlobalTimerInfo);

    gGlobalTimerInfo->timeInclusive = elapsed;
    gGlobalTimerInfo->timeExclusive += elapsed;
    gGlobalTimerInfo->hitCount++;
    gGlobalTimerInfo->name = "[total]";
}

void dumpTimerInfos(std::ostream& output)
{
    const auto cpuTimerFreq = static_cast<double>(calcCpuTimerFreq());

    const auto totalElapsedTime = static_cast<double>(gGlobalTimerInfo->timeInclusive) / cpuTimerFreq;

    output << "CPU timer frequency: " << dbl(12, 0) << cpuTimerFreq << "\n";

    auto* chain = gTimerInfoChain;
    while (chain)
    {
        for (const auto& info : chain->timerInfos)
        {
            if (!info.name)
                break;

            const auto elapsedSelf = static_cast<double>(info.timeExclusive) / cpuTimerFreq;
            const auto elapsedSelfPer = elapsedSelf / totalElapsedTime * 100.0;

            constexpr int nameLen = 25;
            std::string name;
            name.reserve(64);
            name.append(chain->name);
            name.append("::");
            name.append(info.name);
            if (name.length() > nameLen)
                name.erase(0, name.length() - nameLen);

            output << std::setw(nameLen) << name
                   << ": hits: " << std::setw(9) << info.hitCount
                   << ", self: " << dbl(8, 4) << elapsedSelf << "s "
                   << "(" << dbl(4, 1) << elapsedSelfPer << "%)";

            if (info.timeInclusive != info.timeExclusive)
            {
                const auto elapsed = static_cast<double>(info.timeInclusive) / cpuTimerFreq;
                const auto elapsedPer = elapsed / totalElapsedTime * 100.0;

                output << ", total: " << dbl(8, 4) << elapsed << "s "
                       << "(" << dbl(4, 1) << elapsedPer << "%)";
            }

            if (info.processedBytes != 0)
            {
                if (info.timeInclusive == info.timeExclusive)
                {
                    output << ", " << std::setw(24) << " ";
                }

                const auto elapsed = static_cast<double>(info.timeInclusive) / cpuTimerFreq;

                double num{};
                std::string_view unit{};
                humanReadableBytes(info.processedBytes, num, unit);

                const auto gbps = static_cast<double>(info.processedBytes) / 1024.0 / 1024.0 / 1024.0 / elapsed;
                output << ", bytes: " << dbl(4, 1) << num << " " << unit
                       << "(" << dbl(3, 2) << gbps << "GB/s)";
            }

            output << "\n";
        }

        chain = chain->parent;
    }
}

#endif

} // namespace ngn

NGN_INSTRUMENTATION_EPILOG(Instrumentation)

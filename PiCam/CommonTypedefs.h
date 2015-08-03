#pragma once

#include <chrono>

namespace sch = std::chrono;

typedef sch::high_resolution_clock HighResolutionClock;
typedef sch::duration< HighResolutionClock::rep, HighResolutionClock::period > HighResolutionClockDuration;

#ifndef WIN32
typedef pid_t CameraPid;
#else
typedef int CameraPid;
#endif

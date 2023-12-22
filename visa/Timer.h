/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef _TIMER_H_
#define _TIMER_H_
// Enable this macro by default but comment out calls that print out timer info.
// Requirement by 3d team.
#if (defined(_DEBUG) || defined(_INTERNAL) || !defined(DLL_MODE)) &&           \
    !defined(MEASURE_COMPILATION_TIME)
#define MEASURE_COMPILATION_TIME
#endif

#include "Option.h"

// Timer library for the compiler
// To collect compile time information, do the following:
//
// initTimer()  // should be called once before creating any timers
// ...
// Update enum TIMERS,
// Update timerNames array
//
// startTimer/stopTimer can be called multiple times
//
// To read time, either invoke dumpAllTimers()
// or invoke other extern functions in Timer.cpp
// to get individual timer name, ticks, time count.
//
#define DEF_TIMER(ENUM, DESCR) ENUM,
enum class TimerID {
#include "TimerDefs.h"
  NUM_TIMERS
};

int createNewTimer(const char *timerName);
void initTimer();
void startTimer(TimerID timer);
void stopTimer(TimerID timer);
void dumpAllTimers(const char *asmFileName, bool outputTime = false);
void dumpEncoderStats(Options *opt, std::string &asmName);
void resetPerKernel();
// double getTimerUS(unsigned idx);

struct TimerScope {
  const TimerID timerId;
  TimerScope(const TimerID _timerId) : timerId(_timerId) {
    startTimer(timerId);
  }

  TimerScope(const TimerScope&) = delete;
  TimerScope& operator=(const TimerScope&) = delete;
  ~TimerScope() { stopTimer(timerId); }
};

#if defined(MEASURE_COMPILATION_TIME)
#define TIME_SCOPE(TIMER_ID) TimerScope __timerScope(TimerID::TIMER_ID);
#else
#define TIME_SCOPE(TIMER_ID)
#endif

#undef DEF_TIMER

#endif

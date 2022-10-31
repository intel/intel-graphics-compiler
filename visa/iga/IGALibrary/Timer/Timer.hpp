/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef IGA_TIMER_TIMER_HPP
#define IGA_TIMER_TIMER_HPP

#include <cstdint>
#include <string>

// Enable this macro by default but comment out calls that print out timer info.
// Requirement by 3d team.
#if defined(_DEBUG) || defined(_ENABLETIMER)
#define MEASURE_COMPILATION_TIME
#endif

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

int createIGANewTimer(const char *timerName);

void initIGATimer();
void startIGATimer(int timer);
void stopIGATimer(int timer);
void setIGAKernelName(const char *name);
void dumpAllIGATimers(bool outputTime = false);
std::string getIGATimerNames(unsigned int idx);
int64_t getIGATimerTicks(unsigned int idx);
double getIGATimerCounts(unsigned int idx);
double getIGATimerUS(unsigned int idx);

typedef enum TIMERS {
  TIMER_TOTAL = 0,
  TIMER_GED = 1,
  TIMER_NUM_TIMERS = 2
} TIMERS;

#endif // IGA_TIMER_TIMER_HPP

/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX // omit min()/max() macros (favor std::min/std::max)
#endif
#include <Windows.h>
#endif

#if !defined(_WIN32)
#include <time.h> // clock_gettime()
#endif

#include <cstdint>
#include <fstream>
#include <iostream>
#include <string>

#include "Timer.hpp"
#include "../strings.hpp"

// actually do real timing
#ifdef _MSC_VER
// MSVC way of declaring a thread-local variable
#define _THREAD __declspec(thread)
#elif ANDROID
// drop it on android
#define _THREAD
#define CLOCK_TYPE CLOCK_MONOTONIC
#else
// gcc, clang, mingw, and cygwin all use this
#define _THREAD __thread
#define CLOCK_TYPE CLOCK_MONOTONIC
#endif

static const char *timerNames[TIMER_NUM_TIMERS]{"Total", "GED"};

#ifdef _WIN32
[[maybe_unused]] static int64_t CurrTicks() {
  LARGE_INTEGER li;
  QueryPerformanceCounter(&li);
  return li.QuadPart;
}
[[maybe_unused]] static int64_t CurrFreq() {
  LARGE_INTEGER li;
  QueryPerformanceFrequency(&li);
  return li.QuadPart;
}
#else
// Linux
[[maybe_unused]] static int64_t CurrTicks() {
  struct timespec t;
  if (clock_gettime(CLOCK_TYPE, &t) != 0) {
    return 0;
  }
  return 1000000000LL * t.tv_sec + t.tv_nsec;
}
[[maybe_unused]] static int64_t CurrFreq() {
  // clock_gettime is in nanos
  return 1000;
}
#endif

namespace iga {

struct Timer {
  double time;
  int64_t currentStart;
  const char *name;
  int64_t ticks;
  bool started;
};

} // namespace iga

static _THREAD iga::Timer timers[TIMER_NUM_TIMERS];
static _THREAD char kernelAsmName[256] = "";
static _THREAD int64_t proc_freq;
static _THREAD int numTimers = TIMER_NUM_TIMERS;

void initIGATimer() {

  numTimers = 0;
  for (int i = 0; i < TIMER_NUM_TIMERS; i++) {
    timers[i].time = 0;
    timers[i].currentStart = 0;
    timers[i].name = NULL;
    timers[i].ticks = 0;
    timers[i].started = false;
    createIGANewTimer(timerNames[i]);
  }
  proc_freq = CurrFreq();
}

void setIGAKernelName(const char *name) {
  iga::copyOutString(kernelAsmName, sizeof(kernelAsmName), nullptr, name);
}

int createIGANewTimer(const char *name) {
  timers[numTimers].name = name;
  return numTimers++;
}

void startIGATimer(int timer) {
#ifdef MEASURE_COMPILATION_TIME
  if (timer < TIMER_NUM_TIMERS) {
#if defined(_DEBUG) && defined(CHECK_TIMER)
    if (timers[timer].started) {
      std::cerr << "***********************************************\n";
      std::cerr << "Timer already started.\n";
      assert(false);
    }
#endif
    timers[timer].currentStart = CurrTicks();
#if defined(_DEBUG) && defined(CHECK_TIMER)
    timers[timer].started = true;
#endif
  } else {
#ifdef _DEBUG
    std::cerr << "***********************************************\n";
    std::cerr << "Invalid index used when invoking startTimer\n";
#endif
  }
#endif
}

void stopIGATimer(int timer) {
#ifdef MEASURE_COMPILATION_TIME
  if (timer < TIMER_NUM_TIMERS) {
    int64_t stop = CurrTicks();
    timers[timer].time +=
        (double)((stop - timers[timer].currentStart) / (double)proc_freq);
    timers[timer].ticks += (stop - timers[timer].currentStart);
    timers[timer].currentStart = 0;
#if defined(_DEBUG) && defined(CHECK_TIMER)
    timers[timer].started = false;
#endif
  } else {
#ifdef _DEBUG
    std::cerr << "***********************************************\n";
    std::cerr << "Invalid index used when invoking stopTimer\n";
#endif
  }
#endif
}

static unsigned int getIGATotalTimers() { return numTimers; }

std::string getIGATimerNames(unsigned int idx) { return timerNames[idx]; }

double getIGATimerCounts(unsigned int idx) { return timers[idx].time; }

int64_t getIGATimerTicks(unsigned int idx) { return timers[idx].ticks; }

double getIGATimerUS(unsigned int idx) {
  return (timers[idx].ticks * 1000000) / (double)proc_freq;
}

void dumpAllIGATimers(bool outputTime) {
  std::ofstream krnlOutput;
  krnlOutput.open("jit_time.txt", std::ios_base::app);

  krnlOutput << kernelAsmName << "\n";

  for (unsigned int i = 0; i < getIGATotalTimers(); i++) {
    krnlOutput << getIGATimerNames(i).c_str() << "\t";
  }
  krnlOutput << "\n";

  for (unsigned int i = 0; i < getIGATotalTimers(); i++) {
    if (outputTime) {
      krnlOutput << timers[i].time << "\t";
    } else {
      krnlOutput << timers[i].ticks << "\t";
    }
  }

  krnlOutput << "\n";

  krnlOutput.close();
}

/*===================== begin_copyright_notice ==================================

Copyright (c) 2017 Intel Corporation

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.


======================= end_copyright_notice ==================================*/
#ifdef _WIN32
#include <Windows.h>
#define SNPRINTF( dst, size, ... ) sprintf_s( (dst), (size), __VA_ARGS__ )
#else
#define SNPRINTF( dst, size, ... ) snprintf( (dst), (size), __VA_ARGS__  )
#endif


#if !defined(_WIN32)
#include <time.h>       //clock_gettime()
#endif



#include <stdint.h>
#include <string>
#include "Timer.hpp"
#include <fstream>
#include <iostream>

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


static const char* timerNames[TIMER_NUM_TIMERS] = {"Total", "GED"};

#ifdef _WIN32
static int64_t CurrTicks() {
    LARGE_INTEGER li;
    QueryPerformanceCounter(&li);
    return li.QuadPart;
}
static int64_t CurrFreq() {
    LARGE_INTEGER li;
    QueryPerformanceFrequency(&li);
    return li.QuadPart;
}
#else
// Linux
static int64_t CurrTicks()
{
    struct timespec t;
    if (clock_gettime(CLOCK_TYPE, &t) != 0) {
        return 0;
    }
    return 1000000000LL*t.tv_sec + t.tv_nsec;
}
static int64_t CurrFreq() {
    // clock_gettime is in nanos
    return 1000;
}
#endif

using namespace std;

struct Timer {
    double       time;
    int64_t      currentStart;
    const char  *name;
    int64_t      ticks;
    bool         started;
};

static _THREAD Timer    timers[TIMER_NUM_TIMERS];
static _THREAD char     kernelAsmName[256] = "";
static _THREAD int64_t  proc_freq;
static _THREAD int      numTimers = TIMER_NUM_TIMERS;

void initIGATimer()
{

    numTimers = 0;
    for(int i = 0; i < TIMER_NUM_TIMERS; i++)
    {
        timers[i].time = 0;
        timers[i].currentStart = 0;
        timers[i].name = NULL;
        timers[i].ticks = 0;
        timers[i].started = false;
        createIGANewTimer(timerNames[i]);
    }
    proc_freq = CurrFreq();
}


void setIGAKernelName(const char *name)
{
    SNPRINTF(kernelAsmName, 256, "%s", name);
}


int createIGANewTimer( const char* name )
{
    timers[numTimers].name = name;
    return numTimers++;
}


void startIGATimer( int timer )
{
#ifdef MEASURE_COMPILATION_TIME
    if (timer < TIMER_NUM_TIMERS)
    {
#if defined(_DEBUG) && defined(CHECK_TIMER)
        if (timers[timer].started)
        {
            std::cerr << "***********************************************\n";
            std::cerr << "Timer already started.\n";
            assert(false);
        }
#endif
        timers[timer].currentStart = CurrTicks();
#if defined(_DEBUG) && defined(CHECK_TIMER)
        timers[timer].started = true;
#endif
    }
    else
    {
#ifdef _DEBUG
        std::cerr << "***********************************************\n";
        std::cerr << "Invalid index used when invoking startTimer\n";
#endif
    }
#endif
}

void stopIGATimer( int timer )
{
#ifdef MEASURE_COMPILATION_TIME
    if (timer < TIMER_NUM_TIMERS)
    {
        int64_t stop = CurrTicks();
        timers[timer].time += (double)((stop - timers[timer].currentStart) / (double)proc_freq);
        timers[timer].ticks += (stop - timers[timer].currentStart);
        timers[timer].currentStart = 0;
#if defined(_DEBUG) && defined(CHECK_TIMER)
        timers[timer].started = false;
#endif
    }
    else
    {
#ifdef _DEBUG
        std::cerr << "***********************************************\n";
        std::cerr << "Invalid index used when invoking stopTimer\n";
#endif
    }
#endif
}

unsigned int getIGATotalTimers()
{
    return numTimers;
}

std::string getIGATimerNames(unsigned int idx)
{
    return timerNames[idx];
}

double getIGATimerCounts(unsigned int idx)
{
    return timers[idx].time;
}

int64_t getIGATimerTicks(unsigned int idx)
{
    return timers[idx].ticks;
}

double getIGATimerUS(unsigned int idx)
{
    return (timers[idx].ticks * 1000000) / (double)proc_freq;
}

void dumpAllIGATimers(bool outputTime)
{
    std::ofstream krnlOutput;
    krnlOutput.open("jit_time.txt", ios_base::app);

    krnlOutput << kernelAsmName << "\n";

    for (unsigned int i = 0; i < getIGATotalTimers(); i++)
    {
        krnlOutput << getIGATimerNames(i).c_str() << "\t";
    }
    krnlOutput << "\n";

    for (unsigned int i = 0; i < getIGATotalTimers(); i++)
    {
        if (outputTime)
        {
            krnlOutput << timers[i].time << "\t";
        }
        else
        {
            krnlOutput << timers[i].ticks << "\t";
        }
    }

    krnlOutput << "\n";

    krnlOutput.close();
}

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

#include "Option.h"
#include "Timer.h"
#include <iostream>
#include <fstream>
#include <string>
#ifdef _WIN32
#include "Windows.h"
#endif
#include "assert.h"

#undef DEF_TIMER
#define DEF_TIMER(ENUM, DESCR) DESCR,
static const char* timerNames[TIMER_NUM_TIMERS] =
{
    #include "Timer.def"
};

#ifdef MEASURE_COMPILATION_TIME

#define CLOCK_TYPE CLOCK_MONOTONIC

#if   !defined(_WIN32)
    bool QueryPerformanceFrequency(LARGE_INTEGER *lpFrequency)
    {
        struct timespec  Res;
        INT              iRet;

        if ( (iRet = clock_getres(CLOCK_TYPE, &Res)) != 0 )
        {
            return ERROR;
        }

        // resolution (precision) can't be in seconds for current machine and OS
        if (Res.tv_sec != 0)
        {
            return ERROR;
        }
        lpFrequency->QuadPart = 1000000000LL / Res.tv_nsec;

        return SUCCEED;
    }

    bool QueryPerformanceCounter(LARGE_INTEGER *lpPerformanceCount)
    {
        struct timespec     Res;
        struct timespec     t;
        INT                 iRet;

        if ( (iRet = clock_getres (CLOCK_TYPE, &Res)) != 0 )
        {
            return ERROR;
        }
        if (Res.tv_sec != 0)
        { // resolution (precision) can't be in seconds for current machine and OS
            return ERROR;
        }
        if( (iRet = clock_gettime(CLOCK_TYPE, &t)) != 0)
        {
            return ERROR;
        }
        lpPerformanceCount->QuadPart = (1000000000LL * t.tv_sec +
            t.tv_nsec) / Res.tv_nsec;

        return SUCCEED;
    }

#endif
#endif // MEASURE_COMPILATION_TIME

using namespace std;

struct Timer {
    double time;
    LONGLONG currentStart;
    const char* name;
    LONGLONG ticks;
    bool started;
    unsigned int hits;
};

static _THREAD Timer timers[TIMER_NUM_TIMERS];
static _THREAD char kernelAsmName[256] = "";
static _THREAD LARGE_INTEGER proc_freq;
static _THREAD int numTimers = TIMER_NUM_TIMERS;

void initTimer() {

#ifdef MEASURE_COMPILATION_TIME
    numTimers = 0;
    for (int i = 0; i < TIMER_NUM_TIMERS; i++)
    {
        timers[i].time = 0;
        timers[i].currentStart = 0;
        timers[i].name = NULL;
        timers[i].ticks = 0;
        timers[i].started = false;
        timers[i].hits = 0;
        createNewTimer( timerNames[i] );
    }
    QueryPerformanceFrequency(&proc_freq);
#endif
}

void resetPerKernel()
{
    for (int i = 0; i < TIMER_NUM_TIMERS; i++)
    {
        if (i == TIMER_TOTAL                            ||
            i == TIMER_BUILDER                          ||
            i == TIMER_VISA_BUILDER_APPEND_INST         ||
            i == TIMER_VISA_BUILDER_CREATE_VAR          ||
            i == TIMER_VISA_BUILDER_CREATE_OPND         ||
            i == TIMER_VISA_BUILDER_IR_CONSTRUCTION)
        {
            continue;
        }
        timers[i].time = 0;
        timers[i].currentStart = 0;
        timers[i].ticks = 0;
        timers[i].started = false;
        timers[i].hits = 0;
    }
}

void setKernelName(const char *name)
{
    SNPRINTF(kernelAsmName, 256, "%s", name);
}

int createNewTimer(const char* name)
{
    timers[numTimers].name = name;
    return numTimers++;
}

void startTimer(int timer)
{
#ifdef MEASURE_COMPILATION_TIME
    if (timer < TIMER_NUM_TIMERS)
    {
#if defined(_DEBUG) && defined(CHECK_TIMER)
        if(timers[timer].started)
        {
            std::cerr << "***********************************************\n";
            std::cerr << "Timer already started.\n";
            assert(false);
        }
#endif
        LARGE_INTEGER start;
        QueryPerformanceCounter(&start);
        timers[timer].currentStart= start.QuadPart;
        timers[timer].hits++;
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

void stopTimer(int timer)
{
#ifdef MEASURE_COMPILATION_TIME
    if (timer < TIMER_NUM_TIMERS)
    {
        LARGE_INTEGER stop;
        QueryPerformanceCounter(&stop);
        timers[timer].time += (stop.QuadPart - timers[timer].currentStart) / (double) proc_freq.QuadPart;
        timers[timer].ticks += (stop.QuadPart - timers[timer].currentStart);
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

extern "C" unsigned int getTotalTimers()
{
    return numTimers;
}

extern "C" double getTimerCounts(unsigned int idx)
{
    return timers[idx].time;
}

extern "C" int64_t getTimerTicks(unsigned int idx)
{
    return timers[idx].ticks;
}

extern "C" unsigned int getTimerHits(unsigned int idx)
{
    return timers[idx].hits;
}

double getTimerUS(unsigned int idx)
{
    return (timers[idx].ticks * 1000000) / (double)proc_freq.QuadPart;
}

void dumpAllTimers(const char *asmFileName, bool outputTime)
{
    // This generates output like this:
    // TIMER1 TIMER2 TIMER3 ...
    // num1 num2 num3 ...
    std::ofstream krnlOutput;
    krnlOutput.open("jit_time.txt", ios_base::app);
    krnlOutput << kernelAsmName << "\n";

    double totalTime = timers[TIMER_TOTAL].time;
    for (unsigned int i = 0; i < getTotalTimers(); i++)
    {
#ifndef TIME_BUILDER
        if (i == TIMER_VISA_BUILDER_APPEND_INST || i == TIMER_VISA_BUILDER_IR_CONSTRUCTION ||
            i == TIMER_VISA_BUILDER_CREATE_VAR || i == TIMER_VISA_BUILDER_CREATE_OPND)
        {
            continue;
        }
#endif
        krnlOutput << std::left << std::setw(24) << timerNames[i] << "\t";
        if (outputTime)
        {
            krnlOutput << std::left << std::setw(12) << std::setprecision(6) << timers[i].time << "\t";
        }
        else
        {
            krnlOutput << timers[i].ticks << "\t";
        }
        krnlOutput << std::setprecision(4) << (timers[i].time / totalTime * 100) << "%";
        krnlOutput << "\n";
    }

    krnlOutput.close();

    // Print timers like this:
    // TIMER1:num1
    // TIMER2:num2
    // ...
    std::ofstream timerFile;
    std::stringstream ss;
    ss << "timers." << asmFileName;
    timerFile.open(ss.str(), ios_base::out);
    for (unsigned i = 0, e = getTotalTimers(); i < e; i++) {
        timerFile << timerNames[i] << ":";
        if (outputTime) {
            timerFile << timers[i].time << std::endl;
        } else {
            timerFile << timers[i].ticks << std::endl;
        }
    }
    timerFile.close();
}

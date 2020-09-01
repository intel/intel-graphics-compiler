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

#ifndef _TIMER_H_
#define _TIMER_H_
// Enable this macro by default but comment out calls that print out timer info.
// Requirement by 3d team.
#if defined(_DEBUG) || defined(_ENABLETIMER)
#define MEASURE_COMPILATION_TIME
#endif

//#define TIME_BUILDER
//#include "cm_portability.h"

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

int createIGANewTimer(const char* timerName);

void initIGATimer();
void startIGATimer(int timer);
void stopIGATimer(int timer);
void setIGAKernelName(const char *name);
void dumpAllIGATimers(bool outputTime = false);
std::string getIGATimerNames(unsigned int idx);
int64_t getIGATimerTicks(unsigned int idx);
double getIGATimerCounts(unsigned int idx);
double getIGATimerUS(unsigned int idx);

typedef enum TIMERS
{
    TIMER_TOTAL = 0,
    TIMER_GED = 1,
    TIMER_NUM_TIMERS = 2
} TIMERS;

#endif


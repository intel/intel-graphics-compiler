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

#if (defined(_DEBUG) || defined(RELEASE_INTERNAL))  || defined(_INTERNAL)

#include "TimeHelpers.h"
#include <fstream>
#include <string>


#endif

namespace LLVM{

    enum OPTIMIZATION_COUNTER_TYPE
    {
        NUM_FOR_LOOPS = 0,
        NUM_WHILE_LOOPS,
        MAX_OPTIMIZATION_COUNTER_TYPE,
    };

    enum TimingStats{

        //CLANG Statistics
        CLANG_TIME,
        MAX_CLANG_TIMING_COUNTER,

        //LLVM Statistics

        LLVM_TIME,
        MAX_LLVM_TIMING_COUNTER

    };

#if defined(_DEBUG) || defined (RELEASE_INTERNAL) || defined(_INTERNAL)

    struct StatNames{
        char * name;
        char * description;

    };

    StatNames generic_names [] ={

        {"Optimization 1", "Opt 1"},
        {"Optimization 2", "Opt 2"}

    };

    StatNames timing_names []={

        //CLANG Statistics
        {"CLANG Time", "Takes clang's total time to run"}, // Do not remove or reorder
        {"CLANG Placeholder", "Holds the size of the clang stats"},

        //LLVM Statistics
        {"LLVM Time", "Takes LLVM's total time to run"}, // Do not remove or reorder
        {"LLVM Placeholder", "Holds the size of the llvm stats"}
    };

    struct SOptStats{
        DWORD OptCounter[MAX_OPTIMIZATION_COUNTER_TYPE];
        DWORD FailedOptCounter[MAX_OPTIMIZATION_COUNTER_TYPE];
    };

    struct STimingStats{
        long long StartTime[MAX_LLVM_TIMING_COUNTER];
        long long TotalTime[MAX_LLVM_TIMING_COUNTER];
    };

    struct SDebugVariables
    {
        unsigned int OptStatsEnable: 1;
        unsigned int TimeStatsEnable: 1;
    };

    SDebugVariables g_DebugVariables =
    {
        1, // OptStatsEnable
        1 // TimeStatsEnable
    };

    std::string llvm_timing_html(STimingStats * timing){

        std::string htmlcontent = "";

        for(int i = MAX_CLANG_TIMING_COUNTER+1; i < MAX_LLVM_TIMING_COUNTER; i++){
            htmlcontent += "<td class='first-odd'>";

            htmlcontent += std::to_string(timing->TotalTime[i]);

            htmlcontent += "</td>";
        }

        htmlcontent += "</tr>";

        return htmlcontent;
    }

    std::string clang_stats_html (SOptStats * stats){

        std::string htmlcontent = "";

        for(int i = 0; i < MAX_OPTIMIZATION_COUNTER_TYPE; i++){
            htmlcontent += "<td class='first-odd'>";

            htmlcontent += std::to_string(stats->OptCounter[i]);

            htmlcontent += "</td>";
        }

        htmlcontent += "</tr>";

        return htmlcontent;

    }

    void printClangStats(SOptStats * stats){

        std::ofstream offile("..\\..\\..\\reports\\CLANG_Stats_Info.csv", std::ios::app);

        std::string contents = "";
        std::string htmlcontent = "";


        for(int i = 0; i < MAX_OPTIMIZATION_COUNTER_TYPE; i++){

            contents += std::to_string(stats->OptCounter[i]);

            if(i != MAX_OPTIMIZATION_COUNTER_TYPE - 1)
                contents += ",";
        }

        contents += '\n';

        offile << contents;

        offile.close();

        std::ofstream stats_html("..\\..\\..\\reports\\CLANG_Stats_Info.html", std::ios::app);
        htmlcontent = clang_stats_html(stats);
        stats_html << htmlcontent;
        stats_html.close();
    }


    void printLLVMTiming(STimingStats * timing, CTimer * timer){

        std::ofstream ofile("..\\..\\..\\reports\\LLVM_Timing_Info.csv", std::ios::app);

        std::string contents = "";

        for(int i = MAX_CLANG_TIMING_COUNTER+1; i < MAX_LLVM_TIMING_COUNTER; i++){

            contents += std::to_string(timing->TotalTime[i]);

            contents += ",";
            contents += std::to_string(timer->GetTimeInSec(timing->TotalTime[i])/ 100.0);

            if(i != MAX_LLVM_TIMING_COUNTER - 1)
                contents += ",";
        }

        contents += '\n';

        ofile << contents;

        ofile.close();

        std::ofstream frontend_html("..\\..\\..\\reports\\LLVM_Timing_Info.html", std::ios::app);
        std::string htmlcontent = llvm_timing_html(timing);
        frontend_html << htmlcontent;
        frontend_html.close();

    }



    std::string clang_timing_html(STimingStats * timing){

        std::string htmlcontent = "";

        for(int i = 0; i < MAX_CLANG_TIMING_COUNTER; i++){
            htmlcontent += "<td class='first-odd'>";

            htmlcontent += std::to_string(timing->TotalTime[i]);

            htmlcontent += "</td>";
        }

        htmlcontent += "</tr>";

        return htmlcontent;
    }

    void printCLANGTiming(STimingStats * timing, CTimer * timer){

        std::ofstream ofile("..\\..\\..\\reports\\CLANG_Timing_Info.csv", std::ios::app);

        std::string contents = "";

        for(int i = 0; i < MAX_CLANG_TIMING_COUNTER; i++){

            contents += std::to_string(timing->TotalTime[i]);
            contents += ",";
            contents += std::to_string(timer->GetTimeInSec(timing->TotalTime[i])/ 100.0);


            if(i != MAX_CLANG_TIMING_COUNTER - 1)
                contents += ",";
        }

        contents += '\n';

        ofile << contents;

        ofile.close();

        std::ofstream frontend_html("..\\..\\..\\reports\\CLANG_Timing_Info.html", std::ios::app);
        std::string htmlcontent = clang_timing_html(timing);
        frontend_html << htmlcontent;
        frontend_html.close();
    }



#define PRINT_CLANG_TIMING_STATS(timing) \
    LLVM::printCLANGTiming(timing, timer);


#define PRINT_LLVM_TIMING_STATS(timing) \
    LLVM::printLLVMTiming(timing, timer); \

#define PRINT_CLANG_OPTIMIZATION_STATS(stats) \
    LLVM::printClangStats(stats);

 #define CLANG_OPTIMIZATION_STAT_INCREMENT(pointer, stat,value) \
    if( LLVM::g_DebugVariables.OptStatsEnable && \
    pointer != NULL) \
    { \
    pointer->OptCounter[stat]+=value; \
}


#define TIMING_STAT_START(pointer,stat) \
    if(LLVM::g_DebugVariables.TimeStatsEnable && pointer != NULL) \
    {\
    pointer->StartTime[stat] = timer->GetTime(); \
}

#define TIMING_STAT_END(pointer,stat) \
    if(LLVM::g_DebugVariables.TimeStatsEnable && pointer != NULL) \
    {\
    pointer->TotalTime[stat] = timer->getElapsedTime(pointer->StartTime[stat]); \
}

#define CREATE_STATS_OBJECT\
    LLVM::SOptStats * stats = new LLVM::SOptStats();

#define CREATE_TIMING_OBJECT\
    LLVM::STimingStats * timing = new LLVM::STimingStats(); \
    CTimer * timer = new CTimer();

#define DELETE_TIMING_OBJECT\
    delete timing; \
    delete timer;

#define DELETE_STATS_OBJECT\
    delete stats;


#else
#define PRINT_LLVM_TIMING_STATS(timing)
#define PRINT_CLANG_TIMING_STATS(timing)
#define CLANG_OPTIMIZATION_STAT_INCREMENT(pointer,stat,value)
#define PRINT_LLVM_TIMING_STATS(timing)
#define TIMING_STAT_END(pointer,stat)
#define TIMING_STAT_START(pointer,stat)

#define CREATE_TIMING_OBJECT\
    void * timing = NULL;

#define CREATE_STATS_OBJECT\
    void * stats = NULL;

#define DELETE_TIMING_OBJECT

#define DELETE_STATS_OBJECT

#endif

}


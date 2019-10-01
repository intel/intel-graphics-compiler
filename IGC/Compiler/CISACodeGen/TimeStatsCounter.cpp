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

#include "Compiler/CISACodeGen/TimeStatsCounter.h"
#include "Compiler/IGCPassSupport.h"

#include "common/LLVMWarningsPush.hpp"
#include <llvm/Pass.h>
#include "common/LLVMWarningsPop.hpp"

using namespace llvm;
using namespace IGC;

namespace {
    class TimeStatsCounter : public FunctionPass {
        CodeGenContext* ctx;
        COMPILE_TIME_INTERVALS interval;
        TimeStatsCounterStartEndMode mode;
        std::string igcPass;
        TimeStatsCounterType type;

    public:
        static char ID;

        TimeStatsCounter() : FunctionPass(ID) {
            initializeTimeStatsCounterPass(*PassRegistry::getPassRegistry());
        }

        TimeStatsCounter(CodeGenContext* _ctx, COMPILE_TIME_INTERVALS _interval, TimeStatsCounterStartEndMode _mode) : FunctionPass(ID) {
            initializeTimeStatsCounterPass(*PassRegistry::getPassRegistry());
            ctx = _ctx;
            interval = _interval;
            mode = _mode;
            type = STATS_COUNTER_ENUM_TYPE;
        }

        TimeStatsCounter(CodeGenContext* _ctx, std::string _igcPass, TimeStatsCounterStartEndMode _mode) : FunctionPass(ID) {
            initializeTimeStatsCounterPass(*PassRegistry::getPassRegistry());
            ctx = _ctx;
            mode = _mode;
            igcPass = _igcPass;
            type = STATS_COUNTER_LLVM_PASS;
        }

        bool runOnFunction(Function&) override;

    private:

    };
} // End anonymous namespace

FunctionPass* IGC::createTimeStatsCounterPass(CodeGenContext* _ctx, COMPILE_TIME_INTERVALS _interval, TimeStatsCounterStartEndMode _mode) {
    return new TimeStatsCounter(_ctx, _interval, _mode);
}

FunctionPass* IGC::createTimeStatsIGCPass(CodeGenContext* _ctx, std::string _igcPass, TimeStatsCounterStartEndMode _mode)
{
    return new TimeStatsCounter(_ctx, _igcPass, _mode);
}

char TimeStatsCounter::ID = 0;

#define PASS_FLAG     "time-stats-counter"
#define PASS_DESC     "TimeStatsCounter Start/Stop"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
namespace IGC {
    IGC_INITIALIZE_PASS_BEGIN(TimeStatsCounter, PASS_FLAG, PASS_DESC, PASS_CFG_ONLY, PASS_ANALYSIS)
        IGC_INITIALIZE_PASS_END(TimeStatsCounter, PASS_FLAG, PASS_DESC, PASS_CFG_ONLY, PASS_ANALYSIS)
}

bool TimeStatsCounter::runOnFunction(Function& F) {
    if (type == STATS_COUNTER_ENUM_TYPE)
    {
        if (mode == STATS_COUNTER_START)
        {
            COMPILER_TIME_START(ctx, interval);
        }
        else
        {
            COMPILER_TIME_END(ctx, interval);
        }
    }
    else
    {
        if (mode == STATS_COUNTER_START)
        {
            COMPILER_TIME_PASS_START(ctx, igcPass);
        }
        else
        {
            COMPILER_TIME_PASS_END(ctx, igcPass);
        }
    }
    return true;
}


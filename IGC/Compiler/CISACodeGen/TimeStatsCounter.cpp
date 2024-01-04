/*========================== begin_copyright_notice ============================

Copyright (C) 2019-2023 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "Compiler/CISACodeGen/TimeStatsCounter.h"
#include "Compiler/IGCPassSupport.h"

#include "common/LLVMWarningsPush.hpp"
#include <llvm/Pass.h>
#include "common/LLVMWarningsPop.hpp"

using namespace llvm;
using namespace IGC;

namespace {
    class TimeStatsCounter : public ModulePass {
        CodeGenContext* ctx = nullptr;
        COMPILE_TIME_INTERVALS interval{};
        TimeStatsCounterStartEndMode mode{};
        std::string igcPass{};
        TimeStatsCounterType type;

    public:
        static char ID;

        TimeStatsCounter() : ModulePass(ID) {
            initializeTimeStatsCounterPass(*PassRegistry::getPassRegistry());
        }

        TimeStatsCounter(CodeGenContext* _ctx, COMPILE_TIME_INTERVALS _interval, TimeStatsCounterStartEndMode _mode)
            : ModulePass(ID), ctx(_ctx), interval(_interval), mode(_mode), type(STATS_COUNTER_ENUM_TYPE) {
            initializeTimeStatsCounterPass(*PassRegistry::getPassRegistry());
        }

        TimeStatsCounter(CodeGenContext* _ctx, const std::string& _igcPass, TimeStatsCounterStartEndMode _mode)
            : ModulePass(ID), ctx(_ctx), mode(_mode), igcPass(_igcPass), type(STATS_COUNTER_LLVM_PASS) {
            initializeTimeStatsCounterPass(*PassRegistry::getPassRegistry());
        }

        bool runOnModule(Module&) override;

    private:

    };
} // End anonymous namespace

ModulePass* IGC::createTimeStatsCounterPass(CodeGenContext* _ctx, COMPILE_TIME_INTERVALS _interval, TimeStatsCounterStartEndMode _mode) {
    return new TimeStatsCounter(_ctx, _interval, _mode);
}

ModulePass* IGC::createTimeStatsIGCPass(CodeGenContext* _ctx, std::string _igcPass, TimeStatsCounterStartEndMode _mode)
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

bool TimeStatsCounter::runOnModule(Module& F) {
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
    return false;
}


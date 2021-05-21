/*========================== begin_copyright_notice ============================

Copyright (C) 2019-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "common/LLVMWarningsPush.hpp"
#include <llvm/Pass.h>
#include "common/LLVMWarningsPop.hpp"
#include "Compiler/CodeGenPublic.h"
#include "common/Stats.hpp"
#include <string>

namespace IGC {
    enum TimeStatsCounterStartEndMode
    {
        STATS_COUNTER_START,
        STATS_COUNTER_END
    };

    enum TimeStatsCounterType
    {
        STATS_COUNTER_LLVM_PASS,
        STATS_COUNTER_ENUM_TYPE
    };

    llvm::ModulePass* createTimeStatsCounterPass(CodeGenContext* _ctx, COMPILE_TIME_INTERVALS _interval, TimeStatsCounterStartEndMode _mode);
    llvm::ModulePass* createTimeStatsIGCPass(CodeGenContext* _ctx, std::string _igcPass, TimeStatsCounterStartEndMode _mode);
    void initializeTimeStatsCounterPass(llvm::PassRegistry&);
} // End namespace IGC

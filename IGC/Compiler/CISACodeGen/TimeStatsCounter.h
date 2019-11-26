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

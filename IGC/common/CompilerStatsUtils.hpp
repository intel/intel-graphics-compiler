/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "Compiler/CodeGenPublic.h" // needed for IGC::CodeGenContext?
#include "JitterDataStruct.h" // needed for FINALIZER_INFO

namespace IGC
{
    namespace CompilerStatsUtils
    {
        void RecordCompileTimeStats(IGC::CodeGenContext *context);
        void RecordCodeGenCompilerStats(IGC::CodeGenContext *context,
                                        SIMDMode dispatchSize,
                                        CompilerStats &vISACompilerStats,
                                        FINALIZER_INFO *jitInfo);
        void OutputCompilerStats(IGC::CodeGenContext *context);
    }
}

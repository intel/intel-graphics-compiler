/*========================== begin_copyright_notice ============================

Copyright (C) 2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef G4_PASSES_INST_COMBINE_HPP
#define G4_PASSES_INST_COMBINE_HPP

#include "BuildIR.h"
#include "FlowGraph.h"

namespace vISA
{
    void InstCombine(IR_Builder& builder, FlowGraph&  fg);
}


#endif // G4_PASSES_INST_COMBINE_HPP

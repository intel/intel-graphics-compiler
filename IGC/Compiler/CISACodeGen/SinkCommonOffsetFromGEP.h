/*========================== begin_copyright_notice ============================

Copyright (C) 2022 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef _CISA_SINK_COMMON_OFFSET_FROM_GEP_H_
#define _CISA_SINK_COMMON_OFFSET_FROM_GEP_H_

#include <llvm/Pass.h>
#include "common/LLVMWarningsPush.hpp"
#include <llvm/Analysis/PostDominators.h>
#include <llvm/Analysis/LoopInfo.h>
#include "common/LLVMWarningsPop.hpp"

namespace IGC {

llvm::FunctionPass* createSinkCommonOffsetFromGEPPass();
void initializeSinkCommonOffsetFromGEPPass(llvm::PassRegistry &);

} // End namespace IGC

#endif // _CISA_SINK_COMMON_OFFSET_FROM_GEP_H_

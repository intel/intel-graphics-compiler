/*========================== begin_copyright_notice ============================

Copyright (C) 2022 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/
#ifndef _CISA_GENERATEFREQUENCYDATA_HPP_
#define _CISA_GENERATEFREQUENCYDATA_HPP_


// This pass generates static profile information and embed it as meta data
// Static profile information includes block freq

#include "common/LLVMWarningsPush.hpp"
#include <llvm/Pass.h>
#include "common/LLVMWarningsPop.hpp"

#pragma once

namespace IGC {
    llvm::ModulePass* createGenerateFrequencyDataPass();
    void initializeGenerateFrequencyDataPass(llvm::PassRegistry&);
}

#endif // _CISA_GENERATEFREQUENCYDATA_HPP_

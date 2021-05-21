/*========================== begin_copyright_notice ============================

Copyright (C) 2019-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef _CLEARTESSFACTORS_H_
#define _CLEARTESSFACTORS_H_

#include "common/LLVMWarningsPush.hpp"
#include <llvm/Pass.h>
#include "common/LLVMWarningsPop.hpp"

namespace IGC
{
    llvm::FunctionPass* createClearTessFactorsPass();
}

#endif // _CLEARTESSFACTORS_H_

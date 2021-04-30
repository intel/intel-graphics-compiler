/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef _MERGEURBWRITES_H_
#define _MERGEURBWRITES_H_

#include "common/LLVMWarningsPush.hpp"
#include <llvm/Pass.h>
#include "common/LLVMWarningsPop.hpp"

namespace IGC
{

    class CShader;
    llvm::FunctionPass* createMergeURBWritesPass();

}

#endif // _MERGEURBWRITES_H_

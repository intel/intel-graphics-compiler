/*========================== begin_copyright_notice ============================

Copyright (C) 2019-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef IGC_IMPLICITGIDRESTORING_HPP
#define IGC_IMPLICITGIDRESTORING_HPP

#include "Compiler/MetaDataUtilsWrapper.h"
#include "common/LLVMWarningsPush.hpp"
#include <llvm/Pass.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/Instructions.h>
#include "common/LLVMWarningsPop.hpp"

namespace IGC
{
    class ImplicitGIDRestoring : public llvm::FunctionPass
    {
    public:
        static char ID;

        ImplicitGIDRestoring();

        bool runOnFunction(llvm::Function& F) override;
    };
} // namespace IGC

#endif //IGC_IMPLICITGIDRESTORING_HPP


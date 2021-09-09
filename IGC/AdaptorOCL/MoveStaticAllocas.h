/*========================== begin_copyright_notice ============================

Copyright (C) 2018-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "Compiler/CISACodeGen/helper.h"
#include "Compiler/CISACodeGen/ShaderCodeGen.hpp"
#include "Compiler/MetaDataUtilsWrapper.h"
#include "Compiler/MetaDataApi/MetaDataApi.h"

#include "common/LLVMWarningsPush.hpp"
#include <llvm/IR/PassManager.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/InstVisitor.h>
#include "common/LLVMWarningsPop.hpp"

namespace IGC
{
    /*
    This pass searches for "static" alloca instructions in function that are not
    in the entry block and moves them. This prevents llvm from inserting
    @llvm.stacksave and @llvm.stackrestore that are not handled by IGC.
    */
    class MoveStaticAllocas : public llvm::FunctionPass
    {
    public:
        static char ID;

        MoveStaticAllocas();

        ~MoveStaticAllocas() {}

        virtual llvm::StringRef getPassName() const override
        {
            return "MoveStaticAllocasPass";
        }

        virtual bool runOnFunction(llvm::Function &F) override;


    };

} // namespace IGC

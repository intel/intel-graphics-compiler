/*========================== begin_copyright_notice ============================

Copyright (C) 2020-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "common/LLVMWarningsPush.hpp"
#include <llvm/Pass.h>
#include <llvm/IR/InstVisitor.h>
#include <llvm/IR/AssemblyAnnotationWriter.h>
#include "common/LLVMWarningsPop.hpp"
#include "Compiler/CodeGenContextWrapper.hpp"

namespace IGC
{
    class IntrinsicAnnotator : public llvm::AssemblyAnnotationWriter
    {
    public:
        IntrinsicAnnotator() {}
        ~IntrinsicAnnotator() {}
        void emitFunctionAnnot(const llvm::Function* func, llvm::formatted_raw_ostream&) override;
    };
}

/*========================== begin_copyright_notice ============================

Copyright (C) 2024 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "common/LLVMWarningsPush.hpp"
#include "llvm/ADT/SmallVector.h"
#include "llvm/Pass.h"
#include "common/LLVMWarningsPop.hpp"

namespace llvm
{
    class AllocaInst;
} // namespace llvm

namespace IGC
{
////////////////////////////////////////////////////////////////////////////////
/// This pass analyzes all uses of array-of-vector allocas to find unused
/// vector elements. The type of alloca is modified to remove the unused vector
/// elements, e.g., the following piece of IR:
///
///   %alloca = alloca [64 x <4 x float>], align 16
///   %addr = getelementptr [64 x <4 x float>], [64 x <4 x float>]* %alloca, i32 0, i32 %index
///   %vec = load <4 x float>, <4 x float>* %addr1
///   %a = extractelement <4 x float> %vec1, i64 2
///   ret float %a
///
/// is transformed to:
///
///   %alloca = alloca [64 x float], align 16
///   %addr = getelementptr [64 x float], [64 x float]* %alloca, i32 0, i32 %index
///   %a = load float, float* %addr1
///   ret float %a
///////////////////////////////////////////////////////////////////////////////
class ShrinkArrayAllocaPass : public llvm::FunctionPass
{
public:
    static char ID;

    ShrinkArrayAllocaPass();
    ~ShrinkArrayAllocaPass() {};

    virtual bool runOnFunction(llvm::Function& function) override;

    virtual llvm::StringRef getPassName() const override;

private:
    void GatherAllocas(llvm::Function& F);
    bool Resolve();

private:
    using UsageInfo = llvm::SmallVector<bool, 4>;
    llvm::SmallVector<std::pair<llvm::AllocaInst*, UsageInfo>, 4> m_AllocaInfo;
};

void initializeShrinkArrayAllocaPassPass(llvm::PassRegistry&);
} // namespace IGC


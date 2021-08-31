/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "common/LLVMWarningsPush.hpp"
#include <llvm/ADT/SmallVector.h>
#include "common/LLVMWarningsPop.hpp"

namespace llvm
{
class PassRegistry;
class FunctionPass;
class Instruction;
class GenIntrinsicInst;
}
namespace IGC
{
class HoistURBWrites : public llvm::FunctionPass
{
public:
    static char ID; // Pass identification
    HoistURBWrites();
    virtual bool runOnFunction(llvm::Function& F) override;
    virtual void getAnalysisUsage(llvm::AnalysisUsage& AU) const override;
    virtual llvm::StringRef getPassName() const override
    {
        return "HoistURBWrites";
    }
private:
    void GatherURBAccesses(llvm::Function& F);
    llvm::Instruction* IsSafeToHoistURBWriteInstruction(
        llvm::GenIntrinsicInst* inst);
    bool Hoist();

    /// Collection of all URB accesses.
    llvm::SmallVector<llvm::GenIntrinsicInst*, 32> m_URBAccesses;
};

void initializeHoistURBWritesPass(llvm::PassRegistry&);
} // namespace IGC


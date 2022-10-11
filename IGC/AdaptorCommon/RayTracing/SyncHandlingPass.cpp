/*========================== begin_copyright_notice ============================

Copyright (C) 2022 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

//===----------------------------------------------------------------------===//
/// Lower RTX instrinsics into their sync implementations
/// Detailed description TBD
//===----------------------------------------------------------------------===//

#include "Compiler/IGCPassSupport.h"
#include "common/LLVMUtils.h"
#include "CrossingAnalysis.h"

#include <vector>
#include <set>
#include "common/LLVMWarningsPush.hpp"
#include <llvm/IR/ValueHandle.h>
#include <llvm/Transforms/Utils/Cloning.h>
#include <llvm/Transforms/Utils/Local.h>
#include <llvm/Transforms/Utils/SSAUpdaterBulk.h>
#include <llvm/IR/Dominators.h>
#include <llvm/IR/InstIterator.h>
#include <llvm/ADT/MapVector.h>
#include "common/LLVMWarningsPop.hpp"
#include "Probe/Assertion.h"

using namespace llvm;
using namespace IGC;

class SyncHandlingPass : public ModulePass
{
public:
    SyncHandlingPass() : ModulePass(ID)
    {
        initializeSyncHandlingPassPass(*PassRegistry::getPassRegistry());
    }

    bool runOnModule(Module& M) override;
    StringRef getPassName() const override
    {
        return "SyncHandlingPass";
    }

    virtual void getAnalysisUsage(llvm::AnalysisUsage& AU) const override
    {
        AU.addRequired<CodeGenContextWrapper>();
    }

    static char ID;

private:
};

char SyncHandlingPass::ID = 0;

// Register pass to igc-opt
#define PASS_FLAG "sync-handling-new"
#define PASS_DESCRIPTION "Lower RTX instrinsics into their sync implementations"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS_BEGIN(SyncHandlingPass, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_DEPENDENCY(CodeGenContextWrapper)
IGC_INITIALIZE_PASS_END(SyncHandlingPass, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)

bool SyncHandlingPass::runOnModule(Module& M)
{
    return false;
}

namespace IGC
{
    Pass* createSyncHandlingPass(void)
    {
        return new SyncHandlingPass();
    }

} // namespace IGC

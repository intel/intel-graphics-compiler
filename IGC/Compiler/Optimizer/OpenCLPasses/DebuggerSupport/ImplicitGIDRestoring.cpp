/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "common/LLVMWarningsPush.hpp"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/InstIterator.h"
#include "common/LLVMWarningsPop.hpp"
#include "Compiler/Optimizer/OpenCLPasses/DebuggerSupport/ImplicitGIDRestoring.hpp"
#include "Compiler/IGCPassSupport.h"
#include "Probe/Assertion.h"
#include "Compiler/CISACodeGen/helper.h"

using namespace llvm;
using namespace IGC;
using namespace IGC::IGCMD;

// Register pass to igc-opt
#define PASS_FLAG "igc-implicit-gid-restoring"
#define PASS_DESCRIPTION "Implicit Global Id Restoring"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS_BEGIN(ImplicitGIDRestoring, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
//IGC_INITIALIZE_PASS_DEPENDENCY(MetaDataUtilsWrapper)
IGC_INITIALIZE_PASS_END(ImplicitGIDRestoring, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)

char ImplicitGIDRestoring::ID = 0;

ImplicitGIDRestoring::ImplicitGIDRestoring() : FunctionPass(ID)
{
    initializeImplicitGIDRestoringPass(*PassRegistry::getPassRegistry());
}

bool ImplicitGIDRestoring::runOnFunction(Function& F)
{
    for (auto ii = inst_begin(F), ie = inst_end(F); ii != ie; ii++)
    {
        Instruction* I = &*ii;
        if (I->getOpcode() == Instruction::Call)
        {
            if (isa<GenIntrinsicInst>(I) &&
                (static_cast<GenIntrinsicInst*>(I)->getIntrinsicID() == GenISAIntrinsic::GenISA_dummyInstID) &&
                I->getMetadata("implicitGlobalID"))
            {
                Instruction* instToResoreMetadata = static_cast<Instruction*>(I->getOperand(0));
                if (instToResoreMetadata && !instToResoreMetadata->getMetadata("implicitGlobalID"))
                {
                    auto instToResoreMetadataMD = MDNode::get(I->getContext(), nullptr);
                    instToResoreMetadata->setMetadata("implicitGlobalID", instToResoreMetadataMD);
                }
            }
        }
    }

    return false;
}

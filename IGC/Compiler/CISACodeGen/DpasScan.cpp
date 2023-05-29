/*========================== begin_copyright_notice ============================

Copyright (C) 2023 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "common/LLVMWarningsPush.hpp"
#include <llvm/Pass.h>
#include "common/LLVMWarningsPop.hpp"

#include "Compiler/CISACodeGen/DpasScan.hpp"
#include "Compiler/CISACodeGen/WIAnalysis.hpp"
#include "Compiler/IGCPassSupport.h"
#include "Compiler/MetaDataUtilsWrapper.h"
#include "GenISAIntrinsics/GenIntrinsicInst.h"

using namespace llvm;
using namespace IGC;

// Register pass to igc-opt
#define PASS_FLAG "igc-dpas-scan"
#define PASS_DESCRIPTION "Scans for dpas in conditional block to disable EU fusion feature"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS_BEGIN(DpasScan, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_DEPENDENCY(WIAnalysis)
IGC_INITIALIZE_PASS_DEPENDENCY(MetaDataUtilsWrapper)
IGC_INITIALIZE_PASS_END(DpasScan, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)

char DpasScan::ID = 0;

DpasScan::DpasScan() : FunctionPass(DpasScan::ID) {
    initializeDpasScanPass(*PassRegistry::getPassRegistry());
}

void DpasScan::getAnalysisUsage(AnalysisUsage& AU) const {
    AU.addRequired<WIAnalysis>();
    AU.addRequired<MetaDataUtilsWrapper>();
}

bool DpasScan::runOnFunction(Function& F) {
    ModuleMetaData* modMD = getAnalysis<MetaDataUtilsWrapper>().getModuleMetaData();
    // exit early because DisableEUFusion was already set
    if (modMD->compOpt.DisableEUFusion) return false;

    WIAnalysis* analysis = &getAnalysis<WIAnalysis>();
    for (auto IIter = inst_begin(F), EIter = inst_end(F); IIter != EIter; ++IIter) {
        Instruction *I = &*IIter;

        if (auto GInst = dyn_cast<GenIntrinsicInst>(I)) {
            auto intrinsidId = GInst->getIntrinsicID();

            switch (intrinsidId) {
            default: break;
            case GenISAIntrinsic::GenISA_dpas:
            case GenISAIntrinsic::GenISA_sub_group_dpas:
                if (analysis->insideDivergentCF(I)) {
                    modMD->compOpt.DisableEUFusion = true;
                    return true;
                }
            }
        }
    }

    return false;
}

/*========================== begin_copyright_notice ============================

Copyright (C) 2018-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#define DEBUG_TYPE "gen-simplification"
#include "Compiler/CISACodeGen/FixupExtractValuePair.h"
#include "Compiler/CISACodeGen/ShaderCodeGen.hpp"
#include "Compiler/CISACodeGen/TranslationTable.hpp"
#include "Compiler/CISACodeGen/WIAnalysis.hpp"
#include "Compiler/CodeGenContextWrapper.hpp"
#include "Compiler/MetaDataUtilsWrapper.h"
#include "Compiler/IGCPassSupport.h"

#include "common/LLVMWarningsPush.hpp"
#include <llvm/Pass.h>
#include <llvm/IR/InstVisitor.h>
#include <llvm/IR/Instruction.h>
#include "common/LLVMWarningsPop.hpp"

using namespace llvm;
using namespace IGC;
using namespace IGC::IGCMD;

namespace {

    class ExtractValuePairFixup : public FunctionPass {
    public:
        static char ID;

        ExtractValuePairFixup() : FunctionPass(ID) {
            initializeExtractValuePairFixupPass(*PassRegistry::getPassRegistry());
        }

        bool runOnFunction(Function&) override;

        void getAnalysisUsage(AnalysisUsage& AU) const override {
            AU.setPreservesCFG();
            AU.addPreservedID(TranslationTable::ID);
            AU.addPreservedID(WIAnalysis::ID);
            AU.addRequired<CodeGenContextWrapper>();
            AU.addRequired<MetaDataUtilsWrapper>();
        }

    private:
        bool fixup(Function& F) const;
    };

} // End anonymous namespace

char ExtractValuePairFixup::ID = 0;

#define PASS_FLAG     "igc-extractvalue-pair-fixup"
#define PASS_DESC     "Fixup extractvalue pairs"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
namespace IGC {
    IGC_INITIALIZE_PASS_BEGIN(ExtractValuePairFixup, PASS_FLAG, PASS_DESC, PASS_CFG_ONLY, PASS_ANALYSIS)
        IGC_INITIALIZE_PASS_DEPENDENCY(CodeGenContextWrapper)
        IGC_INITIALIZE_PASS_DEPENDENCY(MetaDataUtilsWrapper)
        IGC_INITIALIZE_PASS_END(ExtractValuePairFixup, PASS_FLAG, PASS_DESC, PASS_CFG_ONLY, PASS_ANALYSIS)
}

FunctionPass* IGC::createExtractValuePairFixupPass() {
    return new ExtractValuePairFixup();
}

bool ExtractValuePairFixup::runOnFunction(Function& F) {
    // Skip non-kernel function.
    MetaDataUtils* MDU = nullptr;
    MDU = getAnalysis<MetaDataUtilsWrapper>().getMetaDataUtils();
    auto FII = MDU->findFunctionsInfoItem(&F);
    if (FII == MDU->end_FunctionsInfo())
        return false;
    // Fixup pairs of extractvalue.
    return fixup(F);
}

bool ExtractValuePairFixup::fixup(Function& F) const {
    bool Changed = false;
    // Find 'extractvalue' and pull them just after the definition of their
    // aggregation source.
    for (auto& BB : F) {
        for (auto BI = BB.begin(), BE = BB.end(); BI != BE; /*EMPTY*/) {
            auto EVI = dyn_cast<ExtractValueInst>(&*BI++);
            if (!EVI)
                continue;
            Instruction* I = dyn_cast<Instruction>(EVI->getAggregateOperand());
            if (!I || I->getParent() != EVI->getParent())
                continue;
            auto II = I->getIterator();
            for (++II; II != BI; ++II) {
                if (isa<ExtractValueInst>(II))
                    continue;
                // Move this 'extractvalue' just after the aggregate value.
                EVI->moveBefore(BB, II);
                Changed = true;
                break;
            }
        }
    }
    return Changed;
}

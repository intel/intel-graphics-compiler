/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2022 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "GASPropagator.h"

#include "common/LLVMWarningsPush.hpp"
#include <llvm/Analysis/AliasAnalysis.h>
#include "llvm/ADT/PostOrderIterator.h"
#include "common/LLVMWarningsPop.hpp"

#define DEBUG_TYPE "gas-resolver"

using namespace llvm;
using namespace IGC;
using namespace IGC::IGCMD;
using namespace llvm::PatternMatch;

namespace IGC {

    llvm::FunctionPass* createResolveGASPass();

    class GASResolving : public FunctionPass {
        const unsigned GAS = ADDRESS_SPACE_GENERIC;

        BuilderType* IRB;
        GASPropagator* Propagator;

    public:
        static char ID;

        GASResolving() : FunctionPass(ID), IRB(nullptr), Propagator(nullptr) {
            initializeGASResolvingPass(*PassRegistry::getPassRegistry());
        }

        bool runOnFunction(Function&) override;

        void getAnalysisUsage(AnalysisUsage& AU) const override {
            AU.setPreservesCFG();
            AU.addRequired<LoopInfoWrapperPass>();
            AU.addRequired<AAResultsWrapperPass>();
            AU.addRequired<MetaDataUtilsWrapper>();
        }

    private:
        bool resolveOnFunction(Function*) const;
        bool resolveOnBasicBlock(BasicBlock*) const;

        bool resolveMemoryFromHost(Function&) const;

        bool checkGenericArguments(Function& F) const;
        void convertLoadToGlobal(LoadInst* LI) const;
        bool isLoadGlobalCandidate(LoadInst* LI) const;

        bool canonicalizeAddrSpaceCasts(Function& F) const;
    };
} // End namespace IGC

/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "Compiler/CodeGenPublic.h"
#include "common/LLVMWarningsPush.hpp"
#include "llvm/Config/llvm-config.h"
#include "llvm/Analysis/TargetTransformInfo.h"
#include "llvm/Analysis/TargetTransformInfoImpl.h"
#include "common/LLVMWarningsPop.hpp"

namespace llvm
{
    class DummyPass : public ImmutablePass
    {
    public:
        static char ID;
        DummyPass();
    };

    // This implementation allows us to define our own costs for the GenIntrinsics
    // Did not use BasicTTIImplBase because the overloaded constructors have TragetMachine as an argument,
    // so I inherited from its parent which has only DL as its arguments
    class GenIntrinsicsTTIImpl : public TargetTransformInfoImplCRTPBase<GenIntrinsicsTTIImpl>
    {
        typedef TargetTransformInfoImplCRTPBase<GenIntrinsicsTTIImpl> BaseT;
        typedef TargetTransformInfo TTI;
        friend BaseT;
        IGC::CodeGenContext* ctx;
        DummyPass* dummyPass;
    public:
        GenIntrinsicsTTIImpl(IGC::CodeGenContext* pCtx, DummyPass* pDummyPass) :
            BaseT(pCtx->getModule()->getDataLayout()), ctx(pCtx) {
            dummyPass = pDummyPass;
        }

        bool shouldBuildLookupTables();

        bool isLoweredToCall(const Function* F);

        void* getAdjustedAnalysisPointer(const void* ID);

        // [POC] Implemented to use InferAddressSpaces pass after
        // PrivateMemoryToSLM pass to propagate ADDRESS_SPACE_PRIVATE
        // from variables to memory operations.
        unsigned getFlatAddressSpace();

        void getUnrollingPreferences(Loop* L,
#if LLVM_VERSION_MAJOR >= 7
            ScalarEvolution & SE,
#endif
            TTI::UnrollingPreferences & UP
#if LLVM_VERSION_MAJOR >= 14
        , OptimizationRemarkEmitter* ORE
#endif
        );

#if LLVM_VERSION_MAJOR >= 11
        void getPeelingPreferences(Loop* L, ScalarEvolution& SE,
            TTI::PeelingPreferences& PP);
#endif

        bool isProfitableToHoist(Instruction* I);

#if LLVM_VERSION_MAJOR <= 10
        using BaseT::getCallCost;
        unsigned getCallCost(const Function* F, ArrayRef<const Value*> Args
#if LLVM_VERSION_MAJOR >= 9
            , const User * U
#endif
        );
#elif LLVM_VERSION_MAJOR <= 12
       int getUserCost(const User *U, ArrayRef<const Value *> Operands,
                      TTI::TargetCostKind CostKind);
#else
       llvm::InstructionCost getUserCost(const User* U, ArrayRef<const Value*> Operands,
           TTI::TargetCostKind CostKind);
#endif

    };

}

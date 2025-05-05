
/*========================== begin_copyright_notice ============================

Copyright (C) 2025 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "Compiler/CISACodeGen/MergeUniformStores.hpp"
#include "Compiler/CISACodeGen/helper.h"
#include "common/debug/Debug.hpp"
#include <llvm/IR/Function.h>

using namespace llvm;
using namespace IGC;

#define PASS_FLAG "igc-merge-uniform-stores"
#define PASS_DESCRIPTION "Collapse SIMD32 uniform stores into SIMD1 to reduce pressure"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS_BEGIN(MergeUniformStores, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_END(MergeUniformStores, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)

char MergeUniformStores::ID = 0;

MergeUniformStores::MergeUniformStores() : FunctionPass(ID)
{
    initializeMergeUniformStoresPass(*PassRegistry::getPassRegistry());
}

bool MergeUniformStores::runOnFunction(Function& F)
{
    WI = &getAnalysis<WIAnalysis>();

    visit(F);

    return false;
}


void MergeUniformStores::doInitialization(StoreInst& I)
{
    IRBuilder<> builder(&I);

    Type* TypesBal[2] = { builder.getInt1Ty(), builder.getInt32Ty() };
    mWaveBallotFn = GenISAIntrinsic::getDeclaration(
        I.getModule(), GenISAIntrinsic::GenISA_WaveBallot, TypesBal);
    Value* ArgsBal[2] = { builder.getTrue(), builder.getInt32(0) };
    mWaveBallot = builder.CreateCall(mWaveBallotFn, ArgsBal, "WaveBallot");

    mFirstBitHiFn = GenISAIntrinsic::getDeclaration(
        I.getModule(), GenISAIntrinsic::GenISA_firstbitHi);
    mFirstBitHi = builder.CreateCall(mFirstBitHiFn, mWaveBallot, "FirstBitHi");

    Type* TypesShuf[3] = { I.getOperand(0)->getType(), builder.getInt32Ty(), builder.getInt32Ty() };
    mWaveShuffleIndexFn = GenISAIntrinsic::getDeclaration(
        I.getModule(), GenISAIntrinsic::GenISA_WaveShuffleIndex, TypesShuf);
}

// From
//     store i32 %14, i32 addrspace(3)* null, align 2147483648
// To
//     %ballot = call i32 @llvm.genx.GenISA.WaveBallot(i1 true, i32 0)
//     %lastBit = call i32 @llvm.genx.GenISA.firstbitHi(i32 %ballot)
//     %lastVal = call i32 @llvm.genx.GenISA.WaveShuffleIndex.i32(i32 %14, i32 %lastBit, i32 0)
//     store i32 %lastVal, i32 addrspace(3) * null, align 2147483648

void MergeUniformStores::visitStoreInst(StoreInst& I)
{
    Value* pointerOperand = I.getPointerOperand();
    Value* valueOperand = I.getValueOperand();

    Align InstAlign = IGCLLVM::getAlign(I);
    Align DataAlign = IGCLLVM::getABITypeAlign(
        I.getModule()->getDataLayout(), valueOperand->getType());

    // Base address must be data element aligned.
    if ((IGCLLVM::getAlignmentValue(InstAlign) %
        IGCLLVM::getAlignmentValue(DataAlign)) != 0)
        return;

    // support scalar type only
    if (valueOperand->getType()->isVectorTy())
        return;

    // support SLM only
    if (I.getPointerAddressSpace() != ADDRESS_SPACE_LOCAL)
        return;

    // check if uniform
    if (!WI->isUniform(pointerOperand))
        return;

    // check the value is atomic
    if (!IsAtomicIntrinsic(GetOpCode(cast<Instruction>(valueOperand))))
        return;

    if (!initialized)
    {
        doInitialization(I);
        initialized = true;
    }

    IRBuilder<> builder(&I);
    Value* ArgsShuf[3] = { I.getOperand(0), mFirstBitHi, builder.getInt32(0) };
    Value* waveShuffleIndex = builder.CreateCall(mWaveShuffleIndexFn, ArgsShuf, "WaveShufIdx");
    I.setOperand(0, waveShuffleIndex);
}

#undef PASS_FLAG
#undef PASS_DESCRIPTION
#undef PASS_CFG_ONLY
#undef PASS_ANALYSIS

/*========================== begin_copyright_notice ============================

Copyright (C) 2022 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

//===----------------------------------------------------------------------===//
///
/// After having padded out the spills to the stack frame and vectorized them,
/// we need to trim off anything extra that isn't writing anything useful.
///
//===----------------------------------------------------------------------===//

#include "Compiler/IGCPassSupport.h"
#include "Compiler/CodeGenContextWrapper.hpp"
#include "Compiler/CodeGenPublic.h"
#include "MemRegionAnalysis.h"
#include "llvmWrapper/IR/DerivedTypes.h"
#include "Utils.h"

#include "common/LLVMWarningsPush.hpp"
#include "llvm/IR/InstIterator.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/ADT/Optional.h"
#include "llvm/Analysis/VectorUtils.h"
#include "llvm/Support/MathExtras.h"
#include "common/LLVMWarningsPop.hpp"

using namespace llvm;
using namespace IGC;

class RTSpillShrinkPass : public FunctionPass
{
public:
    RTSpillShrinkPass() : FunctionPass(ID)
    {
        initializeRTSpillShrinkPassPass(*PassRegistry::getPassRegistry());
    }

    void getAnalysisUsage(AnalysisUsage& AU) const override
    {
        AU.setPreservesCFG();
        AU.addRequired<CodeGenContextWrapper>();
    }

    bool runOnFunction(Function& M) override;
    StringRef getPassName() const override
    {
        return "RTSpillShrinkPass";
    }

    static char ID;
private:
    bool isUndef(const Value* V) const;
    Optional<uint64_t> getUndefMask(Value* V, uint32_t &NumElts) const;
    StoreInst* shrinkSpill(
        StoreInst* SI, const DataLayout& DL, uint32_t NewNumElts, uint32_t StartElt);
    static uint32_t getNewSize(uint32_t Size);

    unsigned getAlignment(const DataLayout& DL, StoreInst* ST) const
    {
        unsigned Align = ST->getAlignment();
        if (Align == 0)
            Align = DL.getABITypeAlignment(ST->getType());
        return Align;
    }

    static constexpr uint32_t MAX_ELTS = 64;
};

char RTSpillShrinkPass::ID = 0;
// Register pass to igc-opt
#define PASS_FLAG2 "rt-spill-shrink"
#define PASS_DESCRIPTION2 "shrink spills after vectorization"
#define PASS_CFG_ONLY2 false
#define PASS_ANALYSIS2 false
IGC_INITIALIZE_PASS_BEGIN(RTSpillShrinkPass, PASS_FLAG2, PASS_DESCRIPTION2, PASS_CFG_ONLY2, PASS_ANALYSIS2)
IGC_INITIALIZE_PASS_DEPENDENCY(CodeGenContextWrapper)
IGC_INITIALIZE_PASS_END(RTSpillShrinkPass, PASS_FLAG2, PASS_DESCRIPTION2, PASS_CFG_ONLY2, PASS_ANALYSIS2)

uint32_t RTSpillShrinkPass::getNewSize(uint32_t Size)
{
    if (Size != 3)
        Size = int_cast<uint32_t>(llvm::PowerOf2Ceil(Size));
    return Size;
}

StoreInst* RTSpillShrinkPass::shrinkSpill(
    StoreInst* SI, const DataLayout& DL, uint32_t NewNumElts, uint32_t StartElt)
{
    IRBuilder<> IRB(SI);
    Value* Val = SI->getValueOperand();
    uint32_t Addrspace = SI->getPointerAddressSpace();
    Type* EltTy =
        cast<IGCLLVM::FixedVectorType>(Val->getType())->getElementType();
    Value* Vec = UndefValue::get(
        IGCLLVM::FixedVectorType::get(EltTy, NewNumElts));
    IRB.SetInsertPoint(SI);
    for (uint32_t i = 0; i < NewNumElts; i++)
    {
        auto* CurElt = IRB.CreateExtractElement(Val, StartElt + i);
        Vec = IRB.CreateInsertElement(Vec, CurElt, i);
    }
    Value* NewPtr = SI->getPointerOperand();
    if (StartElt != 0)
    {
        NewPtr = IRB.CreateBitCast(NewPtr, EltTy->getPointerTo(Addrspace));
        NewPtr = IRB.CreateGEP(EltTy, NewPtr, IRB.getInt32(StartElt));
    }
    NewPtr = IRB.CreateBitCast(
        NewPtr,
        Vec->getType()->getPointerTo(Addrspace));
    unsigned OldAlign = getAlignment(DL, SI);
    uint32_t Offset =
        int_cast<uint32_t>((DL.getTypeSizeInBits(EltTy) / 8) * StartElt);
    unsigned NewAlign = gcd(OldAlign, Offset);
    return IRB.CreateAlignedStore(Vec, NewPtr, IGCLLVM::getAlign(NewAlign));
}

bool RTSpillShrinkPass::isUndef(const Value* V) const
{
    if (isa<UndefValue>(V))
        return true;

    while (auto *I = dyn_cast<CastInst>(V))
        V = I->getOperand(0);

    if (auto* RTSA = dyn_cast<SpillAnchorIntrinsic>(V))
        return isa<UndefValue>(RTSA->getValue());

    return false;
}

Optional<uint64_t> RTSpillShrinkPass::getUndefMask(
    Value* V, uint32_t &NumElts) const
{
    if (auto* VTy = dyn_cast<IGCLLVM::FixedVectorType>(V->getType()))
    {
        NumElts = int_cast<uint32_t>(VTy->getNumElements());
        if (NumElts > MAX_ELTS)
            return None;

        uint64_t Ret = QWBITMASK(NumElts);
        for (uint32_t i = 0; i < NumElts; i++)
        {
            if (auto* Elt = llvm::findScalarElement(V, i))
            {
                if (isUndef(Elt))
                    Ret &= ~(1ULL << i);
            }
        }

        return Ret;
    }
    else
    {
        NumElts = 1;
        if (isUndef(V))
            return 0;

        return 1;
    }
}

bool RTSpillShrinkPass::runOnFunction(Function& F)
{
    auto *Ctx = getAnalysis<CodeGenContextWrapper>().getCodeGenContext();
    auto* MMD = Ctx->getModuleMetaData();

    IRBuilder<> IRB(F.getContext());
    bool Changed = false;
    auto& DL = F.getParent()->getDataLayout();
    for (auto II = inst_begin(F), E = inst_end(F); II != E; /* empty */)
    {
        auto* I = &*II++;
        auto* SI = dyn_cast<StoreInst>(I);
        if (!SI || !SI->isSimple())
            continue;

        Value* Val = SI->getValueOperand();
        if (!Val->getType()->isSingleValueType())
            continue;

        if (getRTRegionByAddrspace(SI->getPointerOperand(), *MMD) !=
            RTMemRegion::SWStack)
            continue;

        uint32_t NumElts = 0;
        auto Mask = getUndefMask(Val, NumElts);
        if (!Mask)
            continue;

        if (*Mask == 0)
        {
            Changed = true;
            SI->eraseFromParent();
        }
        else if (NumElts != 1)
        {
            uint32_t CLZ = llvm::countLeadingZeros(*Mask);
            uint32_t CTZ = llvm::countTrailingZeros(*Mask);
            if (uint32_t NewSize = getNewSize(MAX_ELTS - CLZ);
                NewSize < NumElts)
            {
                Changed = true;
                shrinkSpill(SI, DL, NewSize, 0);
                SI->eraseFromParent();
            }
            else if (uint32_t NewSize = getNewSize(NumElts - CTZ);
                     NewSize < NumElts && CTZ + NewSize <= NumElts)
            {
                Changed = true;
                shrinkSpill(SI, DL, NewSize, CTZ);
                SI->eraseFromParent();
            }
        }
    }

    return Changed;
}

namespace IGC
{

Pass* createRTSpillShrinkPass(void)
{
    return new RTSpillShrinkPass();
}

} // namespace IGC

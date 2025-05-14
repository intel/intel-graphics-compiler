/*========================== begin_copyright_notice ============================

Copyright (C) 2025 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "common/LLVMWarningsPush.hpp"
#include <llvm/IR/NoFolder.h>
#include "llvm/IR/Type.h"
#include <llvmWrapper/IR/DerivedTypes.h>
#include "common/LLVMWarningsPop.hpp"

#include "MemOptUtils.h"

using namespace llvm;

namespace IGC {
    std::optional<ALoadInst> ALoadInst::get(Value *value)
    {
        if (isa<LoadInst>(value) || isa<PredicatedLoadIntrinsic>(value))
            return ALoadInst{cast<Instruction>(value)};
        return std::nullopt;
    }

    Type *ALoadInst::getType() const
    {
        if (isa<LoadInst>(m_inst))
            return cast<LoadInst>(m_inst)->getType();

        if (isa<PredicatedLoadIntrinsic>(m_inst))
            return cast<PredicatedLoadIntrinsic>(m_inst)->getType();

        llvm_unreachable("Unknown load instruction");
    }
    Value *ALoadInst::getPointerOperand() const
    {
        if (isa<LoadInst>(m_inst))
            return cast<LoadInst>(m_inst)->getPointerOperand();

        if (isa<PredicatedLoadIntrinsic>(m_inst))
            return cast<PredicatedLoadIntrinsic>(m_inst)->getPointerOperand();

        llvm_unreachable("Unknown load instruction");
    }
    Type *ALoadInst::getPointerOperandType() const
    {
        if (isa<LoadInst>(m_inst))
            return cast<LoadInst>(m_inst)->getPointerOperandType();

        if (isa<PredicatedLoadIntrinsic>(m_inst))
            return cast<PredicatedLoadIntrinsic>(m_inst)->getPointerOperandType();

        llvm_unreachable("Unknown load instruction");
    }
    unsigned ALoadInst::getPointerAddressSpace() const
    {
        if (isa<LoadInst>(m_inst))
            return cast<LoadInst>(m_inst)->getPointerAddressSpace();

        if (isa<PredicatedLoadIntrinsic>(m_inst))
            return cast<PredicatedLoadIntrinsic>(m_inst)->getPointerAddressSpace();

        llvm_unreachable("Unknown load instruction");
    }
    bool ALoadInst::isVolatile() const
    {
        if (isa<LoadInst>(m_inst))
            return cast<LoadInst>(m_inst)->isVolatile();

        if (isa<PredicatedLoadIntrinsic>(m_inst))
            return cast<PredicatedLoadIntrinsic>(m_inst)->isVolatile();

        llvm_unreachable("Unknown load instruction");
    }
    void ALoadInst::setVolatile(bool isVolatile)
    {
        if (isa<LoadInst>(m_inst))
            cast<LoadInst>(m_inst)->setVolatile(isVolatile);

        if (isa<PredicatedLoadIntrinsic>(m_inst))
            cast<PredicatedLoadIntrinsic>(m_inst)->setVolatile(isVolatile);

        llvm_unreachable("Unknown load instruction");
    }
    bool ALoadInst::isSimple() const
    {
        if (isa<LoadInst>(m_inst))
            return cast<LoadInst>(m_inst)->isSimple();

        if (isa<PredicatedLoadIntrinsic>(m_inst))
            return cast<PredicatedLoadIntrinsic>(m_inst)->isSimple();

        llvm_unreachable("Unknown load instruction");
    }

    alignment_t ALoadInst::getAlignmentValue() const
    {
        if (isa<LoadInst>(m_inst))
            return IGCLLVM::getAlignmentValue(cast<LoadInst>(m_inst));

        if (isa<PredicatedLoadIntrinsic>(m_inst))
            return cast<PredicatedLoadIntrinsic>(m_inst)->getAlignment();

        llvm_unreachable("Unknown load instruction");
    }

    bool ALoadInst::isPredicated() const
    {
        return isa<PredicatedLoadIntrinsic>(m_inst);
    }

    Value *ALoadInst::getPredicate() const
    {
        if (isa<PredicatedLoadIntrinsic>(m_inst))
            return cast<PredicatedLoadIntrinsic>(m_inst)->getPredicate();

        return nullptr;
    }

    PredicatedLoadIntrinsic *ALoadInst::getPredicatedLoadIntrinsic() const
    {
        if (isa<PredicatedLoadIntrinsic>(m_inst))
            return cast<PredicatedLoadIntrinsic>(m_inst);

        return nullptr;
    }

    template <typename T>
    Instruction *ALoadInst::CreateLoad(IGCIRBuilder<T> &IRB, Type *Ty, Value *Ptr, Value *MergeValue)
    {
        if (isa<LoadInst>(m_inst))
            return IRB.CreateLoad(Ty, Ptr);

        if (isa<PredicatedLoadIntrinsic>(m_inst))
        {
            IGC_ASSERT(MergeValue);
            PredicatedLoadIntrinsic *PLI = cast<PredicatedLoadIntrinsic>(m_inst);
            Module *M = IRB.GetInsertBlock()->getParent()->getParent();
            auto *F = GenISAIntrinsic::getDeclaration(M,
                                                      GenISAIntrinsic::GenISA_PredicatedLoad,
                                                      {Ty, Ptr->getType(), Ty});
            const DataLayout &DL = M->getDataLayout();

            Value *alignValue = ConstantInt::get(Type::getInt64Ty(IRB.getContext()),
                DL.getABITypeAlign(Ty).value());
            return IRB.CreateCall4(F, Ptr, alignValue, PLI->getPredicate(), MergeValue);
        }

        llvm_unreachable("Unknown load instruction");
    }

    template Instruction *ALoadInst::CreateLoad(IGCIRBuilder<> &IRB, Type *Ty, Value *Ptr, Value *MergeValue);
    template Instruction *ALoadInst::CreateLoad(IGCIRBuilder<NoFolder> &IRB, Type *Ty, Value *Ptr, Value *MergeValue);

    template <typename T>
    Instruction *ALoadInst::CreateAlignedLoad(IGCIRBuilder<T> &IRB, Type *Ty, Value *Ptr, Value* MergeValue, bool isVolatile)
    {
        if (isa<LoadInst>(m_inst))
        {
            LoadInst *LI = cast<LoadInst>(m_inst);
            return IRB.CreateAlignedLoad(Ty, Ptr, IGCLLVM::getAlign(*LI), isVolatile);
        }

        if (isa<PredicatedLoadIntrinsic>(m_inst))
        {
            IGC_ASSERT_MESSAGE(!isVolatile, "PredicatedLoadIntrinsic should not be volatile");
            IGC_ASSERT(MergeValue);
            PredicatedLoadIntrinsic *PLI = cast<PredicatedLoadIntrinsic>(m_inst);
            auto *F = GenISAIntrinsic::getDeclaration(IRB.GetInsertBlock()->getParent()->getParent(),
                                                      GenISAIntrinsic::GenISA_PredicatedLoad,
                                                      {Ty, Ptr->getType(), Ty});
            return IRB.CreateCall4(F, Ptr, PLI->getAlignmentValue(), PLI->getPredicate(), MergeValue);
        }

        llvm_unreachable("Unknown load instruction");
    }

    template Instruction *ALoadInst::CreateAlignedLoad(IGCIRBuilder<> &IRB, Type *Ty, Value *Ptr, Value *MergeValue, bool isVolatile);
    template Instruction *ALoadInst::CreateAlignedLoad(IGCIRBuilder<NoFolder> &IRB, Type *Ty, Value *Ptr, Value *MergeValue, bool isVolatile);

    Value *AStoreInst::getPointerOperand() const
    {
        if (isa<StoreInst>(m_inst))
            return cast<StoreInst>(m_inst)->getPointerOperand();

        if (isa<PredicatedStoreIntrinsic>(m_inst))
            return cast<PredicatedStoreIntrinsic>(m_inst)->getPointerOperand();

        llvm_unreachable("Unknown store instruction");
    }
    Type *AStoreInst::getPointerOperandType() const
    {
        if (isa<StoreInst>(m_inst))
            return cast<StoreInst>(m_inst)->getPointerOperandType();

        if (isa<PredicatedStoreIntrinsic>(m_inst))
            return cast<PredicatedStoreIntrinsic>(m_inst)->getPointerOperandType();

        llvm_unreachable("Unknown store instruction");
    }
    unsigned AStoreInst::getPointerAddressSpace() const
    {
        if (isa<StoreInst>(m_inst))
            return cast<StoreInst>(m_inst)->getPointerAddressSpace();

        if (isa<PredicatedStoreIntrinsic>(m_inst))
            return cast<PredicatedStoreIntrinsic>(m_inst)->getPointerAddressSpace();

        llvm_unreachable("Unknown store instruction");
    }
    Value *AStoreInst::getValueOperand() const
    {
        if (isa<StoreInst>(m_inst))
            return cast<StoreInst>(m_inst)->getValueOperand();

        if (isa<PredicatedStoreIntrinsic>(m_inst))
            return cast<PredicatedStoreIntrinsic>(m_inst)->getValueOperand();

        llvm_unreachable("Unknown store instruction");
    }
    bool AStoreInst::isVolatile() const
    {
        if (isa<StoreInst>(m_inst))
            return cast<StoreInst>(m_inst)->isVolatile();

        if (isa<PredicatedStoreIntrinsic>(m_inst))
            return cast<PredicatedStoreIntrinsic>(m_inst)->isVolatile();

        llvm_unreachable("Unknown store instruction");
    }
    void AStoreInst::setVolatile(bool isVolatile)
    {
        if (isa<StoreInst>(m_inst))
            cast<StoreInst>(m_inst)->setVolatile(isVolatile);

        if (isa<PredicatedStoreIntrinsic>(m_inst))
            cast<PredicatedStoreIntrinsic>(m_inst)->setVolatile(isVolatile);

        llvm_unreachable("Unknown store instruction");
    }
    bool AStoreInst::isSimple() const
    {
        if (isa<StoreInst>(m_inst))
            return cast<StoreInst>(m_inst)->isSimple();

        if (isa<PredicatedStoreIntrinsic>(m_inst))
            return cast<PredicatedStoreIntrinsic>(m_inst)->isSimple();

        llvm_unreachable("Unknown store instruction");
    }

    alignment_t AStoreInst::getAlignmentValue() const
    {
        if (isa<StoreInst>(m_inst))
            return IGCLLVM::getAlignmentValue(cast<StoreInst>(m_inst));

        if (isa<PredicatedStoreIntrinsic>(m_inst))
            return cast<PredicatedStoreIntrinsic>(m_inst)->getAlignment();

        llvm_unreachable("Unknown store instruction");
    }

    bool AStoreInst::isPredicated() const
    {
        return isa<PredicatedStoreIntrinsic>(m_inst);
    }

    Value *AStoreInst::getPredicate() const
    {
        if (isa<PredicatedStoreIntrinsic>(m_inst))
            return cast<PredicatedStoreIntrinsic>(m_inst)->getPredicate();

        return nullptr;
    }

    std::optional<AStoreInst> AStoreInst::get(Value *value)
    {
        if (isa<StoreInst>(value) || isa<PredicatedStoreIntrinsic>(value))
        {
            return AStoreInst{cast<Instruction>(value)};
        }
        return std::nullopt;
    }
    template <typename T>
    Instruction *AStoreInst::CreateAlignedStore(IGCIRBuilder<T> &IRB, Value *Val, Value *Ptr, bool isVolatile)
    {
        if (isa<StoreInst>(m_inst))
        {
            StoreInst *SI = cast<StoreInst>(m_inst);
            return IRB.CreateAlignedStore(Val, Ptr, IGCLLVM::getAlign(*SI), isVolatile);
        }

        if (isa<PredicatedStoreIntrinsic>(m_inst))
        {
            IGC_ASSERT_MESSAGE(!isVolatile, "PredicatedStoreIntrinsic should not be volatile");
            PredicatedStoreIntrinsic *PSI = cast<PredicatedStoreIntrinsic>(m_inst);
            auto *F = GenISAIntrinsic::getDeclaration(IRB.GetInsertBlock()->getParent()->getParent(),
                                                      GenISAIntrinsic::GenISA_PredicatedStore,
                                                      {Ptr->getType(), Val->getType()});
            return IRB.CreateCall4(F, Ptr, Val, PSI->getAlignmentValue(), PSI->getPredicate());
        }

        llvm_unreachable("Unknown store instruction");
    }

    template Instruction *AStoreInst::CreateAlignedStore(IGCIRBuilder<> &IRB, Value *Val, Value *Ptr, bool isVolatile);
    template Instruction *AStoreInst::CreateAlignedStore(IGCIRBuilder<NoFolder> &IRB, Value *Val, Value *Ptr, bool isVolatile);

    // other utility functions, that should take into account abstract interface
    MemoryLocation getLocation(Instruction *I, TargetLibraryInfo *TLI)
    {
        if (LoadInst *LI = dyn_cast<LoadInst>(I))
            return MemoryLocation::get(LI);

        if (StoreInst *SI = dyn_cast<StoreInst>(I))
            return MemoryLocation::get(SI);

        if (isa<LdRawIntrinsic>(I) || isa<StoreRawIntrinsic>(I) ||
            isa<PredicatedLoadIntrinsic>(I) || isa<PredicatedStoreIntrinsic>(I))
            return MemoryLocation::getForArgument(cast<CallInst>(I), 0, TLI);

        if (GenIntrinsicInst *GInst = dyn_cast<GenIntrinsicInst>(I))
            if (GInst->getIntrinsicID() == GenISAIntrinsic::GenISA_simdBlockRead)
                return MemoryLocation::getForArgument(cast<CallInst>(I), 0, TLI);

        // TODO: Do coarse-grained thing so far. Need better checking for
        // non load or store instructions which may read/write memory.
        return MemoryLocation();
    }

    // Symbolic difference of two address values
    // return value:
    //   true  if A1 - A0 = constant in bytes, and return that constant as BO.
    //   false if A1 - A0 != constant. BO will be undefined.
    // BO: byte offset
    bool getDiffIfConstant(Value *A0, Value *A1, int64_t &ConstBO, SymbolicEvaluation &symEval)
    {
        // Using a simple integer symbolic expression (polynomial) as SCEV
        // does not work well for this.
        SymExpr *S0 = symEval.getSymExpr(A0);
        SymExpr *S1 = symEval.getSymExpr(A1);
        return symEval.isOffByConstant(S0, S1, ConstBO);
    }

    // If I0 and I1 are load/store insts, compare their address operands and return
    // the constant difference if it is; return false otherwise.
    bool getAddressDiffIfConstant(Instruction *I0, Instruction *I1, int64_t &BO, SymbolicEvaluation &symEval)
    {
        if (isa<LoadInst>(I0) && isa<LoadInst>(I1) ||
            isa<PredicatedLoadIntrinsic>(I0) && isa<PredicatedLoadIntrinsic>(I1))
        {
            auto LI0 = ALoadInst::get(I0);
            auto LI1 = ALoadInst::get(I1);
            return getDiffIfConstant(LI0->getPointerOperand(), LI1->getPointerOperand(), BO, symEval);
        }
        if (isa<StoreInst>(I0) && isa<StoreInst>(I1) ||
            isa<PredicatedStoreIntrinsic>(I0) && isa<PredicatedStoreIntrinsic>(I1))
        {
            auto SI0 = AStoreInst::get(I0);
            auto SI1 = AStoreInst::get(I1);
            return getDiffIfConstant(SI0->getPointerOperand(), SI1->getPointerOperand(), BO, symEval);
        }
        return false;
    }
} // namespace IGC

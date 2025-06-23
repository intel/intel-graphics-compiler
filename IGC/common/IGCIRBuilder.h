/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2022 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "common/LLVMWarningsPush.hpp"
#include "llvmWrapper/IR/IRBuilder.h"
#include "llvm/ADT/ArrayRef.h"
#include "llvmWrapper/IR/Instructions.h"
#include "llvm/IR/Intrinsics.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/ADT/Twine.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/ConstantFolder.h"
#include "llvm/IR/Function.h"
#include <llvm/Analysis/TargetFolder.h>
#include "llvm/IR/InstrTypes.h"
#include "common/LLVMWarningsPop.hpp"
#include "GenISAIntrinsics/GenIntrinsics.h"
#include "CommonMacros.h"

//This Builder class provides definitions for functions calls that were once available till LLVM version 3.6.0
//===--------------------------------------------------------------------===
// CreateCall Variations which are removed in 3.8 for API Simplification, which IGC still finds convenient to use

//===--------------------------------------------------------------------===
namespace llvm {

    template<typename T = ConstantFolder, typename InserterTy = IRBuilderDefaultInserter >
    class IGCIRBuilder : public IGCLLVM::IRBuilder<T, InserterTy>
    {
    public:
        IGCIRBuilder(LLVMContext &C, const T &F, InserterTy I = InserterTy(),
            MDNode *FPMathTag = nullptr,
            ArrayRef<OperandBundleDef> OpBundles = {})
            : IGCLLVM::IRBuilder<T, InserterTy>(C, F, I, FPMathTag, OpBundles){}

        explicit IGCIRBuilder(LLVMContext &C, MDNode *FPMathTag = nullptr,
            ArrayRef<OperandBundleDef> OpBundles = {})
            : IGCLLVM::IRBuilder<T, InserterTy>(C, FPMathTag, OpBundles){}

        explicit IGCIRBuilder(BasicBlock *TheBB, MDNode *FPMathTag = nullptr)
            : IGCLLVM::IRBuilder<T, InserterTy>(TheBB, FPMathTag){}

        explicit IGCIRBuilder(Instruction *IP, MDNode *FPMathTag = nullptr,
            ArrayRef<OperandBundleDef> OpBundles = {})
            : IGCLLVM::IRBuilder<T, InserterTy>(IP, FPMathTag, OpBundles) {}

        CallInst *CreateCall2(Value *Callee, Value *Arg1, Value *Arg2,
            const Twine &Name = "") {
            IGC_UNUSED(Name);
            Value *Args[] = { Arg1, Arg2 };
            return this->CreateCall(Callee, Args);
        }

        CallInst *CreateCall3(Value *Callee, Value *Arg1, Value *Arg2, Value *Arg3,
            const Twine &Name = "") {
            IGC_UNUSED(Name);
            Value *Args[] = { Arg1, Arg2, Arg3 };
            return this->CreateCall(Callee, Args);
        }

        CallInst *CreateCall4(Value *Callee, Value *Arg1, Value *Arg2, Value *Arg3,
            Value *Arg4, const Twine &Name = "") {
            IGC_UNUSED(Name);
            Value *Args[] = { Arg1, Arg2, Arg3, Arg4 };
            return this->CreateCall(Callee, Args);
        }

        CallInst *CreateCall5(Value *Callee, Value *Arg1, Value *Arg2, Value *Arg3,
            Value *Arg4, Value *Arg5, const Twine &Name = "") {
            IGC_UNUSED(Name);
            Value *Args[] = { Arg1, Arg2, Arg3, Arg4, Arg5 };
            return this->CreateCall(Callee, Args);
        }

        inline Value* CreateAnyValuesNotZero(Value** values, unsigned nvalues)
        {
            Value* f0 = ConstantFP::get(values[0]->getType(), 0.0);
            Value* ne0 = this->CreateFCmpUNE(values[0], f0);
            for (unsigned i = 1; i < nvalues; i++)
            {
                ne0 = this->CreateOr(ne0,
                    this->CreateFCmpUNE(values[i], f0));
            }
            return ne0;
        }

        inline Value* CreateAllValuesAreZeroF(Value** values, unsigned nvalues)
        {
            if (nvalues)
            {
                return CreateAllValuesAreConstantFP(values, nvalues,
                    ConstantFP::get(values[0]->getType(), 0.0));
            }
            return nullptr;
        }

        inline Value* CreateAllValuesAreOneF(Value** values, unsigned nvalues)
        {
            if (nvalues)
            {
                return CreateAllValuesAreConstantFP(values, nvalues,
                    ConstantFP::get(values[0]->getType(), 1.0));
            }
            return nullptr;
        }

        inline Value* CreateExtractElementOrPropagate(Value* vec, Value* idx, const Twine& name = "")
        {
            if(vec == nullptr || idx == nullptr) return nullptr;
            Value* srcVec = vec;

            // Traverse ir that created source vector, looking for
            // insertelement with the index we are interested in
            while(auto* IE = dyn_cast<InsertElementInst>(srcVec))
            {
                Value* srcVal = IE->getOperand(1);
                Value* srcIdx = IE->getOperand(2);

                auto* cSrcIdx = dyn_cast<ConstantInt>(srcIdx);
                auto* cIdx    = dyn_cast<ConstantInt>(idx);
                if(srcIdx == idx ||
                   (cSrcIdx && cIdx &&
                    cSrcIdx->getZExtValue() == cIdx->getZExtValue()))
                {
                    return srcVal;
                }
                else
                {
                    // If index isn't constant we cannot iterate further, since
                    // we don't know which element was replaced in just visited
                    // insertelement.
                    if(!isa<ConstantInt>(idx))
                    {
                        break;
                    }
                    // This is not the instruction we are looking for.
                    // Get it's source and iterate further.
                    srcVec = IE->getOperand(0);
                }
            }

            // We cannot find value to propagate propagate, add extractelement
            return this->CreateExtractElement(vec, idx, name);
        }

        inline Value* CreateExtractElementOrPropagate(Value* vec, uint64_t idx, const Twine& name = "")
        {
            return CreateExtractElementOrPropagate(vec, this->getInt64(idx), name);
        }

        inline Function* llvm_GenISA_staticConstantPatch(Type* RetTy, Type* ArgTy)
        {
            Module* module = this->GetInsertBlock()->getParent()->getParent();

            Type* Tys[] = { RetTy, ArgTy };

            Function* func_llvm_GenISA_staticConstantPatchValue =
                GenISAIntrinsic::getDeclaration(
                    module,
                    GenISAIntrinsic::GenISA_staticConstantPatchValue,
                    Tys);
            return func_llvm_GenISA_staticConstantPatchValue;
        }

        inline CallInst* CreateStaticConstantPatch(Type* RetTy, StringRef patchName, const Twine& Name = "")
        {
            auto* Arg = ConstantDataArray::getString(RetTy->getContext(), patchName, false);
            Function* func = llvm_GenISA_staticConstantPatch(RetTy, Arg->getType());
            return this->CreateCall(func, Arg, Name);
        }

        inline void SetDebugReg(Value* V, const Twine& Name = "")
        {
          Module *M = this->GetInsertBlock()->getParent()->getParent();
          Function *fn = GenISAIntrinsic::getDeclaration(
              M, GenISAIntrinsic::GenISA_SetDebugReg);

          if (!isa<Constant>(V)) {

            // read the first lane because debug register requires the value to
            // be uniform
            Function *waveBallotFn = GenISAIntrinsic::getDeclaration(
                M, GenISAIntrinsic::GenISA_WaveBallot);

            Function *waveShuffleIndexFn = GenISAIntrinsic::getDeclaration(
                M, GenISAIntrinsic::GenISA_WaveShuffleIndex,
                V->getType());

            Function *fblFn = GenISAIntrinsic::getDeclaration(
                M, GenISAIntrinsic::GenISA_firstbitLo);

            CallInst *ballot = this->CreateCall2(waveBallotFn, this->getTrue(),
                                                 this->getInt32(0));

            auto *firstLaneId = this->CreateCall(fblFn, ballot);
            V = this->CreateCall3(waveShuffleIndexFn, V, firstLaneId,
                                  this->getInt32(0));
          }

          if (V->getType() == this->getInt32Ty()) {
            this->CreateCall(fn, V, Name);
            return;
          }

          if (V->getType()->isPointerTy()) {
            V = this->CreatePtrToInt(V, this->getInt64Ty());
            this->CreateCall(fn, this->CreateTrunc(V, this->getInt32Ty()),
                             Name);
            this->CreateCall(
                fn,
                this->CreateTrunc(this->CreateLShr(V, 32), this->getInt32Ty()),
                Name);
            return;
          }

          if (V->getType()->getPrimitiveSizeInBits() == 32) {
            this->CreateCall(fn, this->CreateBitCast(V, this->getInt32Ty()),
                             Name);
            return;
          }

          IGC_ASSERT_MESSAGE(0, "Unhandled?");
        }

        inline void SetDebugReg(uint32_t V, const Twine& Name = "")
        {
          this->SetDebugReg(this->getInt32(V));
        }

    private:

        inline Value* CreateAllValuesAreConstantFP(Value** values,
            unsigned nvalues, Value* constVal)
        {
            if (nvalues)
            {
                Value* aeq = this->CreateFCmpOEQ(values[0], constVal);
                for (unsigned i = 1; i < nvalues; i++)
                {
                    aeq = this->CreateAnd(aeq,
                        this->CreateFCmpOEQ(values[i], constVal));
                }
                return aeq;
            }
            return nullptr;
        }
    };

    } // end namespace llvm


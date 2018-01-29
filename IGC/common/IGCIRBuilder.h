
#include "common/LLVMWarningsPush.hpp"
#include "llvm/ADT/ArrayRef.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Intrinsics.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/ADT/Twine.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/ConstantFolder.h"
#include "llvm/IR/Function.h"
#include <llvm/Analysis/TargetFolder.h>
#include "llvm/IR/InstrTypes.h"
#include "common/LLVMWarningsPop.hpp"

#pragma once
//This Builder class provides definitions for functions calls that were once available till LLVM version 3.6.0
//===--------------------------------------------------------------------===
// CreateCall Variations which are removed in 3.8 for API Simplification, which IGC still finds convenient to use

//===--------------------------------------------------------------------===
namespace llvm {

    template<typename T = ConstantFolder, typename Inserter = IRBuilderDefaultInserter >
    class IGCIRBuilder : public IRBuilder<T, Inserter>
    {
    public:
        IGCIRBuilder(LLVMContext &C, const T &F, Inserter I = Inserter(),
            MDNode *FPMathTag = nullptr,
            ArrayRef<OperandBundleDef> OpBundles = None)
            : IRBuilder<T, Inserter>(C, F, I, FPMathTag, OpBundles){}

        explicit IGCIRBuilder(LLVMContext &C, MDNode *FPMathTag = nullptr,
            ArrayRef<OperandBundleDef> OpBundles = None)
            : IRBuilder<T, Inserter>(C, FPMathTag, OpBundles){}

        explicit IGCIRBuilder(BasicBlock *TheBB, MDNode *FPMathTag = nullptr)
            : IRBuilder<T, Inserter>(TheBB, FPMathTag){}

        explicit IGCIRBuilder(Instruction *IP, MDNode *FPMathTag = nullptr,
            ArrayRef<OperandBundleDef> OpBundles = None)
            : IRBuilder<T, Inserter>(IP, FPMathTag, OpBundles) {}

        CallInst *CreateCall2(Value *Callee, Value *Arg1, Value *Arg2,
            const Twine &Name = "") {
            Value *Args[] = { Arg1, Arg2 };
            return this->CreateCall(Callee, Args);
        }

        CallInst *CreateCall3(Value *Callee, Value *Arg1, Value *Arg2, Value *Arg3,
            const Twine &Name = "") {
            Value *Args[] = { Arg1, Arg2, Arg3 };
            return this->CreateCall(Callee, Args);
        }

        CallInst *CreateCall4(Value *Callee, Value *Arg1, Value *Arg2, Value *Arg3,
            Value *Arg4, const Twine &Name = "") {
            Value *Args[] = { Arg1, Arg2, Arg3, Arg4 };
            return this->CreateCall(Callee, Args);
        }

        CallInst *CreateCall5(Value *Callee, Value *Arg1, Value *Arg2, Value *Arg3,
            Value *Arg4, Value *Arg5, const Twine &Name = "") {
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
    };

} // end namespace llvm


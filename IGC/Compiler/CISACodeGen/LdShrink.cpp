/*===================== begin_copyright_notice ==================================

Copyright (c) 2017 Intel Corporation

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.


======================= end_copyright_notice ==================================*/
#include "common/LLVMWarningsPush.hpp"
#include <llvm/Pass.h>
#include <llvm/IR/DataLayout.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/Support/Debug.h>
#include <llvm/Support/MathExtras.h>
#include <llvm/Support/raw_ostream.h>
#include "common/LLVMWarningsPop.hpp"

#include "Compiler/CISACodeGen/ShaderCodeGen.hpp"
#include "Compiler/IGCPassSupport.h"

#include "Compiler/CISACodeGen/LdShrink.h"

using namespace llvm;
using namespace IGC;

namespace {

    // A simple pass to shrink vector load into scalar or narrow vector load
    // when only partial elements are used.
    class LdShrink : public FunctionPass {
        const DataLayout* DL;

    public:
        static char ID;

        LdShrink() : FunctionPass(ID) {
            initializeLdShrinkPass(*PassRegistry::getPassRegistry());
        }

        bool runOnFunction(Function& F) override;

    private:
        void getAnalysisUsage(AnalysisUsage& AU) const override {
            AU.setPreservesCFG();
        }

        unsigned getExtractIndexMask(LoadInst* LI) const;
    };

    char LdShrink::ID = 0;

} // End anonymous namespace

FunctionPass* createLdShrinkPass() {
    return new LdShrink();
}

#define PASS_FLAG     "igc-ldshrink"
#define PASS_DESC     "IGC Load Shrink"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS_BEGIN(LdShrink, PASS_FLAG, PASS_DESC, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_END(LdShrink, PASS_FLAG, PASS_DESC, PASS_CFG_ONLY, PASS_ANALYSIS)

unsigned LdShrink::getExtractIndexMask(LoadInst* LI) const {
    VectorType* VTy = dyn_cast<VectorType>(LI->getType());
    // Skip non-vector loads.
    if (!VTy)
        return 0;
    // Skip if there are more than 32 elements.
    if (VTy->getNumElements() > 32)
        return 0;
    // Check whether all users are ExtractElement with constant index.
    // Collect index mask at the same time.
    Type* Ty = VTy->getScalarType();
    // Skip non-BYTE addressable data types. So far, check integer types
    // only.
    if (IntegerType * ITy = dyn_cast<IntegerType>(Ty))
        if (!ITy->isPowerOf2ByteWidth())
            return 0;

    unsigned Mask = 0; // Maxmimally 32 elements.

    for (auto UI = LI->user_begin(), UE = LI->user_end(); UI != UE; ++UI) {
        ExtractElementInst* EEI = dyn_cast<ExtractElementInst>(*UI);
        if (!EEI)
            return 0;
        // Skip non-constant index.
        auto Idx = dyn_cast<ConstantInt>(EEI->getIndexOperand());
        if (!Idx)
            return 0;
        assert(Idx->getZExtValue() < 32 && "Index is out of range!");
        Mask |= (1 << Idx->getZExtValue());
    }

    return Mask;
}

bool LdShrink::runOnFunction(Function& F) {
    DL = &F.getParent()->getDataLayout();
    if (!DL)
        return false;

    bool Changed = false;
    for (auto& BB : F) {
        for (auto BI = BB.begin(), BE = BB.end(); BI != BE; /*EMPTY*/) {
            LoadInst* LI = dyn_cast<LoadInst>(BI++);
            // Skip non-load instructions.
            if (!LI)
                continue;
            // Skip non-simple load.
            if (!LI->isSimple())
                continue;
            // Replace it with scalar load or narrow vector load.
            unsigned Mask = getExtractIndexMask(LI);
            if (!Mask)
                continue;
            if (!isShiftedMask_32(Mask))
                continue;
            unsigned Offset = llvm::countTrailingZeros(Mask);
            unsigned Length = llvm::countTrailingZeros((Mask >> Offset) + 1);
            // TODO: So far skip narrow vector.
            if (Length != 1)
                continue;

            IRBuilder<> Builder(LI);

            // Shrink it to scalar load.
            auto Ptr = LI->getPointerOperand();
            Type* Ty = LI->getType();
            Type* ScalarTy = Ty->getScalarType();
            PointerType* PtrTy = cast<PointerType>(Ptr->getType());
            PointerType* ScalarPtrTy
                = PointerType::get(ScalarTy, PtrTy->getAddressSpace());
            Value* ScalarPtr = Builder.CreatePointerCast(Ptr, ScalarPtrTy);
            if (Offset)
                ScalarPtr = Builder.CreateInBoundsGEP(ScalarPtr, Builder.getInt32(Offset));

            unsigned Align
                = int_cast<unsigned int>(MinAlign(LI->getAlignment(),
                    DL->getTypeStoreSize(ScalarTy) * Offset));

            LoadInst* NewLoad = Builder.CreateAlignedLoad(ScalarPtr, Align);
            NewLoad->setDebugLoc(LI->getDebugLoc());

            ExtractElementInst* EEI = cast<ExtractElementInst>(*LI->user_begin());
            EEI->replaceAllUsesWith(NewLoad);
        }
    }

    return Changed;
}

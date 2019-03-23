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

#include "WaveIntrinsicWAPass.h"

#include "common/LLVMWarningsPush.hpp"
#include <llvm/IR/Instruction.h>
#include <llvm/IR/BasicBlock.h>
#include "llvm/IR/InlineAsm.h"
#include "llvm/IR/InstIterator.h"
#include "common/LLVMWarningsPop.hpp"

#include "GenISAIntrinsics/GenIntrinsics.h"
#include "GenISAIntrinsics/GenIntrinsicInst.h"

using namespace llvm;

namespace IGC
{
    bool unsafeToHoist(const CallInst *CI)
    {
        return CI->isConvergent() &&
#if LLVM_VERSION_MAJOR >= 7
            CI->onlyAccessesInaccessibleMemory();
#else
            CI->hasFnAttr(Attribute::InaccessibleMemOnly);
#endif
    }

    // We currently use the combination of 'convergent' and 
    // 'inaccessiblememonly' to prevent code motion of
    // wave intrinsics.  Removing 'readnone' from a callsite
    // is not sufficient to stop LICM from looking back up to the
    // function definition for the attribute.  We can short circuit that
    // by creating an operand bundle.  The name "nohoist" is not
    // significant; anything will do.
    CallInst* setUnsafeToHoistAttr(CallInst *CI)
    {
        CI->setConvergent();
#if LLVM_VERSION_MAJOR >= 7
        CI->setOnlyAccessesInaccessibleMemory();
        CI->removeAttribute(AttributeList::FunctionIndex, Attribute::ReadNone);
#else
        CI->addAttribute(
            AttributeSet::FunctionIndex, Attribute::InaccessibleMemOnly);
        CI->removeAttribute(AttributeSet::FunctionIndex, Attribute::ReadNone);
#endif
        OperandBundleDef OpDef("nohoist", None);

        // An operand bundle cannot be appended onto a call after creation.
        // clone the instruction but add our operandbundle on as well.
        SmallVector<OperandBundleDef, 1> OpBundles;
        CI->getOperandBundlesAsDefs(OpBundles);
        OpBundles.push_back(OpDef);
        CallInst *NewCall = CallInst::Create(CI, OpBundles, CI);
        CI->replaceAllUsesWith(NewCall);
        return NewCall;
    }

    class WaveIntrinsicWAPass : public llvm::ModulePass
    {
    public:
        WaveIntrinsicWAPass() :
            ModulePass(ID)
        { }

        virtual bool runOnModule(llvm::Module & M);

        virtual void getAnalysisUsage(llvm::AnalysisUsage & AU) const
        {
            AU.setPreservesCFG();
        }

        virtual llvm::StringRef getPassName() const { return "WaveIntrinsicWA"; }
    private:
        static char ID;
    };

    bool WaveIntrinsicWAPass::runOnModule(llvm::Module & M)
    {
        bool changed = false;
        llvm::LLVMContext& ctx = M.getContext();
        uint32_t counter = 0;
        for (llvm::Function& F : M)
        {
            for (auto &I : instructions(F))
            {
                if (auto *CI = dyn_cast<CallInst>(&I))
                {
                    if (unsafeToHoist(CI))
                    {
                        changed = true;
                        llvm::FunctionType* voidFuncType = llvm::FunctionType::get(llvm::Type::getVoidTy(ctx), false);
                        std::string asmText = "; " + std::to_string(counter++);
                        llvm::CallInst::Create(llvm::InlineAsm::get(voidFuncType, asmText, "", true), "", &I);
                        asmText = "; " + std::to_string(counter++);
                        auto asmAfterIntrinsic = llvm::CallInst::Create(llvm::InlineAsm::get(voidFuncType, asmText, "", true));
                        asmAfterIntrinsic->insertAfter(&I);
                    }
                }
            }
        }
        return changed;
    }

    char WaveIntrinsicWAPass::ID = 0;

    llvm::ModulePass* createWaveIntrinsicWAPass()
    {
        return new WaveIntrinsicWAPass();
    }
}
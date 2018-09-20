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
#include "common/LLVMWarningsPop.hpp"

#include "GenISAIntrinsics/GenIntrinsics.h"
#include "GenISAIntrinsics/GenIntrinsicInst.h"

namespace IGC
{
    bool unsafeToHoist(llvm::GenISAIntrinsic::ID id)
    {
        return id == llvm::GenISAIntrinsic::GenISA_WaveBallot ||
            id == llvm::GenISAIntrinsic::GenISA_WaveAll ||
            id == llvm::GenISAIntrinsic::GenISA_WavePrefix ||
            id == llvm::GenISAIntrinsic::GenISA_QuadPrefix;
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
            for (llvm::BasicBlock& BB : F)
            {
                for (llvm::Instruction& inst : BB)
                {
                    if (llvm::GenIntrinsicInst* genIntrinsic = llvm::dyn_cast<llvm::GenIntrinsicInst>(&inst))
                    {
                        if (unsafeToHoist(genIntrinsic->getIntrinsicID()))
                        {
                            changed = true;
                            llvm::FunctionType* voidFuncType = llvm::FunctionType::get(llvm::Type::getVoidTy(ctx), false);
                            std::string asmText = "; " + std::to_string(counter++);
                            llvm::CallInst::Create(llvm::InlineAsm::get(voidFuncType, asmText, "", true), "", &inst);
                            asmText = "; " + std::to_string(counter++);
                            auto asmAfterIntrinsic = llvm::CallInst::Create(llvm::InlineAsm::get(voidFuncType, asmText, "", true));
                            asmAfterIntrinsic->insertAfter(&inst);
                        }
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
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

#include "Compiler/Optimizer/OpenCLPasses/SetFastMathFlags/SetFastMathFlags.hpp"
#include "Compiler/MetaDataApi/IGCMetaDataHelper.h"
#include "Compiler/IGCPassSupport.h"

#include "common/LLVMWarningsPush.hpp"
#include <llvm/IR/InstIterator.h>
#include <llvm/IR/IntrinsicInst.h>
#include <llvm/IR/Operator.h>
#include "common/LLVMWarningsPop.hpp"

using namespace llvm;
using namespace IGC;
using namespace IGC::IGCMD;

// Register pass to igc-opt
#define PASS_FLAG "igc-set-fast-math-flags"
#define PASS_DESCRIPTION "Set llvm fast math flags according to compiler options"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS_BEGIN(SetFastMathFlags, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_DEPENDENCY(MetaDataUtilsWrapper)
IGC_INITIALIZE_PASS_END(SetFastMathFlags, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)

char SetFastMathFlags::ID = 0;

SetFastMathFlags::SetFastMathFlags() : ModulePass(ID)
{
    initializeSetFastMathFlagsPass(*PassRegistry::getPassRegistry());
}

bool SetFastMathFlags::runOnModule(Module &M) 
{
    const ModuleMetaData &modMD = *(getAnalysis<MetaDataUtilsWrapper>().getModuleMetaData());
    FastMathFlags fmfs;
    if (modMD.compOpt.FastRelaxedMath)
    {
        // Fast relaxed math implies all other flags.
        fmfs.setFast();
        return setFlags(M, fmfs);
    }
    // Unsafe math implies no signed zeros.
    if (modMD.compOpt.NoSignedZeros || modMD.compOpt.UnsafeMathOptimizations)
    {
        fmfs.setNoSignedZeros();
    }
    if (modMD.compOpt.FiniteMathOnly)
    {
        fmfs.setNoInfs();
        fmfs.setNoNaNs();
    }
    return setFlags(M, fmfs);
}

static bool setMathFlags(IntrinsicInst *II) {
    switch (II->getIntrinsicID()) {
    case Intrinsic::pow:
    case Intrinsic::exp2:
    case Intrinsic::log:
    case Intrinsic::sqrt:
        II->setFast(true);
        return true;
    default:
        break;
    }
    return false;
}

bool SetFastMathFlags::setFlags(Module &M, FastMathFlags fmfs)
{
    if (!fmfs.any())
    {
        return false;
    }
    bool changed = false;
    for (Function &F : M)
    {
        for (inst_iterator i = inst_begin(&F), e = inst_end(&F); i != e; ++i)
        {
            unsigned int op = i->getOpcode();
            switch (op) {
            case Instruction::FAdd:
            case Instruction::FSub:
            case Instruction::FMul:
            case Instruction::FDiv:
            case Instruction::FRem:
                i->setFastMathFlags(fmfs); // this actually does an OR between flags.
                changed = true;
                break;
            case Instruction::Call:
                if (auto II = dyn_cast<IntrinsicInst>(&*i)) {
                    if (fmfs.isFast())
                        changed |= setMathFlags(II);
                }
                break;
            default:
                break;
            }
        }
    }
    return changed;
}

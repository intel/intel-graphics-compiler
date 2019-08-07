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

#include "Compiler/Optimizer/OpenCLPasses/ClampLoopUnroll/ClampLoopUnroll.hpp"
#include "Compiler/IGCPassSupport.h"

#include "common/LLVMWarningsPush.hpp"
#include <llvm/Transforms/Utils/UnrollLoop.h>
#include <llvm/Pass.h>
#include "common/LLVMWarningsPop.hpp"

using namespace llvm;
using namespace IGC;

#define PASS_FLAG "igc-clamp-loop-unroll"
#define PASS_DESCRIPTION "Sets a limit on unrolling metadata to control compile time"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS_BEGIN(ClampLoopUnroll, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_END(ClampLoopUnroll, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)

static const char* const LoopMDName = "llvm.loop";

char ClampLoopUnroll::ID = 0;

ClampLoopUnroll::ClampLoopUnroll() :
    FunctionPass(ID), m_MaxUnrollFactor(0), m_Changed(false)
{
    initializeClampLoopUnrollPass(*PassRegistry::getPassRegistry());
}

ClampLoopUnroll::ClampLoopUnroll(unsigned maxUnrollFactor) :
    FunctionPass(ID), m_MaxUnrollFactor(maxUnrollFactor), m_Changed(false)
{
    initializeClampLoopUnrollPass(*PassRegistry::getPassRegistry());
}

static MDNode* GetUnrollCountMD(const IGCLLVM::TerminatorInst& I, unsigned unroll)
{
    LLVMContext& Context = I.getContext();
    SmallVector<Metadata*, 2> UnrollMD;
    UnrollMD.push_back(MDString::get(Context, "llvm.loop.unroll.count"));
    UnrollMD.push_back(ConstantAsMetadata::get(ConstantInt::get(Type::getInt32Ty(Context), unroll)));
    SmallVector<Metadata*, 2> MDs;
    MDs.push_back(nullptr);
    MDs.push_back(MDNode::get(Context, UnrollMD));

    MDNode* NewUnrollMD = MDNode::get(Context, MDs);
    // Set operand 0 to refer to the loop id itself.
    NewUnrollMD->replaceOperandWith(0, NewUnrollMD);

    return NewUnrollMD;
}

void ClampLoopUnroll::visitTerminatorInst(IGCLLVM::TerminatorInst& I)
{
    MDNode* MD = I.getMetadata(LoopMDName);

    if (!MD)
        return;

    if (!GetUnrollMetadata(MD, "llvm.loop.unroll.enable") &&
        !GetUnrollMetadata(MD, "llvm.loop.unroll.full"))
        return;

    MDNode* NewMD = GetUnrollCountMD(I, m_MaxUnrollFactor);

    I.setMetadata(LoopMDName, NewMD);

    m_Changed = true;
}

bool ClampLoopUnroll::runOnFunction(Function& F)
{
#if 0
    visit(F);
#endif

    return m_Changed;
}

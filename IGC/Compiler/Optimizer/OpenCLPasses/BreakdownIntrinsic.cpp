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

#include "Compiler/Optimizer/OpenCLPasses/BreakdownIntrinsic.h"
#include "Compiler/IGCPassSupport.h"
#include "Compiler/CodeGenPublic.h"
#include "Compiler/MetaDataApi/MetaDataApi.h"

#include "common/LLVMWarningsPush.hpp"
#include <llvm/IR/Module.h>
#include <llvm/IR/Function.h>
#include "common/LLVMWarningsPop.hpp"
#include "GenISAIntrinsics/GenIntrinsics.h"
#include "Compiler/MetaDataApi/IGCMetaDataHelper.h"

using namespace llvm;
using namespace IGC;
using namespace IGC::IGCMD;

// Register pass to igc-opt
#define PASS_FLAG "breakdown-intrinsics"
#define PASS_DESCRIPTION "Breakdown intrinsics into simpler operations to enable better optimization"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS true
IGC_INITIALIZE_PASS_BEGIN(BreakdownIntrinsicPass, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_END(BreakdownIntrinsicPass, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)

char BreakdownIntrinsicPass::ID = 0;

BreakdownIntrinsicPass::BreakdownIntrinsicPass()
    : FunctionPass(ID)
    , m_changed(false)
    , m_pMdUtils(nullptr)
    , modMD(nullptr)
{
    initializeBreakdownIntrinsicPassPass(*PassRegistry::getPassRegistry());
}

void BreakdownIntrinsicPass::visitIntrinsicInst(llvm::IntrinsicInst& I)
{
    //const MetaDataUtils &mdUtils = *(getAnalysis<MetaDataUtilsWrapper>().getMetaDataUtils());
    ModuleMetaData& modMD = *(getAnalysis<MetaDataUtilsWrapper>().getModuleMetaData());
    llvm::IRBuilder<> builder(&I);
    bool md_added = false;

    auto pCtx = getAnalysis<CodeGenContextWrapper>().getCodeGenContext();

    if (I.getIntrinsicID() == llvm::Intrinsic::fmuladd ||
        // For FMA only break it up if unsafe math optimizations are set
        (I.getIntrinsicID() == llvm::Intrinsic::fma && modMD.compOpt.UnsafeMathOptimizations))
    {
        llvm::Value* pMulInst = builder.CreateFMul(I.getOperand(0), I.getOperand(1));
        llvm::Value* pAddInst = builder.CreateFAdd(pMulInst, I.getOperand(2));
        I.replaceAllUsesWith(pAddInst);
        I.eraseFromParent();
        m_changed = true;

        // The presence of fmuladd indicates that the fp_contract needs to be set.
        if (pCtx->m_DriverInfo.NeedsBreakdownMulAdd() && !md_added)
        {
            modMD.compOpt.MadEnable = true;
            md_added = true;
        }
    }
}

bool BreakdownIntrinsicPass::runOnFunction(llvm::Function& F)
{
    m_pMdUtils = getAnalysis<MetaDataUtilsWrapper>().getMetaDataUtils();
    visit(F);
    return m_changed;
}

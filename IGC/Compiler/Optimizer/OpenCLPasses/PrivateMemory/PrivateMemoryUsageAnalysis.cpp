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

#include "Compiler/Optimizer/OpenCLPasses/PrivateMemory/PrivateMemoryUsageAnalysis.hpp"
#include "AdaptorCommon/ImplicitArgs.hpp"
#include "Compiler/IGCPassSupport.h"
#include "Probe/Assertion.h"

using namespace llvm;
using namespace IGC;

// Register pass to igc-opt
#define PASS_FLAG "igc-private-mem-usage-analysis"
#define PASS_DESCRIPTION "Analyzes the presence of private memory allocation"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS_BEGIN(PrivateMemoryUsageAnalysis, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_DEPENDENCY(MetaDataUtilsWrapper)
IGC_INITIALIZE_PASS_DEPENDENCY(CodeGenContextWrapper)
IGC_INITIALIZE_PASS_END(PrivateMemoryUsageAnalysis, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)

char PrivateMemoryUsageAnalysis::ID = 0;

PrivateMemoryUsageAnalysis::PrivateMemoryUsageAnalysis()
    : ModulePass(ID), m_hasPrivateMem(false)
{
    initializePrivateMemoryUsageAnalysisPass(*PassRegistry::getPassRegistry());

}

bool PrivateMemoryUsageAnalysis::runOnModule(Module& M)
{
    bool changed = false;
    IGCMD::MetaDataUtils* pMdUtils = getAnalysis<MetaDataUtilsWrapper>().getMetaDataUtils();

    // Run on all functions defined in this module
    for (Module::iterator I = M.begin(), E = M.end(); I != E; ++I)
    {
        Function* pFunc = &(*I);
        if (pFunc->isDeclaration())
            continue;
        if (pMdUtils->findFunctionsInfoItem(pFunc) == pMdUtils->end_FunctionsInfo())
            continue;
        if (pFunc->hasFnAttribute("IndirectlyCalled"))
            continue;
        if (runOnFunction(*pFunc))
        {
            changed = true;
        }
    }

    return changed;
}

bool PrivateMemoryUsageAnalysis::runOnFunction(Function& F)
{
    // Processing new function
    m_hasPrivateMem = false;

    visit(F);

    CodeGenContext* pCtx = getAnalysis<CodeGenContextWrapper>().getCodeGenContext();
    IGCMD::MetaDataUtils* pMdUtils = getAnalysis<MetaDataUtilsWrapper>().getMetaDataUtils();

    // Add private base to kernels if function pointers are present,
    // since stack might need to be allocated
    if (!m_hasPrivateMem)
    {
        if (pCtx->m_enableFunctionPointer && isEntryFunc(pMdUtils, &F))
        {
            m_hasPrivateMem = true;
        }
    }

    // Struct types always use private memory unless regtomem can
    // promote them.  Check the function signature to see if any
    // structs are passesed as arguments.
    if (!m_hasPrivateMem)
    {
        Function::arg_iterator argument = F.arg_begin();
        for (; argument != F.arg_end(); ++argument)
        {
            Argument* arg = &(*argument);

            if (arg->getType()->isPointerTy())
            {
                Type* type = arg->getType()->getPointerElementType();

                if (StructType * structType = dyn_cast<StructType>(type))
                {
                    if (!structType->isOpaque())
                    {
                        m_hasPrivateMem = true;
                    }
                }
            }
        }
    }
    //Add private memory implicit arg
    SmallVector<ImplicitArg::ArgType, ImplicitArg::NUM_IMPLICIT_ARGS> implicitArgs;
    implicitArgs.push_back(ImplicitArg::R0);
    if (F.hasFnAttribute("visaStackCall"))
    {
        m_hasPrivateMem = true;
    }

    // For double emulation, need to add private base (conservative).
    if (!m_hasPrivateMem)
    {
        // This is the condition that double emulation is used.
        if ((IGC_IS_FLAG_ENABLED(ForceDPEmulation) ||
            (pCtx->m_DriverInfo.NeedFP64(pCtx->platform.getPlatformInfo().eProductFamily) && pCtx->platform.hasNoFP64Inst())))
        {
            m_hasPrivateMem = true;
        }
    }

    if (m_hasPrivateMem)
    {
        implicitArgs.push_back(ImplicitArg::PRIVATE_BASE);
    }
    ImplicitArgs::addImplicitArgs(F, implicitArgs, getAnalysis<MetaDataUtilsWrapper>().getMetaDataUtils());

    return true;
}

void PrivateMemoryUsageAnalysis::visitCallInst(llvm::CallInst& CI)
{
    Function* calledFunc = CI.getCalledFunction();
    if (!calledFunc || calledFunc->hasFnAttribute("visaStackCall"))
    {
        // If a called function is indirect or uses stack call, we need private memory for the parent
        m_hasPrivateMem = true;
    }
}

void PrivateMemoryUsageAnalysis::visitAllocaInst(llvm::AllocaInst& AI)
{
    IGC_ASSERT_MESSAGE(AI.getType()->getAddressSpace() == ADDRESS_SPACE_PRIVATE, "Allocaitons are expected to be in private address space");

    // If we encountered Alloca, then the function uses private memory
    m_hasPrivateMem = true;
}

void PrivateMemoryUsageAnalysis::visitBinaryOperator(llvm::BinaryOperator& I)
{
}

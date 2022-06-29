/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "Compiler/Optimizer/OpenCLPasses/PrivateMemory/PrivateMemoryUsageAnalysis.hpp"
#include "AdaptorCommon/ImplicitArgs.hpp"
#include "AdaptorCommon/AddImplicitArgs.hpp"
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
    m_pMDUtils = getAnalysis<MetaDataUtilsWrapper>().getMetaDataUtils();
    CodeGenContext* pCtx = getAnalysis<CodeGenContextWrapper>().getCodeGenContext();

    bool hasStackCall = false;

    m_hasDPDivSqrtEmu = !pCtx->platform.hasNoFP64Inst() && !pCtx->platform.hasCorrectlyRoundedMacros() && pCtx->m_DriverInfo.NeedFP64DivSqrt();

    // Run on all functions defined in this module
    for (Module::iterator I = M.begin(), E = M.end(); I != E; ++I)
    {
        Function* pFunc = &(*I);
        // Skip functions called from function marked with stackcall attribute
        if (AddImplicitArgs::hasStackCallInCG(pFunc, *pCtx))
        {
            hasStackCall = true;
            continue;
        }
        if (pFunc->isDeclaration())
            continue;
        if (m_pMDUtils->findFunctionsInfoItem(pFunc) == m_pMDUtils->end_FunctionsInfo())
            continue;
        if (runOnFunction(*pFunc))
        {
            changed = true;
        }
    }

    // If there are stack called functions in the module, add PRIVATE_BASE to all kernels to be safe.
    // PRIVATE_BASE is needed for kernel to get the stack base offset.
    // Callee does not require this arg, since all stack access will be done using the stack-pointer
    if (hasStackCall || pCtx->m_enableFunctionPointer)
    {
        for (Module::iterator I = M.begin(), E = M.end(); I != E; ++I)
        {
            Function* pFunc = &(*I);
            if (isEntryFunc(m_pMDUtils, pFunc))
            {
                SmallVector<ImplicitArg::ArgType, 1> implicitArgs;
                implicitArgs.push_back(ImplicitArg::PRIVATE_BASE);
                ImplicitArgs::addImplicitArgs(*pFunc, implicitArgs, m_pMDUtils);
                changed = true;
            }
        }
    }

    // Update LLVM metadata based on IGC MetaDataUtils
    if (changed)
        m_pMDUtils->save(M.getContext());

    return changed;
}

bool PrivateMemoryUsageAnalysis::runOnFunction(Function& F)
{
    // Processing new function
    m_hasPrivateMem = false;

    visit(F);

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

    // For double emulation, need to add private base (conservative).
    if (!m_hasPrivateMem)
    {
        CodeGenContext* pCtx = getAnalysis<CodeGenContextWrapper>().getCodeGenContext();
        // This is the condition that double emulation is used.
        if ((IGC_IS_FLAG_ENABLED(ForceDPEmulation) ||
            (pCtx->m_DriverInfo.NeedFP64(pCtx->platform.getPlatformInfo().eProductFamily) && pCtx->platform.hasNoFP64Inst())))
        {
            m_hasPrivateMem = true;
        }
    }

    //Add private memory implicit arg
    SmallVector<ImplicitArg::ArgType, ImplicitArg::NUM_IMPLICIT_ARGS> implicitArgs;
    implicitArgs.push_back(ImplicitArg::R0);

    if (m_hasPrivateMem)
    {
        implicitArgs.push_back(ImplicitArg::PRIVATE_BASE);
    }
    ImplicitArgs::addImplicitArgs(F, implicitArgs, getAnalysis<MetaDataUtilsWrapper>().getMetaDataUtils());

    return true;
}

void PrivateMemoryUsageAnalysis::visitAllocaInst(llvm::AllocaInst& AI)
{
    IGC_ASSERT_MESSAGE(AI.getType()->getAddressSpace() == ADDRESS_SPACE_PRIVATE, "Allocaitons are expected to be in private address space");

    // If we encountered Alloca, then the function uses private memory
    m_hasPrivateMem = true;
}

void PrivateMemoryUsageAnalysis::visitBinaryOperator(llvm::BinaryOperator& I)
{
    // If we encountered Alloca, then the function uses private memory
    CodeGenContext* pCtx = getAnalysis<CodeGenContextWrapper>().getCodeGenContext();
    if (pCtx->platform.Enable32BitIntDivRemEmu())
    {
        switch (I.getOpcode())
        {
        case Instruction::UDiv:
        case Instruction::URem:
        case Instruction::SDiv:
        case Instruction::SRem:
            m_hasPrivateMem = true;
            break;
        default:
            break;
        }
    }

    // Check if an instruction is fp64 div to enable privMem
    if (m_hasDPDivSqrtEmu)
    {
        if (I.getOpcode() == Instruction::FDiv)
        {
            m_hasPrivateMem = true;
        }
    }
}

void PrivateMemoryUsageAnalysis::visitCallInst(llvm::CallInst& CI)
{
    // Check if a sqrtd builtin is called to enable privMem
    if (m_hasDPDivSqrtEmu && CI.hasName())
    {
        Function* calledFunc = CI.getCalledFunction();
        if (calledFunc->getName().startswith("__builtin_IB_native_sqrtd"))
        {
            m_hasPrivateMem = true;
        }
    }
}

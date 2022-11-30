/*========================== begin_copyright_notice ============================

Copyright (C) 2022 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "Compiler/Optimizer/OpenCLPasses/ScalarArgAsPointer/ScalarArgAsPointer.hpp"
#include "Compiler/CISACodeGen/OpenCLKernelCodeGen.hpp"
#include "Compiler/IGCPassSupport.h"

#include "common/LLVMWarningsPush.hpp"
#include <llvm/IR/Module.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Instructions.h>
#include "common/LLVMWarningsPop.hpp"

#include "Probe/Assertion.h"

using namespace llvm;
using namespace IGC;
using namespace IGC::IGCMD;

// Register pass to igc-opt
#define PASS_FLAG "igc-scalar-arg-as-pointer-analysis"
#define PASS_DESCRIPTION "Analyzes scalar kernel arguments used for global memory access"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS_BEGIN(ScalarArgAsPointerAnalysis, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_DEPENDENCY(MetaDataUtilsWrapper)
IGC_INITIALIZE_PASS_END(ScalarArgAsPointerAnalysis, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)

char ScalarArgAsPointerAnalysis::ID = 0;

ScalarArgAsPointerAnalysis::ScalarArgAsPointerAnalysis() : ModulePass(ID)
{
    initializeScalarArgAsPointerAnalysisPass(*PassRegistry::getPassRegistry());
}

bool ScalarArgAsPointerAnalysis::runOnModule(Module& M)
{
    MetaDataUtils* MDUtils = getAnalysis<MetaDataUtilsWrapper>().getMetaDataUtils();

    bool changed = false;

    for (Function& F : M)
    {
        if (F.isDeclaration())
            continue;

        if (!isEntryFunc(MDUtils, &F))
            continue;

        changed |= analyzeFunction(F);
    }

    // Update LLVM metadata based on IGC MetadataUtils
    if (changed)
        MDUtils->save(M.getContext());

    return changed;
}

bool ScalarArgAsPointerAnalysis::analyzeFunction(llvm::Function& F)
{
    m_matchingArgs.clear();
    m_visitedInst.clear();

    visit(F);

    if (m_matchingArgs.empty())
        return false;

    FunctionMetaData& funcMD = getAnalysis<MetaDataUtilsWrapper>().getModuleMetaData()->FuncMD[&F];

    for (auto it = m_matchingArgs.begin(); it != m_matchingArgs.end(); ++it)
        funcMD.m_OpenCLArgScalarAsPointers.insert((*it)->getArgNo());

    return true;
}

void ScalarArgAsPointerAnalysis::visitStoreInst(llvm::StoreInst& I)
{
    analyzePointer(I.getPointerOperand());
}

void ScalarArgAsPointerAnalysis::visitLoadInst(llvm::LoadInst& I)
{
    analyzePointer(I.getPointerOperand());
}

void ScalarArgAsPointerAnalysis::analyzePointer(llvm::Value* V)
{
    auto* type = dyn_cast<PointerType>(V->getType());

    IGC_ASSERT_MESSAGE(type, "Value should be a pointer");

    if (type->getAddressSpace() != ADDRESS_SPACE_GLOBAL)
        return;

    // If scalar is going to be used as pointer, it has to be an instruction, like casting.
    auto* inst = dyn_cast<Instruction>(V);
    if (!inst)
        return;

    if (const ArgSet* args = searchForArgs(inst))
        m_matchingArgs.insert(args->begin(), args->end());
}

const ScalarArgAsPointerAnalysis::ArgSet* ScalarArgAsPointerAnalysis::searchForArgs(llvm::Instruction* inst)
{
    // Skip already visited instruction
    if (m_visitedInst.count(inst))
        return &(*m_visitedInst[inst]);

    // Mark as visited
    m_visitedInst.try_emplace(inst, nullptr);

    // Check for indirect access. Assume intrinsic are safe simple arithmetics.
    if (isa<LoadInst>(inst) || (isa<CallInst>(inst) && !isa<GenIntrinsicInst>(inst)))
    {
        // (1) Found indirect access, fail search
        return nullptr;
    }

    auto result = std::make_unique<ScalarArgAsPointerAnalysis::ArgSet>();

    for (unsigned int i = 0; i < inst->getNumOperands(); ++i)
    {
        Value* op = inst->getOperand(i);

        if (Argument* arg = dyn_cast<Argument>(op))
        {
            // Consider only integer arguments
            if (arg->getType()->isIntegerTy())
            {
                result->insert(arg);
            }
            else
            {
                // (2) Found non-compatible argument, fail
                return nullptr;
            }
        }
        else if (Instruction* opInst = dyn_cast<Instruction>(op))
        {
            auto* args = searchForArgs(opInst);

            if (!args)
                return nullptr; // propagate fail

            result->insert(args->begin(), args->end());
        }
    }

    m_visitedInst[inst] = std::move(result);
    return &(*m_visitedInst[inst]);
}

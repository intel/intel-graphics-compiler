/*========================== begin_copyright_notice ============================

Copyright (C) 2022 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "Compiler/CISACodeGen/CastToGASAnalysis.h"
#include "Compiler/CodeGenPublicEnums.h"
#include "Compiler/IGCPassSupport.h"
#include "Probe/Assertion.h"
#include "llvmWrapper/Analysis/CallGraph.h"

#include "common/LLVMWarningsPush.hpp"
#include <llvm/Pass.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/InstIterator.h>
#include <llvm/Analysis/CallGraph.h>
#include "common/LLVMWarningsPop.hpp"

using namespace llvm;
using namespace IGC;

char CastToGASWrapperPass::ID = 0;

#define PASS_FLAG "generic-pointer-analysis"
#define PASS_DESCRIPTION "Analyze generic pointer usage"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS true
IGC_INITIALIZE_PASS_BEGIN(CastToGASWrapperPass, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
INITIALIZE_PASS_DEPENDENCY(CallGraphWrapperPass)
IGC_INITIALIZE_PASS_END(CastToGASWrapperPass, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)

void CastToGASWrapperPass::getAllFuncsAccessibleFromKernel(const Function* F, CallGraph& CG, SmallPtrSetImpl<const Function*>& funcs, bool& disruptAnalysis) const
{
    IGC_ASSERT(F->getCallingConv() == CallingConv::SPIR_KERNEL);

    disruptAnalysis = false;

    SmallVector<const Function*, 16> worklist;
    worklist.push_back(F);

    while (!worklist.empty())
    {
        const Function* F = worklist.back();
        worklist.pop_back();

        CallGraphNode& N = *CG[F];
        for (IGCLLVM::CallRecord CE : N)
        {
            const Function* child = CE.second->getFunction();
            if (!child)
            {
                if (CallBase* CB = dyn_cast_or_null<CallBase>(CE.first.getValue()))
                {
                    if (CB->isIndirectCall())
                    {
                        // Continue gathering all functions accessible from kernel "F" even if disruption
                        // happens to find out what functions need additional control flow for GAS accesses
                        disruptAnalysis = true;
                    }
                }
                continue;
            }

            if (child->isDeclaration()) continue;

            const bool notVisited = funcs.insert(child).second;
            if (notVisited)
            {
                worklist.push_back(child);
            }
        }
    }
}

unsigned CastToGASWrapperPass::hasCastsToGeneric(const Function* F)
{
    if (castInfoCache.count(F))
        return castInfoCache[F];

    unsigned castInfo = 0;
    for (auto& I : instructions(F))
    {
        if (castInfo == (HasPrivateToGenericCast | HasLocalToGenericCast)) break;

        if (auto* ASC = dyn_cast<AddrSpaceCastInst>(&I))
        {
            if (ASC->getDestAddressSpace() != ADDRESS_SPACE_GENERIC)
                continue;

            unsigned AS = ASC->getSrcAddressSpace();
            if (AS == ADDRESS_SPACE_PRIVATE)
                castInfo |= HasPrivateToGenericCast;
            else if (AS == ADDRESS_SPACE_LOCAL)
                castInfo |= HasLocalToGenericCast;
        }
    }

    castInfoCache[F] = castInfo;

    return castInfo;
}

void CastToGASWrapperPass::setInfoForGroup(
    llvm::SmallPtrSetImpl<const llvm::Function*>& functionsGroup,
    unsigned castInfo)
{
    for (auto F : functionsGroup)
    {
        auto E = GI.FunctionMap.find(F);
        if (E != GI.FunctionMap.end())
        {
            // If a function already exists in the map, it means that it is called by more than one
            // kernel. Existing element represents a result of an analysis processed on another
            // kernel call graph. Below code makes an OR operation on previous and current analysis
            // results to take both into account.
            GI.FunctionMap[F] = E->second | castInfo;
            continue;
        }

        GI.FunctionMap[F] = castInfo;
    }
}

bool CastToGASWrapperPass::runOnModule(Module& M)
{
    castInfoCache.clear();
    CallGraph& CG = getAnalysis<CallGraphWrapperPass>().getCallGraph();
    for (auto& F : M.functions()) {
        if (F.getCallingConv() != llvm::CallingConv::SPIR_KERNEL)
            continue;

        bool disruptAnalysis = false;
        SmallPtrSet<const Function*, 32> functions;
        getAllFuncsAccessibleFromKernel(&F, CG, functions, disruptAnalysis);
        functions.insert(&F);

        if (disruptAnalysis)
        {
            setInfoForGroup(functions, HasPrivateToGenericCast | HasLocalToGenericCast);
            continue;
        }

        unsigned info = 0;
        for (auto F : functions)
        {
            info |= hasCastsToGeneric(F);

            // early exit if private and local casts have been found
            if (info == (HasPrivateToGenericCast | HasLocalToGenericCast))
                break;
        }

        setInfoForGroup(functions, info);
    }

    return false;
}

/*========================== begin_copyright_notice ============================

Copyright (C) 2019-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

//===----------------------------------------------------------------------===//
///
/// A collection of functions useful for navigating continuations after
/// shader splitting has happened.
///
//===----------------------------------------------------------------------===//

#include "ContinuationUtils.h"
#include "common/LLVMWarningsPush.hpp"
#include <llvm/IR/InstIterator.h>
#include <llvm/IR/Function.h>
#include <llvm/Support/GraphWriter.h>
#include "common/LLVMWarningsPop.hpp"

using namespace IGC;
using namespace llvm;

namespace IGC {

std::vector<Function*> getRootFunctions(CodeGenContext* Ctx, Module& M)
{
    ModuleMetaData* modMD = Ctx->getModuleMetaData();
    auto &FuncMD = modMD->FuncMD;

    // The 'RootFunctions' are the original shaders, not including their
    // continuations.
    std::vector<Function*> RootFunctions;
    for (auto &F : M)
    {
        if (F.isDeclaration())
            continue;

        auto Entry = FuncMD.find(&F);
        if (Entry == FuncMD.end())
            continue;

        auto& MD = Entry->second;
        bool IsRoot =
            MD.functionType == FunctionTypeMD::KernelFunction ||
            (MD.functionType == FunctionTypeMD::CallableShader &&
             !IGC::isContinuation(MD));
        if (IsRoot)
        {
            RootFunctions.push_back(&F);
        }
    }

    return RootFunctions;
}

// Starting at a given function 'F', recursively walk all continuation calls
// to explore the "call graph" from this function to build the collection of
// continuations reachable from this point.
void getFuncGroup(const Function* F, ContMap &Map)
{
    for (auto& I : instructions(*F))
    {
        if (auto* CI = dyn_cast<ContinuationHLIntrinsic>(&I))
        {
            uint32_t ID = CI->getContinuationID();
            Function* ContFn = CI->getContinuationFn();

            if (Map.insert(std::make_pair(ID, ContFn)).second)
            {
                getFuncGroup(ContFn, Map);
            }
        }
    }
}

ContMap getFuncGroup(const Function* F)
{
    ContMap Map;
    getFuncGroup(F, Map);
    return Map;
}

void ContinuationInfo::calculate(Function& RootFunc, const ContMap& Group)
{
    NumContinuations = Group.size();
    FillBlockMap.clear();
    FillIntrinsicMap.clear();
    FillBlockMap.reserve(Group.size());
    DeadFills.clear();

    SmallVector<Function*, 4> Funcs{ &RootFunc };
    for (auto& Pair : Group)
        Funcs.push_back(Pair.second);

    for (auto* F : Funcs)
    {
        for (auto& BB : *F)
        {
            DenseMap<uint64_t, SpillValueIntrinsic*> Spills;
            for (auto& I : BB)
            {
                if (auto* SI = dyn_cast<SpillValueIntrinsic>(&I))
                {
                    Spills[SI->getOffset()] = SI;
                }
                else if (auto *CI = dyn_cast<ContinuationHLIntrinsic>(&I))
                {
                    SuspendPoints[I.getFunction()].push_back(CI);
                    auto *ContFn = Group.find(CI->getContinuationID())->second;
                    auto* FillBlock = &ContFn->getEntryBlock();
                    FillBlockMap[FillBlock].push_back(&BB);
                    for (auto& I : *FillBlock)
                    {
                        if (auto* FI = dyn_cast<FillValueIntrinsic>(&I))
                        {
                            auto* SpillInst =
                                Spills.find(FI->getOffset())->second;
                            FillIntrinsicMap[FI].push_back(SpillInst);
                        }
                    }
                    break;
                }
            }
        }
    }
}

void ContinuationInfo::markDead(FillValueIntrinsic* FI)
{
    DeadFills.insert(FI);
}

void ContinuationInfo::bulkUpdate()
{
    FillIntrinsicMap.remove_if([&](auto &P) {
        return DeadFills.count(P.first) != 0;
    });
    DeadFills.clear();
}

void ContinuationInfo::findSpillRoots(
    Value* V,
    VisitedSet& Visited,
    RootSet& Roots) const
{
    if (!Visited.insert(V).second)
        return;

    auto* FI = dyn_cast<FillValueIntrinsic>(V);
    if (!FI)
    {
        Roots.insert(V);
        return;
    }

    auto& Spills = FillIntrinsicMap.find(FI)->second;
    for (auto* SI : Spills)
    {
        findSpillRoots(SI->getData(), Visited, Roots);
    }
}

ContinuationInfo::RootSet ContinuationInfo::findSpillRoots(Value* V) const
{
    VisitedSet Visited;
    RootSet Roots;
    findSpillRoots(V, Visited, Roots);
    return Roots;
}

Value* ContinuationInfo::findUniqueSpillRoot(Value* V) const
{
    auto Roots = findSpillRoots(V);
    if (Roots.size() == 1)
        return *Roots.begin();

    return nullptr;
}

void ContinuationGraph::buildGraph(
    llvm::Function* F,
    ContinuationGraph::MapTy& Map)
{
    if (Map.count(F) != 0)
        return;

    Mapping[F] = std::make_unique<ContinuationGraphNode>(F, this);
    Map[F];

    for (auto& I : instructions(*F))
    {
        if (auto* CI = dyn_cast<ContinuationHLIntrinsic>(&I))
        {
            Function* ContFn = CI->getContinuationFn();

            buildGraph(ContFn, Map);

            Map[F].push_back(Mapping[ContFn].get());
        }
    }
}

ContinuationGraph::ContinuationGraph(llvm::Function *F)
{
    Mapping[F] = std::make_unique<ContinuationGraphNode>(F, this);
    Root = Mapping[F].get();
    buildGraph(F, AdjacencyMap);

    for (auto& P : AdjacencyMap)
        Nodes.push_back(Mapping[P.first].get());
}

void viewContinuationGraph(llvm::Function* F)
{
    ContinuationGraph CG(F);

    ViewGraph(CG, "continuation-graph");
}

} // namespace IGC

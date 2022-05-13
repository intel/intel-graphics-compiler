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

#pragma once

#include "ShaderProperties.h"
#include "RTStackFormat.h"

#include <vector>
#include "Compiler/CodeGenPublic.h"
#include "common/LLVMWarningsPush.hpp"
#include "llvm/IR/Module.h"
#include "llvm/IR/Function.h"
#include "llvm/ADT/iterator_range.h"
#include <llvm/ADT/MapVector.h>
#include <llvm/ADT/DenseMap.h>
#include <llvm/ADT/SmallVector.h>
#include <llvm/ADT/MapVector.h>
#include <llvm/ADT/GraphTraits.h>
#include <llvm/ADT/iterator.h>
#include <llvm/Support/DOTGraphTraits.h>
#include "common/LLVMWarningsPop.hpp"

namespace IGC {

using ContMap = llvm::MapVector<uint32_t, llvm::Function*>;

// After the SplitAsyncPass, we have a collection of "root shaders" (i.e., the
// portion of the original shader prior to any TraceRay() splits) and their
// associated continuations.  This function will return all of the roots.
std::vector<llvm::Function*> getRootFunctions(
    CodeGenContext* Ctx,
    llvm::Module& M);

// Given a root 'F', build a map from the unique 32-bit ID of the continuation
// to the continuation itself just for that root.  This is useful for operating
// on a root and all of its continuations in a group.
void getFuncGroup(const llvm::Function* F, ContMap& Map);
ContMap getFuncGroup(const llvm::Function* F);

// This is currently utilized by the LateRematPass to build up associations
// between spills and fills to perform optimizations on them.
class ContinuationInfo
{
public:
    ContinuationInfo(CallableShaderTypeMD ShaderTy) : ShaderTy(ShaderTy) {}

    using RootSet = llvm::SmallPtrSet<llvm::Value*, 4>;
    using SpillColl = llvm::SmallVector<llvm::SpillValueIntrinsic*, 8>;
    using SpillFillMap = llvm::MapVector<
        llvm::FillValueIntrinsic*,
        SpillColl>;
    using FillBlockMapTy = llvm::MapVector<
        llvm::BasicBlock*,
        llvm::SmallVector<llvm::BasicBlock*, 4>>;
    // Given a root function, sets up internal structure to be queried.
    // If the IR is modified, you should call this again to recalculate.
    void calculate(llvm::Function& RootFunc, const ContMap& Group);
    // Traces through FillValues to find the actual value at that spill location.
    // If more than one is found, return nullptr.
    llvm::Value* findUniqueSpillRoot(llvm::Value* V) const;

    llvm::iterator_range<SpillFillMap::iterator> spillfills() {
        return make_range(FillIntrinsicMap.begin(), FillIntrinsicMap.end());
    }

    llvm::iterator_range<FillBlockMapTy::iterator> spillfillblocks() {
        return make_range(FillBlockMap.begin(), FillBlockMap.end());
    }

    bool canPromoteContinuations() const
    {
        // This constraint that all BTD'd continuations must be able to fit
        // into the shader identifier can be relaxed if needed. You would need
        // to examine the continuation call graph and ensure that all
        // continuations on a path to a promotion candidate are themselves
        // promoted otherwise there is no guarantee they will have a local
        // pointer available from which they can compute the BTD address.
        using namespace RTStackFormat;
        return ShaderProperties::canPromoteContinuations(ShaderTy) &&
               NumContinuations <= ShaderIdentifier::NumRaygenOpenSlots;
    }

    // Marks fills to be removed but does not actually delete them (from this
    // data structure, not the IR).
    void markDead(llvm::FillValueIntrinsic* FI);
    // Actually delete the references from this data structure.
    void bulkUpdate();

    llvm::DenseMap<llvm::Function*,
        llvm::SmallVector<llvm::ContinuationHLIntrinsic*, 4>> SuspendPoints;

    SpillColl& getSpills(llvm::FillValueIntrinsic* FI)
    {
        return FillIntrinsicMap.find(FI)->second;
    }
private:
    // Map each continuation fill block (i.e., the entry block that has fill
    // intrinsics in it) to the collection of blocks that spill to it.
    FillBlockMapTy FillBlockMap;

    // Maps each fill to the collection of spills to the same address.
    SpillFillMap FillIntrinsicMap;
    // This is used to track fills to be removed from `FillIntrinsicMap`.
    // We want to be able to use `remove_if()` on `FillIntrinsicMap` due to
    // it being a MapVector and `erase()` being expensive.
    llvm::DenseSet<llvm::FillValueIntrinsic*> DeadFills;

    // Traces through FillValues to find the actual value at that spill location.
    RootSet findSpillRoots(llvm::Value* V) const;

    // Just a temporary for recording visited values.
    using VisitedSet = llvm::DenseSet<llvm::Value*>;
    void findSpillRoots(
        llvm::Value* V,
        VisitedSet &Visited,
        RootSet &Roots) const;

    uint32_t NumContinuations = 0;
    CallableShaderTypeMD ShaderTy;
};

struct ContinuationGraph;

struct ContinuationGraphNode
{
    llvm::Function* F = nullptr;
    ContinuationGraph* G = nullptr;
    ContinuationGraphNode(llvm::Function* F, ContinuationGraph* G) :
        F(F), G(G) {}
    ContinuationGraphNode() {}
};

// A simple class that walks through TraceRay() and CallShader() calls to
// build up a call graph of continuations.
struct ContinuationGraph
{
    using NodeTy = ContinuationGraphNode*;
    using ChildrenTy = llvm::SmallVector<NodeTy, 4>;
    using MapTy = llvm::DenseMap<llvm::Function*, ChildrenTy>;

    ContinuationGraph(llvm::Function *F);

    void buildGraph(llvm::Function* F, MapTy& Map);

    NodeTy Root;

    MapTy AdjacencyMap;
    ChildrenTy Nodes;
    llvm::DenseMap<
        llvm::Function*, std::unique_ptr<ContinuationGraphNode>> Mapping;
};

} // namespace IGC

namespace llvm {

// Implement GraphTraits so we get graph algorithms on it for free.  We really
// only have an interest in printing the graph out for debugging right now.
template <> struct GraphTraits<IGC::ContinuationGraph>
{
    using NodeRef = const IGC::ContinuationGraph::NodeTy;
    using ChildIteratorType =
        IGC::ContinuationGraph::ChildrenTy::const_iterator;

    static NodeRef getEntryNode(const IGC::ContinuationGraph& G) { return G.Root; }

    static ChildIteratorType child_begin(NodeRef N)
    {
        return ChildIteratorType(N->G->AdjacencyMap.find(N->F)->second.begin());
    }
    static ChildIteratorType child_end(NodeRef N)
    {
        return ChildIteratorType(N->G->AdjacencyMap.find(N->F)->second.end());
    }

    using nodes_iterator =
        IGC::ContinuationGraph::ChildrenTy::const_iterator;

    static nodes_iterator nodes_begin(const IGC::ContinuationGraph& G)
    {
        return nodes_iterator(G.Nodes.begin());
    }

    static nodes_iterator nodes_end(const IGC::ContinuationGraph& G)
    {
        return nodes_iterator(G.Nodes.end());
    }

    static size_t size(const IGC::ContinuationGraph* G) { return G->Nodes.size(); }
};

// Customize printing the dot file.
template <> struct DOTGraphTraits<IGC::ContinuationGraph> : public DefaultDOTGraphTraits
{
    DOTGraphTraits(bool isSimple = false) : DefaultDOTGraphTraits(isSimple) {}

    std::string getNodeLabel(const IGC::ContinuationGraphNode* N, const IGC::ContinuationGraph& G)
    {
        return N->F->getName().str();
    }
};

} // namespace llvm

namespace IGC {

// Pass in a root function to visualize the follow between the root and
// its continuations.
//
// This can be invoked via IGC::viewContinuationGraph(F) in the debugger
// just as you would do F->dump().
void viewContinuationGraph(llvm::Function* F);

} // namespace IGC

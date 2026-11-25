/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

// SoA helpers shared by PrivateMemoryResolution, SplitPHIsOfAllocaPointers and
// SplitSelectsOfAllocaPointers.

#include "common/LLVMWarningsPush.hpp"
#include <llvm/ADT/ArrayRef.h>
#include <llvm/ADT/DenseMap.h>
#include <llvm/ADT/DenseSet.h>
#include <llvm/ADT/STLFunctionalExtras.h>
#include <llvm/ADT/SmallPtrSet.h>
#include <llvm/ADT/SmallVector.h>
#include "common/LLVMWarningsPop.hpp"

#include <optional>

namespace llvm {
class AllocaInst;
class Argument;
class CallGraph;
class DataLayout;
class Function;
class GetElementPtrInst;
class Instruction;
class IRBuilderBase;
class Module;
class Value;
} // namespace llvm

namespace IGC {
class GenXFunctionGroupAnalysis;
namespace IGCMD {
class MetaDataUtils;
}

namespace SoAAcrossFNUtil {
// A load that reads through a pointer merge (phi/select). Chain is the address chain between the merge and the load,
// in the form replayAddressChain() expects.
struct LoadVia {
  llvm::Instruction *Load;
  llvm::SmallVector<llvm::Instruction *, 4> Chain; // merge → Chain[0] → ... → Load (casts and const-GEPs)
};

void collectSoAUnsupportedFns(llvm::Module &M, llvm::CallGraph &CG, GenXFunctionGroupAnalysis *FGA,
                              IGCMD::MetaDataUtils *MdUtils, llvm::SmallPtrSetImpl<llvm::Function *> &UnsupportedFns);

std::optional<llvm::SmallVector<llvm::Argument *, 8>>
traceUsesToArgs(llvm::Value *Root, const llvm::SmallPtrSetImpl<llvm::Function *> &UnsupportedFns,
                llvm::DenseMap<llvm::Argument *, std::optional<llvm::SmallVector<llvm::Argument *, 8>>> &ArgToArgCache,
                bool AllowMerges = false);

// Returns true if splitting a pointer merge rooted at AI is worthwhile, i.e. AI is a
// large scalar array alloca that stands to benefit from SoA promotion.
bool isWorthwhileForAlloca(llvm::AllocaInst *AI, const llvm::DataLayout &DL);

// Collect the callee arguments that entry-block allocas of SoA-supported functions flow into
// (EnableSOAAcrossCalls). Such arguments are treated as alloca roots of a pointer merge.
// VerifyAlloca restricts the allocas considered to those passing isWorthwhileForAlloca.
void collectRootArgs(llvm::Module &M, GenXFunctionGroupAnalysis *FGA, IGCMD::MetaDataUtils *MdUtils, bool VerifyAlloca,
                     llvm::DenseSet<const llvm::Value *> &RootArgs);

// Walk the pointer V back through the GEPs, bitcasts, addrspacecasts and single-incoming PHIs that the SoA layout
// checker and transpose walk through, and return the value it is derived from (an alloca, a function argument, or
// whatever else stops the walk). OnGEP, if given, is called on each GEP on the way, and returning false from it stops
// the walk with nullptr.
llvm::Value *stripToSoARoot(llvm::Value *V, llvm::function_ref<bool(const llvm::GetElementPtrInst &)> OnGEP = nullptr);

// If V is rooted at an alloca or an ExtraRoots argument, returns V with its leading bitcasts / addrspacecasts stripped
// (GEPs are kept), i.e. the base replayAddressChain() rebuilds from. Otherwise returns nullptr.
llvm::Value *getRootBase(llvm::Value *V, const llvm::DataLayout &DL,
                         const llvm::DenseSet<const llvm::Value *> &ExtraRoots);

// Used when splitting a pointer merge (phi/select) into per-operand accesses.
// Chain is the sequence of bitcasts, addrspacecasts and constant-index GEPs
// between the merge and a load/store: Chain[0] uses the merge, each later
// element uses the previous one, and the last one is the load/store pointer.
// Re-creates that sequence at B's insertion point, starting from Incoming (one
// operand of the merge) instead of the merge, and returns the final address.
//
// If Incoming is rooted at an alloca or an ExtraRoots argument, its outermost
// casts are stripped and every replayed cast is clamped to the stripped base's
// address space. The rebuilt address then stays in that space (e.g. private)
// instead of going private→generic→global, which would make the final access
// read the wrong memory.
llvm::Value *replayAddressChain(llvm::IRBuilderBase &B, llvm::Value *Incoming,
                                llvm::ArrayRef<llvm::Instruction *> Chain, const llvm::DataLayout &DL,
                                const llvm::DenseSet<const llvm::Value *> &ExtraRoots);

// Creates a GenISA_PredicatedLoad at B's insertion point that loads Model's type from Ptr. Model is either:
//  - a LoadInst: the new load is enabled by Pred and yields poison when Pred is false;
//  - a GenISA_PredicatedLoad: the new load is enabled by (Pred && Model's predicate) and yields Model's merge value
//    when that is false.
// The new load takes Model's alignment, debug location, and lsc.cache.ctrl / !nontemporal metadata.
llvm::Instruction *createPredicatedLoad(llvm::IRBuilderBase &B, llvm::Instruction *Model, llvm::Value *Ptr,
                                        llvm::Value *Pred);
} // namespace SoAAcrossFNUtil
} // namespace IGC

/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

// Cross-function SoA helpers shared by PrivateMemoryResolution, SplitPHIsOfAllocaPointers and
// SplitSelectsOfAllocaPointers.

#include "common/LLVMWarningsPush.hpp"
#include <llvm/ADT/DenseMap.h>
#include <llvm/ADT/DenseSet.h>
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
class Module;
class Value;
} // namespace llvm

namespace IGC {
class GenXFunctionGroupAnalysis;
namespace IGCMD {
class MetaDataUtils;
}

namespace SoAAcrossFNUtil {
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
} // namespace SoAAcrossFNUtil
} // namespace IGC

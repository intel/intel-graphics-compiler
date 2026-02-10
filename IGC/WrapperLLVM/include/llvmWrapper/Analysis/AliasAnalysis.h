/*========================== begin_copyright_notice ============================

Copyright (C) 2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef IGCLLVM_ANALYSIS_ALIASANALYSIS_H
#define IGCLLVM_ANALYSIS_ALIASANALYSIS_H

#include "IGC/common/LLVMWarningsPush.hpp"
#include "llvm/Config/llvm-config.h"
#include "llvm/Analysis/AliasAnalysis.h"
#include "IGC/common/LLVMWarningsPop.hpp"
#include "llvmWrapper/TargetParser/Triple.h"
#include "Probe/Assertion.h"

namespace IGCLLVM {
template <typename T>
class AAResultBaseWrapper : public llvm::AAResultBase
#if LLVM_VERSION_MAJOR < 16
                            <T>
#endif // LLVM_VERSION_MAJOR
{
public:
  // On LLVMs <16 this llvm::Instruction parameter is not used, but since
  // LLVM source code operates on 3 parameters on LLVMs below 16 and on 4 patameters on LLVM >=16
  // (Here: llvm\Analysis\AliasAnalysis.h - "return Result.alias(LocA, LocB, AAQI);")
  //
  // Then this approach allows us to:
  // 1) Pass 4th argument without failing compilation on both LLVMs,
  // 2) On LLVM 14 Instruction* is optional - we cannot make it required since LLVM's code expects it to have just 3
  // args,
  //    but it is fine since we will be discarding/ignoring it here anyway,
  // 3) On LLVM 16 Instruction* is required and will fail to build without it (desired behaviour),
  // 3) Is easy to refactor in future - just replace this whole macro with "const llvm::Instruction* CtxI" (update
  // callers and their headers too).
  llvm::AliasResult alias(const llvm::MemoryLocation &LocA, const llvm::MemoryLocation &LocB, llvm::AAQueryInfo &AAQI,
#if LLVM_VERSION_MAJOR < 16
                          const llvm::Instruction *CtxI = nullptr
#else
                          const llvm::Instruction *CtxI
#endif
  ) {
#if LLVM_VERSION_MAJOR >= 16
    return llvm::AAResultBase::alias(LocA, LocB, AAQI, CtxI);
#else
    return llvm::AAResultBase<T>::alias(LocA, LocB, AAQI);
#endif
  }

  bool pointsToConstantMemory(const llvm::MemoryLocation &Loc, llvm::AAQueryInfo &AAQI, bool OrLocal) {
#if LLVM_VERSION_MAJOR < 16
    return llvm::AAResultBase<T>::pointsToConstantMemory(Loc, AAQI, OrLocal);
#else
    return isNoModRef(llvm::AAResultBase::getModRefInfoMask(Loc, AAQI, OrLocal));
#endif
  }
};

using AliasResultEnum = llvm::AliasResult::Kind;

using SimpleAAQueryInfo = llvm::SimpleAAQueryInfo;

#if LLVM_VERSION_MAJOR >= 16
inline SimpleAAQueryInfo createSimpleAAQueryInfo(llvm::AAResults &AAResults) { return SimpleAAQueryInfo(AAResults); }
#else
inline SimpleAAQueryInfo createSimpleAAQueryInfo(llvm::AAResults &AAResults) { return SimpleAAQueryInfo{}; }
#endif
} // namespace IGCLLVM

#endif

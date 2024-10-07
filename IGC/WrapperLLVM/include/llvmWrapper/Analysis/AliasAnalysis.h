/*========================== begin_copyright_notice ============================

Copyright (C) 2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef IGCLLVM_ANALYSIS_ALIASANALYSIS_H
#define IGCLLVM_ANALYSIS_ALIASANALYSIS_H

#include "llvm/Config/llvm-config.h"
#include "llvm/Analysis/AliasAnalysis.h"

namespace IGCLLVM
{
  template <typename T>
  class AAResultBaseWrapper : public llvm::AAResultBase
#if LLVM_VERSION_MAJOR < 16
    <T>
#endif // LLVM_VERSION_MAJOR
    {};

#if LLVM_VERSION_MAJOR < 13
  using AliasResultEnum = llvm::AliasResult;
#else
  using AliasResultEnum = llvm::AliasResult::Kind;
#endif

  using SimpleAAQueryInfo =
#if LLVM_VERSION_MAJOR < 14
      llvm::AAQueryInfo;
#else
      llvm::SimpleAAQueryInfo;
#endif
}

#endif

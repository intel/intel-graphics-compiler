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
#if LLVM_VERSION_MAJOR < 13
  using AliasResultEnum = llvm::AliasResult;
#else
  using AliasResultEnum = llvm::AliasResult::Kind;
#endif
inline llvm::AAQueryInfo makeAAQueryInfo() {
#if LLVM_VERSION_MAJOR >= 14
  return llvm::AAQueryInfo(new llvm::SimpleCaptureInfo());
#else
  return llvm::AAQueryInfo();
#endif
}
}

#endif

/*========================== begin_copyright_notice ============================

Copyright (C) 2019-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef IGCLLVM_ANALYSIS_MEMORYLOCATION_H
#define IGCLLVM_ANALYSIS_MEMORYLOCATION_H

#include "llvm/Config/llvm-config.h"
#include "llvm/Analysis/MemoryLocation.h"

#include "llvm/IR/Instructions.h"

namespace IGCLLVM {

class LocationSize : public llvm::LocationSize {
public:
};
} // namespace IGCLLVM

#endif

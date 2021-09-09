/*========================== begin_copyright_notice ============================

Copyright (C) 2019-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "Compiler/IGCPassSupport.h"
#include "IGC/common/Types.hpp"

#include <llvm/Pass.h>


namespace llvm {class FunctionPass;}
namespace IGC
{
  // replace div and rem with constant divisors with
  // shifts+adds+muls
  llvm::FunctionPass* createIntDivConstantReductionPass();
} // namespace IGC

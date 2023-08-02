/*========================== begin_copyright_notice ============================

Copyright (C) 2023 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/
#pragma once

#include "common/LLVMWarningsPush.hpp"
#include "llvm/IR/Intrinsics.h"
#include "common/LLVMWarningsPop.hpp"

#include <stdint.h>

namespace llvm
{

namespace GenISAIntrinsic {

enum ID : uint32_t
{
    no_intrinsic = llvm::Intrinsic::num_intrinsics,
% for el in intrinsic_definitions:
    ${el.name},
% endfor
    num_genisa_intrinsics
};

} // namespace GenISAIntrinsic

} // namespace llvm

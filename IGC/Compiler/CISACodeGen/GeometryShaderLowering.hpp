/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef GEOMETRYSHADERLOWERING_HPP
#define GEOMETRYSHADERLOWERING_HPP

#include "common/LLVMWarningsPush.hpp"
#include <llvm/Pass.h>
#include "common/LLVMWarningsPop.hpp"

#include <map>

namespace IGC
{

    llvm::FunctionPass* createGeometryShaderLoweringPass();

} // namespace IGC

#endif

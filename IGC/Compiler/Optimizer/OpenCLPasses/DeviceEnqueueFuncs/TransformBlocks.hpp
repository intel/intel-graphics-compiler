/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include <llvm/Pass.h>

extern "C" llvm::ModulePass* createTransformBlocksPass();


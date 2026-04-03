/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

namespace llvm {
class Module;
} // namespace llvm

bool restoreGenISAIntrinsicDeclarations(llvm::Module &M);

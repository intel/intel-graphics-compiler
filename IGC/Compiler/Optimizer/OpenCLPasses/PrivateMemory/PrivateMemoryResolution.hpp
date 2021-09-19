/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

namespace llvm
{
    class ModulePass;
}

namespace IGC
{
    /// Lower down alloca to private memory
    llvm::ModulePass* CreatePrivateMemoryResolution();
}

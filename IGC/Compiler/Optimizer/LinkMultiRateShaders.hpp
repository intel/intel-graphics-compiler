/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

namespace llvm
{
    class Pass;
}

/// This Pass link the sampler and pixel phase of a multi-rate pixel shader
llvm::Pass* CreateLinkMultiRateShaderPass();
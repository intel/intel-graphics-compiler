/*========================== begin_copyright_notice ============================

Copyright (C) 2018-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once


namespace llvm {
    class FunctionPass;
}

namespace IGC
{
    // LSC 2D block address payload field names for updating only
    // (block width/height/numBlock are not updated).
    enum class BlockField {
        BASE=1, WIDTH=2, HEIGHT=3, PITCH=4, BLOCKX=5, BLOCKY=6
    };

    llvm::FunctionPass* createDecompose2DBlockFuncsPass();
} // namespace IGC


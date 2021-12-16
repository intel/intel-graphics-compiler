/*========================== begin_copyright_notice ============================

Copyright (C) 2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

namespace llvm
{
class PassRegistry;
class FunctionPass;
}
namespace IGC
{
void initializeMergeURBReadsPass(llvm::PassRegistry&);
llvm::FunctionPass* createMergeURBReadsPass();
}

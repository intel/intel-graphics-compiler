/*========================== begin_copyright_notice ============================

Copyright (C) 2022-2022 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef __HF_PACKING_OPT_H__
#define __HF_PACKING_OPT_H__

#include <llvm/Pass.h>

namespace IGC {
    FunctionPass* createHFpackingOptPass();
} // End namespace IGC

#endif // __HF_PACKING_OPT_H__

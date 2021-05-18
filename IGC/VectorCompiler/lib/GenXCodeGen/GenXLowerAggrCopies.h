/*========================== begin_copyright_notice ============================

Copyright (C) 2019-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

//
// This file contains the declaration of the VC specific lowering of
// aggregate copies
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_GENX_GENXLOWERAGGRCOPIES_H
#define LLVM_LIB_TARGET_GENX_GENXLOWERAGGRCOPIES_H

namespace llvm {
class FunctionPass;

FunctionPass *createGenXLowerAggrCopiesPass();
}

#endif

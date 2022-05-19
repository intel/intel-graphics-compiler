/*========================== begin_copyright_notice ============================

Copyright (C) 2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef IGCLLVM_IR_DATALAYOUT_H
#define IGCLLVM_IR_DATALAYOUT_H

#include "llvm/IR/DataLayout.h"
#include "llvm/Config/llvm-config.h"

namespace IGCLLVM
{
/* * * * *
 * This section provides compatibility for deprecated
 * unsigned llvm::DataLayout::getPreferredAlignment().
 *
 * In LLVM 10 and earlier llvm::Align was not standarized yet and getPreferredAlignment()
 * was used, which returned unsigned.
 */
    inline unsigned getPreferredAlignValue(llvm::DataLayout* DL, const llvm::GlobalVariable* GV)
    {
#if LLVM_VERSION_MAJOR <= 10
        return DL->getPreferredAlignment(GV);
#else
        return (unsigned)DL->getPreferredAlign(GV).value();
#endif
    }

} // namespace IGCLLVM

#endif // IGCLLVM_IR_DATALAYOUT_H

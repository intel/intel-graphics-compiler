/*===================== begin_copyright_notice ==================================

Copyright (c) 2017 Intel Corporation

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.


======================= end_copyright_notice ==================================*/

#ifndef IGCLLVM_IR_DATALAYOUT_H
#define IGCLLVM_IR_DATALAYOUT_H

#include "llvm/IR/DataLayout.h"
#include "llvm/Config/llvm-config.h"

namespace IGCLLVM {

/* * * * *
 * This section provides compatibility for deprecated
 * unsigned llvm::DataLayout::getPreferredAlignment().
 *
 * In LLVM 10 and earlier llvm::Align was not standarized yet and getPreferredAlignment()
 * was used, which returned unsigned.
 */
#if LLVM_VERSION_MAJOR <= 10
    unsigned getPreferredAlignValue(llvm::DataLayout* DL, const llvm::GlobalVariable* GV) {
        return DL->getPreferredAlignment(GV);
    }
#else
    unsigned getPreferredAlignValue(llvm::DataLayout * DL, const llvm::GlobalVariable * GV) {
        return DL->getPreferredAlign(GV).value();
    }
#endif

} // namespace IGCLLVM

#endif

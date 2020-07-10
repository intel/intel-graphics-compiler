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
#pragma once
#include "common/LLVMWarningsPush.hpp"
#include <llvm/IR/ConstantFolder.h>
#include "common/LLVMWarningsPop.hpp"

namespace IGC
{

class IGCConstantFolder : public llvm::ConstantFolder
{
public:
    IGCConstantFolder();

    llvm::Constant* CreateGradientXFine(llvm::Constant* C0) const;
    llvm::Constant* CreateGradientYFine(llvm::Constant* C0) const;
    llvm::Constant* CreateGradientX(llvm::Constant* C0) const;
    llvm::Constant* CreateGradientY(llvm::Constant* C0) const;
    llvm::Constant* CreateRsq(llvm::Constant* C0) const;
    llvm::Constant* CreateRoundNE(llvm::Constant* C0) const;
    llvm::Constant* CreateFSat(llvm::Constant* C0) const;
    llvm::Constant* CreateFAdd(llvm::Constant* C0, llvm::Constant* C1, llvm::APFloatBase::roundingMode roundingMode) const;
    llvm::Constant* CreateFMul(llvm::Constant* C0, llvm::Constant* C1, llvm::APFloatBase::roundingMode roundingMode) const;
    llvm::Constant* CreateFPTrunc(llvm::Constant* C0, llvm::Type* dstType, llvm::APFloatBase::roundingMode roundingMode) const;
    llvm::Constant* CreateCanonicalize(llvm::Constant* C0, bool flushDenorms = true) const;

private:

    llvm::Constant* CreateGradient(llvm::Constant* C0) const;

};

}

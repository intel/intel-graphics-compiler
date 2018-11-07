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

// vim:ts=2:sw=2:et:
#ifndef __DRIVERINTERFACE_UPGRADER_H__
#define __DRIVERINTERFACE_UPGRADER_H__

#include "common/LLVMWarningsPush.hpp"
#include <llvm/IR/LLVMContext.h>
#include <llvm/Support/ErrorOr.h>
#include <llvm/Support/Error.h>
#include <llvm/Support/MemoryBuffer.h>
#include "common/LLVMWarningsPop.hpp"

namespace upgrader {

llvm::Expected<std::unique_ptr<llvm::Module>> parseBitcodeFile(llvm::MemoryBufferRef Buffer, llvm::LLVMContext &Context);

std::unique_ptr<llvm::MemoryBuffer> upgradeBitcodeFile(llvm::MemoryBufferRef, llvm::LLVMContext &);

llvm::Expected<std::unique_ptr<llvm::Module>> upgradeAndParseBitcodeFile(llvm::MemoryBufferRef, llvm::LLVMContext &);

} // End upgrader namespace

#endif // __DRIVERINTERFACE_UPGRADER_H__

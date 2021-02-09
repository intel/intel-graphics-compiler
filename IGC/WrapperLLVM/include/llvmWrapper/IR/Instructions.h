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

#ifndef IGCLLVM_IR_INSTRUCTIONS_H
#define IGCLLVM_IR_INSTRUCTIONS_H

#include "llvm/Config/llvm-config.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/User.h"
#if LLVM_VERSION_MAJOR <= 7
#include "llvm/Support/Casting.h"
#endif

namespace IGCLLVM
{

    inline llvm::Value* getCalledValue(llvm::CallInst& CI)
    {
#if LLVM_VERSION_MAJOR <= 10
        return CI.getCalledValue();
#else
        return CI.getCalledOperand();
#endif
    }

    inline llvm::Value* getCalledValue(llvm::CallInst* CI)
    {
#if LLVM_VERSION_MAJOR <= 10
        return CI->getCalledValue();
#else
        return CI->getCalledOperand();
#endif
    }

    inline const llvm::Value* getCalledValue(const llvm::CallInst* CI)
    {
#if LLVM_VERSION_MAJOR <= 10
        return CI->getCalledValue();
#else
        return CI->getCalledOperand();
#endif
    }

    inline bool isIndirectCall(const llvm::CallInst& CI)
    {
#if LLVM_VERSION_MAJOR == 7
        const llvm::Value *V = CI.getCalledValue();
        if (llvm::isa<llvm::Function>(V) || llvm::isa<llvm::Constant>(V))
            return false;
        if (CI.isInlineAsm())
            return false;
        return true;
#else
        return CI.isIndirectCall();
#endif
    }

    inline unsigned arg_size(const llvm::CallInst& CI)
    {
#if LLVM_VERSION_MAJOR < 8
        return CI.arg_end() - CI.arg_begin();
#else
        return CI.arg_size();
#endif
    }

    inline llvm::iterator_range<llvm::User::op_iterator> args(llvm::CallInst& CI)
    {
#if LLVM_VERSION_MAJOR < 8
        return CI.arg_operands();
#else
        return CI.args();
#endif
    }


    inline llvm::Constant* getShuffleMaskForBitcode(llvm::ShuffleVectorInst* SVI)
    {
#if LLVM_VERSION_MAJOR < 11
        return SVI->getMask();
#else
        return SVI->getShuffleMaskForBitcode();
#endif
    }
}

#endif

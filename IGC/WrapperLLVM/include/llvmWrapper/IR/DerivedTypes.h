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

#ifndef IGCLLVM_IR_DERIVEDTYPES_H
#define IGCLLVM_IR_DERIVEDTYPES_H

#include "llvm/Config/llvm-config.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Module.h"

namespace IGCLLVM
{

 #if LLVM_VERSION_MAJOR <= 10
     using FixedVectorType = llvm::VectorType;
     const uint32_t VectorTyID = llvm::Type::VectorTyID;
 #else
     using FixedVectorType = llvm::FixedVectorType;
     const uint32_t VectorTyID = llvm::Type::FixedVectorTyID;
 #endif

    inline uint32_t GetVectorTypeBitWidth(llvm::Type* pType)
    {
#if LLVM_VERSION_MAJOR <= 10
        return llvm::cast<llvm::VectorType>(pType)->getBitWidth();
#else
        return (uint32_t)pType->getPrimitiveSizeInBits().getFixedSize();
#endif
    }

    inline bool isScalable(const FixedVectorType &Ty)
    {
#if LLVM_VERSION_MAJOR < 9
        // There were no scalable vectors before LLVM-9
        return false;
#elif LLVM_VERSION_MAJOR < 11
        return Ty.isScalable();
#else
        // Scalable vectors became a separate type since LLVM-11
        return false;
#endif
    }

    inline llvm::StructType *getTypeByName(llvm::Module *M, llvm::StringRef Name) {
#if LLVM_VERSION_MAJOR >= 12
        return llvm::StructType::getTypeByName(M->getContext(), Name);
#else
        return M->getTypeByName(Name);
#endif
    }

}

#endif

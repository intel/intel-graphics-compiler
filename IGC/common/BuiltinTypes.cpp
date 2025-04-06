/*========================== begin_copyright_notice ============================

Copyright (C) 2025 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "common/BuiltinTypes.h"

#include "common/LLVMWarningsPush.hpp"
#include <llvm/ADT/StringRef.h>
#include <llvm/Support/Casting.h>
#include "llvmWrapper/IR/Type.h"
#include "common/LLVMWarningsPop.hpp"

using namespace llvm;

namespace IGC
{

    bool isImageBuiltinType(llvm::Type *builtinTy)
    {
        if (builtinTy->isPointerTy() && !IGCLLVM::isOpaquePointerTy(builtinTy))
            builtinTy = IGCLLVM::getNonOpaquePtrEltTy(builtinTy);

        if (StructType *structTy = dyn_cast<StructType>(builtinTy);
            structTy && structTy->isOpaque())
        {
            StringRef builtinName = structTy->getName();
            llvm::SmallVector<llvm::StringRef, 3> buffer;
            builtinName.split(buffer, ".");
            if (buffer.size() < 2)
                return false;
            bool isOpenCLImage = buffer[0].equals("opencl") && buffer[1].startswith("image") && buffer[1].endswith("_t");
            bool isSPIRVImage = buffer[0].equals("spirv") && (buffer[1].startswith("Image") || buffer[1].startswith("SampledImage"));

            if (isOpenCLImage || isSPIRVImage)
                return true;
        }
#if LLVM_VERSION_MAJOR >= 16
        else if (TargetExtType *targetExtTy = dyn_cast<TargetExtType>(builtinTy);
                 targetExtTy && (targetExtTy->getName() == "spirv.Image" ||
                 targetExtTy->getName() == "spirv.SampledImage"))
        {
            return true;
        }
#endif

        return false;
    }

} // namespace IGC

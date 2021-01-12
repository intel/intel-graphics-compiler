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
#include <llvm/IR/Module.h>
#include <llvm/IR/IRBuilder.h>
#include "common/LLVMWarningsPop.hpp"
#include "Probe/Assertion.h"

namespace IGC
{
    enum RESOURCE_DIMENSION_TYPE
    {
        DIM_TYPED_BUFFER_TYPE = 0,
        DIM_1D_TYPE,
        DIM_1D_ARRAY_TYPE,
        DIM_2D_TYPE,
        DIM_2D_ARRAY_TYPE,
        DIM_3D_TYPE,
        DIM_CUBE_TYPE,
        DIM_CUBE_ARRAY_TYPE,
        NUM_RESOURCE_DIMENSION_TYPES
    };

    const std::string ResourceDimensionTypeName[RESOURCE_DIMENSION_TYPE::NUM_RESOURCE_DIMENSION_TYPES] =
    { "__Buffer_Typed_DIM_Resource",
      "__1D_DIM_Resource", "__1D_ARRAY_DIM_Resource",
      "__2D_DIM_Resource", "__2D_ARRAY_DIM_Resource",
      "__3D_DIM_Resource", "__Cube_DIM_Resource", "__Cube_ARRAY_DIM_Resource" };

    inline void CreateResourceDimensionTypes(llvm::LLVMContext& llvmCtx)
    {
        for (unsigned int resourceDimTypeId = 0; resourceDimTypeId < (unsigned int)RESOURCE_DIMENSION_TYPE::NUM_RESOURCE_DIMENSION_TYPES; resourceDimTypeId++)
        {
            llvm::StructType::create(llvmCtx, ResourceDimensionTypeName[(RESOURCE_DIMENSION_TYPE)resourceDimTypeId]);
        }
    }

    inline llvm::Type* GetResourceDimensionType(llvm::Module& module, RESOURCE_DIMENSION_TYPE resourceDimTypeId)
    {
        IGC_ASSERT((resourceDimTypeId == DIM_TYPED_BUFFER_TYPE ||
            resourceDimTypeId == DIM_1D_TYPE || resourceDimTypeId == DIM_1D_ARRAY_TYPE ||
            resourceDimTypeId == DIM_2D_TYPE || resourceDimTypeId == DIM_2D_ARRAY_TYPE ||
            resourceDimTypeId == DIM_3D_TYPE || resourceDimTypeId == DIM_CUBE_TYPE || resourceDimTypeId == DIM_CUBE_ARRAY_TYPE));

        return module.getTypeByName(ResourceDimensionTypeName[resourceDimTypeId]);
    }
}
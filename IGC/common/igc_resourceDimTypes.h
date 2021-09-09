/*========================== begin_copyright_notice ============================

Copyright (C) 2018-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

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

    const char * const ResourceDimensionTypeName[RESOURCE_DIMENSION_TYPE::NUM_RESOURCE_DIMENSION_TYPES] =
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

    inline llvm::Type* GetResourceDimensionType(const llvm::Module& module, RESOURCE_DIMENSION_TYPE resourceDimTypeId)
    {
        IGC_ASSERT((resourceDimTypeId == DIM_TYPED_BUFFER_TYPE ||
            resourceDimTypeId == DIM_1D_TYPE || resourceDimTypeId == DIM_1D_ARRAY_TYPE ||
            resourceDimTypeId == DIM_2D_TYPE || resourceDimTypeId == DIM_2D_ARRAY_TYPE ||
            resourceDimTypeId == DIM_3D_TYPE || resourceDimTypeId == DIM_CUBE_TYPE || resourceDimTypeId == DIM_CUBE_ARRAY_TYPE));

#if LLVM_VERSION_MAJOR >= 12
        llvm::LLVMContext& llvmCtx = module.getContext();
        return llvm::StructType::getTypeByName(llvmCtx, ResourceDimensionTypeName[resourceDimTypeId]);
#else
         return module.getTypeByName(ResourceDimensionTypeName[resourceDimTypeId]);
#endif
    }
}

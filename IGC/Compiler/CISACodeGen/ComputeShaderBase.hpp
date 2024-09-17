/*========================== begin_copyright_notice ============================

Copyright (C) 2019-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/


#pragma once

#include "Compiler/CISACodeGen/ShaderCodeGen.hpp"
#include "common/LLVMWarningsPush.hpp"
#include <optional>
#include "common/LLVMWarningsPop.hpp"

namespace IGC
{
    class CComputeShaderBase : public CShader
    {
    public:
        CComputeShaderBase(llvm::Function* pFunc, CShaderProgram* pProgram);
        virtual ~CComputeShaderBase();
    protected:
        void selectWalkOrder(
            bool useLinearWalk,
            uint numberOfTypedAccess,
            uint numberOfUntypedAccess,
            uint num1DAccesses,
            uint num2DAccesses,
            uint numSLMAccesses,
            uint threadGroupSize_X,
            uint threadGroupSize_Y,
            uint threadGroupSize_Z,
            SComputeShaderWalkOrder& walkOrderStruct);

        // Determines if HW can handle auto generating local IDs with this
        // order
        static std::optional<CS_WALK_ORDER> checkLegalWalkOrder(
            const std::array<uint32_t, 3>& Dims,
            const WorkGroupWalkOrderMD& WO);
    };
}

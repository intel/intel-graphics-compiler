/*========================== begin_copyright_notice ============================

Copyright (C) 2019-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/


#pragma once

#include "Compiler/CISACodeGen/ShaderCodeGen.hpp"
#include "common/LLVMWarningsPush.hpp"
#include "llvm/ADT/Optional.h"
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
            uint threadGroupSize_Z);

        ThreadIDLayout m_ThreadIDLayout = ThreadIDLayout::X;

        CS_WALK_ORDER m_walkOrder = CS_WALK_ORDER::WO_XYZ;
        enum EMIT_LOCAL_MASK {
            NONE = 0,
            X    = 1,
            XY   = 3,
            XYZ  = 7
        };
        EMIT_LOCAL_MASK m_emitMask = EMIT_LOCAL_MASK::NONE;
        //true if HW generates localIDs and puts them to payload
        //false if SW generates localIDs and prolog kernel loads them from memory
        bool m_enableHWGenerateLID = false;

        void setEmitLocalMask(SGVUsage channelNum);
        static llvm::Optional<CS_WALK_ORDER> selectBestWalkOrder(
            ThreadIDLayout Layout,
            bool is_pow2_x, bool is_pow2_y, bool is_pow2_z);
        // Determines if HW can handle auto generating local IDs with this
        // order
        static llvm::Optional<CS_WALK_ORDER> checkLegalWalkOrder(
            const std::array<uint32_t, 3>& Dims,
            const WorkGroupWalkOrderMD& WO);
        static bool enableHWGenerateLID(
            CS_WALK_ORDER walk_order,
            bool is_pow2_x, bool is_pow2_y, bool is_pow2_z);
        void overrideWalkOrderKeys(
            bool is_pow2_x, bool is_pow2_y, bool is_pow2_z, const ComputeShaderInfo& csInfo);
        static CS_WALK_ORDER getWalkOrder(uint order0, uint order1);
    };
}

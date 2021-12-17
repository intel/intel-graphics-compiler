/*========================== begin_copyright_notice ============================

Copyright (C) 2019-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/


#pragma once

#include "Compiler/CISACodeGen/ShaderCodeGen.hpp"

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

        enum WALK_ORDER {
            WO_XYZ,
            WO_XZY,
            WO_YXZ,
            WO_ZXY,
            WO_YZX,
            WO_ZYX
        };
        WALK_ORDER m_walkOrder = WALK_ORDER::WO_XYZ;
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
    private:
        static WALK_ORDER getWalkOrder(uint order0, uint order1);
    };
}

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
            bool cfgDefaultTileY,
            uint numberOfTypedAccess,
            uint numberOfUntypedAccess,
            uint num1DAccesses,
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
    };
}

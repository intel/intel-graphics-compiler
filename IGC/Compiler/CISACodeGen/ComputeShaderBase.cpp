/*========================== begin_copyright_notice ============================

Copyright (C) 2019-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "common/LLVMWarningsPush.hpp"
#include <llvm/Support/ScaledNumber.h>
#include "common/LLVMWarningsPop.hpp"
#include "Compiler/CISACodeGen/ComputeShaderBase.hpp"
#include "Compiler/CISACodeGen/messageEncoding.hpp"
#include "common/allocator.h"
#include "common/secure_mem.h"
#include <iStdLib/utility.h>
#include <algorithm>
#include "Probe/Assertion.h"

using namespace llvm;

namespace IGC
{
    CComputeShaderBase::CComputeShaderBase(llvm::Function* pFunc, CShaderProgram* pProgram)
        : CShader(pFunc, pProgram) {}

    CComputeShaderBase::~CComputeShaderBase() {}

    void CComputeShaderBase::selectWalkOrder(
        bool useLinearWalk,
        uint numberOfTypedAccess,
        uint numberOfUntypedAccess,
        uint num1DAccesses,
        uint num2DAccesses,
        uint numSLMAccesses,
        uint threadGroupSize_X,
        uint threadGroupSize_Y,
        uint threadGroupSize_Z)
    {
        const CodeGenContext* pCtx = GetContext();
        const ModuleMetaData* MMD = pCtx->getModuleMetaData();

        if (MMD->csInfo.neededThreadIdLayout == ThreadIDLayout::QuadTile)
        {
            m_ThreadIDLayout = ThreadIDLayout::QuadTile;
            return;
        }

        if ((numberOfTypedAccess >= numberOfUntypedAccess) &&
            threadGroupSize_Y % 4 == 0 &&
            !MMD->csInfo.disableLocalIdOrderOptimizations &&
            IGC_IS_FLAG_ENABLED(UseTiledCSThreadOrder)) {
            m_ThreadIDLayout = ThreadIDLayout::TileY;
            m_walkOrder = WO_YXZ;
        }

        bool needsLinearWalk =
            MMD->csInfo.neededThreadIdLayout == ThreadIDLayout::X;
        if (needsLinearWalk)
        {
            m_ThreadIDLayout = ThreadIDLayout::X;
            m_walkOrder = WO_XYZ;
        }
    }
}

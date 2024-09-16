/*========================== begin_copyright_notice ============================

Copyright (C) 2019-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "common/LLVMWarningsPush.hpp"
#include <llvm/Support/ScaledNumber.h>
#include "common/LLVMWarningsPop.hpp"
#include "Compiler/CISACodeGen/ComputeShaderBase.hpp"
#include "Compiler/CISACodeGen/CSWalkOrder.hpp"
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
        uint threadGroupSize_Z,
        SComputeShaderWalkOrder& walkOrderStruct)
    {
        CodeGenContext* pCtx = GetContext();
        const ModuleMetaData* MMD = pCtx->getModuleMetaData();
        ThreadIDLayout& m_ThreadIDLayout = walkOrderStruct.m_threadIDLayout;
        CS_WALK_ORDER& m_walkOrder = walkOrderStruct.m_walkOrder;
        EMIT_LOCAL_MASK& m_emitMask = walkOrderStruct.m_emitMask;
        bool& m_enableHWGenerateLID = walkOrderStruct.m_enableHWGenerateLID;

        selectWalkOrderInPass(
            useLinearWalk,
            numberOfTypedAccess,
            numberOfUntypedAccess,
            num1DAccesses,
            num2DAccesses,
            numSLMAccesses,
            threadGroupSize_X,
            threadGroupSize_Y,
            threadGroupSize_Z,
            pCtx,
            walkOrderStruct);
        return;

        // remove obselete code below
        if (MMD->csInfo.neededThreadIdLayout == ThreadIDLayout::QuadTile)
        {
            m_ThreadIDLayout = ThreadIDLayout::QuadTile;
            return;
        }

        bool is_pow2_x = iSTD::IsPowerOfTwo(threadGroupSize_X);
        bool is_pow2_y = iSTD::IsPowerOfTwo(threadGroupSize_Y);
        bool is_pow2_z = iSTD::IsPowerOfTwo(threadGroupSize_Z);
        if (IGC_IS_FLAG_ENABLED(SetDefaultTileYWalk) && is_pow2_x &&
            m_Platform->enableSetDefaultTileYWalk() && m_DriverInfo->SupportHWGenerateTID()) {
            m_ThreadIDLayout = ThreadIDLayout::TileY;
            m_walkOrder = CS_WALK_ORDER::WO_YXZ;
        }
        if ((numberOfTypedAccess >= numberOfUntypedAccess) &&
            threadGroupSize_Y % 4 == 0 &&
            !MMD->csInfo.disableLocalIdOrderOptimizations &&
            IGC_IS_FLAG_ENABLED(UseTiledCSThreadOrder)) {
            m_ThreadIDLayout = ThreadIDLayout::TileY;
            m_walkOrder = CS_WALK_ORDER::WO_YXZ;
        }

        bool needsLinearWalk =
            MMD->csInfo.neededThreadIdLayout == ThreadIDLayout::X;
        if (m_Platform->supportHWGenerateTID() && m_DriverInfo->SupportHWGenerateTID())
        {
            // If KeepTileYForFlattened == 2, use the platform default value.
            // Otherwise 0 is forced off, 1 is forced on.
            bool KeepTileYForFlattenedValue = IGC_GET_FLAG_VALUE(KeepTileYForFlattened) == 2 ?
                m_Platform->EnableKeepTileYForFlattenedDefault() :
                IGC_IS_FLAG_ENABLED(KeepTileYForFlattened);
            if (!KeepTileYForFlattenedValue && useLinearWalk)
            {
                needsLinearWalk = true;
            }
        }
        if (needsLinearWalk)
        {
            m_ThreadIDLayout = ThreadIDLayout::X;
            m_walkOrder = CS_WALK_ORDER::WO_XYZ;
        }
        if (!(m_Platform->supportHWGenerateTID() && m_DriverInfo->SupportHWGenerateTID()))
            return;

        //if no LID is used ever, HWGenerateLID should be disabled
        if (EMIT_LOCAL_MASK::EM_NONE == m_emitMask) {
            m_enableHWGenerateLID = false;
            return;
        }

        //if TileY is selected AND EnableHWGenerateThreadIDForTileY==0, use sw to generate LID (legacy behavior)
        //ATM, we don't know for sure if tileY improves performance, so, we have a regkey here to tune it.
        //todo: this is for tuning only, remove this if logic once we know for sure if tileY should be enabled for XeHP+
        if ((m_ThreadIDLayout == ThreadIDLayout::TileY) &&
            !IGC_IS_FLAG_ENABLED(EnableHWGenerateThreadIDForTileY)) {
            m_enableHWGenerateLID = false;
            return;
        }

        //in case of HWGenerateLID, Y must be power2
        if ((m_ThreadIDLayout == ThreadIDLayout::TileY) &&
            !iSTD::IsPowerOfTwo(threadGroupSize_Y)) {
            m_ThreadIDLayout = ThreadIDLayout::X;
            m_walkOrder = CS_WALK_ORDER::WO_XYZ;
        }

        //if not all DIMs are used, we can assume not-used DIM vals are 1, so, HW might generate LIDs even if original not-used DIM val != pow2
        //say, (31, 17, 3), if only X dim is used, HW will generate LID for (31,1,1). If both XY are used, then, HW cannot generate LIDs
        if (EMIT_LOCAL_MASK::EM_X == m_emitMask){
            threadGroupSize_Y = 1;
            threadGroupSize_Z = 1;
            //it makes no sense to use TileY if no Y is used at all. Disable it.
            m_ThreadIDLayout = ThreadIDLayout::X;
        }else if (EMIT_LOCAL_MASK::EM_XY == m_emitMask){
            threadGroupSize_Z = 1;
        }//else if (EMIT_LOCAL_MASK::EM_XY == m_emitMask)

        // Will use linear for the case like 64x1x1.
        // Open: How about 1x32x1?
        if (m_ThreadIDLayout == ThreadIDLayout::TileY &&
            threadGroupSize_Y == 1 && threadGroupSize_Z == 1) {
            // 1D thread group
            m_ThreadIDLayout = ThreadIDLayout::X;
            m_walkOrder = CS_WALK_ORDER::WO_XYZ;
        }

        if (!IGC_IS_FLAG_ENABLED(EnableNonOCLWalkOrderSel) || needsLinearWalk) {
            if (threadGroupSize_Y == 1 && threadGroupSize_Z == 1)
            {
                m_walkOrder = CS_WALK_ORDER::WO_YZX;
                m_enableHWGenerateLID = true;
            }
            else if (threadGroupSize_X == 1 && threadGroupSize_Z == 1)
            {
                m_walkOrder = CS_WALK_ORDER::WO_XZY;
                m_enableHWGenerateLID = true;
            }
            else
            {
                m_walkOrder = CS_WALK_ORDER::WO_XYZ;
                m_enableHWGenerateLID = (is_pow2_x && is_pow2_y);
            }
            //disable tileY if walkorder cannot be changed
            m_ThreadIDLayout = ThreadIDLayout::X;
            overrideWalkOrderKeysInPass(is_pow2_x, is_pow2_y, is_pow2_z, walkOrderStruct, m_ctx);
            return;
        }

        // Perfer linear walk for 2D dispatch, but linear UAV surface
        if ((IGC_IS_FLAG_ENABLED(ForceLinearWalkOnLinearUAV) ||
            MMD->compOpt.ForceLinearWalkOnLinearUAV) &&
            (m_ThreadIDLayout == ThreadIDLayout::TileY) &&
            EMIT_LOCAL_MASK::EM_XY == m_emitMask &&
            num1DAccesses)
        {
            m_ThreadIDLayout = ThreadIDLayout::X;
            m_walkOrder = CS_WALK_ORDER::WO_XYZ;
        }

        // If EnableNewTileYCheck == 2, use the platform default value. Otherwise 0 is forced off, 1 is forced on.
        bool b_EnableNewTileYCheck = IGC_GET_FLAG_VALUE(EnableNewTileYCheck) == 2 ?
            m_Platform->EnableNewTileYCheckDefault() : IGC_GET_FLAG_VALUE(EnableNewTileYCheck);
        if (b_EnableNewTileYCheck &&
            IGC_IS_FLAG_ENABLED(SetDefaultTileYWalk) &&
            (m_ThreadIDLayout == ThreadIDLayout::TileY) &&
            EMIT_LOCAL_MASK::EM_XY == m_emitMask)
        {
            // check 1D, 2D, SLM accesses
            int num1D = num1DAccesses + (int)(numSLMAccesses / 4);
            int num2D = num2DAccesses;
            if (num1D > num2D && num2D <= 5)
            {
                m_ThreadIDLayout = ThreadIDLayout::X;
                m_walkOrder = CS_WALK_ORDER::WO_XYZ;
            }
        }

        auto order = selectBestWalkOrderInPass(
            m_ThreadIDLayout, is_pow2_x, is_pow2_y, is_pow2_z);

        if (order) {
            m_walkOrder = *order;
            m_enableHWGenerateLID = true;
        } else {
            // Is 2D or 3D dispatch and isnt pow2, so the HW doesn't support it
            m_enableHWGenerateLID = false;
            m_ThreadIDLayout = ThreadIDLayout::X;
            m_walkOrder = CS_WALK_ORDER::WO_XYZ;
        }
        overrideWalkOrderKeysInPass(is_pow2_x, is_pow2_y, is_pow2_z, walkOrderStruct, m_ctx);
    }

    Optional<CS_WALK_ORDER>
    CComputeShaderBase::checkLegalWalkOrder(
        const std::array<uint32_t, 3>& Dims,
        const WorkGroupWalkOrderMD& WO)
    {
        auto is_pow2 = [](uint32_t dim) {
            return iSTD::IsPowerOfTwo(dim);
        };

        const int walkorder_x = WO.dim0;
        const int walkorder_y = WO.dim1;
        const int walkorder_z = WO.dim2;

        const uint32_t dim_x = Dims[0];
        const uint32_t dim_y = Dims[1];
        const uint32_t dim_z = Dims[2];

        uint order0 = (walkorder_x == 0) ? 0 : (walkorder_y == 0) ? 1 : 2;
        uint order1 = (walkorder_x == 1) ? 0 : (walkorder_y == 1) ? 1 : 2;

        if (order0 != order1
            && ((order0 == 0 && is_pow2(dim_x))
                || (order0 == 1 && is_pow2(dim_y))
                || (order0 == 2 && is_pow2(dim_z)))
            && ((order1 == 0 && is_pow2(dim_x))
                || (order1 == 1 && is_pow2(dim_y))
                || (order1 == 2 && is_pow2(dim_z)))
            )
        {
            // Legal walk order for HW auto-gen
            return getWalkOrderInPass(order0, order1);
        }

        return None;
    }
}

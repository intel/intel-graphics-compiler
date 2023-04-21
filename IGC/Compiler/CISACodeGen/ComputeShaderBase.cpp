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
            if (IGC_IS_FLAG_DISABLED(KeepTileYForFlattened) && useLinearWalk)
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
        if (EMIT_LOCAL_MASK::NONE == m_emitMask) {
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
        if (EMIT_LOCAL_MASK::X == m_emitMask){
            threadGroupSize_Y = 1;
            threadGroupSize_Z = 1;
            //it makes no sense to use TileY if no Y is used at all. Disable it.
            m_ThreadIDLayout = ThreadIDLayout::X;
        }else if (EMIT_LOCAL_MASK::XY == m_emitMask){
            threadGroupSize_Z = 1;
        }//else if (EMIT_LOCAL_MASK::XYZ == m_emitMask)

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
            overrideWalkOrderKeys(is_pow2_x, is_pow2_y, is_pow2_z, MMD->csInfo);
            return;
        }

        // Perfer linear walk for 2D dispatch, but linear UAV surface
        if (IGC_IS_FLAG_ENABLED(ForceLinearWalkOnLinearUAV) &&
            (m_ThreadIDLayout == ThreadIDLayout::TileY) &&
            EMIT_LOCAL_MASK::XY == m_emitMask &&
            num1DAccesses)
        {
            m_ThreadIDLayout = ThreadIDLayout::X;
            m_walkOrder = CS_WALK_ORDER::WO_XYZ;
        }

        if (IGC_IS_FLAG_ENABLED(EnableNewTileYCheck) &&
            IGC_IS_FLAG_ENABLED(SetDefaultTileYWalk) &&
            (m_ThreadIDLayout == ThreadIDLayout::TileY) &&
            EMIT_LOCAL_MASK::XY == m_emitMask)
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

        auto order = selectBestWalkOrder(
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
        overrideWalkOrderKeys(is_pow2_x, is_pow2_y, is_pow2_z, MMD->csInfo);
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
            return getWalkOrder(order0, order1);
        }

        return None;
    }

    Optional<CS_WALK_ORDER>
    CComputeShaderBase::selectBestWalkOrder(
        ThreadIDLayout Layout,
        bool is_pow2_x, bool is_pow2_y, bool is_pow2_z)
    {
        constexpr uint UNDEF = std::numeric_limits<uint>::max();
        uint order0 = UNDEF;
        uint order1 = UNDEF;
        if (Layout == ThreadIDLayout::TileY)
        {
            IGC_ASSERT(is_pow2_y);
            order0 = 1;
            order1 = (is_pow2_x ? 0 : (is_pow2_z ? 2 : UNDEF));
        }
        else
        {
            //below is from HAS p-code except tileY
            //try to find walk_order so that HW can generate LID
            if (is_pow2_x)
            {
                // (pow2,pow2,z) or (pow2,y,pow2) or illegal
                order0 = 0;
                order1 = (is_pow2_y ? 1 : (is_pow2_z ? 2 : UNDEF));
            }
            else if (is_pow2_y)
            {
                // (x,pow2,pow2) or illegal
                order0 = 1;
                order1 = (is_pow2_z ? 2 : UNDEF);
            }
        }

        if (order1 != UNDEF)
        {
            // select walkorder
            return getWalkOrder(order0, order1);
        }

        return None;
    }

    bool
    CComputeShaderBase::enableHWGenerateLID(
        CS_WALK_ORDER walk_order,
        bool is_pow2_x, bool is_pow2_y, bool is_pow2_z)
    {
        bool bEnableHWGenerateLID = false;

        switch (walk_order)
        {
        case CS_WALK_ORDER::WO_XYZ:
        case CS_WALK_ORDER::WO_YXZ:
            bEnableHWGenerateLID = (is_pow2_x && is_pow2_y);
            break;

        case CS_WALK_ORDER::WO_XZY:
        case CS_WALK_ORDER::WO_ZXY:
            bEnableHWGenerateLID = (is_pow2_x && is_pow2_z);
            break;

        case CS_WALK_ORDER::WO_YZX:
        case CS_WALK_ORDER::WO_ZYX:
            bEnableHWGenerateLID = (is_pow2_y && is_pow2_z);
            break;
        }
        return bEnableHWGenerateLID;
    }

    void
    CComputeShaderBase::overrideWalkOrderKeys(
        bool is_pow2_x, bool is_pow2_y, bool is_pow2_z, const ComputeShaderInfo& csInfo)
    {
        if ((IGC_IS_FLAG_ENABLED(ForceTileY) || GetContext()->getModuleMetaData()->csInfo.forceTileYWalk) &&
            m_Platform->supportHWGenerateTID() && m_DriverInfo->SupportHWGenerateTID())
        {
            m_ThreadIDLayout = ThreadIDLayout::TileY;
            m_walkOrder = CS_WALK_ORDER::WO_YXZ;
            m_enableHWGenerateLID = enableHWGenerateLID(m_walkOrder, is_pow2_x, is_pow2_y, is_pow2_z);
        }

        if (csInfo.walkOrderEnabled)
        {
            m_walkOrder = (CS_WALK_ORDER)csInfo.walkOrderOverride;
            m_enableHWGenerateLID = enableHWGenerateLID(m_walkOrder, is_pow2_x, is_pow2_y, is_pow2_z);
        }

        if (IGC_IS_FLAG_ENABLED(OverrideCsWalkOrderEnable))
        {
            m_walkOrder = (CS_WALK_ORDER)IGC_GET_FLAG_VALUE(OverrideCsWalkOrder);
            m_enableHWGenerateLID = enableHWGenerateLID(m_walkOrder, is_pow2_x, is_pow2_y, is_pow2_z);
        }

        if (IGC_IS_FLAG_ENABLED(OverrideCsTileLayoutEnable))
        {
            m_ThreadIDLayout = (ThreadIDLayout)IGC_IS_FLAG_ENABLED(OverrideCsTileLayout);
        }
    }

    //order0: the internal walk dim
    //order1: the intermediate walk dim
    //e.g.: 1, 0 means, YXZ walkorder
    CS_WALK_ORDER CComputeShaderBase::getWalkOrder(uint order0, uint order1)
    {
        auto getWalkOrderValue = [](uint order0, uint order1) constexpr {
            return (order0 << 4 | order1 << 2);
        };

        switch (getWalkOrderValue(order0, order1))
        {
        case getWalkOrderValue(0, 1): return CS_WALK_ORDER::WO_XYZ; //012
        case getWalkOrderValue(0, 2): return CS_WALK_ORDER::WO_XZY; //021
        case getWalkOrderValue(1, 0): return CS_WALK_ORDER::WO_YXZ; //102
        case getWalkOrderValue(1, 2): return CS_WALK_ORDER::WO_YZX; //120
        case getWalkOrderValue(2, 0): return CS_WALK_ORDER::WO_ZXY; //201
        case getWalkOrderValue(2, 1): return CS_WALK_ORDER::WO_ZYX; //210
        default:
            IGC_ASSERT_MESSAGE(0, "unhandled case!");
            return CS_WALK_ORDER::WO_XYZ;
        }
    }

    void CComputeShaderBase::setEmitLocalMask(SGVUsage channelNum) {
        //only 4 patterns are supported: None; X; XY; XYZ
        switch (channelNum)
        {
        case THREAD_ID_IN_GROUP_X:
            m_emitMask = (EMIT_LOCAL_MASK::NONE == m_emitMask) ? EMIT_LOCAL_MASK::X : m_emitMask;
            break;
        case THREAD_ID_IN_GROUP_Y:
            m_emitMask = (EMIT_LOCAL_MASK::NONE == m_emitMask || EMIT_LOCAL_MASK::X == m_emitMask) ? EMIT_LOCAL_MASK::XY : m_emitMask;
            break;
        case THREAD_ID_IN_GROUP_Z:
            m_emitMask = EMIT_LOCAL_MASK::XYZ;
            break;
        default:
            break;
        }
    }
}

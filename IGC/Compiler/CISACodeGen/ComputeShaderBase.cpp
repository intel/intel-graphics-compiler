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
            m_walkOrder = WO_YXZ;
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
            m_walkOrder = WO_XYZ;
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
            m_walkOrder = WO_XYZ;
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
            m_walkOrder = WO_XYZ;
        }

        if (!IGC_IS_FLAG_ENABLED(EnableNonOCLWalkOrderSel) || needsLinearWalk) {
            //reset walkorder
            m_walkOrder = WO_XYZ;
            m_enableHWGenerateLID = (is_pow2_x && is_pow2_y);
            //disable tileY if walkorder cannot be changed
            m_ThreadIDLayout = ThreadIDLayout::X;
            return;
        }

        // Perfer linear walk for 2D dispatch, but linear UAV surface
        if (IGC_IS_FLAG_ENABLED(ForceLinearWalkOnLinearUAV) &&
            (m_ThreadIDLayout == ThreadIDLayout::TileY) &&
            EMIT_LOCAL_MASK::XY == m_emitMask &&
            num1DAccesses)
        {
            m_ThreadIDLayout = ThreadIDLayout::X;
            m_walkOrder = WO_XYZ;
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
                m_walkOrder = WO_XYZ;
            }
        }

        if ((IGC_IS_FLAG_ENABLED(ForceTileY) || MMD->csInfo.forceTileYWalk) &&
            m_Platform->supportHWGenerateTID() && m_DriverInfo->SupportHWGenerateTID()) {
            m_ThreadIDLayout = ThreadIDLayout::TileY;
            m_walkOrder = WO_YXZ;
        }

        uint UNDEF = 999;
        uint order0 = UNDEF;
        uint order1 = UNDEF;
        if (m_ThreadIDLayout == ThreadIDLayout::TileY) {
            IGC_ASSERT(is_pow2_y);
            order0 = 1;
            order1 = (is_pow2_x ? 0 : (is_pow2_z ? 2 : UNDEF));
        }else {
            //below is from HAS p-code except tileY
            //try to find walk_order so that HW can generate LID
            if (is_pow2_x) {
                // (pow2,pow2,z) or (pow2,y,pow2) or illegal
                order0 = 0;
                order1 = (is_pow2_y ? 1 : (is_pow2_z ? 2 : UNDEF));
            }else if (is_pow2_y) {
                // (x,pow2,pow2) or illegal
                order0 = 1;
                order1 = (is_pow2_z ? 2 : UNDEF);
            }
        }

        if (order1 != UNDEF) {
            // select walkorder
            m_walkOrder = getWalkOrder(order0, order1);
            m_enableHWGenerateLID = true;

        }else {
            // Is 2D or 3D dispatch and isnt pow2, so the HW doesn't support it
            m_enableHWGenerateLID = false;
            m_ThreadIDLayout = ThreadIDLayout::X;
            m_walkOrder = WO_XYZ;
            return;
        }
    }
    //order0: the internal walk dim
    //order1: the intermediate walk dim
    //e.g.: 1, 0 means, YXZ walkorder
    CComputeShaderBase::WALK_ORDER CComputeShaderBase::getWalkOrder(uint order0, uint order1)
    {
        auto getWalkOrderValue = [](uint order0, uint order1) constexpr {
            return (order0 << 4 | order1 << 2);
        };

        switch (getWalkOrderValue(order0, order1))
        {
        case getWalkOrderValue(0, 1): return WALK_ORDER::WO_XYZ; //012
        case getWalkOrderValue(0, 2): return WALK_ORDER::WO_XZY; //021
        case getWalkOrderValue(1, 0): return WALK_ORDER::WO_YXZ; //102
        case getWalkOrderValue(1, 2): return WALK_ORDER::WO_YZX; //201
        case getWalkOrderValue(2, 0): return WALK_ORDER::WO_ZXY; //120
        case getWalkOrderValue(2, 1): return WALK_ORDER::WO_ZYX; //210
        default:
            IGC_ASSERT_MESSAGE(0, "unhandled case!");
            return WALK_ORDER::WO_XYZ;
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

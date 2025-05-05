/*========================== begin_copyright_notice ============================

Copyright (C) 2022-2024 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "common/LLVMWarningsPush.hpp"
#include <llvm/Pass.h>
#include <optional>
#include "common/LLVMWarningsPop.hpp"
#include "Compiler/CISACodeGen/ShaderCodeGen.hpp"
#include "Compiler/CISACodeGen/CSWalkOrder.hpp"
#include "Probe/Assertion.h"

using namespace llvm;
using namespace IGC;


void IGC::selectWalkOrderInPass(
    bool useLinearWalk,
    uint numberOfTypedAccess,
    uint numberOfUntypedAccess,
    uint num1DAccesses,
    uint num2DAccesses,
    uint numSLMAccesses,
    uint threadGroupSize_X,
    uint threadGroupSize_Y,
    uint threadGroupSize_Z,
    CodeGenContext* ctx,
    SComputeShaderWalkOrder& walkOrderStruct)
{
    ModuleMetaData* MMD = ctx->getModuleMetaData();
    const CPlatform& platform = ctx->platform;
    const CDriverInfo& driverInfo = ctx->m_DriverInfo;
    ThreadIDLayout& threadIDLayout = walkOrderStruct.m_threadIDLayout;
    CS_WALK_ORDER& walkOrder = walkOrderStruct.m_walkOrder;
    EMIT_LOCAL_MASK &emitMask = walkOrderStruct.m_emitMask;
    bool& enableHWGenerateLID = walkOrderStruct.m_enableHWGenerateLID;

    if (MMD->csInfo.neededThreadIdLayout == ThreadIDLayout::QuadTile)
    {
        threadIDLayout = ThreadIDLayout::QuadTile;
        return;
    }

    bool is_pow2_x = iSTD::IsPowerOfTwo(threadGroupSize_X);
    bool is_pow2_y = iSTD::IsPowerOfTwo(threadGroupSize_Y);
    bool is_pow2_z = iSTD::IsPowerOfTwo(threadGroupSize_Z);
    if (IGC_IS_FLAG_ENABLED(SetDefaultTileYWalk) && is_pow2_x &&
        platform.enableSetDefaultTileYWalk() && driverInfo.SupportHWGenerateTID()) {
        threadIDLayout = ThreadIDLayout::TileY;
        walkOrder = CS_WALK_ORDER::WO_YXZ;
    }
    if ((numberOfTypedAccess >= numberOfUntypedAccess) &&
        threadGroupSize_Y % 4 == 0 &&
        !MMD->csInfo.disableLocalIdOrderOptimizations &&
        IGC_IS_FLAG_ENABLED(UseTiledCSThreadOrder)) {
        threadIDLayout = ThreadIDLayout::TileY;
        walkOrder = CS_WALK_ORDER::WO_YXZ;
    }

    bool needsLinearWalk =
        MMD->csInfo.neededThreadIdLayout == ThreadIDLayout::X;
    if (platform.supportHWGenerateTID() && driverInfo.SupportHWGenerateTID())
    {
        // If KeepTileYForFlattened == 2, use the platform default value.
        // Otherwise 0 is forced off, 1 is forced on.
        bool KeepTileYForFlattenedValue = IGC_GET_FLAG_VALUE(KeepTileYForFlattened) == 2 ?
            platform.EnableKeepTileYForFlattenedDefault() :
            IGC_IS_FLAG_ENABLED(KeepTileYForFlattened);
        if (!KeepTileYForFlattenedValue && useLinearWalk)
        {
            needsLinearWalk = true;
        }
    }
    if (needsLinearWalk)
    {
        threadIDLayout = ThreadIDLayout::X;
        walkOrder = CS_WALK_ORDER::WO_XYZ;
    }
    if (!(platform.supportHWGenerateTID() && driverInfo.SupportHWGenerateTID()))
        return;

    //if no LID is used ever, HWGenerateLID should be disabled
    if (EMIT_LOCAL_MASK::EM_NONE == emitMask) {
        enableHWGenerateLID = false;
        return;
    }

    //if TileY is selected AND EnableHWGenerateThreadIDForTileY==0, use sw to generate LID (legacy behavior)
    //ATM, we don't know for sure if tileY improves performance, so, we have a regkey here to tune it.
    //todo: this is for tuning only, remove this if logic once we know for sure if tileY should be enabled for XeHP+
    if ((threadIDLayout == ThreadIDLayout::TileY) &&
        !IGC_IS_FLAG_ENABLED(EnableHWGenerateThreadIDForTileY)) {
        enableHWGenerateLID = false;
        return;
    }

    //in case of HWGenerateLID, Y must be power2
    if ((threadIDLayout == ThreadIDLayout::TileY) &&
        !iSTD::IsPowerOfTwo(threadGroupSize_Y)) {
        threadIDLayout = ThreadIDLayout::X;
        walkOrder = CS_WALK_ORDER::WO_XYZ;
    }

    //if not all DIMs are used, we can assume not-used DIM vals are 1, so, HW might generate LIDs even if original not-used DIM val != pow2
    //say, (31, 17, 3), if only X dim is used, HW will generate LID for (31,1,1). If both XY are used, then, HW cannot generate LIDs
    if (EMIT_LOCAL_MASK::EM_X == emitMask) {
        threadGroupSize_Y = 1;
        threadGroupSize_Z = 1;
        //it makes no sense to use TileY if no Y is used at all. Disable it.
        threadIDLayout = ThreadIDLayout::X;
    }
    else if (EMIT_LOCAL_MASK::EM_XY == emitMask) {
        threadGroupSize_Z = 1;
    }//else if (EMIT_LOCAL_MASK::EM_XYZ == emitMask)

    // Will use linear for the case like 64x1x1.
    // Open: How about 1x32x1?
    if (threadIDLayout == ThreadIDLayout::TileY &&
        threadGroupSize_Y == 1 && threadGroupSize_Z == 1) {
        // 1D thread group
        threadIDLayout = ThreadIDLayout::X;
        walkOrder = CS_WALK_ORDER::WO_XYZ;
    }

    if (!IGC_IS_FLAG_ENABLED(EnableNonOCLWalkOrderSel) || needsLinearWalk) {
        if (threadGroupSize_Y == 1 && threadGroupSize_Z == 1)
        {
            walkOrder = CS_WALK_ORDER::WO_YZX;
            enableHWGenerateLID = true;
        }
        else if (threadGroupSize_X == 1 && threadGroupSize_Z == 1)
        {
            walkOrder = CS_WALK_ORDER::WO_XZY;
            enableHWGenerateLID = true;
        }
        else
        {
            walkOrder = CS_WALK_ORDER::WO_XYZ;
            enableHWGenerateLID = (is_pow2_x && is_pow2_y);
        }
        //disable tileY if walkorder cannot be changed
        threadIDLayout = ThreadIDLayout::X;
        overrideWalkOrderKeysInPass(is_pow2_x, is_pow2_y, is_pow2_z, walkOrderStruct, ctx);
        return;
    }

    // Perfer linear walk for 2D dispatch, but linear UAV surface
    if ((IGC_IS_FLAG_ENABLED(ForceLinearWalkOnLinearUAV) ||
        MMD->compOpt.ForceLinearWalkOnLinearUAV) &&
        (threadIDLayout == ThreadIDLayout::TileY) &&
        EMIT_LOCAL_MASK::EM_XY == emitMask &&
        num1DAccesses)
    {
        threadIDLayout = ThreadIDLayout::X;
        walkOrder = CS_WALK_ORDER::WO_XYZ;
    }

    // If EnableNewTileYCheck == 2, use the platform default value. Otherwise 0 is forced off, 1 is forced on.
    bool b_EnableNewTileYCheck = IGC_GET_FLAG_VALUE(EnableNewTileYCheck) == 2 ?
        platform.EnableNewTileYCheckDefault() : IGC_GET_FLAG_VALUE(EnableNewTileYCheck);
    if (b_EnableNewTileYCheck &&
        IGC_IS_FLAG_ENABLED(SetDefaultTileYWalk) &&
        (threadIDLayout == ThreadIDLayout::TileY) &&
        EMIT_LOCAL_MASK::EM_XY == emitMask)
    {
        // check 1D, 2D, SLM accesses
        int num1D = num1DAccesses + (int)(numSLMAccesses / 4);
        int num2D = num2DAccesses;
        if (num1D > num2D && num2D <= 5)
        {
            threadIDLayout = ThreadIDLayout::X;
            walkOrder = CS_WALK_ORDER::WO_XYZ;
        }
    }

    auto order = selectBestWalkOrderInPass(
        threadIDLayout, is_pow2_x, is_pow2_y, is_pow2_z);

    if (order) {
        walkOrder = *order;
        enableHWGenerateLID = true;
    }
    else {
        // Is 2D or 3D dispatch and isnt pow2, so the HW doesn't support it
        enableHWGenerateLID = false;
        threadIDLayout = ThreadIDLayout::X;
        walkOrder = CS_WALK_ORDER::WO_XYZ;
    }

    overrideWalkOrderKeysInPass(is_pow2_x, is_pow2_y, is_pow2_z, walkOrderStruct, ctx);
}

void IGC::overrideWalkOrderKeysInPass(
    bool is_pow2_x, bool is_pow2_y, bool is_pow2_z,
    SComputeShaderWalkOrder& walkOrderStruct,
    CodeGenContext* ctx)
{
    ThreadIDLayout& threadIDLayout = walkOrderStruct.m_threadIDLayout;
    CS_WALK_ORDER& walkOrder = walkOrderStruct.m_walkOrder;
    bool& enableHWGenerateLID = walkOrderStruct.m_enableHWGenerateLID;

    ModuleMetaData* MMD = ctx->getModuleMetaData();
    const CPlatform& platform = ctx->platform;
    const CDriverInfo& driverInfo = ctx->m_DriverInfo;

    if ((IGC_IS_FLAG_ENABLED(ForceTileY) || MMD->csInfo.forceTileYWalk) &&
        platform.supportHWGenerateTID() && driverInfo.SupportHWGenerateTID())
    {
        threadIDLayout = ThreadIDLayout::TileY;
        walkOrder = CS_WALK_ORDER::WO_YXZ;
        enableHWGenerateLID = enableHWGenerateLIDInPass(walkOrder, is_pow2_x, is_pow2_y, is_pow2_z);
    }

    if (MMD->csInfo.walkOrderEnabled)
    {
        walkOrder = (CS_WALK_ORDER)MMD->csInfo.walkOrderOverride;
        enableHWGenerateLID = enableHWGenerateLIDInPass(walkOrder, is_pow2_x, is_pow2_y, is_pow2_z);
    }

    if (IGC_IS_FLAG_ENABLED(OverrideCsWalkOrderEnable))
    {
        walkOrder = (CS_WALK_ORDER)IGC_GET_FLAG_VALUE(OverrideCsWalkOrder);
        enableHWGenerateLID = enableHWGenerateLIDInPass(walkOrder, is_pow2_x, is_pow2_y, is_pow2_z);
    }

    if (IGC_IS_FLAG_ENABLED(OverrideCsTileLayoutEnable))
    {
        threadIDLayout = (ThreadIDLayout)IGC_IS_FLAG_ENABLED(OverrideCsTileLayout);
    }

    const IGC::TriboolFlag overrideHWGenerateLID =
        static_cast<TriboolFlag>(IGC_GET_FLAG_VALUE(OverrideHWGenerateLID));
    switch (overrideHWGenerateLID)
    {
    case TriboolFlag::Enabled:
        enableHWGenerateLID = true;
        break;
    case TriboolFlag::Disabled:
        enableHWGenerateLID = false;
        break;
    default:
        break;
    }
}

bool IGC::enableHWGenerateLIDInPass(
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

std::optional<CS_WALK_ORDER>
IGC::selectBestWalkOrderInPass(
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
        return getWalkOrderInPass(order0, order1);
    }

    return std::nullopt;
}

void IGC::setEmitLocalMaskInPass(SGVUsage channelNum, EMIT_LOCAL_MASK& emitMask)
{
    //only 4 patterns are supported: None; X; XY; XYZ
    switch (channelNum)
    {
    case THREAD_ID_IN_GROUP_X:
        emitMask = (EMIT_LOCAL_MASK::EM_NONE == emitMask) ? EMIT_LOCAL_MASK::EM_X : emitMask;
        break;
    case THREAD_ID_IN_GROUP_Y:
        emitMask = (EMIT_LOCAL_MASK::EM_NONE == emitMask || EMIT_LOCAL_MASK::EM_X == emitMask) ?
            EMIT_LOCAL_MASK::EM_XY : emitMask;
        break;
    case THREAD_ID_IN_GROUP_Z:
        emitMask = EMIT_LOCAL_MASK::EM_XYZ;
        break;
    default:
        break;
    }
}

//order0: the internal walk dim
//order1: the intermediate walk dim
//e.g.: 1, 0 means, YXZ walkorder
CS_WALK_ORDER IGC::getWalkOrderInPass(uint order0, uint order1)
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

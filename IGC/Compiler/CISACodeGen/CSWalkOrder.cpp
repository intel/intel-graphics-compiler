/*========================== begin_copyright_notice ============================

Copyright (C) 2022-2024 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "common/LLVMWarningsPush.hpp"
#include <llvm/IR/PatternMatch.h>
#include <llvm/Pass.h>
#include <llvm/Support/Debug.h>
#include <llvm/Support/raw_ostream.h>
#include "common/LLVMWarningsPop.hpp"
#include "GenISAIntrinsics/GenIntrinsics.h"
#include "Compiler/CISACodeGen/ShaderCodeGen.hpp"
#include "Compiler/MetaDataUtilsWrapper.h"
#include "Compiler/CISACodeGen/CSWalkOrder.hpp"
#include "Probe/Assertion.h"

using namespace llvm;
using namespace IGC;


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

Optional<CS_WALK_ORDER>
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

    return None;
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

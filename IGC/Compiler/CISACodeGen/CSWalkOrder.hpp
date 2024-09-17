/*========================== begin_copyright_notice ============================

Copyright (C) 2022-2024 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef __CS_WALK_ORDER_H__
#define __CS_WALK_ORDER_H__

#include <llvm/Pass.h>
#include "Compiler/CISACodeGen/ComputeShaderBase.hpp"
#include <optional>

using namespace IGC;

namespace IGC {
    void selectWalkOrderInPass(
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
        SComputeShaderWalkOrder& walkOrderStruct);

    void overrideWalkOrderKeysInPass(
        bool is_pow2_x, bool is_pow2_y, bool is_pow2_z,
        SComputeShaderWalkOrder& walkOrderStruct,
        CodeGenContext* ctx);
    bool enableHWGenerateLIDInPass(
        CS_WALK_ORDER walk_order,
        bool is_pow2_x, bool is_pow2_y, bool is_pow2_z);
    std::optional<CS_WALK_ORDER> selectBestWalkOrderInPass(
        ThreadIDLayout Layout,
        bool is_pow2_x, bool is_pow2_y, bool is_pow2_z);
    void setEmitLocalMaskInPass(SGVUsage channelNum, EMIT_LOCAL_MASK& emitMask);
    CS_WALK_ORDER getWalkOrderInPass(uint order0, uint order1);
} // End namespace IGC

#endif // __CS_WALK_ORDER_H__

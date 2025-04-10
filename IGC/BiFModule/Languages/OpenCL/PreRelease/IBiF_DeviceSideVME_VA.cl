/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

// This file defines helper builtins of OpenCL VME, VA extension functions.
/*****************************************************************************/
/*                       External device-side VME                            */
/*****************************************************************************/
#include "../../../Implementation/IGCBiF_Intrinsics.cl"
#include "../../../Headers/spirv.h"

// VME helper functions

__spirv_VmeImageINTEL__void_1_0_0_0_0_0_0 SPIRV_OVERLOADABLE SPIRV_BUILTIN(VmeImageINTEL, _i64_i64, )(__spirv_Image, __spirv_Sampler);

// Conversion to and from opaque types.
__spirv_AvcMcePayloadINTEL __builtin_IB_vme_helper_get_as_avc_mce_payload_t(uint4);
uint4 __builtin_IB_vme_helper_get_handle_avc_mce_payload_t(__spirv_AvcMcePayloadINTEL);
__spirv_AvcImePayloadINTEL __builtin_IB_vme_helper_get_as_avc_ime_payload_t(uint4);
uint4 __builtin_IB_vme_helper_get_handle_avc_ime_payload_t(__spirv_AvcImePayloadINTEL);
__spirv_AvcRefPayloadINTEL __builtin_IB_vme_helper_get_as_avc_ref_payload_t(uint4);
uint4 __builtin_IB_vme_helper_get_handle_avc_ref_payload_t(__spirv_AvcRefPayloadINTEL);
__spirv_AvcSicPayloadINTEL __builtin_IB_vme_helper_get_as_avc_sic_payload_t(uint4);
uint4 __builtin_IB_vme_helper_get_handle_avc_sic_payload_t(__spirv_AvcSicPayloadINTEL);
__spirv_AvcMceResultINTEL __builtin_IB_vme_helper_get_as_avc_mce_result_t(uint4);
uint4 __builtin_IB_vme_helper_get_handle_avc_mce_result_t(__spirv_AvcMceResultINTEL);
__spirv_AvcImeResultINTEL __builtin_IB_vme_helper_get_as_avc_ime_result_t(uint4);
uint4 __builtin_IB_vme_helper_get_handle_avc_ime_result_t(__spirv_AvcImeResultINTEL);
__spirv_AvcRefResultINTEL __builtin_IB_vme_helper_get_as_avc_ref_result_t(uint4);
uint4 __builtin_IB_vme_helper_get_handle_avc_ref_result_t(__spirv_AvcRefResultINTEL);
__spirv_AvcSicResultINTEL __builtin_IB_vme_helper_get_as_avc_sic_result_t(uint4);
uint4 __builtin_IB_vme_helper_get_handle_avc_sic_result_t(__spirv_AvcSicResultINTEL);
__spirv_AvcImeResultSingleReferenceStreamoutINTEL __builtin_IB_vme_helper_get_as_avc_ime_result_single_reference_streamout_t(uint8);
uint8 __builtin_IB_vme_helper_get_handle_avc_ime_result_single_reference_streamout_t(__spirv_AvcImeResultSingleReferenceStreamoutINTEL);
__spirv_AvcImeResultDualReferenceStreamoutINTEL __builtin_IB_vme_helper_get_as_avc_ime_result_dual_reference_streamout_t(uint8);
uint8 __builtin_IB_vme_helper_get_handle_avc_ime_result_dual_reference_streamout_t(__spirv_AvcImeResultDualReferenceStreamoutINTEL);
__spirv_AvcImeSingleReferenceStreaminINTEL __builtin_IB_vme_helper_get_as_avc_ime_single_reference_streamin_t(uint);
uint __builtin_IB_vme_helper_get_handle_avc_ime_single_reference_streamin_t(__spirv_AvcImeSingleReferenceStreaminINTEL);
__spirv_AvcImeDualReferenceStreaminINTEL __builtin_IB_vme_helper_get_as_avc_ime_dual_reference_streamin_t(uint2);
uint2 __builtin_IB_vme_helper_get_handle_avc_ime_dual_reference_streamin_t(__spirv_AvcImeDualReferenceStreaminINTEL);

// defines

#define UNIVERSAL_INPUT_MESSAGE_NUM_GRFS 4
#define INPUT_MESSAGE_SIC_NUM_GRFS       4
#define RETURN_MESSAGE_NUM_GRFS          7
#define NUM_DWORD_IN_GRF                 8

enum STREAM_MODE {
    VME_STREAM_DISABLE = 0,
    VME_STREAM_OUT     = 1,
    VME_STREAM_IN      = 2,
    VME_STREAM_INOUT   = 3
};

enum {
    VME_MAJOR_16x16 = 0,
    VME_MAJOR_16x8  = 1,
    VME_MAJOR_8x16  = 2,
    VME_MAJOR_8x8   = 3
};

// Helper overloads

static INLINE OVERLOADABLE uint4 intel_vme_send_ime_new_uint4(uint4 payload, long src_image, long fwd_ref_image, long bwd_ref_image, long vme_accelerator, enum STREAM_MODE mode)
{
    return __builtin_IB_vme_send_ime_new_uint4_uint4(payload, src_image, fwd_ref_image, bwd_ref_image, vme_accelerator, mode);
}
static INLINE OVERLOADABLE uint4 intel_vme_send_ime_new_uint4(uint8 payload, long src_image, long fwd_ref_image, long bwd_ref_image, long vme_accelerator, enum STREAM_MODE mode)
{
    return __builtin_IB_vme_send_ime_new_uint4_uint8(payload, src_image, fwd_ref_image, bwd_ref_image, vme_accelerator, mode);
}
static INLINE OVERLOADABLE uint8 intel_vme_send_ime_new_uint8(uint4 payload, long src_image, long fwd_ref_image, long bwd_ref_image, long vme_accelerator, enum STREAM_MODE mode)
{
    return __builtin_IB_vme_send_ime_new_uint8_uint4(payload, src_image, fwd_ref_image, bwd_ref_image, vme_accelerator, mode);
}
static INLINE OVERLOADABLE uint8 intel_vme_send_ime_new_uint8(uint8 payload, long src_image, long fwd_ref_image, long bwd_ref_image, long vme_accelerator, enum STREAM_MODE mode)
{
    return __builtin_IB_vme_send_ime_new_uint8_uint8(payload, src_image, fwd_ref_image, bwd_ref_image, vme_accelerator, mode);
}

static INLINE OVERLOADABLE uint intel_get_message_phase_dw(uint messagePhases, uint phaseIndex, uint dwIndex)
{
    return __builtin_IB_get_message_phase_dw(messagePhases, phaseIndex, dwIndex);
}
static INLINE OVERLOADABLE uint intel_get_message_phase_dw(uint2 messagePhases, uint phaseIndex, uint dwIndex)
{
    return __builtin_IB_get_message_phase_dw_uint2(messagePhases, phaseIndex, dwIndex);
}
static INLINE OVERLOADABLE uint intel_get_message_phase_dw(uint4 messagePhases, uint phaseIndex, uint dwIndex)
{
    return __builtin_IB_get_message_phase_dw_uint4(messagePhases, phaseIndex, dwIndex);
}
static INLINE OVERLOADABLE uint intel_get_message_phase_dw(uint8 messagePhases, uint phaseIndex, uint dwIndex)
{
    return __builtin_IB_get_message_phase_dw_uint8(messagePhases, phaseIndex, dwIndex);
}

static INLINE OVERLOADABLE ulong intel_get_message_phase_uq(uint messagePhases, uint phaseIndex, uint dwIndex)
{
    return __builtin_IB_get_message_phase_uq(messagePhases, phaseIndex, dwIndex);
}
static INLINE OVERLOADABLE ulong intel_get_message_phase_uq(uint2 messagePhases, uint phaseIndex, uint dwIndex)
{
    return __builtin_IB_get_message_phase_uq_uint2(messagePhases, phaseIndex, dwIndex);
}
static INLINE OVERLOADABLE ulong intel_get_message_phase_uq(uint4 messagePhases, uint phaseIndex, uint dwIndex)
{
    return __builtin_IB_get_message_phase_uq_uint4(messagePhases, phaseIndex, dwIndex);
}
static INLINE OVERLOADABLE ulong intel_get_message_phase_uq(uint8 messagePhases, uint phaseIndex, uint dwIndex)
{
    return __builtin_IB_get_message_phase_uq_uint8(messagePhases, phaseIndex, dwIndex);
}

static INLINE OVERLOADABLE uint intel_set_message_phase_dw(uint messagePhases, uint phaseIndex, uint dwIndex, uint val)
{
    return __builtin_IB_set_message_phase_dw(messagePhases, phaseIndex, dwIndex, val);
}
static INLINE OVERLOADABLE uint2 intel_set_message_phase_dw(uint2 messagePhases, uint phaseIndex, uint dwIndex, uint val)
{
    return __builtin_IB_set_message_phase_dw_uint2(messagePhases, phaseIndex, dwIndex, val);
}
static INLINE OVERLOADABLE uint4 intel_set_message_phase_dw(uint4 messagePhases, uint phaseIndex, uint dwIndex, uint val)
{
    return __builtin_IB_set_message_phase_dw_uint4(messagePhases, phaseIndex, dwIndex, val);
}
static INLINE OVERLOADABLE uint8 intel_set_message_phase_dw(uint8 messagePhases, uint phaseIndex, uint dwIndex, uint val)
{
    return __builtin_IB_set_message_phase_dw_uint8(messagePhases, phaseIndex, dwIndex, val);
}

static INLINE OVERLOADABLE uint intel_get_message_phase(uint messagePhases, uint phaseIndex)
{
    return __builtin_IB_get_message_phase(messagePhases, phaseIndex);
}
static INLINE OVERLOADABLE uint intel_get_message_phase(uint2 messagePhases, uint phaseIndex)
{
    return __builtin_IB_get_message_phase_uint2(messagePhases, phaseIndex);
}
static INLINE OVERLOADABLE uint intel_get_message_phase(uint4 messagePhases, uint phaseIndex)
{
    return __builtin_IB_get_message_phase_uint4(messagePhases, phaseIndex);
}
static INLINE OVERLOADABLE uint intel_get_message_phase(uint8 messagePhases, uint phaseIndex)
{
    return __builtin_IB_get_message_phase_uint8(messagePhases, phaseIndex);
}

static INLINE OVERLOADABLE uint intel_set_message_phase(uint messagePhases, uint phaseIndex, uint val)
{
    return __builtin_IB_set_message_phase(messagePhases, phaseIndex, val);
}
static INLINE OVERLOADABLE uint2 intel_set_message_phase(uint2 messagePhases, uint phaseIndex, uint val)
{
    return __builtin_IB_set_message_phase_uint2(messagePhases, phaseIndex, val);
}
static INLINE OVERLOADABLE uint4 intel_set_message_phase(uint4 messagePhases, uint phaseIndex, uint val)
{
    return __builtin_IB_set_message_phase_uint4(messagePhases, phaseIndex, val);
}
static INLINE OVERLOADABLE uint8 intel_set_message_phase(uint8 messagePhases, uint phaseIndex, uint val)
{
    return __builtin_IB_set_message_phase_uint8(messagePhases, phaseIndex, val);
}

static INLINE OVERLOADABLE ushort intel_get_message_phase_uw(uint messagePhases, uint phaseIndex, uint wIndex)
{
    return __builtin_IB_get_message_phase_uw(messagePhases, phaseIndex, wIndex);
}
static INLINE OVERLOADABLE ushort intel_get_message_phase_uw(uint2 messagePhases, uint phaseIndex, uint wIndex)
{
    return __builtin_IB_get_message_phase_uw_uint2(messagePhases, phaseIndex, wIndex);
}
static INLINE OVERLOADABLE ushort intel_get_message_phase_uw(uint4 messagePhases, uint phaseIndex, uint wIndex)
{
    return __builtin_IB_get_message_phase_uw_uint4(messagePhases, phaseIndex, wIndex);
}
static INLINE OVERLOADABLE ushort intel_get_message_phase_uw(uint8 messagePhases, uint phaseIndex, uint wIndex)
{
    return __builtin_IB_get_message_phase_uw_uint8(messagePhases, phaseIndex, wIndex);
}

static INLINE OVERLOADABLE uint intel_set_message_phase_uw(uint messagePhases, uint phaseIndex, uint dwIndex, ushort val)
{
    return __builtin_IB_set_message_phase_uw(messagePhases, phaseIndex, dwIndex, val);
}
static INLINE OVERLOADABLE uint2 intel_set_message_phase_uw(uint2 messagePhases, uint phaseIndex, uint dwIndex, ushort val)
{
    return __builtin_IB_set_message_phase_uw_uint2(messagePhases, phaseIndex, dwIndex, val);
}
static INLINE OVERLOADABLE uint4 intel_set_message_phase_uw(uint4 messagePhases, uint phaseIndex, uint dwIndex, ushort val)
{
    return __builtin_IB_set_message_phase_uw_uint4(messagePhases, phaseIndex, dwIndex, val);
}
static INLINE OVERLOADABLE uint8 intel_set_message_phase_uw(uint8 messagePhases, uint phaseIndex, uint dwIndex, ushort val)
{
    return __builtin_IB_set_message_phase_uw_uint8(messagePhases, phaseIndex, dwIndex, val);
}

static INLINE OVERLOADABLE uchar intel_get_message_phase_ub(uint messagePhases, uint phaseIndex, uint dwIndex)
{
    return __builtin_IB_get_message_phase_ub(messagePhases, phaseIndex, dwIndex);
}
static INLINE OVERLOADABLE uchar intel_get_message_phase_ub(uint2 messagePhases, uint phaseIndex, uint dwIndex)
{
    return __builtin_IB_get_message_phase_ub_uint2(messagePhases, phaseIndex, dwIndex);
}
static INLINE OVERLOADABLE uchar intel_get_message_phase_ub(uint4 messagePhases, uint phaseIndex, uint dwIndex)
{
    return __builtin_IB_get_message_phase_ub_uint4(messagePhases, phaseIndex, dwIndex);
}
static INLINE OVERLOADABLE uchar intel_get_message_phase_ub(uint8 messagePhases, uint phaseIndex, uint dwIndex)
{
    return __builtin_IB_get_message_phase_ub_uint8(messagePhases, phaseIndex, dwIndex);
}

static INLINE OVERLOADABLE uint intel_set_message_phase_ub(uint messagePhases, uint phaseIndex, uint dwIndex, uchar val)
{
    return __builtin_IB_set_message_phase_ub(messagePhases, phaseIndex, dwIndex, val);
}
static INLINE OVERLOADABLE uint2 intel_set_message_phase_ub(uint2 messagePhases, uint phaseIndex, uint dwIndex, uchar val)
{
    return __builtin_IB_set_message_phase_ub_uint2(messagePhases, phaseIndex, dwIndex, val);
}
static INLINE OVERLOADABLE uint4 intel_set_message_phase_ub(uint4 messagePhases, uint phaseIndex, uint dwIndex, uchar val)
{
    return __builtin_IB_set_message_phase_ub_uint4(messagePhases, phaseIndex, dwIndex, val);
}
static INLINE OVERLOADABLE uint8 intel_set_message_phase_ub(uint8 messagePhases, uint phaseIndex, uint dwIndex, uchar val)
{
    return __builtin_IB_set_message_phase_ub_uint8(messagePhases, phaseIndex, dwIndex, val);
}

static INLINE OVERLOADABLE ulong intel_simd_get_message_phase_uq(uint payload, uint phaseIndex, uint numPhases)
{
    return __builtin_IB_simd_get_message_phase_uq(payload, phaseIndex, numPhases);
}
static INLINE OVERLOADABLE ulong intel_simd_get_message_phase_uq(uint2 payload, uint phaseIndex, uint numPhases)
{
    return __builtin_IB_simd_get_message_phase_uq_uint2(payload, phaseIndex, numPhases);
}
static INLINE OVERLOADABLE ulong intel_simd_get_message_phase_uq(uint4 payload, uint phaseIndex, uint numPhases)
{
    return __builtin_IB_simd_get_message_phase_uq_uint4(payload, phaseIndex, numPhases);
}
static INLINE OVERLOADABLE ulong intel_simd_get_message_phase_uq(uint8 payload, uint phaseIndex, uint numPhases)
{
    return __builtin_IB_simd_get_message_phase_uq_uint8(payload, phaseIndex, numPhases);
}

static INLINE OVERLOADABLE ushort intel_simd_get_message_phase_uw(uint payload, uint phaseIndex, uint numPhases)
{
    return __builtin_IB_simd_get_message_phase_uw(payload, phaseIndex, numPhases);
}
static INLINE OVERLOADABLE ushort intel_simd_get_message_phase_uw(uint2 payload, uint phaseIndex, uint numPhases)
{
    return __builtin_IB_simd_get_message_phase_uw_uint2(payload, phaseIndex, numPhases);
}
static INLINE OVERLOADABLE ushort intel_simd_get_message_phase_uw(uint4 payload, uint phaseIndex, uint numPhases)
{
    return __builtin_IB_simd_get_message_phase_uw_uint4(payload, phaseIndex, numPhases);
}
static INLINE OVERLOADABLE ushort intel_simd_get_message_phase_uw(uint8 payload, uint phaseIndex, uint numPhases)
{
    return __builtin_IB_simd_get_message_phase_uw_uint8(payload, phaseIndex, numPhases);
}

static INLINE OVERLOADABLE ushort intel_broadcast_message_phase_uw(uint payload, uint phaseIndex, uint phaseSubindex, uint width)
{
    return __builtin_IB_broadcast_message_phase_uw(payload, phaseIndex, phaseSubindex, width);
}
static INLINE OVERLOADABLE ushort intel_broadcast_message_phase_uw(uint2 payload, uint phaseIndex, uint phaseSubindex, uint width)
{
    return __builtin_IB_broadcast_message_phase_uw_uint2(payload, phaseIndex, phaseSubindex, width);
}
static INLINE OVERLOADABLE ushort intel_broadcast_message_phase_uw(uint4 payload, uint phaseIndex, uint phaseSubindex, uint width)
{
    return __builtin_IB_broadcast_message_phase_uw_uint4(payload, phaseIndex, phaseSubindex, width);
}
static INLINE OVERLOADABLE ushort intel_broadcast_message_phase_uw(uint8 payload, uint phaseIndex, uint phaseSubindex, uint width)
{
    return __builtin_IB_broadcast_message_phase_uw_uint8(payload, phaseIndex, phaseSubindex, width);
}

static INLINE OVERLOADABLE ulong intel_broadcast_message_phase_uq(uint payload, uint phaseIndex, uint phaseSubindex, uint width)
{
    return __builtin_IB_broadcast_message_phase_uq(payload, phaseIndex, phaseSubindex, width);
}
static INLINE OVERLOADABLE ulong intel_broadcast_message_phase_uq(uint2 payload, uint phaseIndex, uint phaseSubindex, uint width)
{
    return __builtin_IB_broadcast_message_phase_uq_uint2(payload, phaseIndex, phaseSubindex, width);
}
static INLINE OVERLOADABLE ulong intel_broadcast_message_phase_uq(uint4 payload, uint phaseIndex, uint phaseSubindex, uint width)
{
    return __builtin_IB_broadcast_message_phase_uq_uint4(payload, phaseIndex, phaseSubindex, width);
}
static INLINE OVERLOADABLE ulong intel_broadcast_message_phase_uq(uint8 payload, uint phaseIndex, uint phaseSubindex, uint width)
{
    return __builtin_IB_broadcast_message_phase_uq_uint8(payload, phaseIndex, phaseSubindex, width);
}

static INLINE OVERLOADABLE uchar intel_broadcast_message_phase_ub(uint payload, uint phaseIndex, uint phaseSubindex, uint width)
{
    return __builtin_IB_broadcast_message_phase_ub(payload, phaseIndex, phaseSubindex, width);
}
static INLINE OVERLOADABLE uchar intel_broadcast_message_phase_ub(uint2 payload, uint phaseIndex, uint phaseSubindex, uint width)
{
    return __builtin_IB_broadcast_message_phase_ub_uint2(payload, phaseIndex, phaseSubindex, width);
}
static INLINE OVERLOADABLE uchar intel_broadcast_message_phase_ub(uint4 payload, uint phaseIndex, uint phaseSubindex, uint width)
{
    return __builtin_IB_broadcast_message_phase_ub_uint4(payload, phaseIndex, phaseSubindex, width);
}
static INLINE OVERLOADABLE uchar intel_broadcast_message_phase_ub(uint8 payload, uint phaseIndex, uint phaseSubindex, uint width)
{
    return __builtin_IB_broadcast_message_phase_ub_uint8(payload, phaseIndex, phaseSubindex, width);
}

static INLINE OVERLOADABLE uint intel_broadcast_message_phase_dw(uint payload, uint phaseIndex, uint phaseSubindex, uint width)
{
    return __builtin_IB_broadcast_message_phase_dw(payload, phaseIndex, phaseSubindex, width);
}
static INLINE OVERLOADABLE uint intel_broadcast_message_phase_dw(uint2 payload, uint phaseIndex, uint phaseSubindex, uint width)
{
    return __builtin_IB_broadcast_message_phase_dw_uint2(payload, phaseIndex, phaseSubindex, width);
}
static INLINE OVERLOADABLE uint intel_broadcast_message_phase_dw(uint4 payload, uint phaseIndex, uint phaseSubindex, uint width)
{
    return __builtin_IB_broadcast_message_phase_dw_uint4(payload, phaseIndex, phaseSubindex, width);
}
static INLINE OVERLOADABLE uint intel_broadcast_message_phase_dw(uint8 payload, uint phaseIndex, uint phaseSubindex, uint width)
{
    return __builtin_IB_broadcast_message_phase_dw_uint8(payload, phaseIndex, phaseSubindex, width);
}

static INLINE OVERLOADABLE uint intel_simd_set_message_phase_uq(uint messagePhases, uint phaseIndex, uint numPhases, uint subReg, uint numLanes, ulong val)
{
    return __builtin_IB_simd_set_message_phase_uq(messagePhases, phaseIndex, numPhases, subReg, numLanes, val);
}
static INLINE OVERLOADABLE uint2 intel_simd_set_message_phase_uq(uint2 messagePhases, uint phaseIndex, uint numPhases, uint subReg, uint numLanes, ulong val)
{
    return __builtin_IB_simd_set_message_phase_uq_uint2(messagePhases, phaseIndex, numPhases, subReg, numLanes, val);
}
static INLINE OVERLOADABLE uint4 intel_simd_set_message_phase_uq(uint4 messagePhases, uint phaseIndex, uint numPhases, uint subReg, uint numLanes, ulong val)
{
    return __builtin_IB_simd_set_message_phase_uq_uint4(messagePhases, phaseIndex, numPhases, subReg, numLanes, val);
}
static INLINE OVERLOADABLE uint8 intel_simd_set_message_phase_uq(uint8 messagePhases, uint phaseIndex, uint numPhases, uint subReg, uint numLanes, ulong val)
{
    return __builtin_IB_simd_set_message_phase_uq_uint8(messagePhases, phaseIndex, numPhases, subReg, numLanes, val);
}

static INLINE OVERLOADABLE uint intel_simd_set_message_phase_dw(uint messagePhases, uint phaseIndex, uint numPhases, uint subReg, uint numLanes, ulong val)
{
    return __builtin_IB_simd_set_message_phase_dw(messagePhases, phaseIndex, numPhases, subReg, numLanes, val);
}
static INLINE OVERLOADABLE uint2 intel_simd_set_message_phase_dw(uint2 messagePhases, uint phaseIndex, uint numPhases, uint subReg, uint numLanes, ulong val)
{
    return __builtin_IB_simd_set_message_phase_dw_uint2(messagePhases, phaseIndex, numPhases, subReg, numLanes, val);
}
static INLINE OVERLOADABLE uint4 intel_simd_set_message_phase_dw(uint4 messagePhases, uint phaseIndex, uint numPhases, uint subReg, uint numLanes, ulong val)
{
    return __builtin_IB_simd_set_message_phase_dw_uint4(messagePhases, phaseIndex, numPhases, subReg, numLanes, val);
}
static INLINE OVERLOADABLE uint8 intel_simd_set_message_phase_dw(uint8 messagePhases, uint phaseIndex, uint numPhases, uint subReg, uint numLanes, ulong val)
{
    return __builtin_IB_simd_set_message_phase_dw_uint8(messagePhases, phaseIndex, numPhases, subReg, numLanes, val);
}

static INLINE OVERLOADABLE uint intel_simd_set_message_phase_ub(uint messagePhases, uint phaseIndex, uint numPhases, uint subReg, uint numLanes, ulong val)
{
    return __builtin_IB_simd_set_message_phase_ub(messagePhases, phaseIndex, numPhases, subReg, numLanes, val);
}
static INLINE OVERLOADABLE uint2 intel_simd_set_message_phase_ub(uint2 messagePhases, uint phaseIndex, uint numPhases, uint subReg, uint numLanes, ulong val)
{
    return __builtin_IB_simd_set_message_phase_ub_uint2(messagePhases, phaseIndex, numPhases, subReg, numLanes, val);
}
static INLINE OVERLOADABLE uint4 intel_simd_set_message_phase_ub(uint4 messagePhases, uint phaseIndex, uint numPhases, uint subReg, uint numLanes, ulong val)
{
    return __builtin_IB_simd_set_message_phase_ub_uint4(messagePhases, phaseIndex, numPhases, subReg, numLanes, val);
}
static INLINE OVERLOADABLE uint8 intel_simd_set_message_phase_ub(uint8 messagePhases, uint phaseIndex, uint numPhases, uint subReg, uint numLanes, ulong val)
{
    return __builtin_IB_simd_set_message_phase_ub_uint8(messagePhases, phaseIndex, numPhases, subReg, numLanes, val);
}

static INLINE OVERLOADABLE uint intel_simd_set_message_phase_uw(uint messagePhases, uint phaseIndex, uint numPhases, uint subReg, uint numLanes, ulong val)
{
    return __builtin_IB_simd_set_message_phase_uw(messagePhases, phaseIndex, numPhases, subReg, numLanes, val);
}
static INLINE OVERLOADABLE uint2 intel_simd_set_message_phase_uw(uint2 messagePhases, uint phaseIndex, uint numPhases, uint subReg, uint numLanes, ulong val)
{
    return __builtin_IB_simd_set_message_phase_uw_uint2(messagePhases, phaseIndex, numPhases, subReg, numLanes, val);
}
static INLINE OVERLOADABLE uint4 intel_simd_set_message_phase_uw(uint4 messagePhases, uint phaseIndex, uint numPhases, uint subReg, uint numLanes, ulong val)
{
    return __builtin_IB_simd_set_message_phase_uw_uint4(messagePhases, phaseIndex, numPhases, subReg, numLanes, val);
}
static INLINE OVERLOADABLE uint8 intel_simd_set_message_phase_uw(uint8 messagePhases, uint phaseIndex, uint numPhases, uint subReg, uint numLanes, ulong val)
{
    return __builtin_IB_simd_set_message_phase_uw_uint8(messagePhases, phaseIndex, numPhases, subReg, numLanes, val);
}

static INLINE uint
__calc_dual_ref_id_dword(
    long src_image,
    long fwd_ref_image,
    long bwd_ref_image)
{
    uint srcBTI = __builtin_IB_get_image_bti(src_image);
    uint fwdBTI = __builtin_IB_get_image_bti(fwd_ref_image);
    uint bwdBTI = __builtin_IB_get_image_bti(bwd_ref_image);

    uint imm0 = (fwdBTI - srcBTI - 1) >> 1;
    uint imm1 = (bwdBTI - srcBTI - 1) >> 1;

    uint imm = (imm0 << 0) | (imm1 << 4);
    imm |= ( imm << 8 );
    imm |= ( imm << 16 );

    return imm;
}

static INLINE uint
__calc_single_ref_id_dword(
    long src_image,
    long fwd_ref_image)
{
    uint srcBTI = __builtin_IB_get_image_bti(src_image);
    uint fwdBTI = __builtin_IB_get_image_bti(fwd_ref_image);

    uint imm = (fwdBTI - srcBTI - 1) >> 1;

    imm |= ( imm << 4 );
    imm |= ( imm << 8 );
    imm |= ( imm << 16 );

    return imm;
}

static INLINE uint4 OVERLOADABLE
intel_sub_group_payload_set_single_ref_id(
    long src_image,
    long fwd_ref_image,
    uint4 payload )
{
    uint imm = __calc_single_ref_id_dword(src_image, fwd_ref_image);
    return intel_set_message_phase_dw(payload, 1, 6, imm);
}

static INLINE uint8 OVERLOADABLE
intel_sub_group_payload_set_single_ref_id(
    long src_image,
    long fwd_ref_image,
    uint8 payload )
{
    uint imm = __calc_single_ref_id_dword(src_image, fwd_ref_image);
    return intel_set_message_phase_dw(payload, 1, 6, imm);
}

static INLINE uint4 OVERLOADABLE
intel_sub_group_payload_set_dual_ref_id(
    long src_image,
    long fwd_ref_image,
    long bwd_ref_image,
    uint4 payload )
{
    uint imm = __calc_dual_ref_id_dword(src_image, fwd_ref_image, bwd_ref_image);
    return intel_set_message_phase_dw(payload, 1, 6, imm);
}

static INLINE uint8 OVERLOADABLE
intel_sub_group_payload_set_dual_ref_id(
    long src_image,
    long fwd_ref_image,
    long bwd_ref_image,
    uint8 payload )
{
    uint imm = __calc_dual_ref_id_dword(src_image, fwd_ref_image, bwd_ref_image);
    return intel_set_message_phase_dw(payload, 1, 6, imm);
}

/*****************************************************************************\

Description:
    Set ref field polarities.
    - set RefAccess (M0.3 :7)
    - set RefIdpolarity (M1.1 :15:8)

\*****************************************************************************/
static INLINE uint4 OVERLOADABLE
intel_sub_group_payload_set_ref_id_polarities_raw(
    uchar ref_field_polarity,
    uint4 payload)
{
    uint val = intel_get_message_phase_dw(payload, 0, 3);
    val |= (0x1 << 7);
    payload = intel_set_message_phase_dw(payload, 0, 3, val);

    payload = intel_set_message_phase_ub(payload, 1, 1*4+1, ref_field_polarity);

    return payload;
}

/*****************************************************************************\

Description:
    - set ref ids (M1.6)

\*****************************************************************************/
static INLINE uint4 OVERLOADABLE
intel_sub_group_payload_set_ref_id_raw(
    uint packed_ref_ids,
    uint4 payload)
{
    return intel_set_message_phase_dw(payload, 1, 6, packed_ref_ids);
}


// end helper overloads

/////////////////////// User visible functions ///////////////////////

uint2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SubgroupAvcMceGetDefaultHighPenaltyCostTableINTEL, , )( )
{
    uint2 costTable;
    costTable.s0 = 0x4E483D1D;
    costTable.s1 = 0x5C5B5958;
    return costTable;
}

INLINE uint2  OVERLOADABLE
intel_sub_group_avc_mce_get_default_high_penalty_cost_table(void)
{
    return SPIRV_BUILTIN(SubgroupAvcMceGetDefaultHighPenaltyCostTableINTEL, , )();
}

uint2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SubgroupAvcMceGetDefaultMediumPenaltyCostTableINTEL, , )( )
{
   uint2 costTable;
   costTable.s0 = 0x2B1D1A05;
   costTable.s1 = 0x39392F2D;
   return costTable;
}

INLINE uint2  OVERLOADABLE
intel_sub_group_avc_mce_get_default_medium_penalty_cost_table(void)
{
    return SPIRV_BUILTIN(SubgroupAvcMceGetDefaultMediumPenaltyCostTableINTEL, , )();
}

uint2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SubgroupAvcMceGetDefaultLowPenaltyCostTableINTEL, , )( )
{
   uint2 costTable;
   costTable.s0 = 0x09050401;
   costTable.s1 = 0x0F0E0C0A;
   return costTable;
}

INLINE uint2  OVERLOADABLE
intel_sub_group_avc_mce_get_default_low_penalty_cost_table(void)
{
    return SPIRV_BUILTIN(SubgroupAvcMceGetDefaultLowPenaltyCostTableINTEL, , )();
}

/*****************************************************************************\

Description:
    Setup motion vector costing for the search or check.

    ExtendedCostRange has already been shifted by 6 bits.

    - initialize M0 to M2 from payload src
    - set Cost Center (M3.0-M3.7)
    - set Cost Table (M2.3, M2.4)
    - set MV Cost Scale (M1.7 17:16)
    - set NonSkipZMVAdded (M1.7 :5)

\*****************************************************************************/
__spirv_AvcMcePayloadINTEL SPIRV_OVERLOADABLE SPIRV_BUILTIN(SubgroupAvcMceSetMotionVectorCostFunctionINTEL, _i64_v2i32_i8_i64, )(
    ulong packed_cost_center_delta,
    uint2 packed_cost_table,
    uchar cost_precision,
    __spirv_AvcMcePayloadINTEL payload )
{

  uint4 handle = __builtin_IB_vme_helper_get_handle_avc_mce_payload_t(payload);

  // Set Cost Center Delta (M3.0-M3.7)
  const uint FWDCostCenter = (uint)packed_cost_center_delta;
  const uint BWDCostCenter = (uint)(packed_cost_center_delta >> 32);

  for (int comp = 0; comp <= 7; comp += 2) {
    handle = intel_set_message_phase_dw(handle, 3, comp,   FWDCostCenter);
    handle = intel_set_message_phase_dw(handle, 3, comp+1, BWDCostCenter);
  }

  // Set Cost Table (M2.3, M2.4)
  handle = intel_set_message_phase_dw(handle, 2, 3, packed_cost_table.x);
  handle = intel_set_message_phase_dw(handle, 2, 4, packed_cost_table.y);

  // Set MV Cost Scale M1.7[17:16]
  ushort MVCostScaleFactor = intel_get_message_phase_uw(handle, 1, 7 * 2 + 1) | cost_precision;
  handle = intel_set_message_phase_uw(handle, 1, 7 * 2 + 1, MVCostScaleFactor);

  // Set NonSkipZMVAdded M1.7[5:5].
  const uint NonSkipZMvAdded = intel_get_message_phase_dw(handle, 1, 7) | 0x20;
  handle = intel_set_message_phase_dw(handle, 1, 7, NonSkipZMvAdded);

  return __builtin_IB_vme_helper_get_as_avc_mce_payload_t( handle );
}

INLINE intel_sub_group_avc_mce_payload_t OVERLOADABLE
intel_sub_group_avc_mce_set_motion_vector_cost_function(
      ulong packed_cost_center_delta,
      uint2 packed_cost_table,
      uchar cost_precision,
      intel_sub_group_avc_mce_payload_t payload )
{
    __spirv_AvcMcePayloadINTEL spv_payload = __builtin_astype(payload, __spirv_AvcMcePayloadINTEL);
    __spirv_AvcMcePayloadINTEL ret = SPIRV_BUILTIN(SubgroupAvcMceSetMotionVectorCostFunctionINTEL, _i64_v2i32_i8_i64, )(packed_cost_center_delta, packed_cost_table, cost_precision, spv_payload);
    return __builtin_astype(ret, intel_sub_group_avc_mce_payload_t);
}

uint2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SubgroupAvcMceGetDefaultInterMotionVectorCostTableINTEL, _i8_i8, )(
    uchar slice_type,
    uchar qp )
{
    uint2 penalty = 0;
    if( slice_type ==  CLK_AVC_ME_SLICE_TYPE_PRED_INTEL )
    {
        uint2 penalty_table[52] =
        {
            { 0x09050401, 0x0f0e0c0a }, { 0x09050401, 0x0f0e0c0a }, { 0x09050401, 0x0f0e0c0a }, { 0x09050401, 0x0f0e0c0a },
            { 0x09050401, 0x0f0e0c0a }, { 0x09050401, 0x0f0e0c0a }, { 0x09050401, 0x0f0e0c0a }, { 0x09050401, 0x0f0e0c0a },
            { 0x09050401, 0x0f0e0c0a }, { 0x09050401, 0x0f0e0c0a }, { 0x09050401, 0x0f0e0c0a }, { 0x09050401, 0x0f0e0c0a },
            { 0x09050401, 0x0f0e0c0a }, { 0x09050401, 0x0f0e0c0a }, { 0x09050401, 0x0f0e0c0a }, { 0x09050401, 0x0f0e0c0a },
            { 0x190a0802, 0x1f1e1c1a }, { 0x190a0802, 0x1f1e1c1a }, { 0x190a0802, 0x1f1e1c1a }, { 0x190a0802, 0x1f1e1c1a },
            { 0x1e0f0c03, 0x2b2b291f }, { 0x1e0f0c03, 0x2b2b291f }, { 0x1e0f0c03, 0x2b2b291f }, { 0x291a1804, 0x2f2e2c2a },
            { 0x291a1804, 0x2f2e2c2a }, { 0x291a1804, 0x2f2e2c2a }, { 0x2b1d1a05, 0x39392f2d }, { 0x2e1f1c06, 0x3b3b392f },
            { 0x2e1f1c06, 0x3b3b392f }, { 0x38291e07, 0x3d3c3b39 }, { 0x392a2808, 0x3f3e3c3a }, { 0x3a2b2909, 0x48483e3b },
            { 0x3b2d2a0a, 0x49493f3d }, { 0x3c2e2b0b, 0x4a4a483e }, { 0x3f382d0d, 0x4c4b4a48 }, { 0x48392e0e, 0x4d4c4b49 },
            { 0x493a3818, 0x4f4e4c4a }, { 0x4a3b3919, 0x58584e4b }, { 0x4b3d3a1a, 0x59594f4d }, { 0x4d3e3c1c, 0x5b5a594e },
            { 0x4e483d1d, 0x5c5b5958 }, { 0x58493f1f, 0x5e5d5b59 }, { 0x594a4828, 0x5f5e5c5a }, { 0x5a4b4929, 0x68685e5b },
            { 0x5b4d4a2a, 0x69695f5d }, { 0x5d4e4b2b, 0x6b6a685e }, { 0x5e584d2d, 0x6c6b6a68 }, { 0x68594e2e, 0x6d6c6b69 },
            { 0x695a5838, 0x6f6e6c6a }, { 0x6a5b5939, 0x6f6f6e6b }, { 0x6b5d5a3a, 0x6f6f6f6d }, { 0x6d5e5b3b, 0x6f6f6f6e }
        };
        penalty = penalty_table[qp];
    }
    else if( slice_type ==  CLK_AVC_ME_SLICE_TYPE_BPRED_INTEL )
    {
        uint2 penalty_table[52] =
        {
            { 0x06020200, 0x180e0c0a }, { 0x06020200, 0x180e0c0a }, { 0x06020200, 0x180e0c0a }, { 0x06020200, 0x180e0c0a },
            { 0x06020200, 0x180e0c0a }, { 0x06020200, 0x180e0c0a }, { 0x06020200, 0x180e0c0a }, { 0x06020200, 0x180e0c0a },
            { 0x06020200, 0x180e0c0a }, { 0x06020200, 0x180e0c0a }, { 0x06020200, 0x180e0c0a }, { 0x06020200, 0x180e0c0a },
            { 0x06020200, 0x180e0c0a }, { 0x06020200, 0x180e0c0a }, { 0x06020200, 0x180e0c0a }, { 0x06020200, 0x180e0c0a },
            { 0x0c040400, 0x281e1c1a }, { 0x0c040400, 0x281e1c1a }, { 0x0c040400, 0x281e1c1a }, { 0x0c040400, 0x281e1c1a },
            { 0x19060600, 0x2c2b291f }, { 0x19060600, 0x2c2b291f }, { 0x19060600, 0x2c2b291f }, { 0x1c080800, 0x382e2c2a },
            { 0x1c080800, 0x382e2c2a }, { 0x1c080800, 0x382e2c2a }, { 0x1f0a0a00, 0x3a392f2d }, { 0x290c0c00, 0x3c3b392f },
            { 0x290c0c00, 0x3c3b392f }, { 0x2b0e0e00, 0x3e3c3b39 }, { 0x2c181800, 0x483e3c3a }, { 0x2e191900, 0x49483e3b },
            { 0x2f1a1a00, 0x4a493f3d }, { 0x381b1b00, 0x4b4a483e }, { 0x3a1d1d00, 0x4d4b4a48 }, { 0x3b1e1e00, 0x4e4c4b49 },
            { 0x3c282800, 0x584e4c4a }, { 0x3e292900, 0x59584e4b }, { 0x3f2a2a00, 0x5a594f4d }, { 0x492c2c00, 0x5c5a594e },
            { 0x492d2d00, 0x5d5b5958 }, { 0x4b2f2f00, 0x5f5d5b59 }, { 0x4c383800, 0x685e5c5a }, { 0x4e393900, 0x69685e5b },
            { 0x4f3a3a00, 0x6a695f5d }, { 0x583b3b00, 0x6b6a685e }, { 0x5a3d3d00, 0x6d6b6a68 }, { 0x5b3e3e00, 0x6e6c6b69 },
            { 0x5c484800, 0x6f6e6c6a }, { 0x5e494900, 0x6f6f6e6b }, { 0x5f4a4a00, 0x6f6f6f6d }, { 0x694b4b00, 0x6f6f6f6e }
        };
        penalty = penalty_table[qp];
    }
    else
    {
        penalty = 0;
    }

    return penalty;
}

INLINE uint2 OVERLOADABLE
intel_sub_group_avc_mce_get_default_inter_motion_vector_cost_table(
    uchar slice_type,
    uchar qp )
{
    return SPIRV_BUILTIN(SubgroupAvcMceGetDefaultInterMotionVectorCostTableINTEL, _i8_i8, )(slice_type, qp);
}

/*****************************************************************************\

Description:
    Enable AC only Haar transform.
    - set AConlyHAAR M1.7[21:21]

\*****************************************************************************/
__spirv_AvcMcePayloadINTEL SPIRV_OVERLOADABLE SPIRV_BUILTIN(SubgroupAvcMceSetAcOnlyHaarINTEL, _i64, )(
    __spirv_AvcMcePayloadINTEL payload )
{
    uint4 handle = __builtin_IB_vme_helper_get_handle_avc_mce_payload_t(payload);

    const uint AConlyHAAR = intel_get_message_phase_dw(handle, 1, 7) | (1<<21);
    handle = intel_set_message_phase_dw(handle, 1, 7, AConlyHAAR);

    return __builtin_IB_vme_helper_get_as_avc_mce_payload_t( handle );
}

INLINE intel_sub_group_avc_mce_payload_t OVERLOADABLE
intel_sub_group_avc_mce_set_ac_only_haar(
      intel_sub_group_avc_mce_payload_t payload )
{
    __spirv_AvcMcePayloadINTEL spv_payload = __builtin_astype(payload, __spirv_AvcMcePayloadINTEL);
    __spirv_AvcMcePayloadINTEL ret = SPIRV_BUILTIN(SubgroupAvcMceSetAcOnlyHaarINTEL, _i64, )(spv_payload);
    return __builtin_astype(ret, intel_sub_group_avc_mce_payload_t);
}

/*****************************************************************************\

Description:
    Set field polarities.
    - Set SrcAccess (M0.3 : 6)
    - Set SrcFieldPolarity (M1.7 : 19)

\*****************************************************************************/
__spirv_AvcMcePayloadINTEL SPIRV_OVERLOADABLE SPIRV_BUILTIN(SubgroupAvcMceSetSourceInterlacedFieldPolarityINTEL, _i8_i64, )(
    uchar src_field_polarity,
    __spirv_AvcMcePayloadINTEL payload )
{
    uint4 handle = __builtin_IB_vme_helper_get_handle_avc_mce_payload_t(payload);

    uint val = intel_get_message_phase_dw(handle, 0, 3);
    val |= (0x1 << 6);
    handle = intel_set_message_phase_dw(handle, 0, 3, val);

    val = intel_get_message_phase_dw(handle, 1, 7);
    val &= ~(1 << 19);
    val |= (src_field_polarity << 19);
    handle = intel_set_message_phase_dw(handle, 1, 7, val);

    return __builtin_IB_vme_helper_get_as_avc_mce_payload_t( handle );
}

INLINE intel_sub_group_avc_mce_payload_t OVERLOADABLE
intel_sub_group_avc_mce_set_source_interlaced_field_polarity(
     uchar  src_field_polarity,
     intel_sub_group_avc_mce_payload_t payload )
{
    __spirv_AvcMcePayloadINTEL spv_payload = __builtin_astype(payload, __spirv_AvcMcePayloadINTEL);
    __spirv_AvcMcePayloadINTEL ret = SPIRV_BUILTIN(SubgroupAvcMceSetSourceInterlacedFieldPolarityINTEL, _i8_i64, )(src_field_polarity, spv_payload);
    return __builtin_astype(ret, intel_sub_group_avc_mce_payload_t);
}

/*****************************************************************************\

Description:
    Set ref field polarities.
    - set RefAccess (M0.3 :7)
    - set RefIdpolarity (M1.1 :15:8)

\*****************************************************************************/
__spirv_AvcMcePayloadINTEL SPIRV_OVERLOADABLE SPIRV_BUILTIN(SubgroupAvcMceSetSingleReferenceInterlacedFieldPolarityINTEL, _i8_i64, )(
    uchar ref_field_polarity,
    __spirv_AvcMcePayloadINTEL payload )
{
    ref_field_polarity |= ( ref_field_polarity << 1 );
    ref_field_polarity |= ( ref_field_polarity << 2 );

    uint4 p = __builtin_IB_vme_helper_get_handle_avc_mce_payload_t(payload);
    uint4 handle = intel_sub_group_payload_set_ref_id_polarities_raw(
        ref_field_polarity,
        p);

    return __builtin_IB_vme_helper_get_as_avc_mce_payload_t( handle );
}

INLINE intel_sub_group_avc_mce_payload_t OVERLOADABLE
intel_sub_group_avc_mce_set_single_reference_interlaced_field_polarity(
     uchar  ref_field_polarity,
     intel_sub_group_avc_mce_payload_t payload )
{
    __spirv_AvcMcePayloadINTEL ret = SPIRV_BUILTIN(SubgroupAvcMceSetSingleReferenceInterlacedFieldPolarityINTEL, _i8_i64, )(ref_field_polarity, __builtin_astype(payload, __spirv_AvcMcePayloadINTEL));
    return __builtin_astype(ret, intel_sub_group_avc_mce_payload_t);
}

__spirv_AvcMcePayloadINTEL SPIRV_OVERLOADABLE SPIRV_BUILTIN(SubgroupAvcMceSetDualReferenceInterlacedFieldPolaritiesINTEL, _i8_i8_i64, )(
    uchar fwd_ref_field_polarity,
    uchar bwd_ref_field_polarity,
    __spirv_AvcMcePayloadINTEL payload )
{
    fwd_ref_field_polarity |= ( fwd_ref_field_polarity << 1 );
    fwd_ref_field_polarity |= ( fwd_ref_field_polarity << 2 );
    bwd_ref_field_polarity |= ( bwd_ref_field_polarity << 1 );
    bwd_ref_field_polarity |= ( bwd_ref_field_polarity << 2 );

    uchar  ref_field_polarity =
        ( fwd_ref_field_polarity << 0 ) | ( bwd_ref_field_polarity << 4 );

    uint4 p = __builtin_IB_vme_helper_get_handle_avc_mce_payload_t(payload);
    uint4 handle = intel_sub_group_payload_set_ref_id_polarities_raw(
        ref_field_polarity,
        p);

    return __builtin_IB_vme_helper_get_as_avc_mce_payload_t( handle );
}

INLINE intel_sub_group_avc_mce_payload_t OVERLOADABLE
intel_sub_group_avc_mce_set_dual_reference_interlaced_field_polarities(
     uchar  fwd_ref_field_polarity,
     uchar  bwd_ref_field_polarity,
     intel_sub_group_avc_mce_payload_t payload )
{
    __spirv_AvcMcePayloadINTEL ret = SPIRV_BUILTIN(SubgroupAvcMceSetDualReferenceInterlacedFieldPolaritiesINTEL, _i8_i8_i64, )(
        fwd_ref_field_polarity,
        bwd_ref_field_polarity,
        __builtin_astype(payload, __spirv_AvcMcePayloadINTEL));

    return __builtin_astype(ret, intel_sub_group_avc_mce_payload_t);
}

/*****************************************************************************\

Description:
    Get the motion vectors (W1 W2 W3 W4) from IME result payload.

\*****************************************************************************/
ulong SPIRV_OVERLOADABLE SPIRV_BUILTIN(SubgroupAvcMceGetMotionVectorsINTEL, _i64, )(
    __spirv_AvcMceResultINTEL result )
{
    uint4 r = __builtin_IB_vme_helper_get_handle_avc_mce_result_t(result);
    const ulong MVb = intel_simd_get_message_phase_uq(r, 1, 4);
    return MVb;
}

INLINE ulong OVERLOADABLE
intel_sub_group_avc_mce_get_motion_vectors(
    intel_sub_group_avc_mce_result_t  result )
{
    return SPIRV_BUILTIN(SubgroupAvcMceGetMotionVectorsINTEL, _i64, )(__builtin_astype(result, __spirv_AvcMceResultINTEL));
}

/*****************************************************************************\

Description:
    Get the inter distortions (W5) from IME result payload.

\*****************************************************************************/
ushort SPIRV_OVERLOADABLE SPIRV_BUILTIN(SubgroupAvcMceGetInterDistortionsINTEL, _i64, )(
    __spirv_AvcMceResultINTEL result )
{
    return intel_simd_get_message_phase_uw(__builtin_IB_vme_helper_get_handle_avc_mce_result_t(result), 5, 1);
}

INLINE ushort OVERLOADABLE
intel_sub_group_avc_mce_get_inter_distortions(
    intel_sub_group_avc_mce_result_t  result )
{
    return SPIRV_BUILTIN(SubgroupAvcMceGetInterDistortionsINTEL, _i64, )(__builtin_astype(result, __spirv_AvcMceResultINTEL));
}

/*****************************************************************************\

Description:
    Get the inter best distortion W0.2[15:0] from IME result payload.

\*****************************************************************************/
ushort SPIRV_OVERLOADABLE SPIRV_BUILTIN(SubgroupAvcMceGetBestInterDistortionsINTEL, _i64, )(
    __spirv_AvcMceResultINTEL result )
{
    return intel_get_message_phase_uw(__builtin_IB_vme_helper_get_handle_avc_mce_result_t(result), 0, 2*2);
}

INLINE ushort OVERLOADABLE
intel_sub_group_avc_mce_get_best_inter_distortion(
    intel_sub_group_avc_mce_result_t  result )
{
    return SPIRV_BUILTIN(SubgroupAvcMceGetBestInterDistortionsINTEL, _i64, )(__builtin_astype(result, __spirv_AvcMceResultINTEL));
}

/*****************************************************************************\

Description:
    Get the inter major shape W0.0[1:0]) from IME result payload.

\*****************************************************************************/
uchar SPIRV_OVERLOADABLE SPIRV_BUILTIN(SubgroupAvcMceGetInterMajorShapeINTEL, _i64, )(
    __spirv_AvcMceResultINTEL result )
{
    const uchar InterMbMode = intel_get_message_phase_ub(__builtin_IB_vme_helper_get_handle_avc_mce_result_t(result), 0, 0) & 0x3;
    return InterMbMode;
}

INLINE uchar OVERLOADABLE
intel_sub_group_avc_mce_get_inter_major_shape(
    intel_sub_group_avc_mce_result_t  result )
{
    return SPIRV_BUILTIN(SubgroupAvcMceGetInterMajorShapeINTEL, _i64, )(__builtin_astype(result, __spirv_AvcMceResultINTEL));
}

/*****************************************************************************\

Description:
    Get the inter major shape W0.6[15:8] from IME result payload.

\*****************************************************************************/
uchar SPIRV_OVERLOADABLE SPIRV_BUILTIN(SubgroupAvcMceGetInterMinorShapeINTEL, _i64, )(
    __spirv_AvcMceResultINTEL result )
{
    const uchar SubMbShape = intel_get_message_phase_ub(__builtin_IB_vme_helper_get_handle_avc_mce_result_t(result), 0, 6*4+1);
    return SubMbShape;
}

INLINE uchar OVERLOADABLE
intel_sub_group_avc_mce_get_inter_minor_shapes(
    intel_sub_group_avc_mce_result_t  result )
{
    return SPIRV_BUILTIN(SubgroupAvcMceGetInterMinorShapeINTEL, _i64, )(__builtin_astype(result, __spirv_AvcMceResultINTEL));
}

/*****************************************************************************\

Description:
    Get the inter major shape W0.6[23:16] from IME result payload.

\*****************************************************************************/
uchar SPIRV_OVERLOADABLE SPIRV_BUILTIN(SubgroupAvcMceGetInterDirectionsINTEL, _i64, )(
    __spirv_AvcMceResultINTEL result )
{
    const uchar SubMbPredMode = intel_get_message_phase_ub(__builtin_IB_vme_helper_get_handle_avc_mce_result_t(result), 0, 6*4+2);
    return SubMbPredMode;
}

INLINE uchar OVERLOADABLE
intel_sub_group_avc_mce_get_inter_directions(
    intel_sub_group_avc_mce_result_t  result )
{
    return SPIRV_BUILTIN(SubgroupAvcMceGetInterDirectionsINTEL, _i64, )(__builtin_astype(result, __spirv_AvcMceResultINTEL));
}

/*****************************************************************************\

Description:
    Get the count of motion vectors (W0.0 :28:24) from MCE result payload.

\*****************************************************************************/
uchar SPIRV_OVERLOADABLE SPIRV_BUILTIN(SubgroupAvcMceGetInterMotionVectorCountINTEL, _i64, )(
    __spirv_AvcMceResultINTEL result )
{
    return intel_get_message_phase_ub(__builtin_IB_vme_helper_get_handle_avc_mce_result_t(result), 0, 0*4+3) & 0x1F;
}

INLINE uchar OVERLOADABLE
intel_sub_group_avc_mce_get_inter_motion_vector_count(
    intel_sub_group_avc_mce_result_t  result )
{
    return SPIRV_BUILTIN(SubgroupAvcMceGetInterMotionVectorCountINTEL, _i64, )(__builtin_astype(result, __spirv_AvcMceResultINTEL));
}

// ... IME functions ...

ushort2 OVERLOADABLE intel_sub_group_avc_ime_ref_window_size(
    uchar search_window_config,
    char dual_ref )
{
    ushort2 ref_window_size = 0;

    if( search_window_config == CLK_AVC_ME_SEARCH_WINDOW_EXHAUSTIVE_INTEL    ||
        search_window_config == CLK_AVC_ME_SEARCH_WINDOW_DIAMOND_INTEL       ||
        search_window_config == CLK_AVC_ME_SEARCH_WINDOW_LARGE_DIAMOND_INTEL ||
        search_window_config == CLK_AVC_ME_SEARCH_WINDOW_16x12_RADIUS_INTEL ) {
        if( dual_ref ) {
            ref_window_size.x = 32;
            ref_window_size.y = 32;
        }
        else {
            ref_window_size.x = 48;
            ref_window_size.y = 40;
        }
    }
    else if( search_window_config == CLK_AVC_ME_SEARCH_WINDOW_SMALL_INTEL ) {
        ref_window_size.x = 28;
        ref_window_size.y = 28;
    }
    else if( search_window_config == CLK_AVC_ME_SEARCH_WINDOW_TINY_INTEL       ||
             search_window_config == CLK_AVC_ME_SEARCH_WINDOW_4x4_RADIUS_INTEL ) {
        ref_window_size.x = 24;
        ref_window_size.y = 24;
    }
    else if( search_window_config == CLK_AVC_ME_SEARCH_WINDOW_EXTRA_TINY_INTEL ||
             search_window_config == CLK_AVC_ME_SEARCH_WINDOW_2x2_RADIUS_INTEL ) {
        ref_window_size.x = 20;
        ref_window_size.y = 20;
    }

    return ref_window_size;
}

// This function is marked as a deprecated in specification, keeping for backward compatibility.
INLINE ushort2 OVERLOADABLE
intel_sub_group_ime_ref_window_size(
  uchar search_window_config,
  char  dual_ref )
{
    return intel_sub_group_avc_ime_ref_window_size(search_window_config, dual_ref);
}

INLINE ushort2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SubgroupAvcImeRefWindowSizeINTEL, _i8_i8, )(
    uchar search_window_config,
    char dual_ref )
{
    return intel_sub_group_avc_ime_ref_window_size(search_window_config, dual_ref);
}

short2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SubgroupAvcImeAdjustRefOffsetINTEL, _v2i16_v2i16_v2i16_v2i16, )(
    short2 ref_offset,
    ushort2 src_coord_us,
    ushort2 ref_window_size_us,
    ushort2 frame_size_us )
{
    short2 src_coord = as_short2( src_coord_us );
    short2 ref_window_size = as_short2( ref_window_size_us );
    short2 frame_size = as_short2( frame_size_us );

    short2 block_size = 16;
    short2 search_window_size = ref_window_size - block_size;
    short2 ref_window_coord = src_coord - ( search_window_size >> 1 );

    if( ref_window_coord.x + ref_offset.x >= frame_size.x ) {
        ref_offset.x = frame_size.x - ref_window_coord.x - search_window_size.x;
    }
    else if( ref_window_coord.x + ref_offset.x + search_window_size.x < 0 ) {
        ref_offset.x = -ref_window_coord.x;
    }
    if( ref_window_coord.y + ref_offset.y >= frame_size.y ) {
        ref_offset.y = frame_size.y - ref_window_coord.y - search_window_size.y;
    }
    else if( ref_window_coord.y + ref_offset.y + search_window_size.y < 0 ) {
        ref_offset.y = -ref_window_coord.y;
    }

    return ref_offset;
}

INLINE short2 OVERLOADABLE
intel_sub_group_avc_ime_adjust_ref_offset(
   short2  ref_offset,
   ushort2 src_coord_us,
   ushort2 ref_window_size_us,
   ushort2 frame_size_us )
{
    return SPIRV_BUILTIN(SubgroupAvcImeAdjustRefOffsetINTEL, _v2i16_v2i16_v2i16_v2i16, )(ref_offset, src_coord_us, ref_window_size_us, frame_size_us);
}

/*****************************************************************************\

Initialize IME payload M0:
- clear M0 and M1
- set SrcX M0.2[15:0] and SrcY M0.2[31:16]
- set SubMbPartMask M0.3[30:24]
- set InterSAD M0.3[21:20]

\*****************************************************************************/
__spirv_AvcImePayloadINTEL SPIRV_OVERLOADABLE SPIRV_BUILTIN(SubgroupAvcImeInitializeINTEL, _v2i16_i8_i8, )(
    ushort2 src_coord,
    uchar partition_mask,
    uchar sad_adjustment )
{
    // Create and initialize IME payload
    uint4 payload = __builtin_IB_create_message_phases_uint4(UNIVERSAL_INPUT_MESSAGE_NUM_GRFS+4);

    // Set SrcX M0.2[15:0] and SrcY M0.2[31:16].
    payload = intel_set_message_phase_dw(payload, 0, 2, as_uint(src_coord));

    // Set SubMbPartMask M0.3[30:24] and InterSAD M0.3[21:20]
    const uint immValue = (partition_mask << 24) | (sad_adjustment << 20);
    payload = intel_set_message_phase_dw(payload, 0, 3, immValue);

    return __builtin_IB_vme_helper_get_as_avc_ime_payload_t(payload);
}

INLINE intel_sub_group_avc_ime_payload_t OVERLOADABLE
intel_sub_group_avc_ime_initialize(
    ushort2 src_coord,
    uchar partition_mask,
    uchar sad_adjustment )
{
    return __builtin_astype(SPIRV_BUILTIN(SubgroupAvcImeInitializeINTEL, _v2i16_i8_i8, )(src_coord, partition_mask, sad_adjustment), intel_sub_group_avc_ime_payload_t);
}

/*****************************************************************************\

Description:
    Set IME search config.

    - initialize M0 M1 from payload src
    - set MaxNumMVs M1.1[5:0]
    - set Ref0X, Ref0Y, Ref1X, Ref1Y (M0.0)
    - set RefHeight and RefWidth M0.5[16:31]
    - set LenSP & MaxNumSU M1.2[15:0]
    - set search path delta M4.0 and M5.0
    - set AdaptiveEn M1.0[1:1]
    - set SearchCtrl M0.3[10:8]

\*****************************************************************************/
__spirv_AvcImePayloadINTEL __intel_ime_set_reference_helper(
    short2 fwd_ref_offset,
    short2 bwd_ref_offset,
    uchar search_window_config,
    bool multiRef,
    __spirv_AvcImePayloadINTEL payload )
{
    uint4 handle = __builtin_IB_vme_helper_get_handle_avc_ime_payload_t(payload);

    // Set MaxNumMVs M1.1[5:0]
    handle = intel_set_message_phase_ub(handle, 1, 1*4, 0x20);

    // Set the Ref0, Ref0Y (M0.0),
    // RefHeight and RefWidth (M0.5 :16:31), and
    // LenSP & MaxNumSU M1.2[15:0]
    // based on search window config.

    const uint searchConfig = search_window_config;
    short2 offset = (short2)(0, 0);
    ushort dimXY = 0;
    uint startX = 0;
    uint startY = 0;
    ushort lenSPMaxNumSU = 0;
    uint adaptive = 0;

    if( searchConfig == CLK_AVC_ME_SEARCH_WINDOW_EXHAUSTIVE_INTEL || searchConfig == CLK_AVC_ME_SEARCH_WINDOW_16x12_RADIUS_INTEL )
    {
        if( multiRef == false )
        {
            offset = (short2)(-16, -12);
            dimXY = 0x2830;        // 48, 40
            lenSPMaxNumSU = 0x3030;
            // EXHAUSTIVE uses a spiral SP.
            if( searchConfig == CLK_AVC_ME_SEARCH_WINDOW_EXHAUSTIVE_INTEL )
            {
                startX = 4; // ((48-16) >> 3) & 0xf
                startY = 3; // ((40-16) >> 3) & 0xf
            }
        }
        else
        {
            offset = (short2)(-8, -8);
            dimXY = 0x2020;        // 32, 32
            lenSPMaxNumSU = 0x1010;
            // EXHAUSTIVE uses a spiral SP.
            if( searchConfig == CLK_AVC_ME_SEARCH_WINDOW_EXHAUSTIVE_INTEL )
            {
                startX = 2; // ((32-16) >> 3) & 0xf
                startY = 2; // ((32-16) >> 3) & 0xf
            }
        }
    }
    else if( searchConfig == CLK_AVC_ME_SEARCH_WINDOW_SMALL_INTEL )
    {
        offset = (short2)(-6, -6);
        dimXY = 0x1C1C;        // 28, 28
        lenSPMaxNumSU = 0x0909;
        // SMALL uses a spiral SP.
        startX = 1;
        startY = 1;
    }
    else if( searchConfig == CLK_AVC_ME_SEARCH_WINDOW_TINY_INTEL )
    {
        offset = (short2)(-4, -4);
        dimXY = 0x1818;       // 24, 24
        lenSPMaxNumSU = 0x404;
    }
    else if( searchConfig == CLK_AVC_ME_SEARCH_WINDOW_EXTRA_TINY_INTEL || searchConfig == CLK_AVC_ME_SEARCH_WINDOW_2x2_RADIUS_INTEL )
    {
        offset = (short2)(-2, -2);
        dimXY = 0x1414;        // 20, 20
        lenSPMaxNumSU = 0x0101;
    }
    // DIAMOND 48x40 or 32x32 for multiref
    else if( searchConfig == CLK_AVC_ME_SEARCH_WINDOW_DIAMOND_INTEL )
    {
        if( multiRef == false )
        {
            offset = (short2)(-16, -12);
            dimXY = 0x2830;        // 48, 40
            lenSPMaxNumSU = 0x3910;
            startX = 4;
            startY = 3;
        }
        else
        {
            offset = (short2)(-8, -8);
            dimXY = 0x2020;        // 32, 32
            lenSPMaxNumSU = 0x3907;
            startX = 2;
            startY = 2;
        }
        adaptive = 0x1;
    }
    // LARGE DIAMOND 48x40 or 32x32 for multiref
    else if( searchConfig == CLK_AVC_ME_SEARCH_WINDOW_LARGE_DIAMOND_INTEL )
    {
        if( multiRef == false )
        {
            offset = (short2)(-16, -12);
            dimXY = 0x2830;        // 48, 40
            lenSPMaxNumSU = 0x3920;
            startX = 4; // ((48-16) >> 3) & 0xf
            startY = 3; // ((40-16) >> 3) & 0xf
        }
        else
        {
            offset = (short2)(-8, -8);
            dimXY = 0x2020;        // 32, 32
            lenSPMaxNumSU = 0x390A;
            startX = 2; // ((32-16) >> 3) & 0xf
            startY = 2; // ((32-16) >> 3) & 0xf
        }
        adaptive = 0x1;
    }

    // Set the reference window ref0 offset M0.0
    const short2 Ref0 = fwd_ref_offset + offset;
    handle = intel_set_message_phase_dw(handle, 0, 0, as_uint(Ref0));

    // Set the reference window ref1 offset M0.1
    if (multiRef) {
        const short2 Ref1 = bwd_ref_offset + offset;
        handle = intel_set_message_phase_dw(handle, 0, 1, as_uint(Ref1));
    }

    // Set reference window height and width M0.5[16:31]
    handle = intel_set_message_phase_uw(handle, 0, 5*2+1, dimXY);

    // Set LenSP & MaxNumSU (M1.2)...
    handle = intel_set_message_phase_uw(handle, 1, 2*2, lenSPMaxNumSU);

    // Set search path delta (M4.0, M5.0)

    // EXHAUSTIVE and SMALL use spiral search paths.
    if( searchConfig == CLK_AVC_ME_SEARCH_WINDOW_EXHAUSTIVE_INTEL || searchConfig == CLK_AVC_ME_SEARCH_WINDOW_SMALL_INTEL )
    {
        const uint cSearchPathSpiral[14] =
        {
            0x0101F00F, 0x0F0F1010, 0xF0F0F00F, 0x01010101,
            0x10101010, 0x0F0F0F0F, 0xF0F0F00F, 0x0101F0F0,
            0x01010101, 0x10101010, 0x0F0F1010, 0x0F0F0F0F,
            0xF0F0F00F, 0xF0F0F0F0
        };
        // Set M4.0 - M4.7...
        for (int comp = 0; comp < 8; comp++) {
          handle = intel_set_message_phase_dw(handle, 4, comp, cSearchPathSpiral[comp]);
        }

        // Set M5.0 - M5.5...
        for (int comp = 8; comp < 14; comp++) {
          handle = intel_set_message_phase_dw(handle, 5, comp-8, cSearchPathSpiral[comp]);
        }

        // Clear M5.6-M5.7
        handle = intel_set_message_phase_dw(handle, 5, 6, 0);
        handle = intel_set_message_phase_dw(handle, 5, 7, 0);
    }
    // 16x12_RADIUS, 4x4_RADIUS & EXTRA TINY all use raster search paths.
    else if( searchConfig == CLK_AVC_ME_SEARCH_WINDOW_16x12_RADIUS_INTEL ||
             searchConfig == CLK_AVC_ME_SEARCH_WINDOW_2x2_RADIUS_INTEL   ||
             searchConfig == CLK_AVC_ME_SEARCH_WINDOW_EXTRA_TINY_INTEL )
    {
        const uint cSearchPathRaster[12] = {
            0x01010101, 0x10010101, 0x0f0f0f0f, 0x100f0f0f,
            0x01010101, 0x10010101, 0x0f0f0f0f, 0x100f0f0f,
            0x01010101, 0x10010101, 0x0f0f0f0f, 0x000f0f0f
        };

        // Set M4.0 - M4.7...
        for (int comp = 0; comp < 4; comp++) {
          handle = intel_set_message_phase_dw(handle, 4, comp,     cSearchPathRaster[comp]);
          handle = intel_set_message_phase_dw(handle, 4, comp + 4, cSearchPathRaster[comp]);
        }

        // Set M5.0-M5.2...
        handle = intel_set_message_phase_dw(handle, 5, 0, intel_get_message_phase_dw(handle, 4, 0));
        handle = intel_set_message_phase_dw(handle, 5, 1, intel_get_message_phase_dw(handle, 4, 1));
        handle = intel_set_message_phase_dw(handle, 5, 2, intel_get_message_phase_dw(handle, 4, 2));

        // Set M5.3...
        handle = intel_set_message_phase_dw(handle, 5, 3, cSearchPathRaster[11]);

        // Clear M5.4-M5.7
        for (int comp = 4; comp <= 7; ++comp) {
          handle = intel_set_message_phase_dw(handle, 5, comp, 0);
        }
    }
    else if( searchConfig == CLK_AVC_ME_SEARCH_WINDOW_TINY_INTEL )
    {
        const uint cSearchPathTiny = 0x000f1001;

        // Clear M4 and M5
        handle = intel_set_message_phase(handle, 4, 0);
        handle = intel_set_message_phase(handle, 5, 0);

        // Set M4.0...
        handle = intel_set_message_phase_dw(handle, 4, 0, cSearchPathTiny);
    }
    else if( searchConfig == CLK_AVC_ME_SEARCH_WINDOW_DIAMOND_INTEL || searchConfig == CLK_AVC_ME_SEARCH_WINDOW_LARGE_DIAMOND_INTEL )
    {
        uint cSearchPathDiamond[ 11 ] = {
            0x120FF10F, 0x1E22E20D, 0x20E2FF10, 0x2EDD06FC,
            0x11D33FF1, 0xEB1FF33D, 0x4EF1F1F1, 0xF1F21211,
            0x0DFFFFE0, 0x11201F1F, 0x1105F1CF };

        // Clear M5.0-M5.7.
        for (int comp = 0; comp <= 7; ++comp) {
          handle = intel_set_message_phase_dw(handle, 5, comp, 0);
        }

        // Set M4.0 - M4.7...
        for (int comp = 0; comp <= 7; comp++) {
          handle = intel_set_message_phase_dw(handle, 4, comp, cSearchPathDiamond[comp]);
        }

        // Set M5.0 - M5.2...
        for (int comp = 8; comp < 11; comp++) {
          handle = intel_set_message_phase_dw(handle, 5, comp-8, cSearchPathDiamond[comp]);
        }
    }

    // Set Start0X, Start0Y, Start1X, Start1Y M1.2[31:16]

    if (( startX | startY )) {
        ushort startXY = ( startX ) | ( startY << 4 );
        if( multiRef ) {
            startXY |= ( startXY << 8 );
        }
        handle = intel_set_message_phase_uw(handle, 1, 2*2+1, startXY);
    }

    // Set AdaptiveEn M1.0[1:1]

    if (adaptive != 0) {
        handle = intel_set_message_phase_ub(handle, 1, 0, 0x2);
    }

    // Set SearchCtrl M0.3[10:8] to dual reference & dual record

    if (multiRef) {
      const uint SearchCtrl = intel_get_message_phase_dw(handle, 0, 3) | (0x7 << 8);
      handle = intel_set_message_phase_dw(handle, 0, 3, SearchCtrl);
    }

    return __builtin_IB_vme_helper_get_as_avc_ime_payload_t(handle);
}

INLINE intel_sub_group_avc_ime_payload_t OVERLOADABLE
avc_ime_set_reference(
   short2 fwd_ref_offset,
   short2 bwd_ref_offset,
   uchar  search_window_config,
   bool   multiRef,
   intel_sub_group_avc_ime_payload_t payload )
{
    __spirv_AvcImePayloadINTEL ret = __intel_ime_set_reference_helper(
        fwd_ref_offset,
        bwd_ref_offset,
        search_window_config,
        multiRef,
        __builtin_astype(payload, __spirv_AvcImePayloadINTEL));

    return __builtin_astype(ret, intel_sub_group_avc_ime_payload_t);
}

__spirv_AvcImePayloadINTEL SPIRV_OVERLOADABLE SPIRV_BUILTIN(SubgroupAvcImeSetSingleReferenceINTEL, _v2i16_i8_i64, )(
    short2 ref_offset,
    uchar search_window_config,
    __spirv_AvcImePayloadINTEL payload )
{
    return __intel_ime_set_reference_helper(ref_offset, (short2)(0), search_window_config, false, payload);
}

INLINE intel_sub_group_avc_ime_payload_t OVERLOADABLE
intel_sub_group_avc_ime_set_single_reference(
    short2 ref_offset,
    uchar  search_window_config,
    intel_sub_group_avc_ime_payload_t payload )
{
    __spirv_AvcImePayloadINTEL ret = SPIRV_BUILTIN(SubgroupAvcImeSetSingleReferenceINTEL, _v2i16_i8_i64, )(
        ref_offset,
        search_window_config,
        __builtin_astype(payload, __spirv_AvcImePayloadINTEL));

    return __builtin_astype(ret, intel_sub_group_avc_ime_payload_t);
}

__spirv_AvcImePayloadINTEL SPIRV_OVERLOADABLE SPIRV_BUILTIN(SubgroupAvcImeSetDualReferenceINTEL, _v2i16_v2i16_i8_i64, )(
    short2 fwd_ref_offset,
    short2 bwd_ref_offset,
    uchar search_window_config,
    __spirv_AvcImePayloadINTEL payload )
{
    return __intel_ime_set_reference_helper(fwd_ref_offset, bwd_ref_offset, search_window_config, true, payload);
}

INLINE intel_sub_group_avc_ime_payload_t OVERLOADABLE
intel_sub_group_avc_ime_set_dual_reference(
   short2 fwd_ref_offset,
   short2 bwd_ref_offset,
   uchar  search_window_config,
   intel_sub_group_avc_ime_payload_t payload )
{
    __spirv_AvcImePayloadINTEL ret = SPIRV_BUILTIN(SubgroupAvcImeSetDualReferenceINTEL, _v2i16_v2i16_i8_i64, )(
        fwd_ref_offset,
        bwd_ref_offset,
        search_window_config,
        __builtin_astype(payload, __spirv_AvcImePayloadINTEL));

    return __builtin_astype(ret, intel_sub_group_avc_ime_payload_t);
}

/*****************************************************************************\

Description:
    set MaxNumMVs (M1.1 :5:0)

\*****************************************************************************/
__spirv_AvcImePayloadINTEL SPIRV_OVERLOADABLE SPIRV_BUILTIN(SubgroupAvcImeSetMaxMotionVectorCountINTEL, _i8_i64, )(
    uchar max_motion_vector_count,
    __spirv_AvcImePayloadINTEL payload )
{
    uint4 handle = __builtin_IB_vme_helper_get_handle_avc_ime_payload_t(payload);
    uchar curr = intel_get_message_phase_ub(handle, 1, 1*4);
    curr &= ~0x3F;
    curr |= max_motion_vector_count;
    handle = intel_set_message_phase_ub(handle, 1, 1*4, curr);
    payload = __builtin_IB_vme_helper_get_as_avc_ime_payload_t(handle);
    return payload;
}

INLINE intel_sub_group_avc_ime_payload_t OVERLOADABLE
intel_sub_group_avc_ime_set_max_motion_vector_count(
    uchar  max_motion_vector_count,
    intel_sub_group_avc_ime_payload_t payload )
{
    return __builtin_astype(
        SPIRV_BUILTIN(SubgroupAvcImeSetMaxMotionVectorCountINTEL, _i8_i64, )(
            max_motion_vector_count,
            __builtin_astype(payload, __spirv_AvcImePayloadINTEL)),
        intel_sub_group_avc_ime_payload_t);
}

__spirv_AvcMcePayloadINTEL SPIRV_OVERLOADABLE SPIRV_BUILTIN(SubgroupAvcImeConvertToMcePayloadINTEL, _i64, )(
    __spirv_AvcImePayloadINTEL payload )
{
    uint4 handle = __builtin_IB_vme_helper_get_handle_avc_ime_payload_t(payload);
    return __builtin_IB_vme_helper_get_as_avc_mce_payload_t(handle);
}

INLINE intel_sub_group_avc_mce_payload_t OVERLOADABLE
intel_sub_group_avc_ime_convert_to_mce_payload(
      intel_sub_group_avc_ime_payload_t payload )
{
    return __builtin_astype(SPIRV_BUILTIN(SubgroupAvcImeConvertToMcePayloadINTEL, _i64, )(__builtin_astype(payload, __spirv_AvcImePayloadINTEL)), intel_sub_group_avc_mce_payload_t);
}

__spirv_AvcImePayloadINTEL SPIRV_OVERLOADABLE SPIRV_BUILTIN(SubgroupAvcMceConvertToImePayloadINTEL, _i64, )(
    __spirv_AvcMcePayloadINTEL payload )
{
    uint4 handle = __builtin_IB_vme_helper_get_handle_avc_mce_payload_t(payload);
    return __builtin_IB_vme_helper_get_as_avc_ime_payload_t(handle);
}

INLINE intel_sub_group_avc_ime_payload_t OVERLOADABLE
intel_sub_group_avc_mce_convert_to_ime_payload(
      intel_sub_group_avc_mce_payload_t payload )
{
    return __builtin_astype(SPIRV_BUILTIN(SubgroupAvcMceConvertToImePayloadINTEL, _i64, )(__builtin_astype(payload, __spirv_AvcMcePayloadINTEL)), intel_sub_group_avc_ime_payload_t);
}

/*****************************************************************************\

Description:

    Evalulate multiple reference IME operation.

  Inputs:  4 Universal phases + 2 IME phases for search path
  Outputs: 7 phases

\*****************************************************************************/
__spirv_AvcImeResultINTEL SPIRV_OVERLOADABLE SPIRV_BUILTIN(SubgroupAvcImeEvaluateWithDualReferenceINTEL, _i64_i64_i64_i64, )(
    __spirv_VmeImageINTEL__void_1_0_0_0_0_0_0 src_image_vme,
    __spirv_VmeImageINTEL__void_1_0_0_0_0_0_0 fwd_ref_image_vme,
    __spirv_VmeImageINTEL__void_1_0_0_0_0_0_0 bwd_ref_image_vme,
    __spirv_AvcImePayloadINTEL payload )
{
    long src_image = __builtin_IB_get_image(src_image_vme);
    long fwd_ref_image = __builtin_IB_get_image(fwd_ref_image_vme);
    long bwd_ref_image = __builtin_IB_get_image(bwd_ref_image_vme);
    long vme_accelerator = __builtin_IB_get_sampler(src_image_vme);

    uint4 handle = __builtin_IB_vme_helper_get_handle_avc_ime_payload_t(payload);
    handle = intel_sub_group_payload_set_dual_ref_id(src_image, fwd_ref_image, bwd_ref_image, handle);
    uint4 res = intel_vme_send_ime_new_uint4(handle, src_image, fwd_ref_image, bwd_ref_image, vme_accelerator, VME_STREAM_DISABLE);

    return __builtin_IB_vme_helper_get_as_avc_ime_result_t( res );
}

INLINE intel_sub_group_avc_ime_result_t OVERLOADABLE
intel_sub_group_avc_ime_evaluate_with_dual_reference(
      read_only image2d_t src_image,
      read_only image2d_t fwd_ref_image,
      read_only image2d_t bwd_ref_image,
      sampler_t vme_accelerator,
      intel_sub_group_avc_ime_payload_t payload )
{
    __spirv_Image spv_src_image = __builtin_astype(src_image, __spirv_Image);
    __spirv_Image spv_fwd_ref_image = __builtin_astype(fwd_ref_image, __spirv_Image);
    __spirv_Image spv_bwd_ref_image = __builtin_astype(bwd_ref_image, __spirv_Image);;
    __spirv_Sampler spv_vme_accelerator = __builtin_astype(vme_accelerator, __spirv_Sampler);
    __spirv_VmeImageINTEL__void_1_0_0_0_0_0_0 src_image_vme = SPIRV_BUILTIN(VmeImageINTEL, _i64_i64, )(spv_src_image, spv_vme_accelerator);
    __spirv_VmeImageINTEL__void_1_0_0_0_0_0_0 fwd_ref_image_vme = SPIRV_BUILTIN(VmeImageINTEL, _i64_i64, )(spv_fwd_ref_image, spv_vme_accelerator);
    __spirv_VmeImageINTEL__void_1_0_0_0_0_0_0 bwd_ref_image_vme = SPIRV_BUILTIN(VmeImageINTEL, _i64_i64, )(spv_bwd_ref_image, spv_vme_accelerator);

    __spirv_AvcImeResultINTEL ret = SPIRV_BUILTIN(SubgroupAvcImeEvaluateWithDualReferenceINTEL, _i64_i64_i64_i64, )(
        src_image_vme, fwd_ref_image_vme, bwd_ref_image_vme, __builtin_astype(payload, __spirv_AvcImePayloadINTEL));

    return __builtin_astype(ret, intel_sub_group_avc_ime_result_t);
}

__spirv_AvcImeResultINTEL SPIRV_OVERLOADABLE SPIRV_BUILTIN(SubgroupAvcImeEvaluateWithSingleReferenceINTEL, _i64_i64_i64, )(__spirv_VmeImageINTEL__void_1_0_0_0_0_0_0 SrcImage, __spirv_VmeImageINTEL__void_1_0_0_0_0_0_0 RefImage, __spirv_AvcImePayloadINTEL Payload)
{
    long src_image = __builtin_IB_get_image(SrcImage);
    long ref_image = __builtin_IB_get_image(RefImage);
    long vme_accelerator = __builtin_IB_get_sampler(SrcImage);

    uint4 handle = __builtin_IB_vme_helper_get_handle_avc_ime_payload_t(Payload);
    handle = intel_sub_group_payload_set_single_ref_id(src_image, ref_image, handle);
    uint4 res = intel_vme_send_ime_new_uint4(handle, src_image, ref_image, ref_image, vme_accelerator, VME_STREAM_DISABLE);

    return __builtin_IB_vme_helper_get_as_avc_ime_result_t(res);
}

INLINE intel_sub_group_avc_ime_result_t OVERLOADABLE
intel_sub_group_avc_ime_evaluate_with_single_reference(
      read_only image2d_t src_image,
      read_only image2d_t ref_image,
      sampler_t vme_accelerator,
      intel_sub_group_avc_ime_payload_t payload )
{
    __spirv_Image spv_src_image = __builtin_astype(src_image, __spirv_Image);
    __spirv_Image spv_ref_image = __builtin_astype(ref_image, __spirv_Image);
    __spirv_Sampler spv_vme_accelerator = __builtin_astype(vme_accelerator, __spirv_Sampler);

    __spirv_VmeImageINTEL__void_1_0_0_0_0_0_0 src_image_vme = SPIRV_BUILTIN(VmeImageINTEL, _i64_i64, )(spv_src_image, spv_vme_accelerator);
    __spirv_VmeImageINTEL__void_1_0_0_0_0_0_0 ref_image_vme = SPIRV_BUILTIN(VmeImageINTEL, _i64_i64, )(spv_ref_image, spv_vme_accelerator);

    __spirv_AvcImePayloadINTEL spv_payload = __builtin_astype(payload, __spirv_AvcImePayloadINTEL);
    return __builtin_astype(SPIRV_BUILTIN(SubgroupAvcImeEvaluateWithSingleReferenceINTEL, _i64_i64_i64, )(src_image_vme, ref_image_vme, spv_payload), intel_sub_group_avc_ime_result_t);
}

__spirv_AvcImeResultSingleReferenceStreamoutINTEL SPIRV_OVERLOADABLE SPIRV_BUILTIN(SubgroupAvcImeEvaluateWithSingleReferenceStreamoutINTEL, _i64_i64_i64, )(
    __spirv_VmeImageINTEL__void_1_0_0_0_0_0_0 src_image_vme,
    __spirv_VmeImageINTEL__void_1_0_0_0_0_0_0 ref_image_vme,
    __spirv_AvcImePayloadINTEL payload )
{
    long src_image = __builtin_IB_get_image(src_image_vme);
    long ref_image = __builtin_IB_get_image(ref_image_vme);
    long vme_accelerator = __builtin_IB_get_sampler(src_image_vme);

    uint4 handle = __builtin_IB_vme_helper_get_handle_avc_ime_payload_t(payload);
    handle = intel_sub_group_payload_set_single_ref_id(src_image, ref_image, handle);
    uint8 res = intel_vme_send_ime_new_uint8(handle, src_image, ref_image, ref_image, vme_accelerator, VME_STREAM_OUT);

    return __builtin_IB_vme_helper_get_as_avc_ime_result_single_reference_streamout_t(res);
}

INLINE intel_sub_group_avc_ime_result_single_reference_streamout_t OVERLOADABLE
intel_sub_group_avc_ime_evaluate_with_single_reference_streamout(
      read_only image2d_t src_image,
      read_only image2d_t ref_image,
      sampler_t vme_accelerator,
      intel_sub_group_avc_ime_payload_t payload )
{
    __spirv_Image spv_src_image = __builtin_astype(src_image, __spirv_Image);
    __spirv_Image spv_ref_image = __builtin_astype(ref_image, __spirv_Image);
    __spirv_Sampler spv_vme_accelerator = __builtin_astype(vme_accelerator, __spirv_Sampler);
    __spirv_VmeImageINTEL__void_1_0_0_0_0_0_0 src_image_vme = SPIRV_BUILTIN(VmeImageINTEL, _i64_i64, )(spv_src_image, spv_vme_accelerator);
    __spirv_VmeImageINTEL__void_1_0_0_0_0_0_0 ref_image_vme = SPIRV_BUILTIN(VmeImageINTEL, _i64_i64, )(spv_ref_image, spv_vme_accelerator);

    __spirv_AvcImeResultSingleReferenceStreamoutINTEL ret = SPIRV_BUILTIN(SubgroupAvcImeEvaluateWithSingleReferenceStreamoutINTEL, _i64_i64_i64, )(
        src_image_vme,
        ref_image_vme,
        __builtin_astype(payload, __spirv_AvcImePayloadINTEL));

    return __builtin_astype(ret, intel_sub_group_avc_ime_result_single_reference_streamout_t);
}

__spirv_AvcImeResultDualReferenceStreamoutINTEL SPIRV_OVERLOADABLE SPIRV_BUILTIN(SubgroupAvcImeEvaluateWithDualReferenceStreamoutINTEL, _i64_i64_i64_i64, )(
    __spirv_VmeImageINTEL__void_1_0_0_0_0_0_0 src_image_vme,
    __spirv_VmeImageINTEL__void_1_0_0_0_0_0_0 fwd_ref_image_vme,
    __spirv_VmeImageINTEL__void_1_0_0_0_0_0_0 bwd_ref_image_vme,
    __spirv_AvcImePayloadINTEL payload )
{
    long src_image = __builtin_IB_get_image(src_image_vme);
    long fwd_ref_image = __builtin_IB_get_image(fwd_ref_image_vme);
    long bwd_ref_image = __builtin_IB_get_image(bwd_ref_image_vme);
    long vme_accelerator = __builtin_IB_get_sampler(src_image_vme);

    uint4 handle = __builtin_IB_vme_helper_get_handle_avc_ime_payload_t(payload);
    handle = intel_sub_group_payload_set_dual_ref_id(src_image, fwd_ref_image, bwd_ref_image, handle);
    uint8 res = intel_vme_send_ime_new_uint8(handle, src_image, fwd_ref_image, bwd_ref_image, vme_accelerator, VME_STREAM_OUT);

    return __builtin_IB_vme_helper_get_as_avc_ime_result_dual_reference_streamout_t( res );
}

INLINE intel_sub_group_avc_ime_result_dual_reference_streamout_t OVERLOADABLE
intel_sub_group_avc_ime_evaluate_with_dual_reference_streamout(
      read_only image2d_t src_image,
      read_only image2d_t fwd_ref_image,
      read_only image2d_t bwd_ref_image,
      sampler_t vme_accelerator,
      intel_sub_group_avc_ime_payload_t payload )
{
    __spirv_Image spv_src_image = __builtin_astype(src_image, __spirv_Image);
    __spirv_Image spv_fwd_ref_image = __builtin_astype(fwd_ref_image, __spirv_Image);
    __spirv_Image spv_bwd_ref_image = __builtin_astype(bwd_ref_image, __spirv_Image);;
    __spirv_Sampler spv_vme_accelerator = __builtin_astype(vme_accelerator, __spirv_Sampler);
    __spirv_VmeImageINTEL__void_1_0_0_0_0_0_0 src_image_vme = SPIRV_BUILTIN(VmeImageINTEL, _i64_i64, )(spv_src_image, spv_vme_accelerator);
    __spirv_VmeImageINTEL__void_1_0_0_0_0_0_0 fwd_ref_image_vme = SPIRV_BUILTIN(VmeImageINTEL, _i64_i64, )(spv_fwd_ref_image, spv_vme_accelerator);
    __spirv_VmeImageINTEL__void_1_0_0_0_0_0_0 bwd_ref_image_vme = SPIRV_BUILTIN(VmeImageINTEL, _i64_i64, )(spv_bwd_ref_image, spv_vme_accelerator);

    __spirv_AvcImeResultDualReferenceStreamoutINTEL ret = SPIRV_BUILTIN(SubgroupAvcImeEvaluateWithDualReferenceStreamoutINTEL, _i64_i64_i64_i64, )(
        src_image_vme,
        fwd_ref_image_vme,
        bwd_ref_image_vme,
        __builtin_astype(payload, __spirv_AvcImePayloadINTEL));

    return __builtin_astype(ret, intel_sub_group_avc_ime_result_dual_reference_streamout_t);
}

__spirv_AvcImeResultINTEL SPIRV_OVERLOADABLE SPIRV_BUILTIN(SubgroupAvcImeEvaluateWithSingleReferenceStreaminINTEL, _i64_i64_i64_i64, )(
    __spirv_VmeImageINTEL__void_1_0_0_0_0_0_0 src_image_vme,
    __spirv_VmeImageINTEL__void_1_0_0_0_0_0_0 ref_image_vme,
    __spirv_AvcImePayloadINTEL payload,
    __spirv_AvcImeSingleReferenceStreaminINTEL streamin )
{
    long src_image = __builtin_IB_get_image(src_image_vme);
    long ref_image = __builtin_IB_get_image(ref_image_vme);
    long vme_accelerator = __builtin_IB_get_sampler(src_image_vme);

    uint4 handle = __builtin_IB_vme_helper_get_handle_avc_ime_payload_t(payload);
    uint handle_streamin = __builtin_IB_vme_helper_get_handle_avc_ime_single_reference_streamin_t(streamin);

    // Copy streamin to payload IME2 and IME3
    handle = intel_set_message_phase(handle, UNIVERSAL_INPUT_MESSAGE_NUM_GRFS+2, intel_get_message_phase(handle_streamin, 0));
    handle = intel_set_message_phase(handle, UNIVERSAL_INPUT_MESSAGE_NUM_GRFS+3, intel_get_message_phase(handle_streamin, 1));

    handle = intel_sub_group_payload_set_single_ref_id(src_image, ref_image, handle);
    uint4 res = intel_vme_send_ime_new_uint4(handle, src_image, ref_image, ref_image, vme_accelerator, VME_STREAM_IN);

    return __builtin_IB_vme_helper_get_as_avc_ime_result_t(res);
}

intel_sub_group_avc_ime_result_t OVERLOADABLE
intel_sub_group_avc_ime_evaluate_with_single_reference_streamin(
      read_only image2d_t src_image,
      read_only image2d_t ref_image,
      sampler_t vme_accelerator,
      intel_sub_group_avc_ime_payload_t payload,
      intel_sub_group_avc_ime_single_reference_streamin_t streamin )
{
    __spirv_Image spv_src_image = __builtin_astype(src_image, __spirv_Image);
    __spirv_Image spv_ref_image = __builtin_astype(ref_image, __spirv_Image);
    __spirv_Sampler spv_vme_accelerator = __builtin_astype(vme_accelerator, __spirv_Sampler);
    __spirv_VmeImageINTEL__void_1_0_0_0_0_0_0 src_image_vme = SPIRV_BUILTIN(VmeImageINTEL, _i64_i64, )(spv_src_image, spv_vme_accelerator);
    __spirv_VmeImageINTEL__void_1_0_0_0_0_0_0 ref_image_vme = SPIRV_BUILTIN(VmeImageINTEL, _i64_i64, )(spv_ref_image, spv_vme_accelerator);

    __spirv_AvcImeResultINTEL ret = SPIRV_BUILTIN(SubgroupAvcImeEvaluateWithSingleReferenceStreaminINTEL, _i64_i64_i64_i64, )(
        src_image_vme, ref_image_vme,
        __builtin_astype(payload, __spirv_AvcImePayloadINTEL),
        __builtin_astype(streamin, __spirv_AvcImeSingleReferenceStreaminINTEL));

    return __builtin_astype(ret, intel_sub_group_avc_ime_result_t);
}

__spirv_AvcImeResultINTEL SPIRV_OVERLOADABLE SPIRV_BUILTIN(SubgroupAvcImeEvaluateWithDualReferenceStreaminINTEL, _i64_i64_i64_i64_i64, )(
    __spirv_VmeImageINTEL__void_1_0_0_0_0_0_0 src_image_vme,
    __spirv_VmeImageINTEL__void_1_0_0_0_0_0_0 fwd_ref_image_vme,
    __spirv_VmeImageINTEL__void_1_0_0_0_0_0_0 bwd_ref_image_vme,
    __spirv_AvcImePayloadINTEL payload,
    __spirv_AvcImeDualReferenceStreaminINTEL streamin )
{
    long src_image = __builtin_IB_get_image(src_image_vme);
    long fwd_ref_image = __builtin_IB_get_image(fwd_ref_image_vme);
    long bwd_ref_image = __builtin_IB_get_image(bwd_ref_image_vme);
    long vme_accelerator = __builtin_IB_get_sampler(src_image_vme);

    uint8 newPayload = __builtin_IB_create_message_phases_no_init_uint8(16);

    uint4 handle_p = __builtin_IB_vme_helper_get_handle_avc_ime_payload_t(payload);
    uint2 handle_s = __builtin_IB_vme_helper_get_handle_avc_ime_dual_reference_streamin_t(streamin);

    for (int i=0; i < UNIVERSAL_INPUT_MESSAGE_NUM_GRFS+2; i++)
    {
        newPayload = intel_set_message_phase(newPayload, i, intel_get_message_phase(handle_p, i));
    }

    // Copy streamin to payload IME2 ~ IME5
    for (int i=UNIVERSAL_INPUT_MESSAGE_NUM_GRFS+2, j=0; i < 10; i++, j++)
    {
        newPayload = intel_set_message_phase(newPayload, i, intel_get_message_phase(handle_s, j));
    }

    newPayload = intel_sub_group_payload_set_dual_ref_id(src_image, fwd_ref_image, bwd_ref_image, newPayload);
    uint4 res = intel_vme_send_ime_new_uint4(newPayload, src_image, fwd_ref_image, bwd_ref_image, vme_accelerator, VME_STREAM_IN);

    return __builtin_IB_vme_helper_get_as_avc_ime_result_t( res );
}

intel_sub_group_avc_ime_result_t OVERLOADABLE
intel_sub_group_avc_ime_evaluate_with_dual_reference_streamin(
      read_only image2d_t src_image,
      read_only image2d_t fwd_ref_image,
      read_only image2d_t bwd_ref_image,
      sampler_t vme_accelerator,
      intel_sub_group_avc_ime_payload_t payload,
      intel_sub_group_avc_ime_dual_reference_streamin_t streamin )
{
    __spirv_Image spv_src_image = __builtin_astype(src_image, __spirv_Image);
    __spirv_Image spv_fwd_ref_image = __builtin_astype(fwd_ref_image, __spirv_Image);
    __spirv_Image spv_bwd_ref_image = __builtin_astype(bwd_ref_image, __spirv_Image);;
    __spirv_Sampler spv_vme_accelerator = __builtin_astype(vme_accelerator, __spirv_Sampler);
    __spirv_VmeImageINTEL__void_1_0_0_0_0_0_0 src_image_vme = SPIRV_BUILTIN(VmeImageINTEL, _i64_i64, )(spv_src_image, spv_vme_accelerator);
    __spirv_VmeImageINTEL__void_1_0_0_0_0_0_0 fwd_ref_image_vme = SPIRV_BUILTIN(VmeImageINTEL, _i64_i64, )(spv_fwd_ref_image, spv_vme_accelerator);
    __spirv_VmeImageINTEL__void_1_0_0_0_0_0_0 bwd_ref_image_vme = SPIRV_BUILTIN(VmeImageINTEL, _i64_i64, )(spv_bwd_ref_image, spv_vme_accelerator);

    __spirv_AvcImeResultINTEL ret = SPIRV_BUILTIN(SubgroupAvcImeEvaluateWithDualReferenceStreaminINTEL, _i64_i64_i64_i64_i64, )(
        src_image_vme,
        fwd_ref_image_vme,
        bwd_ref_image_vme,
        __builtin_astype(payload, __spirv_AvcImePayloadINTEL),
        __builtin_astype(streamin, __spirv_AvcImeDualReferenceStreaminINTEL));

    return __builtin_astype(ret, intel_sub_group_avc_ime_result_t);
}

__spirv_AvcImeResultSingleReferenceStreamoutINTEL SPIRV_OVERLOADABLE SPIRV_BUILTIN(SubgroupAvcImeEvaluateWithSingleReferenceStreaminoutINTEL, _i64_i64_i64_i64, )(
    __spirv_VmeImageINTEL__void_1_0_0_0_0_0_0 src_image_vme,
    __spirv_VmeImageINTEL__void_1_0_0_0_0_0_0 ref_image_vme,
    __spirv_AvcImePayloadINTEL payload,
    __spirv_AvcImeSingleReferenceStreaminINTEL streamin )
{
    long src_image = __builtin_IB_get_image(src_image_vme);
    long ref_image = __builtin_IB_get_image(ref_image_vme);
    long vme_accelerator = __builtin_IB_get_sampler(src_image_vme);

    uint4 handle = __builtin_IB_vme_helper_get_handle_avc_ime_payload_t(payload);
    uint handle_s = __builtin_IB_vme_helper_get_handle_avc_ime_single_reference_streamin_t(streamin);
    // Copy streamin to payload IME2 and IME3
    handle = intel_set_message_phase(handle, UNIVERSAL_INPUT_MESSAGE_NUM_GRFS+2, intel_get_message_phase(handle_s, 0));
    handle = intel_set_message_phase(handle, UNIVERSAL_INPUT_MESSAGE_NUM_GRFS+3, intel_get_message_phase(handle_s, 1));

    handle = intel_sub_group_payload_set_single_ref_id(src_image, ref_image, handle);
    uint8 res = intel_vme_send_ime_new_uint8(handle, src_image, ref_image, ref_image, vme_accelerator, VME_STREAM_INOUT);

    return __builtin_IB_vme_helper_get_as_avc_ime_result_single_reference_streamout_t( res );
}

INLINE intel_sub_group_avc_ime_result_single_reference_streamout_t OVERLOADABLE
intel_sub_group_avc_ime_evaluate_with_single_reference_streaminout(
      read_only image2d_t src_image,
      read_only image2d_t ref_image,
      sampler_t vme_accelerator,
      intel_sub_group_avc_ime_payload_t payload,
      intel_sub_group_avc_ime_single_reference_streamin_t streamin )
{
    __spirv_Image spv_src_image = __builtin_astype(src_image, __spirv_Image);
    __spirv_Image spv_ref_image = __builtin_astype(ref_image, __spirv_Image);
    __spirv_Sampler spv_vme_accelerator = __builtin_astype(vme_accelerator, __spirv_Sampler);
    __spirv_VmeImageINTEL__void_1_0_0_0_0_0_0 src_image_vme = SPIRV_BUILTIN(VmeImageINTEL, _i64_i64, )(spv_src_image, spv_vme_accelerator);
    __spirv_VmeImageINTEL__void_1_0_0_0_0_0_0 ref_image_vme = SPIRV_BUILTIN(VmeImageINTEL, _i64_i64, )(spv_ref_image, spv_vme_accelerator);

    __spirv_AvcImeResultSingleReferenceStreamoutINTEL ret = SPIRV_BUILTIN(SubgroupAvcImeEvaluateWithSingleReferenceStreaminoutINTEL, _i64_i64_i64_i64, )(
        src_image_vme,
        ref_image_vme,
        __builtin_astype(payload, __spirv_AvcImePayloadINTEL),
        __builtin_astype(streamin, __spirv_AvcImeSingleReferenceStreaminINTEL));

    return __builtin_astype(ret, intel_sub_group_avc_ime_result_single_reference_streamout_t);
}

__spirv_AvcImeResultDualReferenceStreamoutINTEL SPIRV_OVERLOADABLE SPIRV_BUILTIN(SubgroupAvcImeEvaluateWithDualReferenceStreaminoutINTEL, _i64_i64_i64_i64_i64, )(
    __spirv_VmeImageINTEL__void_1_0_0_0_0_0_0 src_image_vme,
    __spirv_VmeImageINTEL__void_1_0_0_0_0_0_0 fwd_ref_image_vme,
    __spirv_VmeImageINTEL__void_1_0_0_0_0_0_0 bwd_ref_image_vme,
    __spirv_AvcImePayloadINTEL payload,
    __spirv_AvcImeDualReferenceStreaminINTEL streamin )
{
    long src_image = __builtin_IB_get_image(src_image_vme);
    long fwd_ref_image = __builtin_IB_get_image(fwd_ref_image_vme);
    long bwd_ref_image = __builtin_IB_get_image(bwd_ref_image_vme);
    long vme_accelerator = __builtin_IB_get_sampler(src_image_vme);

    uint8 newPayload = __builtin_IB_create_message_phases_no_init_uint8(16);

    uint4 handle_p = __builtin_IB_vme_helper_get_handle_avc_ime_payload_t(payload);
    uint2 handle_s = __builtin_IB_vme_helper_get_handle_avc_ime_dual_reference_streamin_t(streamin);

    for (int i=0; i < UNIVERSAL_INPUT_MESSAGE_NUM_GRFS+2; i++)
    {
        newPayload = intel_set_message_phase(newPayload, i, intel_get_message_phase(handle_p, i));
    }

    // Copy streamin to payload IME2 ~ IME5
    for (int i=UNIVERSAL_INPUT_MESSAGE_NUM_GRFS+2, j=0; i < 10; i++, j++)
    {
        newPayload = intel_set_message_phase(newPayload, i, intel_get_message_phase(handle_s, j));
    }

    newPayload = intel_sub_group_payload_set_dual_ref_id(src_image, fwd_ref_image, bwd_ref_image, newPayload);

    uint8 res = intel_vme_send_ime_new_uint8(newPayload, src_image, fwd_ref_image, bwd_ref_image, vme_accelerator, VME_STREAM_INOUT);

    return __builtin_IB_vme_helper_get_as_avc_ime_result_dual_reference_streamout_t( res );
}

INLINE intel_sub_group_avc_ime_result_dual_reference_streamout_t OVERLOADABLE
intel_sub_group_avc_ime_evaluate_with_dual_reference_streaminout(
      read_only image2d_t src_image,
      read_only image2d_t fwd_ref_image,
      read_only image2d_t bwd_ref_image,
      sampler_t vme_accelerator,
      intel_sub_group_avc_ime_payload_t payload,
      intel_sub_group_avc_ime_dual_reference_streamin_t streamin )
{
    __spirv_Image spv_src_image = __builtin_astype(src_image, __spirv_Image);
    __spirv_Image spv_fwd_ref_image = __builtin_astype(fwd_ref_image, __spirv_Image);
    __spirv_Image spv_bwd_ref_image = __builtin_astype(bwd_ref_image, __spirv_Image);;
    __spirv_Sampler spv_vme_accelerator = __builtin_astype(vme_accelerator, __spirv_Sampler);
    __spirv_VmeImageINTEL__void_1_0_0_0_0_0_0 src_image_vme = SPIRV_BUILTIN(VmeImageINTEL, _i64_i64, )(spv_src_image, spv_vme_accelerator);
    __spirv_VmeImageINTEL__void_1_0_0_0_0_0_0 fwd_ref_image_vme = SPIRV_BUILTIN(VmeImageINTEL, _i64_i64, )(spv_fwd_ref_image, spv_vme_accelerator);
    __spirv_VmeImageINTEL__void_1_0_0_0_0_0_0 bwd_ref_image_vme = SPIRV_BUILTIN(VmeImageINTEL, _i64_i64, )(spv_bwd_ref_image, spv_vme_accelerator);

    __spirv_AvcImeResultDualReferenceStreamoutINTEL ret = SPIRV_BUILTIN(SubgroupAvcImeEvaluateWithDualReferenceStreaminoutINTEL, _i64_i64_i64_i64_i64, )(
        src_image_vme,
        fwd_ref_image_vme,
        bwd_ref_image_vme,
        __builtin_astype(payload, __spirv_AvcImePayloadINTEL),
        __builtin_astype(streamin, __spirv_AvcImeDualReferenceStreaminINTEL));

    return __builtin_astype(ret, intel_sub_group_avc_ime_result_dual_reference_streamout_t);
}

__spirv_AvcImeSingleReferenceStreaminINTEL SPIRV_OVERLOADABLE SPIRV_BUILTIN(SubgroupAvcImeGetSingleReferenceStreaminINTEL, _i64, )(
    __spirv_AvcImeResultSingleReferenceStreamoutINTEL result )
{
    uint handle = __builtin_IB_create_message_phases_no_init(2);
    uint8 handle_r = __builtin_IB_vme_helper_get_handle_avc_ime_result_single_reference_streamout_t(result);
    // Copy IME2 from result W7
    handle = intel_set_message_phase(handle, 0, intel_get_message_phase(handle_r, RETURN_MESSAGE_NUM_GRFS));
    // Copy IME3 from result W8
    handle = intel_set_message_phase(handle, 1, intel_get_message_phase(handle_r, RETURN_MESSAGE_NUM_GRFS + 1));

    return __builtin_IB_vme_helper_get_as_avc_ime_single_reference_streamin_t(handle);
}

INLINE intel_sub_group_avc_ime_single_reference_streamin_t OVERLOADABLE
intel_sub_group_avc_ime_get_single_reference_streamin(
   intel_sub_group_avc_ime_result_single_reference_streamout_t result )
{
    __spirv_AvcImeSingleReferenceStreaminINTEL ret = SPIRV_BUILTIN(SubgroupAvcImeGetSingleReferenceStreaminINTEL, _i64, )(
        __builtin_astype(result, __spirv_AvcImeResultSingleReferenceStreamoutINTEL));

    return __builtin_astype(ret, intel_sub_group_avc_ime_single_reference_streamin_t);
}

__spirv_AvcImeDualReferenceStreaminINTEL SPIRV_OVERLOADABLE SPIRV_BUILTIN(SubgroupAvcImeGetDualReferenceStreaminINTEL, _i64, )(
    __spirv_AvcImeResultDualReferenceStreamoutINTEL result )
{
    uint2 handle  = __builtin_IB_create_message_phases_no_init_uint2(4);
    uint8 handle_r = __builtin_IB_vme_helper_get_handle_avc_ime_result_dual_reference_streamout_t(result);
    // Copy IME2~IME5 from result W7~W10
    for (int i = 0; i < 4; i++)
    {
        handle = intel_set_message_phase(handle, i, intel_get_message_phase(handle_r, RETURN_MESSAGE_NUM_GRFS+i));
    }
    return __builtin_IB_vme_helper_get_as_avc_ime_dual_reference_streamin_t(handle);
}

INLINE intel_sub_group_avc_ime_dual_reference_streamin_t OVERLOADABLE
intel_sub_group_avc_ime_get_dual_reference_streamin(
   intel_sub_group_avc_ime_result_dual_reference_streamout_t result )
{
    __spirv_AvcImeDualReferenceStreaminINTEL ret = SPIRV_BUILTIN(SubgroupAvcImeGetDualReferenceStreaminINTEL, _i64, )(
        __builtin_astype(result, __spirv_AvcImeResultDualReferenceStreamoutINTEL));

    return __builtin_astype(ret, intel_sub_group_avc_ime_dual_reference_streamin_t);
}

__spirv_AvcImeResultINTEL SPIRV_OVERLOADABLE SPIRV_BUILTIN(SubgroupAvcImeStripSingleReferenceStreamoutINTEL, _i64, )(
    __spirv_AvcImeResultSingleReferenceStreamoutINTEL result )
{
    uint4 res = __builtin_IB_create_message_phases_no_init_uint4(RETURN_MESSAGE_NUM_GRFS+1);
    uint8 handle_r = __builtin_IB_vme_helper_get_handle_avc_ime_result_single_reference_streamout_t(result);

    for (int i = 0; i < RETURN_MESSAGE_NUM_GRFS; i++) {
        res = intel_set_message_phase(res, i, intel_get_message_phase(handle_r, i));
    }

    return __builtin_IB_vme_helper_get_as_avc_ime_result_t(res);
}

INLINE intel_sub_group_avc_ime_result_t OVERLOADABLE
intel_sub_group_avc_ime_strip_single_reference_streamout(
    intel_sub_group_avc_ime_result_single_reference_streamout_t result )
{
    return __builtin_astype(
        SPIRV_BUILTIN(SubgroupAvcImeStripSingleReferenceStreamoutINTEL, _i64, )(
            __builtin_astype(result, __spirv_AvcImeResultSingleReferenceStreamoutINTEL)),
        intel_sub_group_avc_ime_result_t);
}

__spirv_AvcImeResultINTEL SPIRV_OVERLOADABLE SPIRV_BUILTIN(SubgroupAvcImeStripDualReferenceStreamoutINTEL, _i64, )(
    __spirv_AvcImeResultDualReferenceStreamoutINTEL result )
{
    uint4 res = __builtin_IB_create_message_phases_no_init_uint4(RETURN_MESSAGE_NUM_GRFS + 1);
    uint8 handle_r = __builtin_IB_vme_helper_get_handle_avc_ime_result_dual_reference_streamout_t(result);

    for (int i = 0; i < RETURN_MESSAGE_NUM_GRFS; i++) {
        res = intel_set_message_phase(res, i, intel_get_message_phase(handle_r, i));
    }

    return __builtin_IB_vme_helper_get_as_avc_ime_result_t(res);
}

INLINE intel_sub_group_avc_ime_result_t OVERLOADABLE
intel_sub_group_avc_ime_strip_dual_reference_streamout(
    intel_sub_group_avc_ime_result_dual_reference_streamout_t result )
{
    return __builtin_astype(
        SPIRV_BUILTIN(SubgroupAvcImeStripDualReferenceStreamoutINTEL, _i64, )(
            __builtin_astype(result, __spirv_AvcImeResultDualReferenceStreamoutINTEL)),
        intel_sub_group_avc_ime_result_t);
}

/*****************************************************************************\

Description:
    Get the MVs from the streamout results based on the input major shape and
    direction.

\*****************************************************************************/
uint SPIRV_OVERLOADABLE SPIRV_BUILTIN(SubgroupAvcImeGetStreamoutDualReferenceMajorShapeMotionVectorsINTEL, _i64_i8_i8, )(
    __spirv_AvcImeResultDualReferenceStreamoutINTEL result,
    uchar shape,
    uchar direction )
{
    uint retValue = 0;
    // IME Streamout follows the same format as the IME Streamin message phases (IME2-IME5).
    const uint reg = (direction == CLK_AVC_ME_MAJOR_FORWARD_INTEL) ? /*fwd*/(RETURN_MESSAGE_NUM_GRFS+1) : /*bwd*/(RETURN_MESSAGE_NUM_GRFS+3);
    uint8 handle_r = __builtin_IB_vme_helper_get_handle_avc_ime_result_dual_reference_streamout_t(result);

    if (shape == VME_MAJOR_16x16) {
        // WX+2.5 fwd
        // WX+4.5 bwd
        const uint reg = (direction == CLK_AVC_ME_MAJOR_FORWARD_INTEL) ? /*fwd*/(RETURN_MESSAGE_NUM_GRFS) : /*bwd*/(RETURN_MESSAGE_NUM_GRFS+2);
        retValue = intel_get_message_phase_dw(handle_r, reg, 5);
    }
    else if (shape == VME_MAJOR_16x8) {
        // WX+3.0 and WX+3.1 fwd
        // WX+5.0 and WX+5.1 bwd
        retValue = intel_broadcast_message_phase_dw(handle_r, reg, 0, 2);
    }
    else if (shape == VME_MAJOR_8x16) {
        // WX+3.2 and WX+3.3
        // WX+5.2 and WX+5.3
        retValue = intel_broadcast_message_phase_dw(handle_r, reg, 2, 2);
    }
    else if (shape == VME_MAJOR_8x8) {
        // WX+3.4 ~ WX+3.7
        // WX+5.4 ~ WX+5.7
        retValue = intel_broadcast_message_phase_dw(handle_r, reg, 4, 4);
    }

    return retValue;
}

INLINE uint OVERLOADABLE
intel_sub_group_avc_ime_get_streamout_major_shape_motion_vectors(
    intel_sub_group_avc_ime_result_dual_reference_streamout_t result,
    uchar shape,
    uchar direction )
{
    return SPIRV_BUILTIN(SubgroupAvcImeGetStreamoutDualReferenceMajorShapeMotionVectorsINTEL, _i64_i8_i8, )(
        __builtin_astype(result, __spirv_AvcImeResultDualReferenceStreamoutINTEL),
        shape,
        direction);
}

uint SPIRV_OVERLOADABLE SPIRV_BUILTIN(SubgroupAvcImeGetStreamoutSingleReferenceMajorShapeMotionVectorsINTEL, _i64_i8, )(
    __spirv_AvcImeResultSingleReferenceStreamoutINTEL result,
    uchar major_shape )
{
    uint8 handle = __builtin_IB_vme_helper_get_handle_avc_ime_result_single_reference_streamout_t(result);
    __spirv_AvcImeResultDualReferenceStreamoutINTEL nresult = __builtin_IB_vme_helper_get_as_avc_ime_result_dual_reference_streamout_t(handle);
    return SPIRV_BUILTIN(SubgroupAvcImeGetStreamoutDualReferenceMajorShapeMotionVectorsINTEL, _i64_i8_i8, )(nresult, major_shape, CLK_AVC_ME_MAJOR_FORWARD_INTEL);
}

INLINE uint OVERLOADABLE
intel_sub_group_avc_ime_get_streamout_major_shape_motion_vectors(
    intel_sub_group_avc_ime_result_single_reference_streamout_t result,
    uchar major_shape )
{
    return SPIRV_BUILTIN(SubgroupAvcImeGetStreamoutSingleReferenceMajorShapeMotionVectorsINTEL, _i64_i8, )(
        __builtin_astype(result, __spirv_AvcImeResultSingleReferenceStreamoutINTEL),
        major_shape);
}

/*****************************************************************************\

Description:
    Get the distortions from the streamout results based on the input major shape and
    direction.

\*****************************************************************************/
ushort SPIRV_OVERLOADABLE SPIRV_BUILTIN(SubgroupAvcImeGetStreamoutDualReferenceMajorShapeDistortionsINTEL, _i64_i8_i8, )(
    __spirv_AvcImeResultDualReferenceStreamoutINTEL result,
    uchar shape,
    uchar direction )
{
    ushort retValue = 0;
    uint8 handle = __builtin_IB_vme_helper_get_handle_avc_ime_result_dual_reference_streamout_t(result);
    // IME Streamout follows the same format as the IME Streamin message phases (IME2-IME5).
    const uint reg = (direction == CLK_AVC_ME_MAJOR_FORWARD_INTEL) ?
        /*fwd*/ RETURN_MESSAGE_NUM_GRFS :
        /*bwd*/(RETURN_MESSAGE_NUM_GRFS+2);

    if (shape == VME_MAJOR_16x16) {
        // WX+2.4 [15:0] - Rec0 Shape 16x16 Distortion
        // WX+4.4 [15:0] - Rec1 Shape 16x16 Distortion
        retValue = intel_get_message_phase_uw(handle, reg, 4*2);
    }
    else if (shape == VME_MAJOR_16x8) {
        // WX+2.0 [15:00] - Rec0 Shape 16x8_0 Distortion
        // WX+4.0 [15:00] - Rec1 Shape 16x8_0 Distortion
        retValue = intel_broadcast_message_phase_uw(handle, reg, 0, 2);
    }
    else if (shape == VME_MAJOR_8x16) {
        // WX+2.1 [15:00] - Rec0 Shape 8x16_0 Distortion
        // WX+4.1 [15:00] - Rec1 Shape 8x16_0 Distortion
        retValue = intel_broadcast_message_phase_uw(handle, reg, 1*2, 2);
    }
    else if (shape == VME_MAJOR_8x8) {
        // WX+2.2 [15:00] - Rec0 Shape 8x8_0 Distortion
        // WX+4.2 [15:00] - Rec1 Shape 8x8_0 Distortion
        retValue = intel_broadcast_message_phase_uw(handle, reg, 2*2, 4);
    }

    return retValue;
}

INLINE ushort OVERLOADABLE
intel_sub_group_avc_ime_get_streamout_major_shape_distortions(
    intel_sub_group_avc_ime_result_dual_reference_streamout_t result,
    uchar shape,
    uchar direction )
{
    return SPIRV_BUILTIN(SubgroupAvcImeGetStreamoutDualReferenceMajorShapeDistortionsINTEL, _i64_i8_i8, )(
        __builtin_astype(result, __spirv_AvcImeResultDualReferenceStreamoutINTEL),
        shape,
        direction);
}

ushort SPIRV_OVERLOADABLE SPIRV_BUILTIN(SubgroupAvcImeGetStreamoutSingleReferenceMajorShapeDistortionsINTEL, _i64_i8, )(
    __spirv_AvcImeResultSingleReferenceStreamoutINTEL result,
    uchar major_shape )
{
    uint8 handle = __builtin_IB_vme_helper_get_handle_avc_ime_result_single_reference_streamout_t(result);
    __spirv_AvcImeResultDualReferenceStreamoutINTEL nresult = __builtin_IB_vme_helper_get_as_avc_ime_result_dual_reference_streamout_t(handle);
    return SPIRV_BUILTIN(SubgroupAvcImeGetStreamoutDualReferenceMajorShapeDistortionsINTEL, _i64_i8_i8, )(nresult, major_shape, 0);
}

INLINE ushort OVERLOADABLE
intel_sub_group_avc_ime_get_streamout_major_shape_distortions(
    intel_sub_group_avc_ime_result_single_reference_streamout_t result,
    uchar major_shape )
{
    return SPIRV_BUILTIN(SubgroupAvcImeGetStreamoutSingleReferenceMajorShapeDistortionsINTEL, _i64_i8, )(
        __builtin_astype(result, __spirv_AvcImeResultSingleReferenceStreamoutINTEL),
        major_shape);
}

__spirv_AvcMceResultINTEL SPIRV_OVERLOADABLE SPIRV_BUILTIN(SubgroupAvcImeConvertToMceResultINTEL, _i64, )(
    __spirv_AvcImeResultINTEL result )
{
    uint4 handle = __builtin_IB_vme_helper_get_handle_avc_ime_result_t(result);
    return __builtin_IB_vme_helper_get_as_avc_mce_result_t(handle);
}

INLINE intel_sub_group_avc_mce_result_t OVERLOADABLE
intel_sub_group_avc_ime_convert_to_mce_result(
      intel_sub_group_avc_ime_result_t  result )
{
    return __builtin_astype(
        SPIRV_BUILTIN(SubgroupAvcImeConvertToMceResultINTEL, _i64, )(
            __builtin_astype(result, __spirv_AvcImeResultINTEL)),
        intel_sub_group_avc_mce_result_t);
}

__spirv_AvcImeResultINTEL SPIRV_OVERLOADABLE SPIRV_BUILTIN(SubgroupAvcMceConvertToImeResultINTEL, _i64, )(
    __spirv_AvcMceResultINTEL result )
{
    uint4 handle = __builtin_IB_vme_helper_get_handle_avc_mce_result_t(result);
    return __builtin_IB_vme_helper_get_as_avc_ime_result_t(handle);
}

INLINE intel_sub_group_avc_ime_result_t OVERLOADABLE
intel_sub_group_avc_mce_convert_to_ime_result(
      intel_sub_group_avc_mce_result_t  result )
{
    __spirv_AvcImeResultINTEL ret = SPIRV_BUILTIN(SubgroupAvcMceConvertToImeResultINTEL, _i64, )(
        __builtin_astype(result, __spirv_AvcMceResultINTEL));

    return __builtin_astype(ret, intel_sub_group_avc_ime_result_t);
}

/*****************************************************************************\

Description:
    - set UniMixDis M1.1[28:28]

\*****************************************************************************/
__spirv_AvcImePayloadINTEL SPIRV_OVERLOADABLE SPIRV_BUILTIN(SubgroupAvcImeSetUnidirectionalMixDisableINTEL, _i64, )(
    __spirv_AvcImePayloadINTEL payload )
{
    uint4 handle = __builtin_IB_vme_helper_get_handle_avc_ime_payload_t(payload);
    const uint UniMixDis = intel_get_message_phase_dw(handle, 1, 1) | (1<<28);
    handle = intel_set_message_phase_dw(handle, 1, 1, UniMixDis);

    return __builtin_IB_vme_helper_get_as_avc_ime_payload_t(handle);
}

INLINE intel_sub_group_avc_ime_payload_t OVERLOADABLE
intel_sub_group_avc_ime_set_unidirectional_mix_disable(
    intel_sub_group_avc_ime_payload_t payload )
{
    __spirv_AvcImePayloadINTEL ret = SPIRV_BUILTIN(SubgroupAvcImeSetUnidirectionalMixDisableINTEL, _i64, )(__builtin_astype(payload, __spirv_AvcImePayloadINTEL));
    return __builtin_astype(ret, intel_sub_group_avc_ime_payload_t);
}

/*****************************************************************************\

Description:
    - set EarlyIMESuccessEn (M1.0 :5)
    - set EarlyIMESTop (M1.0 : 31:24)

\*****************************************************************************/
__spirv_AvcImePayloadINTEL SPIRV_OVERLOADABLE SPIRV_BUILTIN(SubgroupAvcImeSetEarlySearchTerminationThresholdINTEL, _i8_i64, )(
    uchar threshold,
    __spirv_AvcImePayloadINTEL payload )
{
    uint4 handle = __builtin_IB_vme_helper_get_handle_avc_ime_payload_t(payload);
    uchar newval = intel_get_message_phase_ub(handle, 1, 0*4) | (1 << 5);
    handle       = intel_set_message_phase_ub(handle, 1, 0*4, newval);
    handle       = intel_set_message_phase_ub(handle, 1, 0*4+3, threshold);

    return __builtin_IB_vme_helper_get_as_avc_ime_payload_t(handle);
}

INLINE intel_sub_group_avc_ime_payload_t OVERLOADABLE
intel_sub_group_avc_ime_set_early_search_termination_threshold(
    uchar threshold,
    intel_sub_group_avc_ime_payload_t payload )
{
    __spirv_AvcImePayloadINTEL ret = SPIRV_BUILTIN(SubgroupAvcImeSetEarlySearchTerminationThresholdINTEL, _i8_i64, )(
        threshold,
        __builtin_astype(payload, __spirv_AvcImePayloadINTEL));

    return __builtin_astype(ret, intel_sub_group_avc_ime_payload_t);
}

// Note: This function is not present in the VME specification, but our tests use it.
// TODO: a ticket to validation team has been submitted to remove the calls in tests. Remove this function when it is fixed.
// As Clang's spir-v generator doesn't recognize this function, it passses the call "as is".
// VME types are built-in in Clang 5.0, so the mangling changed from 4.0 version. Below mangled definiton is to workaround the mangling change for this function.
INLINE intel_sub_group_avc_ime_payload_t
_Z77intel_sub_group_avc_ime_set_early_unidirectional_search_termination_thresholdh37ocl_i64(
    uchar threshold,
    intel_sub_group_avc_ime_payload_t payload )
{
    __spirv_AvcImePayloadINTEL ret = SPIRV_BUILTIN(SubgroupAvcImeSetEarlySearchTerminationThresholdINTEL, _i8_i64, )(threshold, __builtin_astype(payload, __spirv_AvcImePayloadINTEL));
    return __builtin_astype(ret, intel_sub_group_avc_ime_payload_t);
}

/*****************************************************************************\

Description:
    Get the early termination indication (W0.6 :24) from IME result payload.

\*****************************************************************************/
uchar SPIRV_OVERLOADABLE SPIRV_BUILTIN(SubgroupAvcImeGetUnidirectionalEarlySearchTerminationINTEL, _i64, )(
    __spirv_AvcImeResultINTEL result )
{
    uint4 handle = __builtin_IB_vme_helper_get_handle_avc_ime_result_t(result);
    return intel_get_message_phase_ub(handle, 0, 6*4+3) & 0x1;
}

INLINE uchar OVERLOADABLE
intel_sub_group_avc_ime_get_unidirectional_early_search_termination(
    intel_sub_group_avc_ime_result_t  result )
{
    return SPIRV_BUILTIN(SubgroupAvcImeGetUnidirectionalEarlySearchTerminationINTEL, _i64, )(__builtin_astype(result, __spirv_AvcImeResultINTEL));
}

/*****************************************************************************\

Description:
    Get the truncated search indication (W0.6 :25) from IME result payload.

\*****************************************************************************/
uchar SPIRV_OVERLOADABLE SPIRV_BUILTIN(SubgroupAvcImeGetTruncatedSearchIndicationINTEL, _i64, )(
    __spirv_AvcImeResultINTEL result )
{
    uint4 handle = __builtin_IB_vme_helper_get_handle_avc_ime_result_t(result);
    return (intel_get_message_phase_ub(handle, 0, 6*4+3) & (0x1 << 1)) >> 1;
}

INLINE uchar OVERLOADABLE
intel_sub_group_avc_ime_get_truncated_search_indication(
    intel_sub_group_avc_ime_result_t result )
{
    return SPIRV_BUILTIN(SubgroupAvcImeGetTruncatedSearchIndicationINTEL, _i64, )(__builtin_astype(result, __spirv_AvcImeResultINTEL));
}

/*****************************************************************************\

Description:
    - set WeightedSADHAAR M1.7 [20:20]
    - set Weighted SAD Control M1.3[31:0]

\*****************************************************************************/
__spirv_AvcImePayloadINTEL SPIRV_OVERLOADABLE SPIRV_BUILTIN(SubgroupAvcImeSetWeightedSadINTEL, _i32_i64, )(
    uint packed_sad_weights,
    __spirv_AvcImePayloadINTEL payload )
{
    uint4 handle = __builtin_IB_vme_helper_get_handle_avc_ime_payload_t(payload);
    // Set WeightedSADHAAR M1.7[20:20]
    const uint WeightedSADHAAR = intel_get_message_phase_dw(handle, 1, 7) | (1<<20);
    handle = intel_set_message_phase_dw(handle, 1, 7, WeightedSADHAAR);

    // Set Weighted SAD Control M1.3[31:0]
    handle = intel_set_message_phase_dw(handle, 1, 3, packed_sad_weights);

    return __builtin_IB_vme_helper_get_as_avc_ime_payload_t(handle);
}

INLINE intel_sub_group_avc_ime_payload_t OVERLOADABLE
intel_sub_group_avc_ime_set_weighted_sad(
    uint packed_sad_weights,
    intel_sub_group_avc_ime_payload_t payload )
{
    __spirv_AvcImePayloadINTEL ret = SPIRV_BUILTIN(SubgroupAvcImeSetWeightedSadINTEL, _i32_i64, )(packed_sad_weights, __builtin_astype(payload, __spirv_AvcImePayloadINTEL));
    return __builtin_astype(ret, intel_sub_group_avc_ime_payload_t);
}

/*****************************************************************************\

Description:
    Get the motion vector 0 W1.0 [31:0] from IME result payload.

\*****************************************************************************/
uint SPIRV_OVERLOADABLE SPIRV_BUILTIN(SubgroupAvcImeGetWeightingPatternMinimumMotionVectorINTEL, _i64, )(
    __spirv_AvcImeResultINTEL result )
{
    uint4 handle = __builtin_IB_vme_helper_get_handle_avc_ime_result_t(result);
    return intel_get_message_phase_dw(handle, 1, 0);
}

INLINE uint OVERLOADABLE
intel_sub_group_avc_ime_get_weighting_pattern_minimum_motion_vector(
    intel_sub_group_avc_ime_result_t  result )
{
    return SPIRV_BUILTIN(SubgroupAvcImeGetWeightingPatternMinimumMotionVectorINTEL, _i64, )(__builtin_astype(result, __spirv_AvcImeResultINTEL));
}

/*****************************************************************************\

Description:
    Get the inter border reached W5.0[15:0] from IME result payload.

\*****************************************************************************/
ushort SPIRV_OVERLOADABLE SPIRV_BUILTIN(SubgroupAvcImeGetWeightingPatternMinimumDistortionINTEL, _i64, )(
    __spirv_AvcImeResultINTEL result )
{
    uint4 handle = __builtin_IB_vme_helper_get_handle_avc_ime_result_t(result);
    return intel_get_message_phase_uw(handle, 5, 0);
}

INLINE ushort OVERLOADABLE
intel_sub_group_avc_ime_get_weighting_pattern_minimum_distortion(
    intel_sub_group_avc_ime_result_t  result )
{
    return SPIRV_BUILTIN(SubgroupAvcImeGetWeightingPatternMinimumDistortionINTEL, _i64, )(__builtin_astype(result, __spirv_AvcImeResultINTEL));
}

/*****************************************************************************\

Description:
    Get the inter border reached W0.1 from IME result payload.

\*****************************************************************************/
uchar SPIRV_OVERLOADABLE SPIRV_BUILTIN(SubgroupAvcImeGetBorderReachedINTEL, _i8_i64, )(
    uchar frame_select,
    __spirv_AvcImeResultINTEL result )
{
    uint4 handle = __builtin_IB_vme_helper_get_handle_avc_ime_result_t(result);
    uchar boarder_reached = intel_get_message_phase_ub(handle, 0, 1*4);

    // Ref0 select - W0.1[3:0]
    if (frame_select == CLK_AVC_ME_FRAME_FORWARD_INTEL) {
        boarder_reached &= 0x0F;
    }
    // Ref1 frame - W0.1[7:4]
    else if (frame_select == CLK_AVC_ME_FRAME_BACKWARD_INTEL) {
        boarder_reached >>= 4;
    }
    else {
        boarder_reached = 0;
    }

    return boarder_reached;
}

INLINE uchar OVERLOADABLE
intel_sub_group_avc_ime_get_border_reached(
    uchar frame_select,
    intel_sub_group_avc_ime_result_t  result )
{
    return SPIRV_BUILTIN(SubgroupAvcImeGetBorderReachedINTEL, _i8_i64, )(frame_select, __builtin_astype(result, __spirv_AvcImeResultINTEL));
}

// ... REF functions ...

/*****************************************************************************\

    Initialize BME payload:
        - set SrcX (M0.2 :15:0) and SrcY (M0.2 :31:16)
        - set SubBlockMV (M4 - M7)
        - set MbMode (M2.5 :1:0)
        - set SubMbShape (M2.5 :15:8)
        - set SubPredMode (M2.5 :23:16)
        - set SubPelMode (M0.3 :13:12)
        - set InterSAD (M0.3 :21:20)
        - set BMEDisableFBR (M0.3 :18) for FME
        - set BiWeight (M1.1 :21:16) for BME

\*****************************************************************************/
__spirv_AvcRefPayloadINTEL __intel_me_initialize_helper(
    ushort2 src_coord,
    ulong motion_vectors,
    uchar major_shape,
    uchar minor_shapes,
    uchar directions,
    uchar pixel_resolution,
    uchar bidirectional_weight,
    uchar sad_adjustment,
    bool is_bme )
{
    uint4 payload = __builtin_IB_create_message_phases_uint4(UNIVERSAL_INPUT_MESSAGE_NUM_GRFS+INPUT_MESSAGE_SIC_NUM_GRFS);

    // Set SrcX M0.2[15:0] and SrcY M0.2[31:16].
    payload = intel_set_message_phase_dw(payload, 0, 2, as_uint(src_coord));

    // set SubBlockMV (M4 - M7)
    payload = intel_simd_set_message_phase_uq(payload, 4, 4, 0, 4, motion_vectors);

    // Set MbMode M2.5[1:0].
    payload = intel_set_message_phase_ub(payload, 2, 5*4, major_shape);

    // Set SubMbShape M2.5[15:8].
    payload = intel_set_message_phase_ub(payload, 2, 5*4 + 1, minor_shapes);

    // Set SubPredMode M2.5[23:16].
    payload = intel_set_message_phase_ub(payload, 2, 5*4 + 2, directions);

    // Set SubPelMode M0.3[13:12], InterSAD M0.3[21:20]
    const uint imm = (sad_adjustment << 20) | (pixel_resolution << 12);
    payload = intel_set_message_phase_dw(payload, 0, 3, imm);

    // Set BiWeight M1.1[21:16] for BME
    if (is_bme) {
        payload = intel_set_message_phase_ub(payload, 1, 1*4+2, bidirectional_weight);
    }

    // Set BMEDisableFBR M0.3[18:18] for FME.
    if (!is_bme) {
        const uchar BMEDisableFBR = intel_get_message_phase_ub(payload, 0, 3*4+2) | (1<<2);
        payload = intel_set_message_phase_ub(payload, 0, 3*4+2, BMEDisableFBR);
    }

    return __builtin_IB_vme_helper_get_as_avc_ref_payload_t(payload);
}

__spirv_AvcRefPayloadINTEL SPIRV_OVERLOADABLE SPIRV_BUILTIN(SubgroupAvcBmeInitializeINTEL, _v2i16_i64_i8_i8_i8_i8_i8_i8, )(
    ushort2 src_coord,
    ulong motion_vectors,
    uchar major_shape,
    uchar minor_shapes,
    uchar directions,
    uchar pixel_resolution,
    uchar bidirectional_weight,
    uchar sad_adjustment )
{
    return __intel_me_initialize_helper(
        src_coord, motion_vectors, major_shape,
        minor_shapes, directions, pixel_resolution,
        bidirectional_weight, sad_adjustment, true);
}

INLINE intel_sub_group_avc_ref_payload_t OVERLOADABLE
intel_sub_group_avc_bme_initialize(
    ushort2 src_coord,
    ulong motion_vectors,
    uchar major_shape,
    uchar minor_shapes,
    uchar directions,
    uchar pixel_resolution,
    uchar bidirectional_weight,
    uchar sad_adjustment )
{
    __spirv_AvcRefPayloadINTEL ret = SPIRV_BUILTIN(SubgroupAvcBmeInitializeINTEL, _v2i16_i64_i8_i8_i8_i8_i8_i8, )(
        src_coord, motion_vectors, major_shape, minor_shapes, directions, pixel_resolution, bidirectional_weight, sad_adjustment);

    return __builtin_astype(ret, intel_sub_group_avc_ref_payload_t);
}

__spirv_AvcRefPayloadINTEL SPIRV_OVERLOADABLE SPIRV_BUILTIN(SubgroupAvcFmeInitializeINTEL, _v2i16_i64_i8_i8_i8_i8_i8, )(
    ushort2 src_coord,
    ulong motion_vectors,
    uchar major_shape,
    uchar minor_shapes,
    uchar directions,
    uchar pixel_resolution,
    uchar sad_adjustment )
{
    return __intel_me_initialize_helper(
        src_coord, motion_vectors, major_shape,
        minor_shapes, directions, pixel_resolution,
        0, sad_adjustment, false);
}

INLINE intel_sub_group_avc_ref_payload_t OVERLOADABLE
intel_sub_group_avc_fme_initialize(
    ushort2 src_coord,
    ulong motion_vectors,
    uchar major_shape,
    uchar minor_shapes,
    uchar directions,
    uchar pixel_resolution,
    uchar sad_adjustment )
{
    __spirv_AvcRefPayloadINTEL ret = SPIRV_BUILTIN(SubgroupAvcFmeInitializeINTEL, _v2i16_i64_i8_i8_i8_i8_i8, )(
        src_coord,
        motion_vectors,
        major_shape,
        minor_shapes,
        directions,
        pixel_resolution,
        sad_adjustment);

    return __builtin_astype(ret, intel_sub_group_avc_ref_payload_t);
}

__spirv_AvcMcePayloadINTEL SPIRV_OVERLOADABLE SPIRV_BUILTIN(SubgroupAvcRefConvertToMcePayloadINTEL, _i64, )(
    __spirv_AvcRefPayloadINTEL payload )
{
    uint4 handle = __builtin_IB_vme_helper_get_handle_avc_ref_payload_t(payload);
    return __builtin_IB_vme_helper_get_as_avc_mce_payload_t(handle);
}

INLINE intel_sub_group_avc_mce_payload_t OVERLOADABLE
intel_sub_group_avc_ref_convert_to_mce_payload(
      intel_sub_group_avc_ref_payload_t payload )
{
    return __builtin_astype(SPIRV_BUILTIN(SubgroupAvcRefConvertToMcePayloadINTEL, _i64, )(__builtin_astype(payload, __spirv_AvcRefPayloadINTEL)), intel_sub_group_avc_mce_payload_t);
}

__spirv_AvcRefPayloadINTEL SPIRV_OVERLOADABLE SPIRV_BUILTIN(SubgroupAvcMceConvertToRefPayloadINTEL, _i64, )(
    __spirv_AvcMcePayloadINTEL payload )
{
    uint4 handle = __builtin_IB_vme_helper_get_handle_avc_mce_payload_t(payload);
    return __builtin_IB_vme_helper_get_as_avc_ref_payload_t(handle);
}

INLINE intel_sub_group_avc_ref_payload_t OVERLOADABLE
intel_sub_group_avc_mce_convert_to_ref_payload(
      intel_sub_group_avc_mce_payload_t payload )
{
    return __builtin_astype(SPIRV_BUILTIN(SubgroupAvcMceConvertToRefPayloadINTEL, _i64, )(__builtin_astype(payload, __spirv_AvcMcePayloadINTEL)), intel_sub_group_avc_ref_payload_t);
}

/*****************************************************************************\

Description:
    - set BiMixDis M1.0[2:2]

\*****************************************************************************/
__spirv_AvcRefPayloadINTEL SPIRV_OVERLOADABLE SPIRV_BUILTIN(SubgroupAvcRefSetBidirectionalMixDisableINTEL, _i64, )(
    __spirv_AvcRefPayloadINTEL payload )
{
    uint4 handle = __builtin_IB_vme_helper_get_handle_avc_ref_payload_t(payload);
    const uint BiMixDis = intel_get_message_phase_dw(handle, 1, 0) | (1<<2);
    handle = intel_set_message_phase_dw(handle, 1, 0, BiMixDis);

    return __builtin_IB_vme_helper_get_as_avc_ref_payload_t( handle );
}

INLINE intel_sub_group_avc_ref_payload_t OVERLOADABLE
intel_sub_group_avc_ref_set_bidirectional_mix_disable(
    intel_sub_group_avc_ref_payload_t payload )
{
    return __builtin_astype(SPIRV_BUILTIN(SubgroupAvcRefSetBidirectionalMixDisableINTEL, _i64, )(__builtin_astype(payload, __spirv_AvcRefPayloadINTEL)), intel_sub_group_avc_ref_payload_t);
}

/*****************************************************************************\

Description:
    - set BilinearEnable M1.7[18:18]

\*****************************************************************************/
__spirv_AvcRefPayloadINTEL SPIRV_OVERLOADABLE SPIRV_BUILTIN(SubgroupAvcRefSetBilinearFilterEnableINTEL, _i64, )(
    __spirv_AvcRefPayloadINTEL payload )
{
    uint4 handle = __builtin_IB_vme_helper_get_handle_avc_ref_payload_t(payload);
    const uint BilinearEnable = intel_get_message_phase_dw(handle, 1, 7) | (1<<18);
    handle = intel_set_message_phase_dw(handle, 1, 7, BilinearEnable);

    return __builtin_IB_vme_helper_get_as_avc_ref_payload_t(handle);
}

INLINE intel_sub_group_avc_ref_payload_t OVERLOADABLE
intel_sub_group_avc_ref_set_bilinear_filter_enable(
      intel_sub_group_avc_ref_payload_t payload )
{
    return __builtin_astype(SPIRV_BUILTIN(SubgroupAvcRefSetBilinearFilterEnableINTEL, _i64, )(__builtin_astype(payload, __spirv_AvcRefPayloadINTEL)), intel_sub_group_avc_ref_payload_t);
}

/*****************************************************************************\

    Evalulate a single reference REF operation.

    (W0 W1 W2 W3 W4 W5 W6 W7) = (M0 M1 M2 M3 M4 M5) REF_EVALUATE_MULTI_REF ( (M0 M1) (M2 M3) (M4 M5) (M6 M7) )

\*****************************************************************************/
__spirv_AvcRefResultINTEL SPIRV_OVERLOADABLE SPIRV_BUILTIN(SubgroupAvcRefEvaluateWithDualReferenceINTEL, _i64_i64_i64_i64, )(
    __spirv_VmeImageINTEL__void_1_0_0_0_0_0_0 src_image_vme,
    __spirv_VmeImageINTEL__void_1_0_0_0_0_0_0 fwd_ref_image_vme,
    __spirv_VmeImageINTEL__void_1_0_0_0_0_0_0 bwd_ref_image_vme,
    __spirv_AvcRefPayloadINTEL payload )
{
    long src_image = __builtin_IB_get_image(src_image_vme);
    long fwd_ref_image = __builtin_IB_get_image(fwd_ref_image_vme);
    long bwd_ref_image = __builtin_IB_get_image(bwd_ref_image_vme);
    long vme_accelerator = __builtin_IB_get_sampler(src_image_vme);

    uint4 handle = __builtin_IB_vme_helper_get_handle_avc_ref_payload_t(payload);

    handle = intel_sub_group_payload_set_dual_ref_id(src_image, fwd_ref_image, bwd_ref_image, handle);
    uint4 res = __builtin_IB_vme_send_fbr_new(handle, src_image, fwd_ref_image, bwd_ref_image, vme_accelerator);

    return __builtin_IB_vme_helper_get_as_avc_ref_result_t(res);
}

INLINE intel_sub_group_avc_ref_result_t OVERLOADABLE
intel_sub_group_avc_ref_evaluate_with_dual_reference(
      read_only image2d_t src_image,
      read_only image2d_t fwd_ref_image,
      read_only image2d_t bwd_ref_image,
      sampler_t vme_accelerator,
      intel_sub_group_avc_ref_payload_t payload )
{
    __spirv_Image spv_src_image = __builtin_astype(src_image, __spirv_Image);
    __spirv_Image spv_fwd_ref_image = __builtin_astype(fwd_ref_image, __spirv_Image);
    __spirv_Image spv_bwd_ref_image = __builtin_astype(bwd_ref_image, __spirv_Image);;
    __spirv_Sampler spv_vme_accelerator = __builtin_astype(vme_accelerator, __spirv_Sampler);
    __spirv_VmeImageINTEL__void_1_0_0_0_0_0_0 src_image_vme = SPIRV_BUILTIN(VmeImageINTEL, _i64_i64, )(spv_src_image, spv_vme_accelerator);
    __spirv_VmeImageINTEL__void_1_0_0_0_0_0_0 fwd_ref_image_vme = SPIRV_BUILTIN(VmeImageINTEL, _i64_i64, )(spv_fwd_ref_image, spv_vme_accelerator);
    __spirv_VmeImageINTEL__void_1_0_0_0_0_0_0 bwd_ref_image_vme = SPIRV_BUILTIN(VmeImageINTEL, _i64_i64, )(spv_bwd_ref_image, spv_vme_accelerator);

    __spirv_AvcRefResultINTEL ret =SPIRV_BUILTIN(SubgroupAvcRefEvaluateWithDualReferenceINTEL, _i64_i64_i64_i64, )(
        src_image_vme, fwd_ref_image_vme, bwd_ref_image_vme, __builtin_astype(payload, __spirv_AvcRefPayloadINTEL));

    return __builtin_astype(ret, intel_sub_group_avc_ref_result_t);
}

__spirv_AvcRefResultINTEL SPIRV_OVERLOADABLE SPIRV_BUILTIN(SubgroupAvcRefEvaluateWithSingleReferenceINTEL, _i64_i64_i64, )(
    __spirv_VmeImageINTEL__void_1_0_0_0_0_0_0 src_image_vme,
    __spirv_VmeImageINTEL__void_1_0_0_0_0_0_0 ref_image_vme,
    __spirv_AvcRefPayloadINTEL payload )
{
    long src_image = __builtin_IB_get_image(src_image_vme);
    long ref_image = __builtin_IB_get_image(ref_image_vme);
    long vme_accelerator = __builtin_IB_get_sampler(src_image_vme);

    uint4 handle = __builtin_IB_vme_helper_get_handle_avc_ref_payload_t(payload);

    handle = intel_sub_group_payload_set_single_ref_id(src_image, ref_image, handle);
    uint4 res = __builtin_IB_vme_send_fbr_new(handle, src_image, ref_image, ref_image, vme_accelerator);

    return __builtin_IB_vme_helper_get_as_avc_ref_result_t(res);
}

INLINE intel_sub_group_avc_ref_result_t OVERLOADABLE
intel_sub_group_avc_ref_evaluate_with_single_reference(
      read_only image2d_t src_image,
      read_only image2d_t ref_image,
      sampler_t vme_accelerator,
      intel_sub_group_avc_ref_payload_t payload )
{
    __spirv_Image spv_src_image = __builtin_astype(src_image, __spirv_Image);
    __spirv_Image spv_ref_image = __builtin_astype(ref_image, __spirv_Image);
    __spirv_Sampler spv_vme_accelerator = __builtin_astype(vme_accelerator, __spirv_Sampler);
    __spirv_VmeImageINTEL__void_1_0_0_0_0_0_0 src_image_vme = SPIRV_BUILTIN(VmeImageINTEL, _i64_i64, )(spv_src_image, spv_vme_accelerator);
    __spirv_VmeImageINTEL__void_1_0_0_0_0_0_0 ref_image_vme = SPIRV_BUILTIN(VmeImageINTEL, _i64_i64, )(spv_ref_image, spv_vme_accelerator);

    __spirv_AvcRefResultINTEL ret = SPIRV_BUILTIN(SubgroupAvcRefEvaluateWithSingleReferenceINTEL, _i64_i64_i64, )(
        src_image_vme, ref_image_vme, __builtin_astype(payload, __spirv_AvcRefPayloadINTEL));

    return __builtin_astype(ret, intel_sub_group_avc_ref_result_t);
}

__spirv_AvcMceResultINTEL SPIRV_OVERLOADABLE SPIRV_BUILTIN(SubgroupAvcRefConvertToMceResultINTEL, _i64, )(
    __spirv_AvcRefResultINTEL result )
{
    uint4 handle = __builtin_IB_vme_helper_get_handle_avc_ref_result_t(result);
    return __builtin_IB_vme_helper_get_as_avc_mce_result_t(handle);
}

INLINE intel_sub_group_avc_mce_result_t OVERLOADABLE
intel_sub_group_avc_ref_convert_to_mce_result(
      intel_sub_group_avc_ref_result_t result )
{
    return __builtin_astype(
        SPIRV_BUILTIN(SubgroupAvcRefConvertToMceResultINTEL, _i64, )(
            __builtin_astype(result, __spirv_AvcRefResultINTEL)),
        intel_sub_group_avc_mce_result_t);
}

__spirv_AvcRefResultINTEL SPIRV_OVERLOADABLE SPIRV_BUILTIN(SubgroupAvcMceConvertToRefResultINTEL, _i64, )(
    __spirv_AvcMceResultINTEL result )
{
    uint4 handle = __builtin_IB_vme_helper_get_handle_avc_mce_result_t(result);
    return __builtin_IB_vme_helper_get_as_avc_ref_result_t(handle);
}

INLINE intel_sub_group_avc_ref_result_t OVERLOADABLE
intel_sub_group_avc_mce_convert_to_ref_result(
      intel_sub_group_avc_mce_result_t result )
{
    __spirv_AvcRefResultINTEL ret = SPIRV_BUILTIN(SubgroupAvcMceConvertToRefResultINTEL, _i64, )(__builtin_astype(result, __spirv_AvcMceResultINTEL));
    return __builtin_astype(ret, intel_sub_group_avc_ref_result_t);
}

// ... SIC functions ...

/*****************************************************************************\

Description:
    Initialize SIC payload M0:
        - set SrcX M0.2[15:0] and SrcY M0.2[31:16]

\*****************************************************************************/
__spirv_AvcSicPayloadINTEL SPIRV_OVERLOADABLE SPIRV_BUILTIN(SubgroupAvcSicInitializeINTEL, _v2i16, )(
    ushort2 src_coord )
{
    // Create and initialize SIC payload
    uint4 payload = __builtin_IB_create_message_phases_uint4(UNIVERSAL_INPUT_MESSAGE_NUM_GRFS+INPUT_MESSAGE_SIC_NUM_GRFS);

    // set SrcX M0.2[15:0] and SrcY M0.2[31:16]
    payload = intel_set_message_phase_dw(payload, 0, 2, as_uint(src_coord));

    return __builtin_IB_vme_helper_get_as_avc_sic_payload_t(payload);
}

INLINE intel_sub_group_avc_sic_payload_t OVERLOADABLE
intel_sub_group_avc_sic_initialize(
    ushort2 src_coord )
{
    return __builtin_astype(SPIRV_BUILTIN(SubgroupAvcSicInitializeINTEL, _v2i16, )(src_coord), intel_sub_group_avc_sic_payload_t);
}

uint SPIRV_OVERLOADABLE SPIRV_BUILTIN(SubgroupAvcSicGetMotionVectorMaskINTEL, _i32_i8, )(
    uint skip_block_partition_type,
    uchar direction )
{
    uint mask =
        ( skip_block_partition_type == CLK_AVC_ME_SKIP_BLOCK_PARTITION_16x16_INTEL ) ?
            direction + 1 :
            direction + 0x55;
    mask = ( mask << 24 );
    return mask;
}

INLINE uint OVERLOADABLE
intel_sub_group_avc_sic_get_motion_vector_mask(
  uint skip_block_partition_type,
  uchar direction )
{
    return SPIRV_BUILTIN(SubgroupAvcSicGetMotionVectorMaskINTEL, _i32_i8, )(skip_block_partition_type, direction);
}

/*****************************************************************************\

Description:
    Configure SIC payload :
        - set Skip Mode Type (M0.3 :14)
        - set SkipModeEn (M1.0 :0)
        - set SkipCenterMask (M1.7 :31:24)
        - set BiWeight (M1.1 :21:16)
        - set InterSAD (M0.3 :21:20)
        - set SkipCenter Delta XY (M4.0)
        - set IntraComputeType (M5.1 :9:8)

\*****************************************************************************/
__spirv_AvcSicPayloadINTEL SPIRV_OVERLOADABLE SPIRV_BUILTIN(SubgroupAvcSicConfigureSkcINTEL, _i32_i32_i64_i8_i8_i64, )(
    uint skip_block_partition_type,
    uint skip_motion_vector_mask,
    ulong motion_vectors,
    uchar bidirectional_weight,
    uchar skip_sad_adjustment,
    __spirv_AvcSicPayloadINTEL payload )
{
    uint4 handle = __builtin_IB_vme_helper_get_handle_avc_sic_payload_t(payload);

    // Set Skip Mode Type M0.3[14:14] - the value is already shifted by 14 bits.
    handle = intel_set_message_phase_dw(handle, 0, 3, skip_block_partition_type);

    // Set SkipModeEn M1.0[0:0].
    handle = intel_set_message_phase_ub(handle, 1, 0, 1);

    // Set SkipCenterMask M1.7[31:24] - value is shifted by 24 bits already
    const uint SkipCenterMask = intel_get_message_phase_dw(handle, 1, 7) | skip_motion_vector_mask;
    handle = intel_set_message_phase_dw(handle, 1, 7, SkipCenterMask);

    // Set BiWeight M1.1[21:16]
    handle = intel_set_message_phase_ub(handle, 1, 1*4+2, bidirectional_weight);

    // Set InterSAD M0.3[21:20]
    const uchar InterSAD = intel_get_message_phase_ub(handle, 0, 3*4+2) | (skip_sad_adjustment << 4);
    handle = intel_set_message_phase_ub(handle, 0, 3*4+2, InterSAD);

    // Set SkipCenter Delta XY (M4.0)
    handle = intel_simd_set_message_phase_uq(handle, 4, 1, 0, 4, motion_vectors);

    // Set IntraComputeType M5.1[9:8].
    handle = intel_set_message_phase_ub(handle, 5, 1*4+1, 0x1);

    return __builtin_IB_vme_helper_get_as_avc_sic_payload_t( handle );
}

INLINE intel_sub_group_avc_sic_payload_t OVERLOADABLE
intel_sub_group_avc_sic_configure_skc(
    uint skip_block_partition_type,
    uint skip_motion_vector_mask,
    ulong motion_vectors,
    uchar bidirectional_weight,
    uchar skip_sad_adjustment,
    intel_sub_group_avc_sic_payload_t payload )
{
    __spirv_AvcSicPayloadINTEL ret = SPIRV_BUILTIN(SubgroupAvcSicConfigureSkcINTEL, _i32_i32_i64_i8_i8_i64, )(
        skip_block_partition_type, skip_motion_vector_mask, motion_vectors, bidirectional_weight, skip_sad_adjustment, __builtin_astype(payload, __spirv_AvcSicPayloadINTEL));
    return __builtin_astype(ret, intel_sub_group_avc_sic_payload_t);
}

/*****************************************************************************\

Description:
    Configure SIC payload :
        - set IntraPartMask (M1.7 :4:0)
        - set MbIntraStruct (M1.7 :15:8)
        - set LeftEdgeLuma A [-1, 15] to [-1, 0] (M6.0-M6.3)
        - set LeftTopCornerLuma D (M5.1 :31:24)
        - set UpperEdgeLuma B [15, -1] to [0, -1] (M5.5-M5.2)
        - set UpperRightEdgeLuma C [23, -1] to [16, -1] (M5.7-M5.6)
        - set LeftEdgeChroma A [-1, 7] to [-1, 0] (M8.0-M3.3]
        - set LeftTopCornerChroma D (M7.5 :15:0)
        - set UpperEdgeChroma B [7, -1] to [0, -1] (M8.4-M8.7)
        - set IntraSAD (M0.3 :23:22)

\*****************************************************************************/
__spirv_AvcSicPayloadINTEL SPIRV_OVERLOADABLE SPIRV_BUILTIN(SubgroupAvcSicConfigureIpeLumaINTEL, _i8_i8_i8_i8_i8_i8_i8_i64, )(
    uchar luma_intra_partition_mask,
    uchar intra_neighbour_availabilty,
    uchar left_edge_pixels,
    uchar left_upper_edge_pixel,
    uchar upper_edge_pixels,
    uchar upper_right_edge_pixels,
    uchar intra_sad_adjustment,
    __spirv_AvcSicPayloadINTEL sic_payload )
{
    uint4 payload = __builtin_IB_vme_helper_get_handle_avc_sic_payload_t(sic_payload);

    // Set IntraPartMask (M1.7 :4:0). (M1.7 :7:4 are set to zero too)
    payload = intel_set_message_phase_ub(payload, 1, 7*4, luma_intra_partition_mask);

    // Set MbIntraStruct (M1.7 :15:8).
    payload = intel_set_message_phase_ub(payload, 1, 7*4+1, intra_neighbour_availabilty);

    // Set LeftEdge A [-1, 15] to [-1, 0] at M6.0-M6.3
    payload = intel_simd_set_message_phase_ub(payload, 6, 1, 0, 16, left_edge_pixels);

    // Set LeftTopCorner D (M5.1 :31:24) - (M5.1 :23:16 MBZ)
    payload = intel_set_message_phase_ub(payload, 5, 1*4+3, left_upper_edge_pixel);

    // Set UpperEdge B [15, -1] to [0, -1] (M5.5 - M5.2)
    payload = intel_simd_set_message_phase_ub(payload, 5, 1, 2*4, 16, upper_edge_pixels);

    // Set UpperRightEdge C [23, -1] to [16, -1] (M5.7-M5.6).
    payload = intel_simd_set_message_phase_ub(payload, 5, 1, 6*4, 8, upper_right_edge_pixels);

    // Set IntraSAD M0.3[23:22]
    const uint IntraSAD = intel_get_message_phase_dw(payload, 0, 3) | (intra_sad_adjustment << 22);
    payload = intel_set_message_phase_dw(payload, 0, 3, IntraSAD);

    // Set IntraComputeType M5.1[9:8] to 01 for Luma only.
    payload = intel_set_message_phase_ub(payload, 5, 1*4+1, 1);

    return __builtin_IB_vme_helper_get_as_avc_sic_payload_t(payload);
}

INLINE intel_sub_group_avc_sic_payload_t OVERLOADABLE
intel_sub_group_avc_sic_configure_ipe(
    uchar luma_intra_partition_mask,
    uchar intra_neighbour_availabilty,
    uchar left_edge_pixels,
    uchar left_upper_edge_pixel,
    uchar upper_edge_pixels,
    uchar upper_right_edge_pixels,
    uchar intra_sad_adjustment,
    intel_sub_group_avc_sic_payload_t sic_payload)
{
    __spirv_AvcSicPayloadINTEL ret = SPIRV_BUILTIN(SubgroupAvcSicConfigureIpeLumaINTEL, _i8_i8_i8_i8_i8_i8_i8_i64, )(
        luma_intra_partition_mask,
        intra_neighbour_availabilty,
        left_edge_pixels,
        left_upper_edge_pixel,
        upper_edge_pixels,
        upper_right_edge_pixels,
        intra_sad_adjustment,
        __builtin_astype(sic_payload, __spirv_AvcSicPayloadINTEL));

    return __builtin_astype(ret, intel_sub_group_avc_sic_payload_t);
}

/*****************************************************************************\

Description:
    Configure SIC payload :
        - set IntraPartMask (M1.7 :4:0)
        - set MbIntraStruct (M1.7 :15:8)
        - set LeftEdgeLuma A [-1, 15] to [-1, 0] (M6.0-M6.3)
        - set LeftTopCornerLuma D (M5.1 :31:24)
        - set UpperEdgeLuma B [15, -1] to [0, -1] (M5.5-M5.2)
        - set UpperRightEdgeLuma C [23, -1] to [16, -1] (M5.7-M5.6)
        - set LeftEdgeChroma A [-1, 7] to [-1, 0] (M8.0-M3.3]
        - set LeftTopCornerChroma D (M7.5 :15:0)
        - set UpperEdgeChroma B [7, -1] to [0, -1] (M8.4-M8.7)
        - set IntraSAD (M0.3 :23:22)

\*****************************************************************************/
__spirv_AvcSicPayloadINTEL SPIRV_OVERLOADABLE SPIRV_BUILTIN(SubgroupAvcSicConfigureIpeLumaChromaINTEL, _i8_i8_i8_i8_i8_i8_i16_i16_i16_i8_i64, )(
    uchar luma_intra_partition_mask,
    uchar intra_neighbour_availabilty,
    uchar left_edge_luma_pixels,
    uchar left_upper_edge_luma_pixel,
    uchar upper_edge_luma_pixels,
    uchar upper_right_edge_luma_pixels,
    ushort left_edge_chroma_pixels,
    ushort upper_left_corner_chroma_pixel,
    ushort upper_edge_chroma_pixels,
    uchar intra_sad_adjustment,
    __spirv_AvcSicPayloadINTEL sic_payload )
{
    uint4 payload = __builtin_IB_vme_helper_get_handle_avc_sic_payload_t(sic_payload);

    // Set IntraPartMask (M1.7 [4:0]). (M1.7 [7:4] are set to zero too)
    payload = intel_set_message_phase_ub(payload, 1, 7*4, luma_intra_partition_mask);

    // Set MbIntraStruct (M1.7 [15:8]).
    payload = intel_set_message_phase_ub(payload, 1, 7*4+1, intra_neighbour_availabilty);

    // Set LeftEdge A [-1, 15] to [-1, 0] at M6.0-M6.3
    payload = intel_simd_set_message_phase_ub(payload, 6, 1, 0, 16, left_edge_luma_pixels);

    // Set LeftTopCorner D (M5.1 :31:24) - (M5.1 :23:16 MBZ)
    payload = intel_set_message_phase_ub(payload, 5, 1*4+3, left_upper_edge_luma_pixel);

    // Set UpperEdge B [15, -1] to [0, -1] (M5.5 - M5.2)
    payload = intel_simd_set_message_phase_ub(payload, 5, 1, 2*4, 16, upper_edge_luma_pixels);

    // Set UpperRightEdge C [23, -1] to [16, -1] (M5.7-M5.6).
    payload = intel_simd_set_message_phase_ub(payload, 5, 1, 6*4, 8, upper_right_edge_luma_pixels);

    // Set LeftEdgeChroma A [-1, 7] to [-1, 0] (M7.0-M7.3)
    payload = intel_simd_set_message_phase_uw(payload, 7, 1, 0, 8, left_edge_chroma_pixels);

    // Set LeftTopCornerChroma D M6.5 [15:0]
    payload = intel_set_message_phase_uw(payload, 6, 5*2, upper_left_corner_chroma_pixel);

    // Set UpperEdgeChroma B [7, -1] to [0, -1] (M7.7 - M7.4)
    payload = intel_simd_set_message_phase_uw(payload, 7, 1, 4*2, 8, upper_edge_chroma_pixels);

    // Set IntraSAD M0.3[23:22]
    const uint IntraSAD = intel_get_message_phase_dw(payload, 0, 3) | (intra_sad_adjustment << 22);
    payload = intel_set_message_phase_dw(payload, 0, 3, IntraSAD);

    // Set IntraComputeType M5.1[9:8] to 0 for Luma + Chroma enabled
    payload = intel_set_message_phase_uw(payload, 5, 1*2, 0);

    return __builtin_IB_vme_helper_get_as_avc_sic_payload_t( payload );
}

INLINE intel_sub_group_avc_sic_payload_t OVERLOADABLE
intel_sub_group_avc_sic_configure_ipe(
    uchar  luma_intra_partition_mask,
    uchar  intra_neighbour_availabilty,
    uchar  left_edge_luma_pixels,
    uchar  left_upper_edge_luma_pixel,
    uchar  upper_edge_luma_pixels,
    uchar  upper_right_edge_luma_pixels,
    ushort left_edge_chroma_pixels,
    ushort upper_left_corner_chroma_pixel,
    ushort upper_edge_chroma_pixels,
    uchar intra_sad_adjustment,
    intel_sub_group_avc_sic_payload_t sic_payload )
{
    __spirv_AvcSicPayloadINTEL ret = SPIRV_BUILTIN(SubgroupAvcSicConfigureIpeLumaChromaINTEL, _i8_i8_i8_i8_i8_i8_i16_i16_i16_i8_i64, )(
        luma_intra_partition_mask,
        intra_neighbour_availabilty,
        left_edge_luma_pixels,
        left_upper_edge_luma_pixel,
        upper_edge_luma_pixels,
        upper_right_edge_luma_pixels,
        left_edge_chroma_pixels,
        upper_left_corner_chroma_pixel,
        upper_edge_chroma_pixels,
        intra_sad_adjustment,
        __builtin_astype(sic_payload, __spirv_AvcSicPayloadINTEL));

    return __builtin_astype(ret, intel_sub_group_avc_sic_payload_t);
}

__spirv_AvcMcePayloadINTEL SPIRV_OVERLOADABLE SPIRV_BUILTIN(SubgroupAvcSicConvertToMcePayloadINTEL, _i64, )(
    __spirv_AvcSicPayloadINTEL payload )
{
    uint4 handle = __builtin_IB_vme_helper_get_handle_avc_sic_payload_t(payload);
    return __builtin_IB_vme_helper_get_as_avc_mce_payload_t(handle);
}

INLINE intel_sub_group_avc_mce_payload_t OVERLOADABLE
intel_sub_group_avc_sic_convert_to_mce_payload(
      intel_sub_group_avc_sic_payload_t payload )
{
    return __builtin_astype(SPIRV_BUILTIN(SubgroupAvcSicConvertToMcePayloadINTEL, _i64, )(__builtin_astype(payload, __spirv_AvcSicPayloadINTEL)), intel_sub_group_avc_mce_payload_t);
}

__spirv_AvcSicPayloadINTEL SPIRV_OVERLOADABLE SPIRV_BUILTIN(SubgroupAvcMceConvertToSicPayloadINTEL, _i64, )(
    __spirv_AvcMcePayloadINTEL payload )
{
    uint4 handle = __builtin_IB_vme_helper_get_handle_avc_mce_payload_t(payload);
    return __builtin_IB_vme_helper_get_as_avc_sic_payload_t(handle);
}

INLINE intel_sub_group_avc_sic_payload_t OVERLOADABLE
intel_sub_group_avc_mce_convert_to_sic_payload(
      intel_sub_group_avc_mce_payload_t payload )
{
    return __builtin_astype(SPIRV_BUILTIN(SubgroupAvcMceConvertToSicPayloadINTEL, _i64, )(__builtin_astype(payload, __spirv_AvcMcePayloadINTEL)), intel_sub_group_avc_sic_payload_t);
}

/*****************************************************************************\

Description:
    - set BilinearEnable M1.7[18:18]

\*****************************************************************************/
__spirv_AvcSicPayloadINTEL SPIRV_OVERLOADABLE SPIRV_BUILTIN(SubgroupAvcSicSetBilinearFilterEnableINTEL, _i64, )(
    __spirv_AvcSicPayloadINTEL payload )
{
    uint4 handle = __builtin_IB_vme_helper_get_handle_avc_sic_payload_t(payload);
    const uint BilinearEnable = intel_get_message_phase_dw(handle, 1, 7) | (1<<18);
    handle = intel_set_message_phase_dw(handle, 1, 7, BilinearEnable);

    return __builtin_IB_vme_helper_get_as_avc_sic_payload_t(handle);
}

INLINE intel_sub_group_avc_sic_payload_t OVERLOADABLE
intel_sub_group_avc_sic_set_skc_bilinear_filter_enable(
      intel_sub_group_avc_sic_payload_t payload )
{
    return __builtin_astype(SPIRV_BUILTIN(SubgroupAvcSicSetBilinearFilterEnableINTEL, _i64, )(__builtin_astype(payload, __spirv_AvcSicPayloadINTEL)), intel_sub_group_avc_sic_payload_t);
}

/*****************************************************************************\
Description:
    - set FTEnable: M0.3[17:17]
    - set SIC Forward Transform Coeff Threshold Matrix[0] (M2.6, M2.7)

\*****************************************************************************/
__spirv_AvcSicPayloadINTEL SPIRV_OVERLOADABLE SPIRV_BUILTIN(SubgroupAvcSicSetSkcForwardTransformEnableINTEL, _i64_i64, )(
    ulong packed_sad_coefficients,
    __spirv_AvcSicPayloadINTEL payload )
{
    uint4 handle = __builtin_IB_vme_helper_get_handle_avc_sic_payload_t(payload);
    // Set FTEnable: M0.3[17:17]
    const uint FTEnable = intel_get_message_phase_dw(handle, 0, 3) | (1<<17);
    handle = intel_set_message_phase_dw(handle, 0, 3, FTEnable);

    // Set SIC Forward Transform Coeff Threshold Matrix (M2.6, M2.7)
    handle = intel_set_message_phase_dw(handle, 2, 6, (uint)packed_sad_coefficients);
    handle = intel_set_message_phase_dw(handle, 2, 7, (uint)(packed_sad_coefficients >> 32));

    return __builtin_IB_vme_helper_get_as_avc_sic_payload_t(handle);
}

INLINE intel_sub_group_avc_sic_payload_t OVERLOADABLE
intel_sub_group_avc_sic_set_skc_forward_transform_enable(
      ulong  packed_sad_coefficients,
      intel_sub_group_avc_sic_payload_t payload )
{
    __spirv_AvcSicPayloadINTEL ret = SPIRV_BUILTIN(SubgroupAvcSicSetSkcForwardTransformEnableINTEL, _i64_i64, )(
        packed_sad_coefficients,
        __builtin_astype(payload, __spirv_AvcSicPayloadINTEL));

    return __builtin_astype(ret, intel_sub_group_avc_sic_payload_t);
}

/*****************************************************************************\
Description:
    - set BlockBasedSkipEnabled (M0.3 :19)
    - set T8x8FlagForInterEn (M1.0 : 7)

\*****************************************************************************/
__spirv_AvcSicPayloadINTEL SPIRV_OVERLOADABLE SPIRV_BUILTIN(SubgroupAvcSicSetBlockBasedRawSkipSadINTEL, _i8_i64, )(
    uchar block_based_skip_block_type,
    __spirv_AvcSicPayloadINTEL payload )
{
    uint4 handle = __builtin_IB_vme_helper_get_handle_avc_sic_payload_t(payload);
    uint dw = intel_get_message_phase_dw(handle, 0, 3) | (0x1 << 19);
    handle = intel_set_message_phase_dw(handle, 0, 3, dw);

    uchar flag = intel_get_message_phase_ub(handle, 1, 0*4);
    flag &= ~0x80;
    flag |= block_based_skip_block_type;
    handle = intel_set_message_phase_ub(handle, 1, 0*4, flag);

    return __builtin_IB_vme_helper_get_as_avc_sic_payload_t(handle);
}

INLINE intel_sub_group_avc_sic_payload_t OVERLOADABLE
intel_sub_group_avc_sic_set_block_based_raw_skip_sad(
      uchar block_based_skip_block_type,
      intel_sub_group_avc_sic_payload_t payload )
{
    __spirv_AvcSicPayloadINTEL ret = SPIRV_BUILTIN(SubgroupAvcSicSetBlockBasedRawSkipSadINTEL, _i8_i64, )(
        block_based_skip_block_type,
        __builtin_astype(payload, __spirv_AvcSicPayloadINTEL));

    return __builtin_astype(ret, intel_sub_group_avc_sic_payload_t);
}

__spirv_AvcSicResultINTEL SPIRV_OVERLOADABLE SPIRV_BUILTIN(SubgroupAvcSicEvaluateIpeINTEL, _i64_i64, )(
    __spirv_VmeImageINTEL__void_1_0_0_0_0_0_0 src_image_vme,
    __spirv_AvcSicPayloadINTEL payload )
{
    long src_image = __builtin_IB_get_image(src_image_vme);
    long vme_accelerator = __builtin_IB_get_sampler(src_image_vme);

    uint4 handle = __builtin_IB_vme_helper_get_handle_avc_sic_payload_t(payload);
    uint4 res = __builtin_IB_vme_send_sic_new(handle, src_image, src_image, src_image, vme_accelerator);

    return __builtin_IB_vme_helper_get_as_avc_sic_result_t(res);
}

INLINE intel_sub_group_avc_sic_result_t OVERLOADABLE
intel_sub_group_avc_sic_evaluate_ipe(
      read_only image2d_t src_image,
      sampler_t vme_accelerator,
      intel_sub_group_avc_sic_payload_t payload )
{
    __spirv_Image spv_src_image = __builtin_astype(src_image, __spirv_Image);
    __spirv_Sampler spv_vme_accelerator = __builtin_astype(vme_accelerator, __spirv_Sampler);
    __spirv_VmeImageINTEL__void_1_0_0_0_0_0_0 src_image_vme = SPIRV_BUILTIN(VmeImageINTEL, _i64_i64, )(spv_src_image, spv_vme_accelerator);

    __spirv_AvcSicResultINTEL ret = SPIRV_BUILTIN(SubgroupAvcSicEvaluateIpeINTEL, _i64_i64, )(
        src_image_vme,
        __builtin_astype(payload, __spirv_AvcSicPayloadINTEL));

    return __builtin_astype(ret, intel_sub_group_avc_sic_result_t);
}

__spirv_AvcSicResultINTEL SPIRV_OVERLOADABLE SPIRV_BUILTIN(SubgroupAvcSicEvaluateWithSingleReferenceINTEL, _i64_i64_i64, )(
    __spirv_VmeImageINTEL__void_1_0_0_0_0_0_0 src_image_vme,
    __spirv_VmeImageINTEL__void_1_0_0_0_0_0_0 ref_image_vme,
    __spirv_AvcSicPayloadINTEL payload )
{
    long src_image = __builtin_IB_get_image(src_image_vme);
    long ref_image = __builtin_IB_get_image(ref_image_vme);
    long vme_accelerator = __builtin_IB_get_sampler(src_image_vme);

    uint4 handle = __builtin_IB_vme_helper_get_handle_avc_sic_payload_t(payload);
    handle = intel_sub_group_payload_set_single_ref_id(src_image, ref_image, handle);
    uint4 res = __builtin_IB_vme_send_sic_new(handle, src_image, ref_image, ref_image, vme_accelerator);

    return __builtin_IB_vme_helper_get_as_avc_sic_result_t(res);
}

INLINE intel_sub_group_avc_sic_result_t OVERLOADABLE
intel_sub_group_avc_sic_evaluate_with_single_reference(
      read_only image2d_t src_image,
      read_only image2d_t ref_image,
      sampler_t vme_accelerator,
      intel_sub_group_avc_sic_payload_t payload )
{
    __spirv_Image spv_src_image = __builtin_astype(src_image, __spirv_Image);
    __spirv_Image spv_ref_image = __builtin_astype(ref_image, __spirv_Image);
    __spirv_Sampler spv_vme_accelerator = __builtin_astype(vme_accelerator, __spirv_Sampler);
    __spirv_VmeImageINTEL__void_1_0_0_0_0_0_0 src_image_vme = SPIRV_BUILTIN(VmeImageINTEL, _i64_i64, )(spv_src_image, spv_vme_accelerator);
    __spirv_VmeImageINTEL__void_1_0_0_0_0_0_0 ref_image_vme = SPIRV_BUILTIN(VmeImageINTEL, _i64_i64, )(spv_ref_image, spv_vme_accelerator);

    __spirv_AvcSicResultINTEL ret = SPIRV_BUILTIN(SubgroupAvcSicEvaluateWithSingleReferenceINTEL, _i64_i64_i64, )(
        src_image_vme,
        ref_image_vme,
        __builtin_astype(payload, __spirv_AvcSicPayloadINTEL));

    return __builtin_astype(ret, intel_sub_group_avc_sic_result_t);
}

__spirv_AvcSicResultINTEL SPIRV_OVERLOADABLE SPIRV_BUILTIN(SubgroupAvcSicEvaluateWithDualReferenceINTEL, _i64_i64_i64_i64, )(
    __spirv_VmeImageINTEL__void_1_0_0_0_0_0_0 src_image_vme,
    __spirv_VmeImageINTEL__void_1_0_0_0_0_0_0 fwd_ref_image_vme,
    __spirv_VmeImageINTEL__void_1_0_0_0_0_0_0 bwd_ref_image_vme,
    __spirv_AvcSicPayloadINTEL payload )
{
    long src_image = __builtin_IB_get_image(src_image_vme);
    long fwd_ref_image = __builtin_IB_get_image(fwd_ref_image_vme);
    long bwd_ref_image = __builtin_IB_get_image(bwd_ref_image_vme);
    long vme_accelerator = __builtin_IB_get_sampler(src_image_vme);

    uint4 handle = __builtin_IB_vme_helper_get_handle_avc_sic_payload_t(payload);
    handle = intel_sub_group_payload_set_dual_ref_id(src_image, fwd_ref_image, bwd_ref_image, handle);
    uint4 res = __builtin_IB_vme_send_sic_new(handle, src_image, fwd_ref_image, bwd_ref_image, vme_accelerator);

    return __builtin_IB_vme_helper_get_as_avc_sic_result_t(res);
}

INLINE intel_sub_group_avc_sic_result_t OVERLOADABLE
intel_sub_group_avc_sic_evaluate_with_dual_reference(
      read_only image2d_t src_image,
      read_only image2d_t fwd_ref_image,
      read_only image2d_t bwd_ref_image,
      sampler_t vme_accelerator,
      intel_sub_group_avc_sic_payload_t payload )
{
    __spirv_Image spv_src_image = __builtin_astype(src_image, __spirv_Image);
    __spirv_Image spv_fwd_ref_image = __builtin_astype(fwd_ref_image, __spirv_Image);
    __spirv_Image spv_bwd_ref_image = __builtin_astype(bwd_ref_image, __spirv_Image);;
    __spirv_Sampler spv_vme_accelerator = __builtin_astype(vme_accelerator, __spirv_Sampler);
    __spirv_VmeImageINTEL__void_1_0_0_0_0_0_0 src_image_vme = SPIRV_BUILTIN(VmeImageINTEL, _i64_i64, )(spv_src_image, spv_vme_accelerator);
    __spirv_VmeImageINTEL__void_1_0_0_0_0_0_0 fwd_ref_image_vme = SPIRV_BUILTIN(VmeImageINTEL, _i64_i64, )(spv_fwd_ref_image, spv_vme_accelerator);
    __spirv_VmeImageINTEL__void_1_0_0_0_0_0_0 bwd_ref_image_vme = SPIRV_BUILTIN(VmeImageINTEL, _i64_i64, )(spv_bwd_ref_image, spv_vme_accelerator);

    __spirv_AvcSicResultINTEL ret = SPIRV_BUILTIN(SubgroupAvcSicEvaluateWithDualReferenceINTEL, _i64_i64_i64_i64, )(
        src_image_vme,
        fwd_ref_image_vme,
        bwd_ref_image_vme,
        __builtin_astype(payload, __spirv_AvcSicPayloadINTEL));

    return __builtin_astype(ret, intel_sub_group_avc_sic_result_t);
}

__spirv_AvcMceResultINTEL SPIRV_OVERLOADABLE SPIRV_BUILTIN(SubgroupAvcSicConvertToMceResultINTEL, _i64, )(
    __spirv_AvcSicResultINTEL result )
{
    uint4 handle = __builtin_IB_vme_helper_get_handle_avc_sic_result_t(result);
    return __builtin_IB_vme_helper_get_as_avc_mce_result_t(handle);
}

INLINE intel_sub_group_avc_mce_result_t OVERLOADABLE
intel_sub_group_avc_sic_convert_to_mce_result(
      intel_sub_group_avc_sic_result_t result )
{
    return __builtin_astype(
        SPIRV_BUILTIN(SubgroupAvcSicConvertToMceResultINTEL, _i64, )(
            __builtin_astype(result, __spirv_AvcSicResultINTEL)),
        intel_sub_group_avc_mce_result_t);
}

__spirv_AvcSicResultINTEL SPIRV_OVERLOADABLE SPIRV_BUILTIN(SubgroupAvcMceConvertToSicResultINTEL, _i64, )(
    __spirv_AvcMceResultINTEL result )
{
    uint4 handle = __builtin_IB_vme_helper_get_handle_avc_mce_result_t(result);
    return __builtin_IB_vme_helper_get_as_avc_sic_result_t(handle);
}

INLINE intel_sub_group_avc_sic_result_t OVERLOADABLE
intel_sub_group_avc_mce_convert_to_sic_result(
      intel_sub_group_avc_mce_result_t result )
{
    __spirv_AvcSicResultINTEL ret = SPIRV_BUILTIN(SubgroupAvcMceConvertToSicResultINTEL, _i64, )(
        __builtin_astype(result, __spirv_AvcMceResultINTEL));

    return __builtin_astype(ret, intel_sub_group_avc_sic_result_t);
}

/*****************************************************************************\

Description:
    Get the intra luma shape W0.0[5:4] from SIC result payload.

\*****************************************************************************/
uchar SPIRV_OVERLOADABLE SPIRV_BUILTIN(SubgroupAvcSicGetIpeLumaShapeINTEL, _i64, )(
    __spirv_AvcSicResultINTEL result )
{
    uint4 handle = __builtin_IB_vme_helper_get_handle_avc_sic_result_t(result);
    const uchar IntraMbMode = intel_get_message_phase_ub(handle, 0, 0);
    return (IntraMbMode >> 4) & 0x3;
}

INLINE uchar OVERLOADABLE
intel_sub_group_avc_sic_get_ipe_luma_shape(
      intel_sub_group_avc_sic_result_t result )
{
    return SPIRV_BUILTIN(SubgroupAvcSicGetIpeLumaShapeINTEL, _i64, )(__builtin_astype(result, __spirv_AvcSicResultINTEL));
}

/*****************************************************************************\

Description:
    Get the intra luma distortion W0.3[15:0] from SIC result payload.

\*****************************************************************************/
ushort SPIRV_OVERLOADABLE SPIRV_BUILTIN(SubgroupAvcSicGetBestIpeLumaDistortionINTEL, _i64, )(
    __spirv_AvcSicResultINTEL result )
{
    uint4 handle = __builtin_IB_vme_helper_get_handle_avc_sic_result_t(result);
    return intel_get_message_phase_uw(handle, 0, 3*2);
}

INLINE ushort OVERLOADABLE
intel_sub_group_avc_sic_get_best_ipe_luma_distortion(
      intel_sub_group_avc_sic_result_t result )
{
    return SPIRV_BUILTIN(SubgroupAvcSicGetBestIpeLumaDistortionINTEL, _i64, )(__builtin_astype(result, __spirv_AvcSicResultINTEL));
}

/*****************************************************************************\

Description:
    Get the intra chroma distortion (W0.3 :31:16) from SIC result payload.

\*****************************************************************************/
ushort SPIRV_OVERLOADABLE SPIRV_BUILTIN(SubgroupAvcSicGetBestIpeChromaDistortionINTEL, _i64, )(
    __spirv_AvcSicResultINTEL result )
{
    uint4 handle = __builtin_IB_vme_helper_get_handle_avc_sic_result_t(result);
    return intel_get_message_phase_uw(handle, 0, 3*2+1);
}

INLINE ushort OVERLOADABLE
intel_sub_group_avc_sic_get_best_ipe_chroma_distortion(
      intel_sub_group_avc_sic_result_t result )
{
    return SPIRV_BUILTIN(SubgroupAvcSicGetBestIpeChromaDistortionINTEL, _i64, )(__builtin_astype(result, __spirv_AvcSicResultINTEL));
}

/*****************************************************************************\

Description:
    Get the intra packed luma mode (W0.4-W0.5) from SIC result payload.

\*****************************************************************************/
ulong SPIRV_OVERLOADABLE SPIRV_BUILTIN(SubgroupAvcSicGetPackedIpeLumaModesINTEL, _i64, )(
    __spirv_AvcSicResultINTEL result )
{
    uint4 handle = __builtin_IB_vme_helper_get_handle_avc_sic_result_t(result);
    return intel_get_message_phase_uq(handle, 0, 4/2);
}

INLINE ulong OVERLOADABLE
intel_sub_group_avc_sic_get_packed_ipe_luma_modes(
      intel_sub_group_avc_sic_result_t result )
{
    return SPIRV_BUILTIN(SubgroupAvcSicGetPackedIpeLumaModesINTEL, _i64, )(__builtin_astype(result, __spirv_AvcSicResultINTEL));
}

/*****************************************************************************\

Description:
    Get the intra luma mode W0.6[1:0] from SIC result payload.

\*****************************************************************************/
uchar SPIRV_OVERLOADABLE SPIRV_BUILTIN(SubgroupAvcSicGetIpeChromaModeINTEL, _i64, )(
    __spirv_AvcSicResultINTEL result )
{
    uint4 handle = __builtin_IB_vme_helper_get_handle_avc_sic_result_t(result);
    const uchar MbIntraStruct = intel_get_message_phase_ub(handle, 0, 6*4);
    return (MbIntraStruct & 0x3);
}

INLINE uchar OVERLOADABLE
intel_sub_group_avc_sic_get_ipe_chroma_mode(
      intel_sub_group_avc_sic_result_t result )
{
    return SPIRV_BUILTIN(SubgroupAvcSicGetIpeChromaModeINTEL, _i64, )(__builtin_astype(result, __spirv_AvcSicResultINTEL));
}

/*****************************************************************************\

Description:
    Get the skc luma nzc W6.1 from SIC result payload.

\*****************************************************************************/
uint SPIRV_OVERLOADABLE SPIRV_BUILTIN(SubgroupAvcSicGetPackedSkcLumaCountThresholdINTEL, _i64, )(
    __spirv_AvcSicResultINTEL result )
{
    uint4 handle = __builtin_IB_vme_helper_get_handle_avc_sic_result_t(result);
    return intel_get_message_phase_dw(handle, 6, 1);
}

INLINE uint OVERLOADABLE
intel_sub_group_avc_sic_get_packed_skc_luma_count_threshold(
      intel_sub_group_avc_sic_result_t result )
{
    return SPIRV_BUILTIN(SubgroupAvcSicGetPackedSkcLumaCountThresholdINTEL, _i64, )(__builtin_astype(result, __spirv_AvcSicResultINTEL));
}

/*****************************************************************************\

Description:
    Get the skc luma coeff mag clip sum (W6.2-W6.3) from SIC result payload.

\*****************************************************************************/
ulong SPIRV_OVERLOADABLE SPIRV_BUILTIN(SubgroupAvcSicGetPackedSkcLumaSumThresholdINTEL, _i64, )(
    __spirv_AvcSicResultINTEL result )
{
    uint4 handle = __builtin_IB_vme_helper_get_handle_avc_sic_result_t(result);
    return intel_get_message_phase_uq(handle, 6, 2/2);
}

INLINE ulong OVERLOADABLE
intel_sub_group_avc_sic_get_packed_skc_luma_sum_threshold(
      intel_sub_group_avc_sic_result_t result )
{
    return SPIRV_BUILTIN(SubgroupAvcSicGetPackedSkcLumaSumThresholdINTEL, _i64, )(__builtin_astype(result, __spirv_AvcSicResultINTEL));
}

/*****************************************************************************\

Description:
    Get the raw skip distortion (W0.2 :31:16) from SIC result payload.

\*****************************************************************************/
ushort SPIRV_OVERLOADABLE SPIRV_BUILTIN(SubgroupAvcSicGetInterRawSadsINTEL, _i64, )(
    __spirv_AvcSicResultINTEL result )
{
    uint4 handle = __builtin_IB_vme_helper_get_handle_avc_sic_result_t(result);
    return intel_get_message_phase_uw(handle, 0, 2*2+1);
}

INLINE ushort OVERLOADABLE
intel_sub_group_avc_sic_get_inter_raw_sads(
      intel_sub_group_avc_sic_result_t result )
{
    return SPIRV_BUILTIN(SubgroupAvcSicGetInterRawSadsINTEL, _i64, )(__builtin_astype(result, __spirv_AvcSicResultINTEL));
}

// ... Common function wrappers ...

INLINE intel_sub_group_avc_ime_payload_t OVERLOADABLE
intel_sub_group_avc_ime_set_motion_vector_cost_function(
      ulong packed_cost_center_delta,
      uint2 packed_cost_table,
      uchar cost_precision,
      intel_sub_group_avc_ime_payload_t payload )
{
    intel_sub_group_avc_mce_payload_t mpayload =
        intel_sub_group_avc_ime_convert_to_mce_payload(payload);

    intel_sub_group_avc_mce_payload_t ret = intel_sub_group_avc_mce_set_motion_vector_cost_function(
        packed_cost_center_delta, packed_cost_table, cost_precision, mpayload);

    return intel_sub_group_avc_mce_convert_to_ime_payload(ret);
}

INLINE intel_sub_group_avc_ref_payload_t OVERLOADABLE
intel_sub_group_avc_ref_set_motion_vector_cost_function(
      ulong packed_cost_center_delta,
      uint2 packed_cost_table,
      uchar cost_precision,
      intel_sub_group_avc_ref_payload_t payload )
{
    intel_sub_group_avc_mce_payload_t mpayload =
        intel_sub_group_avc_ref_convert_to_mce_payload(payload);

    intel_sub_group_avc_mce_payload_t ret = intel_sub_group_avc_mce_set_motion_vector_cost_function(
        packed_cost_center_delta, packed_cost_table, cost_precision, mpayload);

    return intel_sub_group_avc_mce_convert_to_ref_payload(ret);
}

INLINE intel_sub_group_avc_sic_payload_t OVERLOADABLE
intel_sub_group_avc_sic_set_motion_vector_cost_function(
      ulong packed_cost_center_delta,
      uint2 packed_cost_table,
      uchar cost_precision,
      intel_sub_group_avc_sic_payload_t payload )
{
    intel_sub_group_avc_mce_payload_t mpayload =
        intel_sub_group_avc_sic_convert_to_mce_payload(payload);

    intel_sub_group_avc_mce_payload_t ret = intel_sub_group_avc_mce_set_motion_vector_cost_function(
        packed_cost_center_delta, packed_cost_table, cost_precision, mpayload);

    return intel_sub_group_avc_mce_convert_to_sic_payload(ret);
}

__spirv_AvcImePayloadINTEL SPIRV_OVERLOADABLE SPIRV_BUILTIN(SubgroupAvcImeSetAcOnlyHaarINTEL, _i64, )(
    __spirv_AvcImePayloadINTEL payload )
{
    __spirv_AvcMcePayloadINTEL mpayload =
        SPIRV_BUILTIN(SubgroupAvcImeConvertToMcePayloadINTEL, _i64, )( payload );
    mpayload =
        SPIRV_BUILTIN(SubgroupAvcMceSetAcOnlyHaarINTEL, _i64, )( mpayload );
    return SPIRV_BUILTIN(SubgroupAvcMceConvertToImePayloadINTEL, _i64, )( mpayload );
}

INLINE intel_sub_group_avc_ime_payload_t OVERLOADABLE
intel_sub_group_avc_ime_set_ac_only_haar(
    intel_sub_group_avc_ime_payload_t payload )
{
    return __builtin_astype(SPIRV_BUILTIN(SubgroupAvcImeSetAcOnlyHaarINTEL, _i64, )(__builtin_astype(payload, __spirv_AvcImePayloadINTEL)), intel_sub_group_avc_ime_payload_t);
}

__spirv_AvcRefPayloadINTEL SPIRV_OVERLOADABLE SPIRV_BUILTIN(SubgroupAvcRefSetAcOnlyHaarINTEL, _i64, )(
    __spirv_AvcRefPayloadINTEL payload )
{
    __spirv_AvcMcePayloadINTEL mpayload =
        SPIRV_BUILTIN(SubgroupAvcRefConvertToMcePayloadINTEL, _i64, )( payload );
    mpayload =
        SPIRV_BUILTIN(SubgroupAvcMceSetAcOnlyHaarINTEL, _i64, )( mpayload );
    return SPIRV_BUILTIN(SubgroupAvcMceConvertToRefPayloadINTEL, _i64, )( mpayload );
}

INLINE intel_sub_group_avc_ref_payload_t OVERLOADABLE
intel_sub_group_avc_ref_set_ac_only_haar(
    intel_sub_group_avc_ref_payload_t payload )
{
    __spirv_AvcRefPayloadINTEL ret = SPIRV_BUILTIN(SubgroupAvcRefSetAcOnlyHaarINTEL, _i64, )(__builtin_astype(payload, __spirv_AvcRefPayloadINTEL));
    return __builtin_astype(ret, intel_sub_group_avc_ref_payload_t);
}

__spirv_AvcSicPayloadINTEL SPIRV_OVERLOADABLE SPIRV_BUILTIN(SubgroupAvcSicSetAcOnlyHaarINTEL, _i64, )(
    __spirv_AvcSicPayloadINTEL payload )
{
    __spirv_AvcMcePayloadINTEL mpayload =
        SPIRV_BUILTIN(SubgroupAvcSicConvertToMcePayloadINTEL, _i64, )( payload );
    mpayload =
        SPIRV_BUILTIN(SubgroupAvcMceSetAcOnlyHaarINTEL, _i64, )( mpayload );
    return SPIRV_BUILTIN(SubgroupAvcMceConvertToSicPayloadINTEL, _i64, )( mpayload );
}

INLINE intel_sub_group_avc_sic_payload_t OVERLOADABLE
intel_sub_group_avc_sic_set_ac_only_haar(
    intel_sub_group_avc_sic_payload_t payload )
{
    __spirv_AvcSicPayloadINTEL ret = SPIRV_BUILTIN(SubgroupAvcSicSetAcOnlyHaarINTEL, _i64, )(__builtin_astype(payload, __spirv_AvcSicPayloadINTEL));
    return __builtin_astype(ret, intel_sub_group_avc_sic_payload_t);
}

ulong SPIRV_OVERLOADABLE SPIRV_BUILTIN(SubgroupAvcImeGetMotionVectorsINTEL, _i64, )(
    __spirv_AvcImeResultINTEL result )
{
    __spirv_AvcMceResultINTEL mresult =
      SPIRV_BUILTIN(SubgroupAvcImeConvertToMceResultINTEL, _i64, )( result );
    return SPIRV_BUILTIN(SubgroupAvcMceGetMotionVectorsINTEL, _i64, )( mresult );
}

INLINE ulong OVERLOADABLE
intel_sub_group_avc_ime_get_motion_vectors(
    intel_sub_group_avc_ime_result_t result )
{
    return SPIRV_BUILTIN(SubgroupAvcImeGetMotionVectorsINTEL, _i64, )(__builtin_astype(result, __spirv_AvcImeResultINTEL));
}

ulong SPIRV_OVERLOADABLE SPIRV_BUILTIN(SubgroupAvcRefGetMotionVectorsINTEL, _i64, )(
    __spirv_AvcRefResultINTEL result )
{
    __spirv_AvcMceResultINTEL mresult =
      SPIRV_BUILTIN(SubgroupAvcRefConvertToMceResultINTEL, _i64, )( result );
    return SPIRV_BUILTIN(SubgroupAvcMceGetMotionVectorsINTEL, _i64, )( mresult );
}

INLINE ulong OVERLOADABLE
intel_sub_group_avc_ref_get_motion_vectors(
    intel_sub_group_avc_ref_result_t result )
{
    return SPIRV_BUILTIN(SubgroupAvcRefGetMotionVectorsINTEL, _i64, )(__builtin_astype(result, __spirv_AvcRefResultINTEL));
}

ushort SPIRV_OVERLOADABLE SPIRV_BUILTIN(SubgroupAvcImeGetInterDistortionsINTEL, _i64, )(
    __spirv_AvcImeResultINTEL result )
{
    __spirv_AvcMceResultINTEL mresult =
      SPIRV_BUILTIN(SubgroupAvcImeConvertToMceResultINTEL, _i64, )( result );
    return SPIRV_BUILTIN(SubgroupAvcMceGetInterDistortionsINTEL, _i64, )( mresult );
}

INLINE ushort OVERLOADABLE
intel_sub_group_avc_ime_get_inter_distortions(
    intel_sub_group_avc_ime_result_t result )
{
    return SPIRV_BUILTIN(SubgroupAvcImeGetInterDistortionsINTEL, _i64, )(__builtin_astype(result, __spirv_AvcImeResultINTEL));
}

ushort SPIRV_OVERLOADABLE SPIRV_BUILTIN(SubgroupAvcRefGetInterDistortionsINTEL, _i64, )(
    __spirv_AvcRefResultINTEL result )
{
    __spirv_AvcMceResultINTEL mresult =
      SPIRV_BUILTIN(SubgroupAvcRefConvertToMceResultINTEL, _i64, )( result );
    return SPIRV_BUILTIN(SubgroupAvcMceGetInterDistortionsINTEL, _i64, )( mresult );
}

INLINE ushort OVERLOADABLE
intel_sub_group_avc_ref_get_inter_distortions(
    intel_sub_group_avc_ref_result_t result )
{
    return SPIRV_BUILTIN(SubgroupAvcRefGetInterDistortionsINTEL, _i64, )(__builtin_astype(result, __spirv_AvcRefResultINTEL));
}

ushort SPIRV_OVERLOADABLE SPIRV_BUILTIN(SubgroupAvcSicGetInterDistortionsINTEL, _i64, )(
    __spirv_AvcSicResultINTEL result )
{
    __spirv_AvcMceResultINTEL mresult =
        SPIRV_BUILTIN(SubgroupAvcSicConvertToMceResultINTEL, _i64, )( result );
    return SPIRV_BUILTIN(SubgroupAvcMceGetInterDistortionsINTEL, _i64, )( mresult );
}

INLINE ushort OVERLOADABLE
intel_sub_group_avc_sic_get_inter_distortions(
    intel_sub_group_avc_sic_result_t result )
{
    return SPIRV_BUILTIN(SubgroupAvcSicGetInterDistortionsINTEL, _i64, )(__builtin_astype(result, __spirv_AvcSicResultINTEL));
}

ushort SPIRV_OVERLOADABLE SPIRV_BUILTIN(SubgroupAvcImeGetBestInterDistortionINTEL, _i64, )(
    __spirv_AvcImeResultINTEL result )
{
    __spirv_AvcMceResultINTEL mresult =
      SPIRV_BUILTIN(SubgroupAvcImeConvertToMceResultINTEL, _i64, )( result );
    return SPIRV_BUILTIN(SubgroupAvcMceGetBestInterDistortionsINTEL, _i64, )( mresult );
}

INLINE ushort OVERLOADABLE
intel_sub_group_avc_ime_get_best_inter_distortion(
    intel_sub_group_avc_ime_result_t result )
{
    return SPIRV_BUILTIN(SubgroupAvcImeGetBestInterDistortionINTEL, _i64, )(__builtin_astype(result, __spirv_AvcImeResultINTEL));
}

ushort SPIRV_OVERLOADABLE SPIRV_BUILTIN(SubgroupAvcRefGetBestInterDistortionINTEL, _i64, )(
    __spirv_AvcRefResultINTEL result )
{
    __spirv_AvcMceResultINTEL mresult =
      SPIRV_BUILTIN(SubgroupAvcRefConvertToMceResultINTEL, _i64, )( result );
    return SPIRV_BUILTIN(SubgroupAvcMceGetBestInterDistortionsINTEL, _i64, )( mresult );
}

INLINE ushort OVERLOADABLE
intel_sub_group_avc_ref_get_best_inter_distortion(
    intel_sub_group_avc_ref_result_t result )
{
    return SPIRV_BUILTIN(SubgroupAvcRefGetBestInterDistortionINTEL, _i64, )(__builtin_astype(result, __spirv_AvcRefResultINTEL));
}

uchar SPIRV_OVERLOADABLE SPIRV_BUILTIN(SubgroupAvcImeGetInterMajorShapeINTEL, _i64, )(
    __spirv_AvcImeResultINTEL result )
{
    __spirv_AvcMceResultINTEL mresult =
      SPIRV_BUILTIN(SubgroupAvcImeConvertToMceResultINTEL, _i64, )( result );
    return SPIRV_BUILTIN(SubgroupAvcMceGetInterMajorShapeINTEL, _i64, )( mresult );
}

INLINE uchar OVERLOADABLE
intel_sub_group_avc_ime_get_inter_major_shape(
    intel_sub_group_avc_ime_result_t  result )
{
    return SPIRV_BUILTIN(SubgroupAvcImeGetInterMajorShapeINTEL, _i64, )(__builtin_astype(result, __spirv_AvcImeResultINTEL));
}

uchar SPIRV_OVERLOADABLE SPIRV_BUILTIN(SubgroupAvcRefGetInterMajorShapeINTEL, _i64, )(
    __spirv_AvcRefResultINTEL result )
{
    __spirv_AvcMceResultINTEL mresult =
      SPIRV_BUILTIN(SubgroupAvcRefConvertToMceResultINTEL, _i64, )( result );
    return SPIRV_BUILTIN(SubgroupAvcMceGetInterMajorShapeINTEL, _i64, )( mresult );
}

INLINE uchar OVERLOADABLE
intel_sub_group_avc_ref_get_inter_major_shape(
    intel_sub_group_avc_ref_result_t  result )
{
    return SPIRV_BUILTIN(SubgroupAvcRefGetInterMajorShapeINTEL, _i64, )(__builtin_astype(result, __spirv_AvcRefResultINTEL));
}

uchar SPIRV_OVERLOADABLE SPIRV_BUILTIN(SubgroupAvcImeGetInterMinorShapesINTEL, _i64, )(
    __spirv_AvcImeResultINTEL result )
{
    __spirv_AvcMceResultINTEL mresult =
      SPIRV_BUILTIN(SubgroupAvcImeConvertToMceResultINTEL, _i64, )( result );
    return SPIRV_BUILTIN(SubgroupAvcMceGetInterMinorShapeINTEL, _i64, )( mresult );
}

INLINE uchar OVERLOADABLE
intel_sub_group_avc_ime_get_inter_minor_shapes(
    intel_sub_group_avc_ime_result_t result )
{
    return SPIRV_BUILTIN(SubgroupAvcImeGetInterMinorShapesINTEL, _i64, )(__builtin_astype(result, __spirv_AvcImeResultINTEL));
}

uchar SPIRV_OVERLOADABLE SPIRV_BUILTIN(SubgroupAvcRefGetInterMinorShapesINTEL, _i64, )(
    __spirv_AvcRefResultINTEL result )
{
    __spirv_AvcMceResultINTEL mresult =
      SPIRV_BUILTIN(SubgroupAvcRefConvertToMceResultINTEL, _i64, )( result );
    return SPIRV_BUILTIN(SubgroupAvcMceGetInterMinorShapeINTEL, _i64, )( mresult );
}

INLINE uchar OVERLOADABLE
intel_sub_group_avc_ref_get_inter_minor_shapes(
    intel_sub_group_avc_ref_result_t result )
{
    return SPIRV_BUILTIN(SubgroupAvcRefGetInterMinorShapesINTEL, _i64, )(__builtin_astype(result, __spirv_AvcRefResultINTEL));
}

uchar SPIRV_OVERLOADABLE SPIRV_BUILTIN(SubgroupAvcImeGetInterDirectionsINTEL, _i64, )(
    __spirv_AvcImeResultINTEL result )
{
    __spirv_AvcMceResultINTEL mresult =
      SPIRV_BUILTIN(SubgroupAvcImeConvertToMceResultINTEL, _i64, )( result );
    return SPIRV_BUILTIN(SubgroupAvcMceGetInterDirectionsINTEL, _i64, )( mresult );
}

INLINE uchar OVERLOADABLE
intel_sub_group_avc_ime_get_inter_directions(
    intel_sub_group_avc_ime_result_t result )
{
    return SPIRV_BUILTIN(SubgroupAvcImeGetInterDirectionsINTEL, _i64, )(__builtin_astype(result, __spirv_AvcImeResultINTEL));
}

uchar SPIRV_OVERLOADABLE SPIRV_BUILTIN(SubgroupAvcRefGetInterDirectionsINTEL, _i64, )(
    __spirv_AvcRefResultINTEL result )
{
    __spirv_AvcMceResultINTEL mresult =
      SPIRV_BUILTIN(SubgroupAvcRefConvertToMceResultINTEL, _i64, )( result );
    return SPIRV_BUILTIN(SubgroupAvcMceGetInterDistortionsINTEL, _i64, )( mresult );
}

INLINE uchar OVERLOADABLE
intel_sub_group_avc_ref_get_inter_directions(
    intel_sub_group_avc_ref_result_t result )
{
    return SPIRV_BUILTIN(SubgroupAvcRefGetInterDirectionsINTEL, _i64, )(__builtin_astype(result, __spirv_AvcRefResultINTEL));
}

uchar SPIRV_OVERLOADABLE SPIRV_BUILTIN(SubgroupAvcImeGetInterMotionVectorCountINTEL, _i64, )(
    __spirv_AvcImeResultINTEL result )
{
    __spirv_AvcMceResultINTEL mresult =
      SPIRV_BUILTIN(SubgroupAvcImeConvertToMceResultINTEL, _i64, )( result );
    return SPIRV_BUILTIN(SubgroupAvcMceGetInterMotionVectorCountINTEL, _i64, )( mresult );
}

INLINE uchar OVERLOADABLE
intel_sub_group_avc_ime_get_inter_motion_vector_count(
    intel_sub_group_avc_ime_result_t  result )
{
    return SPIRV_BUILTIN(SubgroupAvcImeGetInterMotionVectorCountINTEL, _i64, )(__builtin_astype(result, __spirv_AvcImeResultINTEL));
}

uchar SPIRV_OVERLOADABLE SPIRV_BUILTIN(SubgroupAvcRefGetInterMotionVectorCountINTEL, _i64, )(
    __spirv_AvcRefResultINTEL result )
{
    __spirv_AvcMceResultINTEL mresult =
      SPIRV_BUILTIN(SubgroupAvcRefConvertToMceResultINTEL, _i64, )( result );
    return SPIRV_BUILTIN(SubgroupAvcMceGetInterMotionVectorCountINTEL, _i64, )( mresult );
}

INLINE uchar OVERLOADABLE
intel_sub_group_avc_ref_get_inter_motion_vector_count(
    intel_sub_group_avc_ref_result_t  result )
{
    return SPIRV_BUILTIN(SubgroupAvcRefGetInterMotionVectorCountINTEL, _i64, )(__builtin_astype(result, __spirv_AvcRefResultINTEL));
}

uchar SPIRV_OVERLOADABLE SPIRV_BUILTIN(SubgroupAvcMceGetDefaultInterBaseMultiReferencePenaltyINTEL, _i8_i8, )(
    uchar slice_type,
    uchar qp )
{
    uchar penalty = 0;
    if( slice_type ==  CLK_AVC_ME_SLICE_TYPE_PRED_INTEL ||
        slice_type ==  CLK_AVC_ME_SLICE_TYPE_BPRED_INTEL ) {
        uchar penalty_table[52] = {
            0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04,
            0x04, 0x04, 0x04, 0x08, 0x08, 0x08, 0x08, 0x0c, 0x0c, 0x0c, 0x18, 0x18, 0x18,
            0x1a, 0x1c, 0x1c, 0x1e, 0x28, 0x29, 0x2a, 0x2b, 0x2d, 0x2e, 0x38, 0x39, 0x3a,
            0x3c, 0x3d, 0x3f, 0x48, 0x49, 0x4a, 0x4b, 0x4d, 0x4e, 0x58, 0x59, 0x5a, 0x5b
        };
        penalty = penalty_table[qp];
    }
    else {
        penalty = 0;
    }

    return penalty;
}

INLINE uchar OVERLOADABLE
intel_sub_group_avc_mce_get_default_inter_base_multi_reference_penalty(
    uchar slice_type,
    uchar qp )
{
    return SPIRV_BUILTIN(SubgroupAvcMceGetDefaultInterBaseMultiReferencePenaltyINTEL, _i8_i8, )(slice_type, qp);
}

/*****************************************************************************\

Description:
    Set the multi-reference base penalty when HW assisted multi-reference
    search is performed.

    - set RefIDCost (M2.2 : 23:16)
    - set NonSkipZMVAdded (M1.7 :5)

\*****************************************************************************/

__spirv_AvcMcePayloadINTEL SPIRV_OVERLOADABLE SPIRV_BUILTIN(SubgroupAvcMceSetInterBaseMultiReferencePenaltyINTEL, _i8_i64, )(
    uchar reference_base_penalty,
    __spirv_AvcMcePayloadINTEL payload )
{
    uint4 handle = __builtin_IB_vme_helper_get_handle_avc_mce_payload_t(payload);

    // Set RefIDCost (M2.2 : 23:16)
    handle = intel_set_message_phase_ub(handle, 2, 2*4+2, reference_base_penalty);

    // Set NonSkipZMVAdded (M1.7 : 5)
    const uint NonSkipZMvAdded = intel_get_message_phase_dw(handle, 1, 7) | 0x20;
    handle = intel_set_message_phase_dw(handle, 1, 7, NonSkipZMvAdded);

    return __builtin_IB_vme_helper_get_as_avc_mce_payload_t(handle);
}

INLINE intel_sub_group_avc_mce_payload_t OVERLOADABLE
intel_sub_group_avc_mce_set_inter_base_multi_reference_penalty(
    uchar reference_base_penalty,
    intel_sub_group_avc_mce_payload_t payload )
{
    __spirv_AvcMcePayloadINTEL ret = SPIRV_BUILTIN(SubgroupAvcMceSetInterBaseMultiReferencePenaltyINTEL, _i8_i64, )(reference_base_penalty, __builtin_astype(payload, __spirv_AvcMcePayloadINTEL));
    return __builtin_astype(ret, intel_sub_group_avc_mce_payload_t);
}

ulong SPIRV_OVERLOADABLE SPIRV_BUILTIN(SubgroupAvcMceGetDefaultInterShapePenaltyINTEL, _i8_i8, )(
    uchar slice_type,
    uchar qp )
{
    ulong packed_penalty = 0;

    if( slice_type ==  CLK_AVC_ME_SLICE_TYPE_PRED_INTEL ) {
        ulong packed_penalty_table[52] = {
            0x0506040208, 0x0506040208, 0x0506040208, 0x0506040208, 0x0506040208, 0x0506040208, 0x0506040208, 0x0506040208, 0x0506040208, 0x0506040208, 0x0506040208, 0x0506040208, 0x0506040208,
            0x0506040208, 0x0506040208, 0x0506040208, 0x0b0d090519, 0x0b0d090519, 0x0b0d090519, 0x0b0d090519, 0x181a0d071d, 0x181a0d071d, 0x181a0d071d, 0x1b1d190a29, 0x1b1d190a29, 0x1b1d190a29,
            0x1e281c0d2b, 0x282a1e0f2d, 0x282a1e0f2d, 0x2a2c28192f, 0x2b2d291b39, 0x2c2f2a1c3a, 0x2e382c1d3b, 0x2f392d1f3c, 0x393b2f293e, 0x3a3c38293f, 0x3b3d392b49, 0x3c3f3a2c4a, 0x3e483c2d4b,
            0x484a3d2f4c, 0x494a3e384d, 0x4a4c483a4f, 0x4b4d493b59, 0x4c4f4a3c5a, 0x4e584c3d5b, 0x4f594d3f5c, 0x595b4f485e, 0x5a5c58495f, 0x5b5d594b69, 0x5c5f5a4c6a, 0x5e685c4d6b, 0x68695d4f6c
        };
        packed_penalty = packed_penalty_table[qp];
    }
    else if( slice_type ==  CLK_AVC_ME_SLICE_TYPE_BPRED_INTEL ) {
        ulong packed_penalty_table[52] = {
            0x060a08060c, 0x060a08060c, 0x060a08060c, 0x060a08060c, 0x060a08060c, 0x060a08060c, 0x060a08060c, 0x060a08060c, 0x060a08060c, 0x060a08060c, 0x060a08060c, 0x060a08060c, 0x060a08060c,
            0x060a08060c, 0x060a08060c, 0x060a08060c, 0x0c1b190d1c, 0x0c1b190d1c, 0x0c1b190d1c, 0x0c1b190d1c, 0x19281d1a29, 0x19281d1a29, 0x19281d1a29, 0x1c2b291d2c, 0x1c2b291d2c, 0x1c2b291d2c,
            0x1f2d2b282f, 0x29382d2a39, 0x29382d2a39, 0x2b392f2b3b, 0x2c3b392d3c, 0x2e3c3a2f3e, 0x2f3d3b383f, 0x383e3c3948, 0x3a493e3b4a, 0x3b493f3b4b, 0x3c4b493d4c, 0x3e4c4a3f4e, 0x3f4d4b484f,
            0x494f4c4959, 0x49584d4a59, 0x4b5a4f4c5b, 0x4c5b594d5c, 0x4e5c5a4f5e, 0x4f5d5b585f, 0x585f5c5968, 0x5a685e5a6a, 0x5b695f5c6b, 0x5c6b695d6c, 0x5e6c6a5f6e, 0x5f6d6b686f, 0x696f6c6979
        };
        packed_penalty = packed_penalty_table[qp];
    }
    else {
        packed_penalty = 0;
    }

    return packed_penalty;
}

INLINE ulong OVERLOADABLE
intel_sub_group_avc_mce_get_default_inter_shape_penalty(
    uchar slice_type,
    uchar qp )
{
    return SPIRV_BUILTIN(SubgroupAvcMceGetDefaultInterShapePenaltyINTEL, _i8_i8, )(slice_type, qp);
}

/*****************************************************************************\

Description:
    Set inter shape penalty.

    - set Mode Cost 4-8 (M2.1 : 31:0 & M2.2 : 7:0)
    - set NonSkipZModeAdded (M1.7 :6)

\*****************************************************************************/
__spirv_AvcMcePayloadINTEL SPIRV_OVERLOADABLE SPIRV_BUILTIN(SubgroupAvcMceSetInterShapePenaltyINTEL, _i64_i64, )(
    ulong packed_shape_penalty,
    __spirv_AvcMcePayloadINTEL payload )
{
    uint4 handle = __builtin_IB_vme_helper_get_handle_avc_mce_payload_t(payload);

    // set Mode Cost 4-8 (M2.1 : 31:0 & M2.2 : 7:0)
    handle = intel_set_message_phase_dw(handle, 2, 1, (uint)packed_shape_penalty);
    handle = intel_set_message_phase_ub(handle, 2, 2*4, (uint)(packed_shape_penalty >> 32));

    // set NonSkipZModeAdded (M1.7 :6)
    const uint NonSkipZModeAdded = intel_get_message_phase_dw(handle, 1, 7) | 0x40;
    handle = intel_set_message_phase_dw(handle, 1, 7, NonSkipZModeAdded);

    return __builtin_IB_vme_helper_get_as_avc_mce_payload_t(handle);
}

INLINE intel_sub_group_avc_mce_payload_t OVERLOADABLE
intel_sub_group_avc_mce_set_inter_shape_penalty(
     ulong packed_shape_penalty,
     intel_sub_group_avc_mce_payload_t payload )
{
    __spirv_AvcMcePayloadINTEL ret = SPIRV_BUILTIN(SubgroupAvcMceSetInterShapePenaltyINTEL, _i64_i64, )(packed_shape_penalty, __builtin_astype(payload, __spirv_AvcMcePayloadINTEL));
    return __builtin_astype(ret, intel_sub_group_avc_mce_payload_t);
}

uchar SPIRV_OVERLOADABLE SPIRV_BUILTIN(SubgroupAvcMceGetDefaultInterDirectionPenaltyINTEL, _i8_i8, )(
    uchar slice_type,
    uchar qp )
{
    uchar penalty = 0;
    if( slice_type ==  CLK_AVC_ME_SLICE_TYPE_BPRED_INTEL ) {
        uchar penalty_table[52] = {
            0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02,
            0x02, 0x02, 0x02, 0x04, 0x04, 0x04, 0x04, 0x06, 0x06, 0x06, 0x08, 0x08, 0x08,
            0x0a, 0x0c, 0x0c, 0x0e, 0x18, 0x19, 0x1a, 0x1b, 0x1d, 0x1e, 0x28, 0x29, 0x2a,
            0x2c, 0x2d, 0x2f, 0x38, 0x39, 0x3a, 0x3b, 0x3d, 0x3e, 0x48, 0x49, 0x4a, 0x4b
        };
        penalty = penalty_table[qp];
    }
    else {
        penalty = 0;
    }

    return penalty;
}

INLINE uchar OVERLOADABLE
intel_sub_group_avc_mce_get_default_inter_direction_penalty(
    uchar slice_type,
    uchar qp )
{
    return SPIRV_BUILTIN(SubgroupAvcMceGetDefaultInterDirectionPenaltyINTEL, _i8_i8, )(slice_type, qp);
}

/*****************************************************************************\

Description:
    Set multi ref penalty.

    - set Mode 9 Cost : MODE_INTER_BWD (M2.2 : 15:8)
    - set NonSkipZModeAdded (M1.7 :6)

\*****************************************************************************/

__spirv_AvcMcePayloadINTEL SPIRV_OVERLOADABLE SPIRV_BUILTIN(SubgroupAvcMceSetInterDirectionPenaltyINTEL, _i8_i64, )(
    uchar direction_cost,
    __spirv_AvcMcePayloadINTEL payload )
{
    uint4 handle = __builtin_IB_vme_helper_get_handle_avc_mce_payload_t(payload);

    // set Mode 9 Cost : MODE_INTER_BWD (M2.2 : 15:8)
    handle = intel_set_message_phase_ub(handle, 2, 2*4+1, direction_cost);

    // set NonSkipZModeAdded (M1.7 :6)
    const uint NonSkipZModeAdded = intel_get_message_phase_dw(handle, 1, 7) | 0x40;
    handle = intel_set_message_phase_dw(handle, 1, 7, NonSkipZModeAdded);

    return __builtin_IB_vme_helper_get_as_avc_mce_payload_t(handle);
}

INLINE intel_sub_group_avc_mce_payload_t OVERLOADABLE
intel_sub_group_avc_mce_set_inter_direction_penalty(
     uchar direction_cost,
     intel_sub_group_avc_mce_payload_t payload )
{
    __spirv_AvcMcePayloadINTEL ret = SPIRV_BUILTIN(SubgroupAvcMceSetInterDirectionPenaltyINTEL, _i8_i64, )(direction_cost, __builtin_astype(payload, __spirv_AvcMcePayloadINTEL));
    return __builtin_astype(ret, intel_sub_group_avc_mce_payload_t);
}

uint SPIRV_OVERLOADABLE SPIRV_BUILTIN(SubgroupAvcMceGetDefaultIntraLumaShapePenaltyINTEL, _i8_i8, )(
    uchar slice_type,
    uchar qp )
{
    uint packed_penalty = 0;

    if( slice_type ==  CLK_AVC_ME_SLICE_TYPE_PRED_INTEL ) {
        uint packed_penalty_table[52] = {
            0x391e1a00, 0x391e1a00, 0x391e1a00, 0x391e1a00, 0x391e1a00, 0x391e1a00, 0x391e1a00, 0x391e1a00, 0x391e1a00, 0x391e1a00, 0x391e1a00, 0x391e1a00, 0x391e1a00,
            0x391e1a00, 0x391e1a00, 0x391e1a00, 0x492e2a00, 0x492e2a00, 0x492e2a00, 0x492e2a00, 0x4d3b2f00, 0x4d3b2f00, 0x4d3b2f00, 0x593e3a00, 0x593e3a00, 0x593e3a00,
            0x5b493d00, 0x5d4b3f00, 0x5d4b3f00, 0x5f4c4900, 0x694e4a00, 0x6a584b00, 0x6b594d00, 0x6c5a4e00, 0x6e5b5800, 0x6f5c5900, 0x795e5a00, 0x7a685b00, 0x7b695d00,
            0x7d6a5e00, 0x7e6b6800, 0x886d6900, 0x896e6a00, 0x8a786b00, 0x8b796d00, 0x8c7a6e00, 0x8e7b7800, 0x8f7c7900, 0x8f7e7a00, 0x8f887b00, 0x8f897d00, 0x8f8a7e00
        };
        packed_penalty = packed_penalty_table[qp];
    }
    else if( slice_type ==  CLK_AVC_ME_SLICE_TYPE_BPRED_INTEL ) {
        uint packed_penalty_table[52] = {
            0x3a2a2900, 0x3a2a2900, 0x3a2a2900, 0x3a2a2900, 0x3a2a2900, 0x3a2a2900, 0x3a2a2900, 0x3a2a2900, 0x3a2a2900, 0x3a2a2900, 0x3a2a2900, 0x3a2a2900, 0x3a2a2900,
            0x3a2a2900, 0x3a2a2900, 0x3a2a2900, 0x4a3a3900, 0x4a3a3900, 0x4a3a3900, 0x4a3a3900, 0x4f3f3d00, 0x4f3f3d00, 0x4f3f3d00, 0x5a4a4900, 0x5a4a4900, 0x5a4a4900,
            0x5d4d4b00, 0x5f4f4d00, 0x5f4f4d00, 0x69594f00, 0x6a5a5900, 0x6b5b5a00, 0x6d5d5b00, 0x6e5e5c00, 0x78685e00, 0x79695f00, 0x7a6a6900, 0x7b6b6a00, 0x7d6d6b00,
            0x7e6e6c00, 0x88786d00, 0x89796f00, 0x8a7a7900, 0x8b7b7a00, 0x8d7d7b00, 0x8e7e7c00, 0x8f887e00, 0x8f897f00, 0x8f8a8900, 0x8f8b8a00, 0x8f8d8b00, 0x8f8e8c00
        };
        packed_penalty = packed_penalty_table[qp];
    }
    else {
        uint packed_penalty_table[52] = {
            0x2f000000, 0x2f000000, 0x2f000000, 0x2f000000, 0x2f000000, 0x2f000000, 0x2f000000, 0x2f000000, 0x2f000000, 0x2f000000, 0x2f000000, 0x2f000000, 0x2f000000,
            0x2f000000, 0x2f000000, 0x2f000000, 0x3f000000, 0x3f000000, 0x3f000000, 0x3f020000, 0x4b030000, 0x4b030000, 0x4b030000, 0x4f050000, 0x4f050000, 0x4f050000,
            0x59060000, 0x5b070000, 0x5b070000, 0x5d080000, 0x5f0a0000, 0x68290000, 0x692a0000, 0x6a2b0000, 0x6c2d0000, 0x6d3b0000, 0x6f3c0000, 0x783e0000, 0x793f0000,
            0x7b480000, 0x7c490000, 0x7e4a0000, 0x7f4b0000, 0x884e0000, 0x89580000, 0x8b590000, 0x8c5a0000, 0x8d5d0000, 0x8f5f0000, 0x8f680000, 0x8f690000, 0x8f6a0000
        };
       packed_penalty = packed_penalty_table[qp];
    }

    return packed_penalty;
}

INLINE uint OVERLOADABLE
intel_sub_group_avc_mce_get_default_intra_luma_shape_penalty(
    uchar slice_type,
    uchar qp )
{
    return SPIRV_BUILTIN(SubgroupAvcMceGetDefaultIntraLumaShapePenaltyINTEL, _i8_i8, )(slice_type, qp);
}

/*****************************************************************************\

Description:
    Set inter shape penalty.

    - set Mode Cost 1-3 (M2.0 : 31:8)

\*****************************************************************************/

INLINE intel_sub_group_avc_mce_payload_t OVERLOADABLE
intel_sub_group_avc_mce_set_intra_luma_shape_penalty(
     uint packed_shape_cost,
     intel_sub_group_avc_mce_payload_t payload )
{
    intel_sub_group_avc_sic_payload_t mpayload =
        intel_sub_group_avc_mce_convert_to_sic_payload(payload);

    intel_sub_group_avc_sic_payload_t ret = intel_sub_group_avc_sic_set_intra_luma_shape_penalty(
        packed_shape_cost,
        mpayload);

    return intel_sub_group_avc_sic_convert_to_mce_payload(ret);
}

uchar SPIRV_OVERLOADABLE SPIRV_BUILTIN(SubgroupAvcMceGetDefaultIntraLumaModePenaltyINTEL, _i8_i8, )(
    uchar slice_type,
    uchar qp )
{
    uchar penalty = 0;
    if( slice_type == CLK_AVC_ME_SLICE_TYPE_PRED_INTEL ||
        slice_type == CLK_AVC_ME_SLICE_TYPE_BPRED_INTEL ) {
        uchar penalty_table[52] =  {
            0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07,
            0x07, 0x07, 0x07, 0x0e, 0x0e, 0x0e, 0x0e, 0x1b, 0x1b, 0x1b, 0x1e, 0x1e, 0x1e,
            0x29, 0x2b, 0x2b, 0x2c, 0x2e, 0x38, 0x39, 0x3a, 0x3b, 0x3c, 0x3e, 0x48, 0x49,
            0x4a, 0x4b, 0x4d, 0x4e, 0x58, 0x59, 0x5a, 0x5b, 0x5c, 0x5e, 0x68, 0x69, 0x6a
        };
        penalty = penalty_table[qp];
    }
    else {
        uchar penalty_table[52] =  {
            0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f,
            0x0f, 0x0f, 0x18, 0x28, 0x28, 0x28, 0x28, 0x2c, 0x2c, 0x2c, 0x2d, 0x2d, 0x2d,
            0x38, 0x38, 0x38, 0x39, 0x3b, 0x3b, 0x3c, 0x3d, 0x3f, 0x3f, 0x49, 0x4a, 0x4b,
            0x4c, 0x4d, 0x4f, 0x59, 0x5a, 0x5b, 0x5c, 0x5e, 0x5c, 0x5d, 0x5f, 0x68, 0x69
        };
        penalty = penalty_table[qp];
    }

    return penalty;
}

INLINE uchar OVERLOADABLE
intel_sub_group_avc_mce_get_default_intra_luma_mode_penalty(
    uchar slice_type,
    uchar qp )
{
    return SPIRV_BUILTIN(SubgroupAvcMceGetDefaultIntraLumaModePenaltyINTEL, _i8_i8, )(slice_type, qp);
}

uint SPIRV_OVERLOADABLE SPIRV_BUILTIN(SubgroupAvcMceGetDefaultNonDcLumaIntraPenaltyINTEL, , )( )
{
    union {
        struct {
            uchar s0, s1, s2, s3;
        } x;
        uint y;
    } packed_penalty;

    packed_penalty.y = 0;

    packed_penalty.x.s0 = 36;
    packed_penalty.x.s1 = 12;
    packed_penalty.x.s2 = 4;

    return packed_penalty.y;
}

INLINE uint OVERLOADABLE
intel_sub_group_avc_mce_get_default_non_dc_luma_intra_penalty( void )
{
    return SPIRV_BUILTIN(SubgroupAvcMceGetDefaultNonDcLumaIntraPenaltyINTEL, , )();
}

/*****************************************************************************\

Description:
    Set multi ref penalty.

    - set Mode Cost 0 : MODE_INTRA_NONPRED (M2.0 : 7:0)
    - set IntraMxMPredMode (M6.4)
    - set NonDCPredMode (M6.7)

\*****************************************************************************/

INLINE intel_sub_group_avc_mce_payload_t OVERLOADABLE
intel_sub_group_avc_mce_set_intra_luma_mode_cost_function(
     uchar luma_mode_penalty,
     uint luma_packed_neighbor_modes,
     uint luma_packed_non_dc_penalty,
     intel_sub_group_avc_mce_payload_t payload )
{
    intel_sub_group_avc_sic_payload_t mpayload =
        intel_sub_group_avc_mce_convert_to_sic_payload(payload);

    intel_sub_group_avc_sic_payload_t ret = intel_sub_group_avc_sic_set_intra_luma_mode_cost_function(
        luma_mode_penalty,
        luma_packed_neighbor_modes,
        luma_packed_non_dc_penalty,
        mpayload);

    return intel_sub_group_avc_sic_convert_to_mce_payload(ret);
}

uchar SPIRV_OVERLOADABLE SPIRV_BUILTIN(SubgroupAvcMceGetDefaultIntraChromaModeBasePenaltyINTEL, , )( )
{
    return 0;
}

INLINE uchar OVERLOADABLE
intel_sub_group_avc_mce_get_default_intra_chroma_mode_base_penalty( void )
{
    return SPIRV_BUILTIN(SubgroupAvcMceGetDefaultIntraChromaModeBasePenaltyINTEL, , )();
}

/*****************************************************************************\

Description:
    Set inter shape penalty.

    - set Chroma Intra Mode Cost (M2.2 : 31:24)

\*****************************************************************************/

INLINE intel_sub_group_avc_mce_payload_t OVERLOADABLE
intel_sub_group_avc_mce_set_intra_chroma_mode_cost_function(
     uchar chroma_mode_base_penalty,
     intel_sub_group_avc_mce_payload_t payload )
{
    intel_sub_group_avc_sic_payload_t mpayload =
        intel_sub_group_avc_mce_convert_to_sic_payload(payload);

    intel_sub_group_avc_sic_payload_t ret = intel_sub_group_avc_sic_set_intra_chroma_mode_cost_function(
        chroma_mode_base_penalty,
        mpayload);

    return intel_sub_group_avc_sic_convert_to_mce_payload(ret);
}

/*****************************************************************************\

Description:
    Get the inter reference ids (W6.0 :31:0) from IME result payload.

\*****************************************************************************/

uint SPIRV_OVERLOADABLE SPIRV_BUILTIN(SubgroupAvcMceGetInterReferenceIdsINTEL, _i64, )(
    __spirv_AvcMceResultINTEL result )
{
    uint4 handle = __builtin_IB_vme_helper_get_handle_avc_mce_result_t(result);
    return intel_get_message_phase_dw(handle, 6, 0);
}

INLINE uint OVERLOADABLE
intel_sub_group_avc_mce_get_inter_reference_ids(
    intel_sub_group_avc_mce_result_t result)
{
    return SPIRV_BUILTIN(SubgroupAvcMceGetInterReferenceIdsINTEL, _i64, )(__builtin_astype(result, __spirv_AvcMceResultINTEL));
}

uchar SPIRV_OVERLOADABLE SPIRV_BUILTIN(SubgroupAvcMceGetInterReferenceInterlacedFieldPolaritiesINTEL, _i32_i32_i64, )(
    uint packed_reference_ids,
    uint packed_reference_parameter_field_polarities,
    __spirv_AvcMceResultINTEL result )
{
    uchar fwd_polarity = 0;

    for( uchar i = 0; i < 4; i++ ) {
        uchar blk_reference_id = ( packed_reference_ids >> ( i * 8 ) ) & 0xF;
        uchar blk_bit_offset = blk_reference_id * 2;
        uchar blk_polarity = ( packed_reference_parameter_field_polarities >> blk_bit_offset ) & 0x1;
        fwd_polarity |= ( blk_polarity << i );
    }

    uchar bwd_polarity = 0;

    packed_reference_ids = ( packed_reference_ids >> 4 );

    for( uchar i = 0; i < 4; i++ ) {
        uchar blk_reference_id = ( packed_reference_ids >> ( i * 8 ) ) & 0xF;
        uchar blk_bit_offset = blk_reference_id * 2 + 1;
        uchar blk_polarity = ( packed_reference_parameter_field_polarities >> blk_bit_offset ) & 0x1;
        bwd_polarity |= ( blk_polarity << i );
    }

    uchar field_polarities = ( fwd_polarity << 0 ) | ( bwd_polarity << 4 );

    return field_polarities;
}

INLINE uchar OVERLOADABLE
intel_sub_group_avc_mce_get_inter_reference_interlaced_field_polarities(
    uint packed_reference_ids,
    uint  packed_reference_parameter_field_polarities,
    intel_sub_group_avc_mce_result_t  result )
{
    return SPIRV_BUILTIN(SubgroupAvcMceGetInterReferenceInterlacedFieldPolaritiesINTEL, _i32_i32_i64, )(
        packed_reference_ids,
        packed_reference_parameter_field_polarities,
        __builtin_astype(result, __spirv_AvcMceResultINTEL));
}

INLINE intel_sub_group_avc_ime_payload_t OVERLOADABLE
intel_sub_group_avc_ime_set_inter_base_multi_reference_penalty(
    uchar reference_base_penalty,
    intel_sub_group_avc_ime_payload_t payload )
{
    intel_sub_group_avc_mce_payload_t mpayload =
        intel_sub_group_avc_ime_convert_to_mce_payload(payload);

    intel_sub_group_avc_mce_payload_t ret = intel_sub_group_avc_mce_set_inter_base_multi_reference_penalty(
        reference_base_penalty,
        mpayload);

    return intel_sub_group_avc_mce_convert_to_ime_payload(ret);
}

INLINE intel_sub_group_avc_ime_payload_t OVERLOADABLE
intel_sub_group_avc_ime_set_inter_shape_penalty(
     ulong packed_shape_cost,
     intel_sub_group_avc_ime_payload_t payload )
{
    intel_sub_group_avc_mce_payload_t mpayload =
        intel_sub_group_avc_ime_convert_to_mce_payload(payload);

    intel_sub_group_avc_mce_payload_t ret = intel_sub_group_avc_mce_set_inter_shape_penalty(
        packed_shape_cost,
        mpayload);

    return intel_sub_group_avc_mce_convert_to_ime_payload(ret);
}

INLINE intel_sub_group_avc_ime_payload_t OVERLOADABLE
intel_sub_group_avc_ime_set_inter_direction_penalty(
     uchar direction_cost,
     intel_sub_group_avc_ime_payload_t payload )
{
    intel_sub_group_avc_mce_payload_t mpayload =
        intel_sub_group_avc_ime_convert_to_mce_payload(payload);

    intel_sub_group_avc_mce_payload_t ret = intel_sub_group_avc_mce_set_inter_direction_penalty(
        direction_cost,
        mpayload);

    return intel_sub_group_avc_mce_convert_to_ime_payload(ret);
}

uint SPIRV_OVERLOADABLE SPIRV_BUILTIN(SubgroupAvcImeGetInterReferenceIdsINTEL, _i64, )(
    __spirv_AvcImeResultINTEL result )
{
    __spirv_AvcMceResultINTEL mresult =
      SPIRV_BUILTIN(SubgroupAvcImeConvertToMceResultINTEL, _i64, )( result );
    return SPIRV_BUILTIN(SubgroupAvcMceGetInterReferenceIdsINTEL, _i64, )( mresult );
}

INLINE uint OVERLOADABLE
intel_sub_group_avc_ime_get_inter_reference_ids(
    __spirv_AvcImeResultINTEL  result )
{
    return SPIRV_BUILTIN(SubgroupAvcImeGetInterReferenceIdsINTEL, _i64, )(result);
}

INLINE intel_sub_group_avc_ref_payload_t OVERLOADABLE
intel_sub_group_avc_ref_set_inter_base_multi_reference_penalty(
    uchar reference_base_penalty,
    intel_sub_group_avc_ref_payload_t payload )
{
    intel_sub_group_avc_mce_payload_t mpayload =
        intel_sub_group_avc_ref_convert_to_mce_payload(payload);

    intel_sub_group_avc_mce_payload_t ret = intel_sub_group_avc_mce_set_inter_base_multi_reference_penalty(
        reference_base_penalty,
        mpayload);

    return intel_sub_group_avc_mce_convert_to_ref_payload(ret);
}

INLINE intel_sub_group_avc_ref_payload_t OVERLOADABLE
intel_sub_group_avc_ref_set_inter_shape_penalty(
     ulong packed_shape_cost,
     intel_sub_group_avc_ref_payload_t payload )
{
    intel_sub_group_avc_mce_payload_t mpayload =
        intel_sub_group_avc_ref_convert_to_mce_payload(payload);

    intel_sub_group_avc_mce_payload_t ret = intel_sub_group_avc_mce_set_inter_shape_penalty(
        packed_shape_cost,
        mpayload);

    return intel_sub_group_avc_mce_convert_to_ref_payload(ret);
}

INLINE intel_sub_group_avc_ref_payload_t OVERLOADABLE
intel_sub_group_avc_ref_set_inter_direction_penalty(
     uchar direction_cost,
     intel_sub_group_avc_ref_payload_t payload )
{
    intel_sub_group_avc_mce_payload_t mpayload =
        intel_sub_group_avc_ref_convert_to_mce_payload(payload);

    intel_sub_group_avc_mce_payload_t ret = intel_sub_group_avc_mce_set_inter_direction_penalty(
        direction_cost,
        mpayload);

    return intel_sub_group_avc_mce_convert_to_ref_payload(ret);
}

__spirv_AvcRefResultINTEL SPIRV_OVERLOADABLE SPIRV_BUILTIN(SubgroupAvcRefEvaluateWithMultiReferenceINTEL, _i64_i32_i64, )(
    __spirv_VmeImageINTEL__void_1_0_0_0_0_0_0 src_image_vme,
    uint packed_reference_ids,
    __spirv_AvcRefPayloadINTEL payload )
{
    long src_image = __builtin_IB_get_image(src_image_vme);
    long vme_accelerator = __builtin_IB_get_sampler(src_image_vme);

    uint4 handle = __builtin_IB_vme_helper_get_handle_avc_ref_payload_t(payload);
    handle = intel_set_message_phase_dw(handle, 1, 6, packed_reference_ids);

    uint4 res = __builtin_IB_vme_send_fbr_new(handle, src_image, src_image, src_image, vme_accelerator);

    return __builtin_IB_vme_helper_get_as_avc_ref_result_t(res);
}

INLINE intel_sub_group_avc_ref_result_t OVERLOADABLE
intel_sub_group_avc_ref_evaluate_with_multi_reference(
      read_only image2d_t src_image,
      uint packed_reference_ids,
      sampler_t vme_accelerator,
      intel_sub_group_avc_ref_payload_t payload )
{
    __spirv_Image spv_src_image = __builtin_astype(src_image, __spirv_Image);
    __spirv_Sampler spv_vme_accelerator = __builtin_astype(vme_accelerator, __spirv_Sampler);
    __spirv_VmeImageINTEL__void_1_0_0_0_0_0_0 src_image_vme = SPIRV_BUILTIN(VmeImageINTEL, _i64_i64, )(spv_src_image, spv_vme_accelerator);

    __spirv_AvcRefResultINTEL ret = SPIRV_BUILTIN(SubgroupAvcRefEvaluateWithMultiReferenceINTEL, _i64_i32_i64, )(
        src_image_vme, packed_reference_ids, __builtin_astype(payload, __spirv_AvcRefPayloadINTEL));

    return __builtin_astype(ret, intel_sub_group_avc_ref_result_t);
}

__spirv_AvcRefResultINTEL SPIRV_OVERLOADABLE SPIRV_BUILTIN(SubgroupAvcRefEvaluateWithMultiReferenceInterlacedINTEL, _i64_i32_i8_i64, )(
    __spirv_VmeImageINTEL__void_1_0_0_0_0_0_0 src_image_vme,
    uint packed_reference_ids,
    uchar packed_reference_field_polarities,
    __spirv_AvcRefPayloadINTEL payload )
{
    long src_image = __builtin_IB_get_image(src_image_vme);
    long vme_accelerator = __builtin_IB_get_sampler(src_image_vme);

    uint4 handle = __builtin_IB_vme_helper_get_handle_avc_ref_payload_t(payload);
    handle = intel_sub_group_payload_set_ref_id_raw(
        packed_reference_ids,
        handle);
    handle = intel_sub_group_payload_set_ref_id_polarities_raw(
        packed_reference_field_polarities,
        handle);

    uint4 res = __builtin_IB_vme_send_fbr_new(handle, src_image, src_image, src_image, vme_accelerator);

    return __builtin_IB_vme_helper_get_as_avc_ref_result_t(res);
}

INLINE intel_sub_group_avc_ref_result_t OVERLOADABLE
intel_sub_group_avc_ref_evaluate_with_multi_reference(
      read_only image2d_t src_image,
      uint packed_reference_ids,
      uchar packed_reference_field_polarities,
      sampler_t vme_accelerator,
      intel_sub_group_avc_ref_payload_t payload )
{
    __spirv_Image spv_src_image = __builtin_astype(src_image, __spirv_Image);
    __spirv_Sampler spv_vme_accelerator = __builtin_astype(vme_accelerator, __spirv_Sampler);
    __spirv_VmeImageINTEL__void_1_0_0_0_0_0_0 src_image_vme = SPIRV_BUILTIN(VmeImageINTEL, _i64_i64, )(spv_src_image, spv_vme_accelerator);

    __spirv_AvcRefResultINTEL ret = SPIRV_BUILTIN(SubgroupAvcRefEvaluateWithMultiReferenceInterlacedINTEL, _i64_i32_i8_i64, )(
        src_image_vme, packed_reference_ids, packed_reference_field_polarities, __builtin_astype(payload, __spirv_AvcRefPayloadINTEL));

    return __builtin_astype(ret, intel_sub_group_avc_ref_result_t);
}

uint SPIRV_OVERLOADABLE SPIRV_BUILTIN(SubgroupAvcRefGetInterReferenceIdsINTEL, _i64, )(
    __spirv_AvcRefResultINTEL result )
{
    __spirv_AvcMceResultINTEL mresult =
      SPIRV_BUILTIN(SubgroupAvcRefConvertToMceResultINTEL, _i64, )( result );
    return SPIRV_BUILTIN(SubgroupAvcMceGetInterReferenceIdsINTEL, _i64, )( mresult );
}

INLINE uint OVERLOADABLE
intel_sub_group_avc_ref_get_inter_reference_ids(
    intel_sub_group_avc_ref_result_t  result )
{
    return SPIRV_BUILTIN(SubgroupAvcRefGetInterReferenceIdsINTEL, _i64, )(__builtin_astype(result, __spirv_AvcRefResultINTEL));
}

INLINE uchar OVERLOADABLE
intel_sub_group_avc_ime_get_inter_reference_interlaced_field_polarities(
    uint packed_reference_ids,
    uint  packed_reference_parameter_field_polarities,
    intel_sub_group_avc_ime_result_t  result )
{
    intel_sub_group_avc_mce_result_t mresult =
        intel_sub_group_avc_ime_convert_to_mce_result(result);

    return intel_sub_group_avc_mce_get_inter_reference_interlaced_field_polarities(
        packed_reference_ids, packed_reference_parameter_field_polarities, mresult);
}

INLINE uchar OVERLOADABLE
intel_sub_group_avc_ref_get_inter_reference_interlaced_field_polarities(
    uint packed_reference_ids,
    uint packed_reference_parameter_field_polarities,
    intel_sub_group_avc_ref_result_t  result )
{
    intel_sub_group_avc_mce_result_t mresult =
        intel_sub_group_avc_ref_convert_to_mce_result(result);

    return intel_sub_group_avc_mce_get_inter_reference_interlaced_field_polarities(
        packed_reference_ids,
        packed_reference_parameter_field_polarities,
        mresult);
}

INLINE intel_sub_group_avc_sic_payload_t OVERLOADABLE
intel_sub_group_avc_sic_set_inter_base_multi_reference_penalty(
    uchar reference_base_penalty,
    intel_sub_group_avc_sic_payload_t payload )
{
    intel_sub_group_avc_mce_payload_t mpayload =
        intel_sub_group_avc_sic_convert_to_mce_payload(payload);

    intel_sub_group_avc_mce_payload_t ret = intel_sub_group_avc_mce_set_inter_base_multi_reference_penalty(
        reference_base_penalty,
        mpayload);

    return intel_sub_group_avc_mce_convert_to_sic_payload(ret);
}

INLINE intel_sub_group_avc_sic_payload_t OVERLOADABLE
intel_sub_group_avc_sic_set_inter_shape_penalty(
     ulong packed_shape_cost,
     intel_sub_group_avc_sic_payload_t payload )
{
    intel_sub_group_avc_mce_payload_t mpayload =
        intel_sub_group_avc_sic_convert_to_mce_payload(payload);

    intel_sub_group_avc_mce_payload_t ret = intel_sub_group_avc_mce_set_inter_shape_penalty(
        packed_shape_cost,
        mpayload);

    return intel_sub_group_avc_mce_convert_to_sic_payload(ret);
}

INLINE intel_sub_group_avc_sic_payload_t OVERLOADABLE
intel_sub_group_avc_sic_set_inter_direction_penalty(
     uchar direction_cost,
     intel_sub_group_avc_sic_payload_t payload )
{
    intel_sub_group_avc_mce_payload_t mpayload =
        intel_sub_group_avc_sic_convert_to_mce_payload(payload);

    intel_sub_group_avc_mce_payload_t ret = intel_sub_group_avc_mce_set_inter_direction_penalty(
        direction_cost,
        mpayload);

    return intel_sub_group_avc_mce_convert_to_sic_payload(ret);
}

__spirv_AvcSicPayloadINTEL SPIRV_OVERLOADABLE SPIRV_BUILTIN(SubgroupAvcSicSetIntraLumaShapePenaltyINTEL, _i32_i64, )(
    uint packed_shape_cost,
    __spirv_AvcSicPayloadINTEL payload )
{
    uint4 handle = __builtin_IB_vme_helper_get_handle_avc_sic_payload_t(payload);

    // set Mode Cost 1-3 (M2.0 : 31:8)
    uchar b1 = (uchar)(packed_shape_cost >> 8);
    uchar b2 = (uchar)(packed_shape_cost >> 16);
    uchar b3 = (uchar)(packed_shape_cost >> 24);
    handle = intel_set_message_phase_ub(handle, 2, 1, b1);
    handle = intel_set_message_phase_ub(handle, 2, 2, b2);
    handle = intel_set_message_phase_ub(handle, 2, 3, b3);

    return __builtin_IB_vme_helper_get_as_avc_sic_payload_t(handle);
}

INLINE intel_sub_group_avc_sic_payload_t OVERLOADABLE
intel_sub_group_avc_sic_set_intra_luma_shape_penalty(
     uint packed_shape_cost,
     intel_sub_group_avc_sic_payload_t payload )
{
    __spirv_AvcSicPayloadINTEL ret = SPIRV_BUILTIN(SubgroupAvcSicSetIntraLumaShapePenaltyINTEL, _i32_i64, )(
        packed_shape_cost,
        __builtin_astype(payload, __spirv_AvcSicPayloadINTEL));

    return __builtin_astype(ret, intel_sub_group_avc_sic_payload_t);
}

/*****************************************************************************\

Description:
    Set inter shape penalty.

    - set Chroma Intra Mode Cost (M2.2 : 31:24)

\*****************************************************************************/

__spirv_AvcSicPayloadINTEL SPIRV_OVERLOADABLE SPIRV_BUILTIN(SubgroupAvcSicSetIntraLumaModeCostFunctionINTEL, _i8_i32_i32_i64, )(
    uchar luma_mode_penalty,
    uint luma_packed_neighbor_modes,
    uint luma_packed_non_dc_penalty,
    __spirv_AvcSicPayloadINTEL payload )
{
    uint4 handle = __builtin_IB_vme_helper_get_handle_avc_sic_payload_t(payload);

    // set Mode Cost 0 : MODE_INTRA_NONPRED (M2.0 : 7:0)
    handle = intel_set_message_phase_ub(handle, 2, 0, luma_mode_penalty);

    // set IntraMxMPredMode (M6.4)
    handle = intel_set_message_phase_dw(handle, 6, 4, luma_packed_neighbor_modes);

    // set NonDCPredMode (M6.7)
    handle = intel_set_message_phase_dw(handle, 6, 7, luma_packed_non_dc_penalty);

    return __builtin_IB_vme_helper_get_as_avc_sic_payload_t(handle);
}

INLINE intel_sub_group_avc_sic_payload_t OVERLOADABLE
intel_sub_group_avc_sic_set_intra_luma_mode_cost_function(
     uchar luma_mode_penalty,
     uint luma_packed_neighbor_modes,
     uint luma_packed_non_dc_penalty,
     intel_sub_group_avc_sic_payload_t payload )
{
    __spirv_AvcSicPayloadINTEL ret = SPIRV_BUILTIN(SubgroupAvcSicSetIntraLumaModeCostFunctionINTEL, _i8_i32_i32_i64, )(
        luma_mode_penalty,
        luma_packed_neighbor_modes,
        luma_packed_non_dc_penalty,
        __builtin_astype(payload, __spirv_AvcSicPayloadINTEL));

    return __builtin_astype(ret, intel_sub_group_avc_sic_payload_t);
}

__spirv_AvcSicPayloadINTEL SPIRV_OVERLOADABLE SPIRV_BUILTIN(SubgroupAvcSicSetIntraChromaModeCostFunctionINTEL, _i8_i64, )(
    uchar chroma_mode_penalty,
    __spirv_AvcSicPayloadINTEL payload )
{
    uint4 handle = __builtin_IB_vme_helper_get_handle_avc_sic_payload_t(payload);

    // set Chroma Intra Mode Cost (M2.2 : 31:24)
    handle = intel_set_message_phase_ub(handle, 2, 2 * 4 + 3, chroma_mode_penalty);

    return __builtin_IB_vme_helper_get_as_avc_sic_payload_t(handle);
}

INLINE intel_sub_group_avc_sic_payload_t OVERLOADABLE
intel_sub_group_avc_sic_set_intra_chroma_mode_cost_function(
     uchar chroma_mode_penalty,
    intel_sub_group_avc_sic_payload_t payload )
{
    __spirv_AvcSicPayloadINTEL ret = SPIRV_BUILTIN(SubgroupAvcSicSetIntraChromaModeCostFunctionINTEL, _i8_i64, )(
        chroma_mode_penalty,
        __builtin_astype(payload, __spirv_AvcSicPayloadINTEL));

    return __builtin_astype(ret, intel_sub_group_avc_sic_payload_t);
}

////

INLINE intel_sub_group_avc_ime_payload_t OVERLOADABLE
intel_sub_group_avc_ime_set_source_interlaced_field_polarity(
     uchar  src_field_polarity,
     intel_sub_group_avc_ime_payload_t payload )
{
    intel_sub_group_avc_mce_payload_t mpayload =
        intel_sub_group_avc_ime_convert_to_mce_payload(payload);

    intel_sub_group_avc_mce_payload_t ret = intel_sub_group_avc_mce_set_source_interlaced_field_polarity(
        src_field_polarity,
        mpayload);

    return intel_sub_group_avc_mce_convert_to_ime_payload(ret);
}

INLINE intel_sub_group_avc_ref_payload_t OVERLOADABLE
intel_sub_group_avc_ref_set_source_interlaced_field_polarity(
     uchar  src_field_polarity,
     intel_sub_group_avc_ref_payload_t payload )
{
    intel_sub_group_avc_mce_payload_t mpayload =
        intel_sub_group_avc_ref_convert_to_mce_payload(payload);

    intel_sub_group_avc_mce_payload_t ret = intel_sub_group_avc_mce_set_source_interlaced_field_polarity(
        src_field_polarity,
        mpayload);

    return intel_sub_group_avc_mce_convert_to_ref_payload(ret);
}

INLINE intel_sub_group_avc_sic_payload_t OVERLOADABLE
intel_sub_group_avc_sic_set_source_interlaced_field_polarity(
     uchar  src_field_polarity,
     intel_sub_group_avc_sic_payload_t payload )
{
    intel_sub_group_avc_mce_payload_t mpayload =
        intel_sub_group_avc_sic_convert_to_mce_payload(payload);

    intel_sub_group_avc_mce_payload_t ret = intel_sub_group_avc_mce_set_source_interlaced_field_polarity(
        src_field_polarity,
        mpayload);

    return intel_sub_group_avc_mce_convert_to_sic_payload(ret);
}

INLINE intel_sub_group_avc_ime_payload_t OVERLOADABLE
intel_sub_group_avc_ime_set_single_reference_interlaced_field_polarity(
     uchar  ref_field_polarity,
     intel_sub_group_avc_ime_payload_t payload )
{
    intel_sub_group_avc_mce_payload_t mpayload =
        intel_sub_group_avc_ime_convert_to_mce_payload(payload);

    intel_sub_group_avc_mce_payload_t ret = intel_sub_group_avc_mce_set_single_reference_interlaced_field_polarity(
        ref_field_polarity,
        mpayload);

    return intel_sub_group_avc_mce_convert_to_ime_payload(ret);
}

INLINE intel_sub_group_avc_ref_payload_t OVERLOADABLE
intel_sub_group_avc_ref_set_single_reference_interlaced_field_polarity(
     uchar  ref_field_polarity,
     intel_sub_group_avc_ref_payload_t payload )
{
    intel_sub_group_avc_mce_payload_t mpayload =
        intel_sub_group_avc_ref_convert_to_mce_payload(payload);

    intel_sub_group_avc_mce_payload_t ret = intel_sub_group_avc_mce_set_single_reference_interlaced_field_polarity(
        ref_field_polarity,
        mpayload);

    return intel_sub_group_avc_mce_convert_to_ref_payload(ret);
}

INLINE intel_sub_group_avc_sic_payload_t OVERLOADABLE
intel_sub_group_avc_sic_set_single_reference_interlaced_field_polarity(
     uchar  ref_field_polarity,
     intel_sub_group_avc_sic_payload_t payload )
{
    intel_sub_group_avc_mce_payload_t mpayload =
        intel_sub_group_avc_sic_convert_to_mce_payload(payload);

    intel_sub_group_avc_mce_payload_t ret = intel_sub_group_avc_mce_set_single_reference_interlaced_field_polarity(
        ref_field_polarity,
        mpayload);

    return intel_sub_group_avc_mce_convert_to_sic_payload(ret);
}

INLINE intel_sub_group_avc_ime_payload_t OVERLOADABLE
intel_sub_group_avc_ime_set_dual_reference_interlaced_field_polarities(
     uchar  fwd_ref_field_polarity,
     uchar  bwd_ref_field_polarity,
     intel_sub_group_avc_ime_payload_t payload )
{
    intel_sub_group_avc_mce_payload_t mpayload =
        intel_sub_group_avc_ime_convert_to_mce_payload(payload);

    intel_sub_group_avc_mce_payload_t ret = intel_sub_group_avc_mce_set_dual_reference_interlaced_field_polarities(
        fwd_ref_field_polarity,
        bwd_ref_field_polarity,
        mpayload);

    return intel_sub_group_avc_mce_convert_to_ime_payload(ret);
}

INLINE intel_sub_group_avc_ref_payload_t OVERLOADABLE
intel_sub_group_avc_ref_set_dual_reference_interlaced_field_polarities(
     uchar  fwd_ref_field_polarity,
     uchar  bwd_ref_field_polarity,
     intel_sub_group_avc_ref_payload_t payload )
{
    intel_sub_group_avc_mce_payload_t mpayload =
        intel_sub_group_avc_ref_convert_to_mce_payload(payload);

    intel_sub_group_avc_mce_payload_t ret = intel_sub_group_avc_mce_set_dual_reference_interlaced_field_polarities(
        fwd_ref_field_polarity,
        bwd_ref_field_polarity,
        mpayload);

    return intel_sub_group_avc_mce_convert_to_ref_payload(ret);
}

INLINE intel_sub_group_avc_sic_payload_t OVERLOADABLE
intel_sub_group_avc_sic_set_dual_reference_interlaced_field_polarities(
     uchar  fwd_ref_field_polarity,
     uchar  bwd_ref_field_polarity,
     intel_sub_group_avc_sic_payload_t payload )
{
    intel_sub_group_avc_mce_payload_t mpayload =
        intel_sub_group_avc_sic_convert_to_mce_payload(payload);

    intel_sub_group_avc_mce_payload_t ret = intel_sub_group_avc_mce_set_dual_reference_interlaced_field_polarities(
        fwd_ref_field_polarity,
        bwd_ref_field_polarity,
        mpayload);

    return intel_sub_group_avc_mce_convert_to_sic_payload(ret);
}

////

__spirv_AvcSicResultINTEL SPIRV_OVERLOADABLE SPIRV_BUILTIN(SubgroupAvcSicEvaluateWithMultiReferenceINTEL, _i64_i32_i64, )(
    __spirv_VmeImageINTEL__void_1_0_0_0_0_0_0 src_image_vme,
    uint packed_reference_ids,
    __spirv_AvcSicPayloadINTEL payload )
{
    long src_image = __builtin_IB_get_image(src_image_vme);
    long vme_accelerator = __builtin_IB_get_sampler(src_image_vme);

    uint4 handle = __builtin_IB_vme_helper_get_handle_avc_sic_payload_t(payload);
    handle = intel_set_message_phase_dw(handle, 1, 6, packed_reference_ids);

    uint4 res = __builtin_IB_vme_send_sic_new(handle, src_image, src_image, src_image, vme_accelerator);

    return __builtin_IB_vme_helper_get_as_avc_sic_result_t(res);
}

INLINE intel_sub_group_avc_sic_result_t OVERLOADABLE
intel_sub_group_avc_sic_evaluate_with_multi_reference(
      read_only image2d_t src_image,
      uint packed_reference_ids,
      sampler_t vme_accelerator,
      intel_sub_group_avc_sic_payload_t payload )
{
    __spirv_Image spv_src_image = __builtin_astype(src_image, __spirv_Image);
    __spirv_Sampler spv_vme_accelerator = __builtin_astype(vme_accelerator, __spirv_Sampler);
    __spirv_VmeImageINTEL__void_1_0_0_0_0_0_0 src_image_vme = SPIRV_BUILTIN(VmeImageINTEL, _i64_i64, )(spv_src_image, spv_vme_accelerator);

    __spirv_AvcSicResultINTEL ret = SPIRV_BUILTIN(SubgroupAvcSicEvaluateWithMultiReferenceINTEL, _i64_i32_i64, )(
        src_image_vme,
        packed_reference_ids,
        __builtin_astype(payload, __spirv_AvcSicPayloadINTEL));

    return __builtin_astype(ret, intel_sub_group_avc_sic_result_t);
}

__spirv_AvcSicResultINTEL SPIRV_OVERLOADABLE SPIRV_BUILTIN(SubgroupAvcSicEvaluateWithMultiReferenceInterlacedINTEL, _i64_i32_i8_i64, )(
    __spirv_VmeImageINTEL__void_1_0_0_0_0_0_0 src_image_vme,
    uint packed_reference_ids,
    uchar packed_reference_field_polarities,
    __spirv_AvcSicPayloadINTEL payload )
{
    long src_image = __builtin_IB_get_image(src_image_vme);
    long vme_accelerator = __builtin_IB_get_sampler(src_image_vme);

    uint4 handle = __builtin_IB_vme_helper_get_handle_avc_sic_payload_t(payload);

    handle = intel_sub_group_payload_set_ref_id_raw(
        packed_reference_ids,
        handle);
    handle = intel_sub_group_payload_set_ref_id_polarities_raw(
        packed_reference_field_polarities,
        handle);

    uint4 res = __builtin_IB_vme_send_sic_new(handle, src_image, src_image, src_image, vme_accelerator);

    return __builtin_IB_vme_helper_get_as_avc_sic_result_t(res);
}

INLINE intel_sub_group_avc_sic_result_t OVERLOADABLE
intel_sub_group_avc_sic_evaluate_with_multi_reference(
      read_only image2d_t src_image,
      uint packed_reference_ids,
      uchar packed_reference_field_polarities,
      sampler_t vme_accelerator,
      intel_sub_group_avc_sic_payload_t payload )
{
    __spirv_Image spv_src_image = __builtin_astype(src_image, __spirv_Image);
    __spirv_Sampler spv_vme_accelerator = __builtin_astype(vme_accelerator, __spirv_Sampler);
    __spirv_VmeImageINTEL__void_1_0_0_0_0_0_0 src_image_vme = SPIRV_BUILTIN(VmeImageINTEL, _i64_i64, )(spv_src_image, spv_vme_accelerator);

    __spirv_AvcSicResultINTEL ret = SPIRV_BUILTIN(SubgroupAvcSicEvaluateWithMultiReferenceInterlacedINTEL, _i64_i32_i8_i64, )(
        src_image_vme,
        packed_reference_ids,
        packed_reference_field_polarities,
        __builtin_astype(payload, __spirv_AvcSicPayloadINTEL));

    return __builtin_astype(ret, intel_sub_group_avc_sic_result_t);
}

uchar SPIRV_OVERLOADABLE SPIRV_BUILTIN(SubgroupAvcImeGetStreamoutDualReferenceMajorShapeReferenceIdsINTEL, _i64_i8_i8, )(
    __spirv_AvcImeResultDualReferenceStreamoutINTEL result,
    uchar major_shape,
    uchar direction )
{
    uchar retValue = 0;
    // IME Streamout follows the same format as the IME Streamin message phases (IME2-IME5).
    const uint reg = (direction == CLK_AVC_ME_MAJOR_FORWARD_INTEL) ?
        /*fwd*/ RETURN_MESSAGE_NUM_GRFS :
        /*bwd*/(RETURN_MESSAGE_NUM_GRFS+2);

    uint8 handle = __builtin_IB_vme_helper_get_handle_avc_ime_result_dual_reference_streamout_t(result);
    if(major_shape == VME_MAJOR_16x16)
    {
        // WX+2.4 [19:16] - Rec0 Shape 16x16 RefID
        // WX+4.4 [19:16] - Rec1 Shape 16x16 RefID
        retValue = intel_get_message_phase_ub(handle, reg, 4*4+2) & 0xF;
    }
    else if(major_shape == VME_MAJOR_16x8)
    {
        // WX+2.6 [3:0] - Rec0 Shape 16x8_0 RefID
        // WX+2.6 [7:4] - Rec0 Shape 16x8_1 RefID
        // WX+4.6 [3:0] - Rec1 Shape 16x8_0 RefID
        // WX+4.6 [7:4] - Rec1 Shape 16x8_1 RefID
        uchar val = intel_get_message_phase_ub(handle, reg, 6*4);
        val >>= get_sub_group_local_id() * 4;
        retValue = val & 0xF;
    }
    else if(major_shape == VME_MAJOR_8x16)
    {
        // WX+2.6 [11:8]  - Rec0 Shape 8x16_0 RefID
        // WX+2.6 [15:12] - Rec0 Shape 8x16_1 RefID
        // WX+4.6 [11:8]  - Rec1 Shape 8x16_0 RefID
        // WX+4.6 [15:12] - Rec1 Shape 8x16_1 RefID
        uchar val = intel_get_message_phase_ub(handle, reg, 6*4+1);
        val >>= get_sub_group_local_id() * 4;
        retValue = val & 0xF;
    }
    else // 8x8
    {
        // WX+2.6 [19:16] - Rec0 Shape 8x8_0 RefID
        // WX+2.6 [23:20] - Rec0 Shape 8x8_1 RefID
        // WX+2.6 [27:24] - Rec0 Shape 8x8_2 RefID
        // WX+2.6 [31:28] - Rec0 Shape 8x8_3 RefID
        // WX+4.6 [19:16] - Rec1 Shape 8x8_0 RefID
        // WX+4.6 [23:20] - Rec1 Shape 8x8_1 RefID
        // WX+4.6 [27:24] - Rec1 Shape 8x8_2 RefID
        // WX+4.6 [31:28] - Rec1 Shape 8x8_3 RefID
        ushort val = intel_get_message_phase_uw(handle, reg, 6*2+1);
        val >>= get_sub_group_local_id() * 4;
        retValue = (uchar)(val & 0xF);
    }

    return retValue;
}

INLINE uchar OVERLOADABLE
intel_sub_group_avc_ime_get_streamout_major_shape_reference_ids(
    intel_sub_group_avc_ime_result_dual_reference_streamout_t result,
    uchar major_shape,
    uchar direction)
{
    return SPIRV_BUILTIN(SubgroupAvcImeGetStreamoutDualReferenceMajorShapeReferenceIdsINTEL, _i64_i8_i8, )(
        __builtin_astype(result, __spirv_AvcImeResultDualReferenceStreamoutINTEL),
        major_shape,
        direction);
}

uchar SPIRV_OVERLOADABLE SPIRV_BUILTIN(SubgroupAvcImeGetStreamoutSingleReferenceMajorShapeReferenceIdsINTEL, _i64_i8, )(
    __spirv_AvcImeResultSingleReferenceStreamoutINTEL result,
    uchar major_shape )
{
    uint8 handle = __builtin_IB_vme_helper_get_handle_avc_ime_result_single_reference_streamout_t(result);
    __spirv_AvcImeResultDualReferenceStreamoutINTEL nresult = __builtin_IB_vme_helper_get_as_avc_ime_result_dual_reference_streamout_t(handle);

    return SPIRV_BUILTIN(SubgroupAvcImeGetStreamoutDualReferenceMajorShapeReferenceIdsINTEL, _i64_i8_i8, )(
        nresult,
        major_shape,
        CLK_AVC_ME_MAJOR_FORWARD_INTEL);
}

INLINE uchar OVERLOADABLE
intel_sub_group_avc_ime_get_streamout_major_shape_reference_ids(
    intel_sub_group_avc_ime_result_single_reference_streamout_t result,
    uchar major_shape )
{
    return SPIRV_BUILTIN(SubgroupAvcImeGetStreamoutSingleReferenceMajorShapeReferenceIdsINTEL, _i64_i8, )(
        __builtin_astype(result, __spirv_AvcImeResultSingleReferenceStreamoutINTEL),
        major_shape);
}


/*****************************************************************************/
/*                       VA (Video Analytics)                                */
/*****************************************************************************/

INLINE void OVERLOADABLE intel_work_group_va_boolcentroid(
    __local void* dst,
    int2 i_coord,
    int2 size,
    image2d_t image,
    sampler_t a )
{
    // placeholder for future implementation
}

INLINE void OVERLOADABLE intel_work_group_va_boolsum(
    __local void* dst,
    int2 i_coord,
    int2 size,
    image2d_t image,
    sampler_t a )
{
    // placeholder for future implementation
}

INLINE void OVERLOADABLE intel_work_group_va_centroid(
    __local void* dst,
    int2 i_coord,
    int size,
    image2d_t image,
    sampler_t a )
{
    // placeholder for future implementation
}

INLINE void OVERLOADABLE intel_work_group_va_convolve_16x4(
    __local void* dst,
    int2 i_coord,
    image2d_t image,
    sampler_t a )
{
    // placeholder for future implementation
}

INLINE void OVERLOADABLE intel_work_group_va_dilate_64x4(
    __local void* dst,
    int2 i_coord,
    image2d_t image,
    sampler_t a )
{
    // placeholder for future implementation
}

INLINE void OVERLOADABLE intel_work_group_va_erode_64x4(
    __local void* dst,
    int2 i_coord,
    image2d_t image,
    sampler_t a )
{
    // placeholder for future implementation
}

INLINE void OVERLOADABLE intel_work_group_va_minmax(
    __local void* dst,
    int2 i_coord,
    image2d_t image,
    sampler_t a )
{
    // placeholder for future implementation
}

INLINE void OVERLOADABLE intel_work_group_va_minmaxfilter_16x4(
    __local void* dst,
    int2 i_coord,
    image2d_t image,
    sampler_t a )
{
    // placeholder for future implementation
}

INLINE void OVERLOADABLE intel_work_group_va_boolcentroid(
    __local void* registers,
    float2 coordsNorm,
    int2 size,
    image2d_t srcImg,
    sampler_t accelerator )
{
    long i_image = (long)__builtin_astype( srcImg, __global void* );
    __builtin_IB_va_boolcentroid(
        registers,
        coordsNorm,
        size,
        i_image,
        __builtin_IB_convert_sampler_to_int(accelerator) );
}

INLINE void OVERLOADABLE intel_work_group_va_boolsum(
    __local void* registers,
    float2 coordsNorm,
    int2 size,
    image2d_t srcImg,
    sampler_t accelerator )
{
    long i_image = (long)__builtin_astype( srcImg, __global void* );
    __builtin_IB_va_boolsum(
        registers,
        coordsNorm,
        size,
        i_image,
        __builtin_IB_convert_sampler_to_int(accelerator) );
}

INLINE void OVERLOADABLE intel_work_group_va_centroid(
    __local void* registers,
    float2 coordsNorm,
    int size,
    image2d_t srcImg,
    sampler_t accelerator )
{
    long i_image = (long)__builtin_astype( srcImg, __global void* );
    __builtin_IB_va_centroid(
        registers,
        coordsNorm,
        size,
        i_image,
        __builtin_IB_convert_sampler_to_int(accelerator) );
}

INLINE void OVERLOADABLE intel_work_group_va_convolve_16x4(
    __local void* registers,
    float2 coordsNorm,
    image2d_t srcImg,
    sampler_t accelerator )
{
    __builtin_IB_va_convolve_16x4_SLM(
        registers,
        coordsNorm,
        (long)__builtin_astype( srcImg, __global void* ),
        __builtin_IB_convert_sampler_to_int(accelerator) );
}

INLINE void OVERLOADABLE intel_work_group_va_dilate_64x4(
    __local void* registers,
    float2 coordsNorm,
    image2d_t srcImg,
    sampler_t accelerator )
{
    long i_image = (long)__builtin_astype( srcImg, __global void* );
    __builtin_IB_va_dilate_64x4(
        registers,
        coordsNorm,
        i_image,
        __builtin_IB_convert_sampler_to_int(accelerator) );
}

INLINE void OVERLOADABLE intel_work_group_va_erode_64x4(
    __local void* registers,
    float2 coordsNorm,
    image2d_t srcImg,
    sampler_t accelerator )
{
    long i_image = (long)__builtin_astype( srcImg, __global void* );
    __builtin_IB_va_erode_64x4(
        registers,
        coordsNorm,
        i_image,
        __builtin_IB_convert_sampler_to_int(accelerator) );
}

INLINE void OVERLOADABLE intel_work_group_va_minmax(
    __local void* registers,
    float2 coordsNorm,
    image2d_t srcImg,
    sampler_t accelerator )
{
    long i_image = (long)__builtin_astype( srcImg, __global void* );
    __builtin_IB_va_minmax(
        registers,
        coordsNorm,
        i_image,
        __builtin_IB_convert_sampler_to_int(accelerator) );
}

INLINE void OVERLOADABLE intel_work_group_va_minmaxfilter_16x4(
    __local void* registers,
    float2 coordsNorm,
    image2d_t srcImg,
    sampler_t accelerator )
{
    long i_image = (long)__builtin_astype( srcImg, __global void* );
    __builtin_IB_va_minmaxfilter_16x4_SLM(
        registers,
        coordsNorm,
        i_image,
        __builtin_IB_convert_sampler_to_int(accelerator) );
}

INLINE short OVERLOADABLE intel_work_group_va_convolve_16x1(
    float2 coordsNorm,
    image2d_t srcImg,
    sampler_t accelerator )
{
    // placeholder for future implementation
}

INLINE short4 OVERLOADABLE intel_work_group_va_convolve_16x4(
    float2 coordsNorm,
    image2d_t srcImg,
    sampler_t accelerator )
{
    // placeholder for future implementation
}

INLINE uchar OVERLOADABLE intel_work_group_va_minfilter_16x1(
    float2 coordsNorm,
    image2d_t srcImg,
    sampler_t accelerator )
{
    // placeholder for future implementation
}

INLINE uchar4 OVERLOADABLE intel_work_group_va_minfilter_16x4(
    float2 coordsNorm,
    image2d_t srcImg,
    sampler_t accelerator )
{
    // placeholder for future implementation
}

INLINE uchar OVERLOADABLE intel_work_group_va_maxfilter_16x1(
    float2 coordsNorm,
    image2d_t srcImg,
    sampler_t accelerator )
{
    // placeholder for future implementation
}

INLINE uchar4 OVERLOADABLE intel_work_group_va_maxfilter_16x4(
    float2 coordsNorm,
    image2d_t srcImg,
    sampler_t accelerator )
{
    // placeholder for future implementation
}


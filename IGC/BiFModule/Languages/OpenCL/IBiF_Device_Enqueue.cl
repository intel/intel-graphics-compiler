/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

//===-  IGILBiF_Device_Enqueue.cl - IGIL device enqueue functions   -===//
//
// This file defines IGIL builtin versions of OpenCL device enqueue.
//
//===----------------------------------------------------------------===//

#ifndef __BIF_DEVICE_ENQUEUE_CL__
#define __BIF_DEVICE_ENQUEUE_CL__

#define __EXECUTION_MODEL_DEBUG
#include "DeviceEnqueueHelpers.h"

#define to_spirv_event(e)                                                                                              \
  (__builtin_IB_convert_object_type_to_spirv_deviceevent(__builtin_IB_cast_object_to_generic_ptr(e)))
#define to_ocl_event(e) (__builtin_IB_convert_object_type_to_ocl_clk_event(e))

INLINE void OVERLOADABLE retain_event(clk_event_t event) { __spirv_RetainEvent(to_spirv_event(event)); }

INLINE void OVERLOADABLE release_event(clk_event_t event) { __spirv_ReleaseEvent(to_spirv_event(event)); }

INLINE clk_event_t OVERLOADABLE create_user_event() { return to_ocl_event(__spirv_CreateUserEvent()); }

INLINE void OVERLOADABLE set_user_event_status(clk_event_t e, int state) {
  __spirv_SetUserEventStatus(to_spirv_event(e), state);
}

INLINE void OVERLOADABLE capture_event_profiling_info(clk_event_t e, clk_profiling_info name, __global void *value) {
  __spirv_CaptureEventProfilingInfo(to_spirv_event(e), name, value);
}

INLINE bool OVERLOADABLE is_valid_event(clk_event_t event) { return __spirv_IsValidEvent(to_spirv_event(event)); }

INLINE OVERLOADABLE queue_t get_default_queue() {
  return __builtin_IB_convert_object_type_to_ocl_queue(__spirv_GetDefaultQueue());
}

#endif // __BIF_DEVICE_ENQUEUE_CL__

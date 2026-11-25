/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

int __intel_enqueue_marker_impl(queue_t q, uint numEventsInWaitList, const __generic clk_event_t *waitList,
                                __generic clk_event_t *returnEvent);

uint __attribute__((overloadable)) __spirv_EnqueueMarker(__spirv_Queue Queue, uint NumEvents,
                                                         __spirv_DeviceEvent private *WaitEvents,
                                                         __spirv_DeviceEvent private *RetEvent) {
  return __intel_enqueue_marker_impl(__builtin_IB_convert_object_type_to_ocl_queue(Queue), NumEvents,
                                     __builtin_IB_convert_object_type_to_ocl_clk_event_ptr(WaitEvents),
                                     __builtin_IB_convert_object_type_to_ocl_clk_event_ptr(RetEvent));
}

uint __attribute__((overloadable)) __spirv_EnqueueMarker(__spirv_Queue Queue, uint NumEvents,
                                                         __spirv_DeviceEvent local *WaitEvents,
                                                         __spirv_DeviceEvent local *RetEvent) {
  return __intel_enqueue_marker_impl(__builtin_IB_convert_object_type_to_ocl_queue(Queue), NumEvents,
                                     __builtin_IB_convert_object_type_to_ocl_clk_event_ptr(WaitEvents),
                                     __builtin_IB_convert_object_type_to_ocl_clk_event_ptr(RetEvent));
}

uint __attribute__((overloadable)) __spirv_EnqueueMarker(__spirv_Queue Queue, uint NumEvents,
                                                         __spirv_DeviceEvent generic *WaitEvents,
                                                         __spirv_DeviceEvent generic *RetEvent) {
  return __intel_enqueue_marker_impl(__builtin_IB_convert_object_type_to_ocl_queue(Queue), NumEvents,
                                     __builtin_IB_convert_object_type_to_ocl_clk_event_ptr(WaitEvents),
                                     __builtin_IB_convert_object_type_to_ocl_clk_event_ptr(RetEvent));
}

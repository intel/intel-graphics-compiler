/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

// Device-Side Enqueue Instructions

// Implementation/ doesn't include cth.  Forward declare this so below users compile.
// FIXME: this should really be defined in Implementation/ and used in Languages/.
int __intel_enqueue_marker_impl(queue_t q, uint numEventsInWaitList, const __generic clk_event_t* waitList, __generic clk_event_t* returnEvent );


uint SPIRV_OVERLOADABLE SPIRV_BUILTIN(EnqueueMarker, _i64_i32_p0i64_p0i64, )(__spirv_Queue Queue, uint NumEvents, __spirv_DeviceEvent private* WaitEvents, __spirv_DeviceEvent private* RetEvent)
{
  return __intel_enqueue_marker_impl(__builtin_astype(Queue, queue_t), NumEvents, __builtin_astype(WaitEvents, const generic clk_event_t*), __builtin_astype(RetEvent, generic clk_event_t*));
}

uint SPIRV_OVERLOADABLE SPIRV_BUILTIN(EnqueueMarker, _i64_i32_p3i64_p3i64, )(__spirv_Queue Queue, uint NumEvents, __spirv_DeviceEvent local* WaitEvents, __spirv_DeviceEvent local* RetEvent)
{
  return __intel_enqueue_marker_impl(__builtin_astype(Queue, queue_t), NumEvents, __builtin_astype(WaitEvents, const generic clk_event_t*), __builtin_astype(RetEvent, generic clk_event_t*));
}

uint SPIRV_OVERLOADABLE SPIRV_BUILTIN(EnqueueMarker, _i64_i32_p4i64_p4i64, )(__spirv_Queue Queue, uint NumEvents, __spirv_DeviceEvent generic* WaitEvents, __spirv_DeviceEvent generic* RetEvent)
{
  return __intel_enqueue_marker_impl(__builtin_astype(Queue, queue_t), NumEvents, __builtin_astype(WaitEvents, const generic clk_event_t*), __builtin_astype(RetEvent, generic clk_event_t*));
}

#define DEFN_GET_KERNEL_WORK_GROUP_SIZE(ADDRSPACE_NUMBER, ADDRSPACE_NAME)                                                                                                \
uint __builtin_spirv_OpGetKernelWorkGroupSize_p0func_p##ADDRSPACE_NUMBER##i8_i32_i32(uchar* Invoke, ADDRSPACE_NAME uchar *Param, uint ParamSize, uint ParamAlign)  \
{                                                                                                                                                                        \
  return __builtin_IB_get_max_workgroup_size();                                                                                                                          \
}
DEFN_GET_KERNEL_WORK_GROUP_SIZE(0, private)
DEFN_GET_KERNEL_WORK_GROUP_SIZE(1, global)
DEFN_GET_KERNEL_WORK_GROUP_SIZE(2, constant)
DEFN_GET_KERNEL_WORK_GROUP_SIZE(3, local)
DEFN_GET_KERNEL_WORK_GROUP_SIZE(4, generic)

uint __get_kernel_work_group_size_impl(uchar* Block, uchar* Params)
{
  return __builtin_IB_get_max_workgroup_size();
}

int OVERLOADABLE IGIL_RetainEvent( __spirv_DeviceEvent );
void SPIRV_OVERLOADABLE SPIRV_BUILTIN(RetainEvent, _i64, )(__spirv_DeviceEvent Event)
{
  IGIL_RetainEvent(Event);
}

int OVERLOADABLE IGIL_ReleaseEvent( __spirv_DeviceEvent in_event );
void SPIRV_OVERLOADABLE SPIRV_BUILTIN(ReleaseEvent, _i64, )(__spirv_DeviceEvent Event)
{
  IGIL_ReleaseEvent(Event);
}

__spirv_DeviceEvent IGIL_CreateUserEvent();

__spirv_DeviceEvent SPIRV_OVERLOADABLE SPIRV_BUILTIN(CreateUserEvent, , )()
{
  return IGIL_CreateUserEvent();
}

bool OVERLOADABLE IGIL_Valid_Event( __spirv_DeviceEvent in_event );

bool SPIRV_OVERLOADABLE SPIRV_BUILTIN(IsValidEvent, _i64, )(__spirv_DeviceEvent Event)
{
  return IGIL_Valid_Event(Event);
}

void OVERLOADABLE IGIL_SetUserEventStatus( __spirv_DeviceEvent event, int state );

void SPIRV_OVERLOADABLE SPIRV_BUILTIN(SetUserEventStatus, _i64_i32, )(__spirv_DeviceEvent Event, int Status)
{
    IGIL_SetUserEventStatus( Event, Status );
}

void OVERLOADABLE IGIL_CaptureEventProfilingInfo( __spirv_DeviceEvent event, clk_profiling_info name,  __global void *value );

void SPIRV_OVERLOADABLE SPIRV_BUILTIN(CaptureEventProfilingInfo, _i64_i32_p1i8, )(__spirv_DeviceEvent Event, int ProfilingInfo, global char *Value)
{
    // SPIR-V CmdExecTime has a different enum value than CLK_PROFILING_COMMAND_EXEC_TIME.
    // Perform the mapping from SPIR-V enum to our internal/Clang enum before calling
    // into our target implementation

    clk_profiling_info profilingInfo = (ProfilingInfo == CmdExecTime)
        ? CLK_PROFILING_COMMAND_EXEC_TIME
        : ~CLK_PROFILING_COMMAND_EXEC_TIME; //known bad value

    IGIL_CaptureEventProfilingInfo(Event, profilingInfo, Value);
}

INLINE uint __intel_calc_kernel_local_size_for_sub_group_count(uint subgroupCount, uint simdSize)
{
    return subgroupCount * simdSize;
}

INLINE uint __intel_calc_kernel_max_num_subgroups(uint simdSize)
{
    // Note: We truncate here because the OpGetKernelMaxNumSubgroups is asking
    // for the number of _whole_ subgroups that can execute.
    return __builtin_IB_get_max_workgroup_size() / simdSize;
}

#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

__spirv_Queue SPIRV_OVERLOADABLE SPIRV_BUILTIN(GetDefaultQueue, , )()
{
    return __builtin_astype(__builtin_IB_get_default_device_queue(), __spirv_Queue);
}

#endif // __OPENCL_C_VERSION__ >= CL_VERSION_2_0


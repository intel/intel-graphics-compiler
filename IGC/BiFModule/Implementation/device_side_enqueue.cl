/*===================== begin_copyright_notice ==================================

Copyright (c) 2017 Intel Corporation

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.


======================= end_copyright_notice ==================================*/

// Device-Side Enqueue Instructions

// Implementation/ doesn't include cth.  Forward declare this so below users compile.
// FIXME: this should really be defined in Implementation/ and used in Languages/.
int __intel_enqueue_marker_impl(queue_t q, uint numEventsInWaitList, const __generic clk_event_t* waitList, __generic clk_event_t* returnEvent );


uint __builtin_spirv_OpEnqueueMarker_i64_i32_p0i64_p0i64(Queue_t Queue, uint NumEvents, private ClkEvent_t *WaitEvents, private ClkEvent_t *RetEvent)
{
  return __intel_enqueue_marker_impl(Queue, NumEvents, WaitEvents, RetEvent);
}

uint __builtin_spirv_OpEnqueueMarker_i64_i32_p3i64_p3i64(Queue_t Queue, uint NumEvents, local ClkEvent_t *WaitEvents, local ClkEvent_t *RetEvent)
{
  return __intel_enqueue_marker_impl(Queue, NumEvents, WaitEvents, RetEvent);
}

uint __builtin_spirv_OpEnqueueMarker_i64_i32_p4i64_p4i64(Queue_t Queue, uint NumEvents, generic ClkEvent_t *WaitEvents, generic ClkEvent_t *RetEvent)
{
  return __intel_enqueue_marker_impl(Queue, NumEvents, WaitEvents, RetEvent);
}

#define DEFN_GET_KERNEL_WORK_GROUP_SIZE(ADDRSPACE_NUMBER, ADDRSPACE_NAME)                                                                                                \
uint __builtin_spirv_OpGetKernelWorkGroupSize_p0func_p##ADDRSPACE_NUMBER##i8_i32_i32(void (^Invoke)(int), ADDRSPACE_NAME uchar *Param, uint ParamSize, uint ParamAlign)  \
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

int OVERLOADABLE IGIL_RetainEvent( clk_event_t in_event );
void __builtin_spirv_OpRetainEvent_i64(ClkEvent_t Event)
{
  IGIL_RetainEvent(Event);
}

int OVERLOADABLE IGIL_ReleaseEvent( clk_event_t in_event );
void __builtin_spirv_OpReleaseEvent_i64(ClkEvent_t Event)
{
  IGIL_ReleaseEvent(Event);
}

clk_event_t IGIL_CreateUserEvent();

ClkEvent_t __builtin_spirv_OpCreateUserEvent()
{
  return IGIL_CreateUserEvent();
}

bool OVERLOADABLE IGIL_Valid_Event( clk_event_t in_event );

bool __builtin_spirv_OpIsValidEvent_i64(ClkEvent_t Event)
{
  return IGIL_Valid_Event(Event);
}

void OVERLOADABLE IGIL_SetUserEventStatus( clk_event_t event, int state );

void __builtin_spirv_OpSetUserEventStatus_i64_i32(ClkEvent_t Event, uint Status)
{
    IGIL_SetUserEventStatus( Event, Status );
}

void OVERLOADABLE IGIL_CaptureEventProfilingInfo( clk_event_t event, clk_profiling_info name,  __global void *value );

void __builtin_spirv_OpCaptureEventProfilingInfo_i64_i32_p1i8(ClkEvent_t Event, uint ProfilingInfo, global uchar *Value)
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

Queue_t __builtin_spirv_OpGetDefaultQueue()
{
    //return get_default_queue();
    __global void* deviceQ = __builtin_IB_get_default_device_queue();
    return __builtin_astype(deviceQ, Queue_t);
}

#endif // __OPENCL_C_VERSION__ >= CL_VERSION_2_0


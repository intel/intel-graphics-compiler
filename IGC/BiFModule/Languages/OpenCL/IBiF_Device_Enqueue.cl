/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

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

extern __constant int __DashGSpecified;

#define exec_offsetof( x, y ) (int)(&((x*)(0))->y)

// float passed as int
extern __constant int __ProfilingTimerResolution;
INLINE float __intel__getProfilingTimerResolution()
{
    return as_float(__ProfilingTimerResolution);
}

//===----------------------------------------------------------------------===//
// Internal Helper Functions for Events
//===----------------------------------------------------------------------===//

///////////////////////////////////////////////////////////////////////////
//
// If -g is specified, we are allowed to return a more specific error code
// indicating why enqueue_kernel() failed.
//
INLINE int __intel_ErrorCode(int code)
{
    if (__DashGSpecified)
    {
        return code;
    }
    else
    {
        return CLK_ENQUEUE_FAILURE;
    }
}

 __global IGIL_EventPool* IGIL_GetEventPool()
{
    return (__global IGIL_EventPool*)__builtin_IB_get_event_pool();
}

__global IGIL_DeviceEvent* IGIL_GetDeviceEvents()
{
    __global IGIL_EventPool *pool = IGIL_GetEventPool();

    return (__global IGIL_DeviceEvent *)(pool + 1);
}

INLINE bool OVERLOADABLE IGIL_Valid_Event( __spirv_DeviceEvent in_event )
{
     // Get the event pool
    __global IGIL_EventPool *pool = IGIL_GetEventPool();

    bool retValue = true;

    if( ( ( int )(__builtin_astype(in_event, __private void*)) >= pool->m_size ) ||
        ( IGIL_EVENT_INVALID_HANDLE == (size_t)__builtin_astype(in_event, __private void*) ) )
    {
        retValue = false;
    }

    return retValue;
}

INLINE int IGIL_AcquireEvent()
{
    // Get the event pool
    __global IGIL_EventPool *pool = IGIL_GetEventPool();

    // offset into the event data in the pool
    __global IGIL_DeviceEvent *events = IGIL_GetDeviceEvents();

    uint poolSize = pool->m_size;
    uint poolHead = pool->m_head;

    int eventIndex = IGIL_EVENT_INVALID_HANDLE;

    // Get an event index
    while( poolHead < poolSize )
    {
        int attemptIndex = atomic_cmpxchg( &pool->m_head, poolHead, poolHead + 1 );

        if( attemptIndex == poolHead )
        {
            eventIndex = attemptIndex;
            break;
        }
        else
        {
            poolHead = pool->m_head;
        }
    }

    // Event pool has filled up - do a linear search for previously
    // freed events
    if( eventIndex == IGIL_EVENT_INVALID_HANDLE )
    {
        for( int i = 0; i < poolSize; i++ )
        {
            int status = atomic_cmpxchg( &events[i].m_state, IGIL_EVENT_UNUSED, IGIL_EVENT_QUEUED );

            if( IGIL_EVENT_UNUSED == status )
            {
                // found an unused event. return this handle.
                eventIndex = i;
            }
        }
    }

    if( eventIndex != IGIL_EVENT_INVALID_HANDLE )
    {
        // creation of event sets reference count to 1
        events[eventIndex].m_refCount = 1;

        // create with no outstanding child
        // act of enqueue using this event will increment num children
        // hence, a kernel with an m_event is its own child; this count is decremented in UpdateEventStatus
        events[eventIndex].m_numChildren = 0;

        // no commands have been made dependent on this event yet, refcount = 0
        events[eventIndex].m_numDependents = 0;

        // track parent event associated with this event
        // when this event is CL_COMPLETE, notify parent
        events[eventIndex].m_parentEvent = IGIL_EVENT_INVALID_HANDLE;

        events[eventIndex].m_eventType = IGIL_EVENT_TYPE_NORMAL;

        // set initial state to submitted
        events[eventIndex].m_state = CL_SUBMITTED;
    }

    return eventIndex;
}

INLINE void OVERLOADABLE IGIL_FreeEvent( clk_event_t event )
{
    // offset into the event data
    __global IGIL_DeviceEvent *events = IGIL_GetDeviceEvents();

    atomic_xchg( &events[(int)__builtin_astype(event, __private void*)].m_state, IGIL_EVENT_UNUSED );
}

INLINE int OVERLOADABLE IGIL_RetainEvent( __spirv_DeviceEvent in_event )
{
    // offset into the event data
    __global IGIL_DeviceEvent *events = IGIL_GetDeviceEvents();

    int status = CLK_SUCCESS;

    if( IGIL_Valid_Event( in_event ) == false )
    {
        status = CLK_EVENT_ALLOCATION_FAILURE;
    }
    else
    {
        atomic_inc( &events[(int)__builtin_astype(in_event, __private void*)].m_refCount );
    }

    return status;
}

INLINE int OVERLOADABLE IGIL_ReleaseEvent( __spirv_DeviceEvent in_event )
{
    // offset into the event data
    __global IGIL_DeviceEvent *events = IGIL_GetDeviceEvents();

    int status = CLK_SUCCESS;

    if( IGIL_Valid_Event( in_event ) == false )
    {
      status = CLK_EVENT_ALLOCATION_FAILURE;
    }
    else
    {
        atomic_dec( &events[(int)__builtin_astype(in_event, __private void*)].m_refCount );

        // May not be required to be this aggressive freeing events
        if( ( events[(int)__builtin_astype(in_event, __private void*)].m_refCount <= 0 ) &&
            ( events[(int)__builtin_astype(in_event, __private void*)].m_numChildren <= 0 ) &&
            ( events[(int)__builtin_astype(in_event, __private void*)].m_numDependents <= 0 ) )
        {
            atomic_xchg( &events[(int)__builtin_astype(in_event, __private void*)].m_state, IGIL_EVENT_UNUSED );
        }
    }

    return status;
}

INLINE __spirv_DeviceEvent IGIL_CreateUserEvent()
{
    __spirv_DeviceEvent newEvent = __builtin_astype((__private void*)(size_t)IGIL_AcquireEvent(), __spirv_DeviceEvent);

    if( IGIL_Valid_Event(newEvent) == false)
    {
        // Now what?  OpenCL 2 2.0 rev5 defines no return code for this function
    }
    else
    {
        __global IGIL_DeviceEvent *events = IGIL_GetDeviceEvents();

        events[(int)__builtin_astype(newEvent, __private void*)].m_eventType = IGIL_EVENT_TYPE_USER;
        events[(int)__builtin_astype(newEvent, __private void*)].m_state = CL_SUBMITTED;
    }

    return newEvent;
}

INLINE void OVERLOADABLE IGIL_SetUserEventStatus( __spirv_DeviceEvent event, int state )
{
    __global IGIL_DeviceEvent *events = IGIL_GetDeviceEvents();

    if( IGIL_Valid_Event( event ) == false )
    {
        // Now what?  OpenCL 2 2.0 rev5 defines no return code for this function
    }
    else if( events[(int)__builtin_astype(event, __private void*)].m_eventType & IGIL_EVENT_TYPE_USER )
    {
        // state must be CL_COMPLETE or a negative value
        if( ( state == CL_COMPLETE ) || ( state & 0x80000000 ) )
        {
            events[(int)__builtin_astype(event, __private void*)].m_state = state;
        }
    }
}

INLINE void OVERLOADABLE IGIL_CaptureEventProfilingInfo( __spirv_DeviceEvent event, clk_profiling_info name,  __global void *value )
{
    int status = CLK_SUCCESS;
    if( IGIL_Valid_Event( event ) == false )
    {
        status = CLK_EVENT_ALLOCATION_FAILURE;
    }
    else if( name != CLK_PROFILING_COMMAND_EXEC_TIME )
    {
        status = CLK_ENQUEUE_FAILURE;
    }
    else
    {
        __global IGIL_DeviceEvent *events = IGIL_GetDeviceEvents();
        events[(int)__builtin_astype(event, __private void*)].m_eventType |= IGIL_EVENT_TYPE_PROFILING;
        events[(int)__builtin_astype(event, __private void*)].m_pProfiling = ( ulong ) value;
        //if this function is called after event is already transitioned to CL_COMPLETE state,it means that timestamp are present, update pointer data
        if( events[(int)__builtin_astype(event, __private void*)].m_state == CL_COMPLETE )
        {
            __global ulong* retValue = ( __global ulong* ) value;

            ulong StartTime                = events[(int)__builtin_astype(event, __private void*)].m_profilingCmdStart;
            ulong EndTime                  = events[(int)__builtin_astype(event, __private void*)].m_profilingCmdEnd;
            ulong CompleteTime             = events[(int)__builtin_astype(event, __private void*)].m_profilingCmdComplete;
            ulong CLEndTransitionTime      = 0;
            ulong CLCompleteTransitionTime = 0;

            //check if timer didn't reset by hitting max value
            if( CompleteTime > StartTime )
            {
                CLEndTransitionTime      = EndTime - StartTime;
                CLCompleteTransitionTime = CompleteTime - StartTime;
            }
            //if we hit this else it means that GPU timer reset to 0, compute proper delta
            else
            {
                if( EndTime < StartTime )
                {
                    CLEndTransitionTime = PROFILING_MAX_TIMER_VALUE - StartTime + EndTime;
                }
                else
                {
                    CLEndTransitionTime = EndTime - StartTime;
                }
                CLCompleteTransitionTime = PROFILING_MAX_TIMER_VALUE - StartTime + CompleteTime;
            }

            //first value is END - START timestamp
            retValue[ 0 ] = ( ulong )( ( float )CLEndTransitionTime * __intel__getProfilingTimerResolution() );
            //second value is COMPLETE - START timestamp
            retValue[ 1 ] = ( ulong )( ( float )CLCompleteTransitionTime * __intel__getProfilingTimerResolution() );
        }
    }
    return;
}

//===----------------------------------------------------------------------===//
// Internal Helper Functions for Enqueue
//===----------------------------------------------------------------------===//
INLINE __global IGIL_CommandQueue* IGIL_GetCommandQueue( queue_t q )
{
    return __builtin_astype(q, __global IGIL_CommandQueue*);
}

INLINE bool IGIL_ValidCommandQueue( queue_t q )
{
   __global IGIL_CommandQueue *pQueue =  IGIL_GetCommandQueue( q );

   if( pQueue == NULL || ( pQueue->m_magic != IGIL_MAGIC_NUMBER ))
   {
        return false;
   }
   else
   {
        return true;
   }
}

INLINE __global IGIL_CommandHeader* IGIL_GetCommandHeader( queue_t q, uint offset )
{
    __global uchar *pQueueRaw = __builtin_astype(q, __global uchar*);

    __global IGIL_CommandHeader* pCommand = (__global IGIL_CommandHeader*)(pQueueRaw + offset);

    return pCommand;
}

INLINE void OVERLOADABLE IGIL_MEMCPY_PTOG( __global void* pDst, __private void* pSrc, int numBytes )
{
    numBytes = numBytes >> 2;
    for( int i = 0; i < numBytes; i++ ) {
        ((__global int*)pDst)[i] = ((__private int*)pSrc)[i];
    }
}

INLINE int OVERLOADABLE IGIL_ComputeRoundedBlockSize( int size )
{
    return ( 3 + size ) & ~3;
}

INLINE int OVERLOADABLE IGIL_ComputeRoundedCommandAlignment( int size )
{
    // align to multiple of an IGIL_CommandHeader.
    return ( sizeof(IGIL_CommandHeader) - ( size % sizeof(IGIL_CommandHeader) ) );
}

INLINE int OVERLOADABLE IGIL_ComputeRoundedCacheline( int size )
{
    return ( 64 + size ) & ~64;
}

INLINE int OVERLOADABLE IGIL_AcquireQueueSpace( queue_t q, uint numBytes )
{
    __global IGIL_CommandQueue *pQueue = IGIL_GetCommandQueue( q );

    int queueSpace = -1;

    if( ( numBytes & 0x7fffffff ) == numBytes )
    {
        uint requestedSize = numBytes;

        // align head pointer to sizeof(IGIL_CommandHeader) - Can the runtime do this
        // instead of this  being checked for every enqueue?
        if( pQueue->m_head == 0 )
        {
            uint startingAlignment = IGIL_DEVICE_QUEUE_HEAD_INIT;
            atomic_cmpxchg( &pQueue->m_head, 0, startingAlignment );
        }

        uint queueHead = pQueue->m_head;
        uint queueSize = pQueue->m_size;

        // request space for this command
        while( ( queueHead < queueSize ) &&
               ( ( queueHead + requestedSize ) < queueSize) )
        {
            int attemptSpace = atomic_cmpxchg( &pQueue->m_head, queueHead, queueHead + requestedSize );

            if( attemptSpace == queueHead )
            {
                queueSpace = attemptSpace;
                break;
            }
            else
            {
                queueHead = pQueue->m_head;
            }
        }
    }

    return queueSpace;
}

//===----------------------------------------------------------------------===//
// API Entry Points for Events
//===----------------------------------------------------------------------===//

#define to_spirv_event(e) (__builtin_astype(e, __spirv_DeviceEvent))
#define to_ocl_event(e)   (__builtin_astype(e, clk_event_t))

INLINE void OVERLOADABLE retain_event(clk_event_t event)
{
    SPIRV_BUILTIN(RetainEvent, _i64, )(to_spirv_event(event));
}

INLINE void OVERLOADABLE release_event( clk_event_t event )
{
    SPIRV_BUILTIN(ReleaseEvent, _i64, )(to_spirv_event(event));
}

INLINE clk_event_t OVERLOADABLE create_user_event()
{
    return to_ocl_event(SPIRV_BUILTIN(CreateUserEvent, , )());
}

INLINE void OVERLOADABLE set_user_event_status( clk_event_t e, int state )
{
    SPIRV_BUILTIN(SetUserEventStatus, _i64_i32, )(to_spirv_event(e), state);
}

INLINE void OVERLOADABLE capture_event_profiling_info(clk_event_t e, clk_profiling_info name, __global void* value)
{
    SPIRV_BUILTIN(CaptureEventProfilingInfo, _i64_i32_p1i8, )(to_spirv_event(e), name, value);
}

INLINE bool OVERLOADABLE is_valid_event (clk_event_t event)
{
    return SPIRV_BUILTIN(IsValidEvent, _i64, )(to_spirv_event(event));
}

INLINE OVERLOADABLE queue_t get_default_queue()
{
    return __builtin_astype(SPIRV_BUILTIN(GetDefaultQueue, , )(), queue_t);
}

#undef exec_offsetof

#endif // __BIF_DEVICE_ENQUEUE_CL__

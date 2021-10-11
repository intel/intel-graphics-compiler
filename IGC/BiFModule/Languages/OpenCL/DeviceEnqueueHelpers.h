/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef __DEVICE_ENQUEUE_HELPERS__
#define __DEVICE_ENQUEUE_HELPERS__

#ifdef __EXECUTION_MODEL_DEBUG
#pragma pack( push )
#pragma pack( 1 )
#include "DeviceEnqueueInternalTypes.h"
#pragma pack( pop )
#else
#include "DeviceEnqueueInternalTypes.h"
#endif

INLINE int __intel_ErrorCode(int code);
__global IGIL_EventPool* IGIL_GetEventPool( void );
__global IGIL_DeviceEvent* IGIL_GetDeviceEvents();
INLINE bool OVERLOADABLE IGIL_Valid_Event( __spirv_DeviceEvent in_event );
INLINE int IGIL_AcquireEvent();
INLINE void OVERLOADABLE IGIL_FreeEvent( clk_event_t event );
INLINE int OVERLOADABLE IGIL_RetainEvent( __spirv_DeviceEvent in_event );
INLINE int OVERLOADABLE IGIL_ReleaseEvent( __spirv_DeviceEvent in_event );
INLINE __spirv_DeviceEvent IGIL_CreateUserEvent( void );
INLINE void OVERLOADABLE IGIL_SetUserEventStatus( __spirv_DeviceEvent event, int state );
INLINE void OVERLOADABLE IGIL_CaptureEventProfilingInfo( __spirv_DeviceEvent event, clk_profiling_info name,  __global void *value );
INLINE __global IGIL_CommandQueue* IGIL_GetCommandQueue( queue_t q );
INLINE bool IGIL_ValidCommandQueue( queue_t q );
INLINE __global IGIL_CommandHeader* IGIL_GetCommandHeader( queue_t q, uint offset );
INLINE void OVERLOADABLE IGIL_MEMCPY_PTOG( __global void* pDst, __private void* pSrc, int numBytes );
INLINE int OVERLOADABLE IGIL_ComputeRoundedBlockSize( int size );
INLINE int OVERLOADABLE IGIL_ComputeRoundedCommandAlignment( int size );
INLINE int OVERLOADABLE IGIL_ComputeRoundedCacheline( int size );
INLINE int OVERLOADABLE IGIL_AcquireQueueSpace( queue_t q, uint numBytes );

#endif // __DEVICE_ENQUEUE_HELPERS__

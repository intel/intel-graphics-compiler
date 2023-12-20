/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

//===----------------------------------------------------------------------===//
// API Entry Points for ndrange_t
//===----------------------------------------------------------------------===//

#include "DeviceEnqueueHelpers.h"

// IGIL_SetIGIL_ndrange sets properites of IGIL_ndrange_t from ndrage_t
INLINE void IGIL_SetIGIL_ndrange(volatile IGIL_ndrange_t *igil_range, __private ndrange_t *range )
{
    igil_range->m_dispatchDimensions  = range->workDimension;
    igil_range->m_globalWorkOffset[0] = range->globalWorkOffset[0];
    igil_range->m_globalWorkOffset[1] = range->globalWorkOffset[1];
    igil_range->m_globalWorkOffset[2] = range->globalWorkOffset[2];
    igil_range->m_globalWorkSize[0]   = range->globalWorkSize[0];
    igil_range->m_globalWorkSize[1]   = range->globalWorkSize[1];
    igil_range->m_globalWorkSize[2]   = range->globalWorkSize[2];
    igil_range->m_localWorkSize[0]    = range->localWorkSize[0];
    igil_range->m_localWorkSize[1]    = range->localWorkSize[1];
    igil_range->m_localWorkSize[2]    = range->localWorkSize[2];
}

//   IGIL_ValidNdrange validates the ndrange parameters with respect to workDimension
//   current implementation works for workDimension 1,2 and 3
INLINE bool IGIL_ValidNdrange( const ndrange_t range )
{
    uint kernel_work_group_size = __builtin_IB_get_max_workgroup_size();
    uint lws_sum = range.localWorkSize[0];
    uint lws_mul = range.localWorkSize[0];
    uint gws_mul = range.globalWorkSize[0];

    if( range.workDimension == 0 )
    {
        return false;
    }
    else if( range.workDimension > 3 )
    {
        return false;
    }
    else if( range.workDimension == 2 )
    {
        lws_sum = range.localWorkSize[0] + range.localWorkSize[1];
        lws_mul = range.localWorkSize[0] * range.localWorkSize[1];
        gws_mul = range.globalWorkSize[0] * range.globalWorkSize[1];
    }
    else if( range.workDimension == 3 )
    {
        lws_sum = range.localWorkSize[0] + range.localWorkSize[1] + range.localWorkSize[2];
        lws_mul = range.localWorkSize[0] * range.localWorkSize[1] * range.localWorkSize[2];
        gws_mul = range.globalWorkSize[0] * range.globalWorkSize[1] * range.globalWorkSize[2];
    }

    if( lws_mul > kernel_work_group_size ) return false;

    // Relax enqueue checks as OCL 2.1 (going through SPIRV path) allows them.
    bool nonZeroEnqueue = (gws_mul != 0);
    uint check = BIF_FLAG_CTRL_GET(IsSPIRV) ? nonZeroEnqueue : 1;

    if ((lws_mul == 0) & (lws_sum > 0) & check)
        return false;

    if ((gws_mul == 0) & !BIF_FLAG_CTRL_GET(IsSPIRV))
        return false;

    if( range.localWorkSize[0] > range.globalWorkSize[0] ) return false;
    if(( range.workDimension > 1 ) & ( range.localWorkSize[1] > range.globalWorkSize[1] )) return false;
    if(( range.workDimension > 2 ) & ( range.localWorkSize[2] > range.globalWorkSize[2] )) return false;

    return true;
}

INLINE bool IGIL_ZeroSizedEnqueue(const ndrange_t range)
{
    return BIF_FLAG_CTRL_GET(IsSPIRV) ?
        range.globalWorkSize[0] * range.globalWorkSize[1] * range.globalWorkSize[2] == 0 :
        false;
}

INLINE __global IGIL_CommandHeader* OVERLOADABLE IGIL_EnqueueKernelShared(
    queue_t q,
    unsigned  blockId,
    __private void* scalarParamBuf,
    unsigned sizeofscalarParamBuf,
    __private void* globalArgBuf,
    __private void* globalPtrArgMappingBuf,
    unsigned  numGlobalArgBuf,
    __private void* getobjectidMappingBuf,
    unsigned numArgMappings,
    int numLocalPtrs,
    __private int* localMemSizes,
    ndrange_t range,
    kernel_enqueue_flags_t flags,
    int numHandles,  // number of uints to reserve for "other stuff"
    clk_event_t event)
{
    __global IGIL_CommandQueue *pQueue = IGIL_GetCommandQueue( q );

    __global IGIL_CommandHeader* pCommand = 0;

    // amount of space = space for Command header + space for arguments
    uint numBytes = exec_offsetof( IGIL_CommandHeader, m_data );

    // ... plus the size of the execution_model scalar arguments, rounded up
    unsigned sizeOfScalarArgsinBuffer = IGIL_ComputeRoundedBlockSize( sizeofscalarParamBuf );
    numBytes += sizeOfScalarArgsinBuffer;

    // plus size of argument mapping buffer = numBuffers * 4
    unsigned sizeOfArgMapBuf = IGIL_ComputeRoundedBlockSize( numGlobalArgBuf*4 );
    numBytes += sizeOfArgMapBuf;
    // plus size of captured global buffers = numBuffers * 8
    unsigned sizeOfGlobalArgInfo = IGIL_ComputeRoundedBlockSize( numGlobalArgBuf*8 );
    numBytes += sizeOfGlobalArgInfo;
    // ... plus the size of local memory arguments in dwords, rounded up
    unsigned sizeOflocalMemSizesMap = ( numLocalPtrs * sizeof(uint) );
    numBytes += sizeOflocalMemSizesMap;
    // ... plus the size of getobjectid mappings
    unsigned sizeOfgetobjectidArgMapBuf = IGIL_ComputeRoundedBlockSize( numArgMappings*4*2 );
    numBytes += sizeOfgetobjectidArgMapBuf;

    // ... plus bytes for other stuff (event dependencies)
    numBytes += ( numHandles * sizeof(uint) );

    // ...round up to sizeof(IGIL_CommandHeader)
    numBytes += IGIL_ComputeRoundedCommandAlignment( numBytes );

    // ...round up to sizeof cacheline
    numBytes += IGIL_ComputeRoundedCacheline( numBytes );

    // attempt to reserve space for this command
    int offset = IGIL_AcquireQueueSpace( q, numBytes );

    if( offset >= 0 )
    {
        pCommand = IGIL_GetCommandHeader( q, offset );

        pCommand->m_commandSize           = numBytes;
        pCommand->m_magic                 = IGIL_COMMAND_MAGIC_NUMBER;
        pCommand->m_kernelId              = blockId;
        pCommand->m_totalLocalSize        = 0;
        pCommand->m_event                 = ( int )__builtin_astype(event, __private void*);
        pCommand->m_numScalarArguments    = sizeofscalarParamBuf >> 2; // assume all scalar args are DWORD sized for now sine they occupy a dword of the curbe.
        pCommand->m_sizeOfScalarArguments = sizeofscalarParamBuf;
        pCommand->m_numDependencies       = 0;
        pCommand->m_numOfLocalPtrSizes    = numLocalPtrs;
        pCommand->m_numGlobalCapturedBuffer = numGlobalArgBuf;
        pCommand->m_numGlobalArguments = numArgMappings;

        //IGIL_MEMCPY_PTOG( &pCommand->m_range, &range, sizeof(ndrange_t) );
        //don't want to depend on sizeof(IGIL_ndrange_t) eq sizeof(ndrange_t)
        IGIL_SetIGIL_ndrange(&pCommand->m_range, &range);

        atomic_inc( &pQueue->m_controls.m_TotalNumberOfQueues );

        // if associated event, update its count of children
        if( IGIL_EVENT_INVALID_HANDLE != (size_t)__builtin_astype(event, __private void*) )
        {
            __global IGIL_DeviceEvent *events = IGIL_GetDeviceEvents();

            atomic_inc( &events[(int)__builtin_astype(event, __private void*)].m_numChildren );
        }

        __global void* variableParamAddr = &pCommand->m_data[numHandles];
        // copy scalar arguments
        IGIL_MEMCPY_PTOG( variableParamAddr, scalarParamBuf, sizeofscalarParamBuf );
        variableParamAddr = sizeOfScalarArgsinBuffer + (__global char*)variableParamAddr;

        // copy argument to global ptr mapping buffer
        IGIL_MEMCPY_PTOG( variableParamAddr, globalPtrArgMappingBuf, sizeOfArgMapBuf );
        variableParamAddr = sizeOfArgMapBuf + (__global char*)variableParamAddr;
        // copy global buffer addresses
        IGIL_MEMCPY_PTOG( variableParamAddr, globalArgBuf, sizeOfGlobalArgInfo );
        variableParamAddr = sizeOfGlobalArgInfo + (__global char*)variableParamAddr;

        // copy local memory sizes
        IGIL_MEMCPY_PTOG( variableParamAddr, localMemSizes, sizeOflocalMemSizesMap );
        variableParamAddr = sizeOflocalMemSizesMap + (__global char*)variableParamAddr;

        // copy objectid mappings
        IGIL_MEMCPY_PTOG( variableParamAddr, getobjectidMappingBuf, sizeOfgetobjectidArgMapBuf );
    }

    return pCommand;
}

INLINE ndrange_t OVERLOADABLE IGIL_CreateNDRangeT( )
{
    ndrange_t range;
    //these are default values, local work size should be 0 to indicate NULL case
    range.workDimension  = 1;
    range.globalWorkSize[0]   = 1;
    range.globalWorkSize[1]   = 1;
    range.globalWorkSize[2]   = 1;
    range.localWorkSize[0]    = 0;
    range.localWorkSize[1]    = 0;
    range.localWorkSize[2]    = 0;
    range.globalWorkOffset[0] = 0;
    range.globalWorkOffset[1] = 0;
    range.globalWorkOffset[2] = 0;

    return range;
}

INLINE int OVERLOADABLE IGIL_EnqueueKernel(
    queue_t q,
    unsigned  blockId,
    __private void* scalarParamBuf,
    unsigned sizeofscalarParamBuf,
    __private void* globalArgBuf,
    __private void* globalPtrArgMappingBuf,
    unsigned  numGlobalArgBuf,
    __private void* getobjectidMappingBuf,
    unsigned numArgMappings,
    ndrange_t range,
    kernel_enqueue_flags_t flags,
    clk_event_t event)
{
    int status = CLK_SUCCESS;

  //checking inputs and setting status
  if( !IGIL_ValidCommandQueue( q ) )
  {
      status = __intel_ErrorCode(CLK_INVALID_QUEUE);
  }
  else if( !IGIL_ValidNdrange( range ) )
  {
      status = __intel_ErrorCode(CLK_INVALID_NDRANGE);
  }
  else if (IGIL_ZeroSizedEnqueue(range))
  {
      // noop
  }
  else
  {
        __global IGIL_CommandHeader* pCommand = IGIL_EnqueueKernelShared( q, blockId,
                                                                          scalarParamBuf, sizeofscalarParamBuf,
                                                                          globalArgBuf, globalPtrArgMappingBuf, numGlobalArgBuf,
                                                                          getobjectidMappingBuf, numArgMappings,
                                                                          0, NULL, range, flags, 0, event );

        if( pCommand == 0 )
        {
            status = __intel_ErrorCode(CLK_DEVICE_QUEUE_FULL);
        }
  }

    return status;
}

INLINE int OVERLOADABLE IGIL_EnqueueKernelWithLocalParams(
    queue_t q,
    int numLocalPtrs,
    __private int* localMemSizes,
    unsigned  blockId,
    __private void* scalarParamBuf,
    unsigned sizeofscalarParamBuf,
    __private void* globalArgBuf,
    __private void* globalPtrArgMappingBuf,
    unsigned numGlobalArgBuf,
    __private void* getobjectidMappingBuf,
    unsigned numArgMappings,
    ndrange_t range,
    kernel_enqueue_flags_t flags,
    clk_event_t event )
{
    int status = CLK_SUCCESS;

  //checking inputs and setting status
  if( !IGIL_ValidCommandQueue( q ) )
  {
      status = __intel_ErrorCode(CLK_INVALID_QUEUE);
  }
  else if( !IGIL_ValidNdrange( range ) )
  {
      status = __intel_ErrorCode(CLK_INVALID_NDRANGE);
  }
  else if( (ulong)numLocalPtrs & (ulong)localMemSizes )
  {
      status = __intel_ErrorCode(CLK_INVALID_ARG_SIZE);
  }
  else if (IGIL_ZeroSizedEnqueue(range))
  {
      // noop
  }
  else
  {
        __global IGIL_CommandHeader* pCommand = IGIL_EnqueueKernelShared( q, blockId,
                                                                          scalarParamBuf, sizeofscalarParamBuf,
                                                                          globalArgBuf, globalPtrArgMappingBuf, numGlobalArgBuf,
                                                                          getobjectidMappingBuf, numArgMappings,
                                                                          numLocalPtrs, localMemSizes,
                                                                          range, flags, 0, event );
        if( pCommand != 0 )
        {
           // copy local pointer sizes into command
            for( int i = 0; i < numLocalPtrs; i++ )
            {
                if( localMemSizes[i] == 0 )
                {
                     status = __intel_ErrorCode(CLK_INVALID_ARG_SIZE);
                }
                pCommand->m_totalLocalSize += localMemSizes[i];
            }
        }
        else
        {
            status = __intel_ErrorCode(CLK_DEVICE_QUEUE_FULL);
        }
    }

    return status;
}

INLINE int OVERLOADABLE IGIL_EnqueueKernelWithEvents(
    queue_t q,
    unsigned  blockId,
    __private void* scalarParamBuf,
    unsigned sizeofscalarParamBuf,
    __private void* globalArgBuf,
    __private void* globalPtrArgMappingBuf,
    unsigned  numGlobalArgBuf,
    __private void* getobjectidMappingBuf,
    unsigned  numArgMappings,
    int numLocalPtrs,
    __private int* localMemSizes,
    ndrange_t range,
    kernel_enqueue_flags_t flags,
    int numEvents,
    const clk_event_t* in_events,
    clk_event_t* out_event,
    clk_event_t parentEvent)
{
    int status = CLK_SUCCESS;

    // event associated with this command is the parent...
    // unless we need to acquire an event
    clk_event_t myEvent = parentEvent;

    __global IGIL_DeviceEvent *events = IGIL_GetDeviceEvents();

    // if returning an event, acquire an event
    if( 0 != out_event )
    {
        myEvent = __builtin_astype((__private void *)(size_t)IGIL_AcquireEvent(), clk_event_t);

        if( IGIL_EVENT_INVALID_HANDLE == (size_t)__builtin_astype(myEvent, __private void*) )
        {
          status = CLK_EVENT_ALLOCATION_FAILURE;
        }
        else if( IGIL_EVENT_INVALID_HANDLE != (size_t)__builtin_astype(parentEvent, __private void*) )
        {
            // if parent event, update its count of children to include newly created event

            events[(int)__builtin_astype(myEvent, __private void*)].m_parentEvent = (size_t)__builtin_astype(parentEvent, __private void*);
            atomic_inc( &events[(int)__builtin_astype(parentEvent, __private void*)].m_numChildren );
            events[(int)__builtin_astype(myEvent, __private void*)].m_state = CL_QUEUED;
        }
    }

    //checking inputs and setting status
    if( status != CLK_SUCCESS )
    {
        //do nothing , error status already set.
    }
    else if( !IGIL_ValidCommandQueue( q ) )
    {
        status = __intel_ErrorCode(CLK_INVALID_QUEUE);
    }
    else if( !IGIL_ValidNdrange( range ) )
    {
        status = __intel_ErrorCode(CLK_INVALID_NDRANGE);
    }
    else if(( in_events == NULL && numEvents > 0 ) || ( in_events != NULL && numEvents == 0 ))
    {
        status = __intel_ErrorCode(CLK_INVALID_EVENT_WAIT_LIST);
    }
    else if( numEvents > 0 && !is_valid_event(in_events[numEvents-1]) )
    {
        status = __intel_ErrorCode(CLK_INVALID_EVENT_WAIT_LIST);
    }
    else
    {
        // acquire & init command

        if (IGIL_ZeroSizedEnqueue(range))
        {
            blockId                = IGIL_KERNEL_ID_ENQUEUE_MARKER;
            scalarParamBuf         = 0;
            sizeofscalarParamBuf   = 0;
            globalArgBuf           = 0;
            globalPtrArgMappingBuf = 0;
            numGlobalArgBuf        = 0;
            getobjectidMappingBuf  = 0;
            numArgMappings         = 0;
            numLocalPtrs           = 0;
            localMemSizes          = 0;
            flags                  = 0;
            range                  = IGIL_CreateNDRangeT();
        }

        __global IGIL_CommandHeader* pCommand = IGIL_EnqueueKernelShared( q, blockId,
                                                                          scalarParamBuf, sizeofscalarParamBuf,
                                                                          globalArgBuf, globalPtrArgMappingBuf, numGlobalArgBuf,
                                                                          getobjectidMappingBuf, numArgMappings,
                                                                          numLocalPtrs, localMemSizes,
                                                                          range, flags, numEvents, myEvent );

        // if success, copy event dependencies
        if( pCommand != 0 )
        {

            // copy local pointer sizes into command
            for( int i = 0; i < numLocalPtrs; i++ )
            {
                pCommand->m_totalLocalSize += localMemSizes[i];
            }

            pCommand->m_numDependencies = numEvents;

            // copy event handles
            // FIXME: does not check for invalid event handles!
            for( int i = 0; i < numEvents; i++ )
            {
                clk_event_t d = in_events[i];

                pCommand->m_data[i] = (unsigned)(__builtin_astype(d, __private void*));
                atomic_inc( &events[(int)__builtin_astype(d, __private void*)].m_numDependents );
            }
        }
        else
        {
            status = CLK_DEVICE_QUEUE_FULL;

            // if enqueue failed, do we have an event handle that must be freed?
            if( NULL != out_event )
            {
                IGIL_FreeEvent( myEvent );
                myEvent = __builtin_astype((__private void *)IGIL_EVENT_INVALID_HANDLE, clk_event_t);
            }
        }
    }

    // set the output event on either success or failure
    if( NULL != out_event )
    {
        *out_event = myEvent;
    } // end if acquired event

    return status;
}

//===----------------------------------------------------------------------===//
// API Entry Points for Device Enqueue
//===----------------------------------------------------------------------===//
INLINE int enqueue_IB_kernel( queue_t q,
                                kernel_enqueue_flags_t flags,
                                const ndrange_t range,
                                unsigned block_id,
                                __private void* scalarParamBuf,
                                unsigned sizeofscalarParamBuf,
                                __private void* globalArgBuf,
                                unsigned  numGlobalArgBuf,
                                __private void* globalPtrArgMappingBuf,
                                __private void* getobjectidMappingBuf,
                                unsigned  numArgMappings )

{
    return IGIL_EnqueueKernel( q, block_id,
                               scalarParamBuf, sizeofscalarParamBuf,
                               globalArgBuf, globalPtrArgMappingBuf, numGlobalArgBuf,
                               getobjectidMappingBuf, numArgMappings,
                               range, flags,
                               __builtin_astype((__private void *)(size_t)__builtin_IB_get_parent_event(),
                               clk_event_t) );
}

INLINE int enqueue_IB_kernel_local( queue_t q,
                                      kernel_enqueue_flags_t flags,
                                      const ndrange_t range,
                                      unsigned block_id,
                                      __private void* scalarParamBuf,
                                      unsigned sizeofscalarParamBuf,
                                      __private void* globalArgBuf,
                                      unsigned  numGlobalArgBuf,
                                      __private int* local_size_buf,
                                      uint sizeof_local_size_buf,
                                      __private void* globalPtrArgMappingBuf,
                                      __private void* getobjectidMappingBuf ,
                                      unsigned  numArgMappings )
{
    // contains the scalar captured variable info
    return IGIL_EnqueueKernelWithLocalParams( q, sizeof_local_size_buf, local_size_buf,
                                              block_id, scalarParamBuf, sizeofscalarParamBuf,
                                              globalArgBuf, globalPtrArgMappingBuf, numGlobalArgBuf,
                                              getobjectidMappingBuf, numArgMappings,
                                              range, flags,
                                              __builtin_astype((__private void *)(size_t)__builtin_IB_get_parent_event(),
                                              clk_event_t) );
}

INLINE int enqueue_IB_kernel_events(
    queue_t q,
    kernel_enqueue_flags_t flags,
    const ndrange_t range,
    uint numEventsInWaitList,
    const __private clk_event_t* waitList,
    __private clk_event_t* returnEvent,
    unsigned block_id,
    __private void* scalarParamBuf,
    unsigned sizeofscalarParamBuf,
    __private void* globalArgBuf,
    unsigned  numGlobalArgBuf,
    __private void* globalPtrArgMappingBuf,
    __private void* getobjectidMappingBuf,
    unsigned  numArgMappings )
{
    return IGIL_EnqueueKernelWithEvents( q,
                                         block_id,
                                         scalarParamBuf, sizeofscalarParamBuf,
                                         globalArgBuf, globalPtrArgMappingBuf, numGlobalArgBuf,
                                         getobjectidMappingBuf, numArgMappings,
                                         0,
                                         NULL,
                                         range,
                                         flags,
                                         numEventsInWaitList,
                                         waitList,
                                         returnEvent,
                                         __builtin_astype((__private void *)(size_t)__builtin_IB_get_parent_event(), clk_event_t) );
}

INLINE int enqueue_IB_kernel_local_events(
    queue_t q,
    kernel_enqueue_flags_t flags,
    const ndrange_t range,
    uint numEventsInWaitList,
    const __private clk_event_t* waitList,
    __private clk_event_t* returnEvent,
    unsigned block_id,
    __private void* scalarParamBuf,
    unsigned sizeofscalarParamBuf,
    __private void* globalArgBuf,
    unsigned  numGlobalArgBuf,
    __private int* local_size_buf,
    uint sizeof_local_size_buf,
    __private void* globalPtrArgMappingBuf,
    __private void* getobjectidMappingBuf,
    unsigned  numArgMappings )
{
    return IGIL_EnqueueKernelWithEvents( q,
                                         block_id,
                                         scalarParamBuf, sizeofscalarParamBuf,
                                         globalArgBuf, globalPtrArgMappingBuf, numGlobalArgBuf,
                                         getobjectidMappingBuf, numArgMappings,
                                         sizeof_local_size_buf,
                                         local_size_buf,
                                         range,
                                         flags,
                                         numEventsInWaitList,
                                         waitList,
                                         returnEvent,
                                         __builtin_astype((__private void *)(size_t)__builtin_IB_get_parent_event(), clk_event_t) );
}

INLINE uint IGIL_calc_sub_group_count_for_ndrange( const ndrange_t range, uint block_simd_size)
{
  uint result = 0;
  if( IGIL_ValidNdrange(range) ) {
    uint dim = range.workDimension;
    uint x = range.localWorkSize[0];
    uint y = range.localWorkSize[1];
    uint z = range.localWorkSize[2];

    uint work_size = x * ( dim > 1 ? y : 1 ) * ( dim > 2 ? z : 1);
    //assuming block_simd_size is power of 2
    result = (work_size >> ctz(block_simd_size)) + ( (work_size & (block_simd_size - 1)) ? 1 : 0);
  }
  return result;
}

int __intel_enqueue_marker_impl(
    queue_t q,
    uint numEventsInWaitList,
    const generic clk_event_t* waitList,
    generic clk_event_t* returnEvent )
{
    ndrange_t range = IGIL_CreateNDRangeT();

    kernel_enqueue_flags_t flags = 0;

    if( returnEvent != NULL )
    {
        return IGIL_EnqueueKernelWithEvents(
            q,
            IGIL_KERNEL_ID_ENQUEUE_MARKER,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            NULL,
            range,
            flags,
            numEventsInWaitList,
            waitList,
            returnEvent,
            __builtin_astype((__private void *)(size_t)__builtin_IB_get_parent_event(), clk_event_t) );
    }
    else
    {
        return CLK_SUCCESS;
    }
}

// The clang header currently only has a private* overload of enqueue_marker()
// available.  Implement that here by just calling into the generic version.
// If the clang header changes to a generic overload we can do that here
// as well.  Clang complains of ambiguity if we have a private* and generic*
// overload of this function.
int OVERLOADABLE enqueue_marker(
    queue_t q,
    uint numEventsInWaitList,
    const private clk_event_t* waitList,
    private clk_event_t* returnEvent )
{
    return __intel_enqueue_marker_impl(q, numEventsInWaitList, waitList, returnEvent);
}

INLINE ndrange_t OVERLOADABLE IGIL_CreateNDRangeT(void);

INLINE ndrange_t OVERLOADABLE ndrange_1D(
    size_t global_work_size )
{
    ndrange_t range = IGIL_CreateNDRangeT();

    range.workDimension = 1;
    range.globalWorkSize[0] = global_work_size;

    return range;
}

INLINE ndrange_t OVERLOADABLE ndrange_1D(
    size_t global_work_size,
    size_t local_work_size )
{
    ndrange_t range = IGIL_CreateNDRangeT();

    range.workDimension = 1;
    range.globalWorkSize[0] = global_work_size;
    range.localWorkSize[0]  = local_work_size;

    return range;
}

INLINE ndrange_t OVERLOADABLE ndrange_1D(
    size_t global_work_offset,
    size_t global_work_size,
    size_t local_work_size )
{
    ndrange_t range = IGIL_CreateNDRangeT();

    range.workDimension = 1;
    range.globalWorkOffset[0] = global_work_offset;
    range.globalWorkSize[0]   = global_work_size;
    range.localWorkSize[0]    = local_work_size;

    return range;
}

INLINE ndrange_t OVERLOADABLE ndrange_2D(
    const size_t global_work_size[2] )
{
    ndrange_t range = IGIL_CreateNDRangeT();

    range.workDimension = 2;
    range.globalWorkSize[0] = global_work_size[0];
    range.globalWorkSize[1] = global_work_size[1];

    return range;
}

INLINE ndrange_t OVERLOADABLE ndrange_2D(
    const size_t global_work_size[2],
    const size_t local_work_size[2] )
{
    ndrange_t range = IGIL_CreateNDRangeT();

    range.workDimension = 2;
    range.globalWorkSize[0] = global_work_size[0];
    range.globalWorkSize[1] = global_work_size[1];
    range.localWorkSize[0]  = local_work_size[0];
    range.localWorkSize[1]  = local_work_size[1];

    return range;
}

INLINE ndrange_t OVERLOADABLE ndrange_2D(
    const size_t global_work_offset[2],
    const size_t global_work_size[2],
    const size_t local_work_size[2] )
{
    ndrange_t range = IGIL_CreateNDRangeT();

    range.workDimension = 2;
    range.globalWorkOffset[0] = global_work_offset[0];
    range.globalWorkOffset[1] = global_work_offset[1];
    range.globalWorkSize[0]   = global_work_size[0];
    range.globalWorkSize[1]   = global_work_size[1];
    range.localWorkSize[0]    = local_work_size[0];
    range.localWorkSize[1]    = local_work_size[1];

    return range;
}

INLINE ndrange_t OVERLOADABLE ndrange_3D(
    const size_t global_work_size[3] )
{
    ndrange_t range = IGIL_CreateNDRangeT();

    range.workDimension = 3;
    range.globalWorkSize[0] = global_work_size[0];
    range.globalWorkSize[1] = global_work_size[1];
    range.globalWorkSize[2] = global_work_size[2];

    return range;
}

INLINE ndrange_t OVERLOADABLE ndrange_3D(
    const size_t global_work_size[3],
    const size_t local_work_size[3] )
{
    ndrange_t range = IGIL_CreateNDRangeT();

    range.workDimension = 3;
    range.globalWorkSize[0] = global_work_size[0];
    range.globalWorkSize[1] = global_work_size[1];
    range.globalWorkSize[2] = global_work_size[2];
    range.localWorkSize[0]  = local_work_size[0];
    range.localWorkSize[1]  = local_work_size[1];
    range.localWorkSize[2]  = local_work_size[2];

    return range;
}

INLINE ndrange_t OVERLOADABLE ndrange_3D(
    const size_t global_work_offset[3],
    const size_t global_work_size[3],
    const size_t local_work_size[3] )
{
    ndrange_t range = IGIL_CreateNDRangeT();

    range.workDimension = 3;
    range.globalWorkOffset[0] = global_work_offset[0];
    range.globalWorkOffset[1] = global_work_offset[1];
    range.globalWorkOffset[2] = global_work_offset[2];
    range.globalWorkSize[0]   = global_work_size[0];
    range.globalWorkSize[1]   = global_work_size[1];
    range.globalWorkSize[2]   = global_work_size[2];
    range.localWorkSize[0]    = local_work_size[0];
    range.localWorkSize[1]    = local_work_size[1];
    range.localWorkSize[2]    = local_work_size[2];

    return range;
}

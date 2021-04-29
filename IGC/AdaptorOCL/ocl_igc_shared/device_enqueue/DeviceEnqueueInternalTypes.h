/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

/*****************************************************************************\
Notes: Common file that will be used for C99 device enqueue kernels and Runtime CLT's
\*****************************************************************************/

#ifndef DEVICE_ENQUEUE_INTERNAL_TYPES_H
#define DEVICE_ENQUEUE_INTERNAL_TYPES_H

#define IGIL_KERNEL_ID_ENQUEUE_MARKER               -1

// IGIL Event Flags
#define IGIL_EVENT_UNUSED                           -501
#define IGIL_EVENT_QUEUED                           -502
#define IGIL_EVENT_INVALID_HANDLE                   0xffffffff

// IGIL Event Types
#define IGIL_EVENT_TYPE_NORMAL                      0x0
#define IGIL_EVENT_TYPE_USER                        0x1
#define IGIL_EVENT_TYPE_PROFILING                   0x2

// Canary values
#define IGIL_MAGIC_NUMBER                           0x494E5443
#define IGIL_COMMAND_MAGIC_NUMBER                   0x494E544347505500

//!!! Make sure value of this define equals PARALLEL_SCHEDULER_HW_GROUPS in DeviceEnqueue.h
#define MAX_NUMBER_OF_PARALLEL_GPGPU_WALKERS        ( 64 )
#define MAX_NUMBER_OF_ENQUEUE_MARKER                ( 128 )
#define MAX_NUMBER_OF_EVENTS_TO_UPDATE              ( MAX_NUMBER_OF_PARALLEL_GPGPU_WALKERS + MAX_NUMBER_OF_ENQUEUE_MARKER )


//timestamp written by pipe control needs to be multipled by 80 ns, TODO : this is different on SKL and BXT. code needs to be added to handle them correctly.
#define    PROFILING_TIMER_RESOLUTION                                                   80
//timestamp value is written on 36 bits
#define    PROFILING_MAX_TIMER_VALUE                                                    0xFFFFFFFFF

#define exec_offsetof( x, y ) (int)(&((x*)(0))->y)

typedef union ptr64_t
{
    long*  m_ptr;
    ulong  m_value;
} IGIL_ptr64_t;

typedef struct
{
    uint  m_dispatchDimensions;
    ulong m_globalWorkOffset[3];
    ulong m_globalWorkSize[3];
    ulong m_localWorkSize[3];
} IGIL_ndrange_t;

typedef int IGIL_clk_event_t;

typedef int IGIL_kernel_enqueue_flags_t;

// internal device representation of an event
typedef struct
{
    uint m_state;                   // unused, queued, submitted, running, complete.
    uint m_eventType;               // user event, profiling enabled...
    int m_refCount;                 // enqueues that depend on this event. free event when all 0: {refCount, numChildren, numDependents}
    int m_numChildren;              // this event triggers success when all children complete
    int m_numDependents;            // number of events waiting for this event to reach CL_COMPLETE
    uint m_parentEvent;             // when this child completes (m_numChildren=0):
                                    //   1. set state = CL_COMPLETE
                                    //   2. decrement the parent's m_numChildren (if parent valid)
                                    //   3. if parent's numChildren == 0, goto #1
    //!!!!! make sure that profiling variables are aligned to 64 bits, be extremly precaucious when modifiying this structure, in case of broken alignement PIPE CONTROL will write to wrong offset!!!!!!
    ulong m_profilingCmdStart;      // timestamp when this command starts -> it is event returned by some enqueue and timestamp start for this is after scheduler which enqueued this cmd
    ulong m_profilingCmdEnd;        // timestamp when this command ends -> timestamp write after kernel directly associated with this event.
    ulong m_profilingCmdComplete;   // timestamp when this event is complete, all childs are done, so when this event transitions to CL_COMPLETE.
    ulong m_pProfiling;      // address to write profiling info to (if enabled) //turned off becasue of pointer size problems todo:resolve
} IGIL_DeviceEvent;

// internal device event pool representation
typedef struct
{
    ulong m_CLcompleteTimestamp;    // only scheduler updates state of events, here is timestamp used for profiling to indicate when this transition happened.
    float m_TimestampResolution;    // resolution of the timestamp counter
    uint m_padding;                 // padding is needed because of alignment requirements for events
    uint m_head;                    // pool head point in IGIL_DeviceEvent units (0 means first event)
    uint m_size;                    // number of events there is space for after m_size
    // variable legnth part starts here
    //  m_size * sizeof(IGIL_DeviceEvent) bytes long
} IGIL_EventPool;

// internal device enqueue command representation
typedef struct
{
    uint   m_commandSize;           // size in bytes, including variable part and padding to 64bytes and sizeof(IGIL_CommandHeader)
    ulong  m_magic;                 // 'I' 'N' 'T' 'C' 'G' 'P' 'U' canary
    int    m_kernelId;              // this value will be used to choose kernel for GPGPU walker.
    IGIL_ndrange_t    m_range;      // real version would have dimensions, offsets, multiple ranges
    IGIL_clk_event_t  m_event;      // handle to event associated with this command, if any
    uint   m_numScalarArguments;    // number of scalars to patch in curbe including values
    uint   m_sizeOfScalarArguments; // size of scalars. TODO : needed?
    uint   m_numOfLocalPtrSizes;    // number of local sizes passed into m_data
    uint   m_totalLocalSize;        // total amount of SLM used within kernel.
    uint   m_numGlobalCapturedBuffer; // total number of global buffer passes from parent to child
    uint   m_numDependencies;       // events this command depends on. handles to them will be the first members of m_args
    uint   m_commandState;          // command state , may not be needed.
    IGIL_kernel_enqueue_flags_t m_enqueueFlags;    // flags that were used during enqueue
    uint   m_numGlobalArguments;    //total number of global arguments passed as kernel arguments, excluding global pointers.
    uint   m_data[1];
    // variable length part starts here
    //   Event Data: # number of events of size sizeof(clk_event_t) store events IDS ( m_numDependencies )
    //   Scalar Captured Variable Data: # number of scalar kernel arguments with values ( m_numScalarArguments )
    //   Global UAV Argument Data: arg number associated with each of the global memory pointer
    //                             size of each argument is 2B. size: m_numGlobalCapturedBuffer * 2
    //   Global UAV Address Data: address of global mem surfaces: uav address(64 bit).
    //                        size: m_numGlobalCapturedBuffer * 8
    //   Local: # number of local surfaces sizes ( DWORD each )
    //   Global arguments data : arn number associated with each of the global memory argument
    //   Global argument unique id : argument unique ID that can identify this resource.
} IGIL_CommandHeader;

// intneral device controls/flags
typedef struct
{
    uint m_StackSize;
    uint m_StackTop;
    uint m_PreviousHead;
    uint m_TotalNumberOfQueues;
    uint m_SecondLevelBatchOffset;
    uint m_PreviousNumberOfQueues;
    uint m_LastScheduleEventNumber;
    uint m_IsProfilingEnabled;
    uint m_DebugNextBlockID;
    uint m_DebugNextBlockGWS;
    uint m_DebugParentEvent;
    uint m_SchedulerConstantBufferSize;
    uint m_SchedulerDSHOffset;
    uint m_DynamicHeapSizeInBytes;
    uint m_DynamicHeapStart;
    uint m_IDTstart;
    uint m_QstorageSize;
    uint m_QstorageTop;
    ulong m_EventTimestampAddress;
    uint m_CurrentIDToffset;
    uint m_CurrentDSHoffset;
    uint m_PreviousStorageTop;
    uint m_PreviousStackTop;
    uint m_IDTAfterFirstPhase;
    uint m_CurrentScheduleEventNumber;
    uint m_EnqueueMarkerScheduled;
    ulong m_DummyAtomicOperationPlaceholder;
    uint m_StartBlockID;
    int m_SLBENDoffsetInBytes;
    uint m_BTbaseOffset;
    uint m_BTmaxSize;
    uint m_CurrentSSHoffset;
    uint m_ErrorCode;
    uint m_CriticalSection;
    uint m_ParentDSHOffset;         // Offset to DSH in DSHMemInfo.pBuffer
    IGIL_clk_event_t m_EventDependencies[ MAX_NUMBER_OF_EVENTS_TO_UPDATE ];
    ulong m_CleanupSectionAddress;
    uint m_CleanupSectionSize;
    uint m_IsSimulation;
    //temporary place for experiments.
    uint m_SchedulerEarlyReturnCounter;
    uint m_SchedulerEarlyReturn;
    uint Temporary[10];//for debug
} IGIL_ExecutionControls;

// internal device command queue representation
typedef struct
{
    uint m_magic;            // 'I' 'N' 'T' C'
    uint m_head;             // next free location in the queue
    uint m_size;             // size of the queue in bytes
    IGIL_ExecutionControls m_controls;

    // The header must be aligned to sizeof(IGIL_CommandHeader)

    // variable length part starts here
    // m_size bytes used to fill
} IGIL_CommandQueue;

typedef struct
{
    uint m_parameterType;
    uint m_parameterSize;
    uint m_patchOffset;
    uint m_sourceOffset; // for tokens that use 3 dimensions, 0 , 4, 8 indicates dimension
} IGIL_KernelCurbeParams;
typedef struct
{
    uint            m_KernelDataOffset;
    uint            m_SamplerHeapOffset;       // Offset to SamplerHeap ( BorderColorState and SamplerStateArray ) on KRS
    uint            m_SamplerParamsOffset;
    uint            m_ConstantBufferOffset;
    uint            m_SSHTokensOffset;
    uint            m_BTSoffset;
    uint            m_BTSize;
}IGIL_KernelAddressData;
typedef struct
{
    uint                      m_numberOfCurbeParams; // number of paramters to patch
    uint                      m_numberOfCurbeTokens;
    uint                      m_numberOfSamplerStates;
    uint                      m_SizeOfSamplerHeap;              // BorderColorState with SamplerStateArray
    uint                      m_SamplerBorderColorStateOffsetOnDSH;    // Offset to SamplerStateArray on block's DSH
    uint                      m_SamplerStateArrayOffsetOnDSH;    // Offset to SamplerStateArray on block's DSH
    uint                      m_sizeOfConstantBuffer;
    ulong                     m_PatchTokensMask;
    ulong                     m_ScratchSpacePatchValue;
    uint                      m_SIMDSize;
    uint                      m_HasBarriers;
    uint                      m_RequiredWkgSizes[3];
    uint                      m_InilineSLMSize;
    uint                      m_NeedLocalIDS;
    uint                      m_PayloadSize;
    uint                      m_DisablePreemption;
    uint                      m_CanRunConcurently;
    IGIL_KernelCurbeParams    m_data[1]; //IGIL_KernelCurbeParams
} IGIL_KernelData;

typedef struct
{
    ulong                  m_numberOfKernels; //number of kernels.
    uint                   m_ParentImageDataOffset;
    uint                   m_ParentKernelImageCount;
    uint                   m_ParentSamplerParamsOffset;
    uint                   m_ParentSamplerCount;
    IGIL_KernelAddressData m_data[1]; //offsets for n x kernel data.
} IGIL_KernelDataHeader;

typedef struct
{
    uint                   m_Width;
    uint                   m_Height;
    uint                   m_Depth;
    uint                   m_ArraySize;
    uint                   m_NumMipLevels;
    uint                   m_NumSamples;
    uint                   m_ChannelOrder;
    uint                   m_ChannelDataType;
    uint                   m_ObjectID;
} IGIL_ImageParamters;


typedef struct
{
    uint                   m_ArgID;                     // Block's argument id
    uint                   m_SamplerStateOffset;        // Offset of specific ( with m_ArgID ) Sampler state on per-block DSH
} IGIL_SamplerParams;

typedef struct
{
    uint                   m_ObjectID;                  // Sampler Object id

    uint                   m_AddressingMode;
    uint                   NormalizedCoords;
    uint                   CoordinateSnapRequired;
} IGIL_ParentSamplerParams;                             // Parent's Sampler Curbe data



#define IGIL_QUEUE_PROLOG_SIZE ( sizeof(IGIL_CommandQueue) )
#define IGIL_QUEUE_COMMAND_SIZE ( sizeof(IGIL_CommandHeader) )
#define IGIL_QUEUE_PROLOG_COMMAND_SIZE ( IGIL_QUEUE_PROLOG_SIZE + IGIL_QUEUE_COMMAND_SIZE )
// IGIL_CommandQueue.m_head must be aligned to sizeof(IGIL_CommandHeader).
// This macro sets m_head to the correct initial value
#define IGIL_DEVICE_QUEUE_HEAD_INIT ( IGIL_QUEUE_COMMAND_SIZE > IGIL_QUEUE_PROLOG_SIZE ? sizeof(IGIL_CommandHeader) : ( IGIL_QUEUE_PROLOG_SIZE + ( IGIL_QUEUE_COMMAND_SIZE - IGIL_QUEUE_PROLOG_COMMAND_SIZE % IGIL_QUEUE_COMMAND_SIZE ) ) )

#endif

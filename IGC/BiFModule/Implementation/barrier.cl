/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2022 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../Languages/OpenCL/IBiF_SPIRV_Utils.cl"

// EmitVISAPass support for __builtin_IB_memfence/__builtin_IB_typedmemfence requires some arguments to
// be constants as those are used to prepare a message descriptor, so must be known at compile time.
// To assure that all the arguments are constants for O0 path, there is a special function
// marked with __attribute__((optnone)) which implements seperate call instruction created for each
// arguments configuration.

// MEMFENCE IMPLEMENTATION

// Below choice of some HW scopes is explained.
// 1. TODO: CrossDevice should be mapped to LSC_FS_SYSTEM_RELEASE or LSC_FS_SYSTEM_ACQUIRE.
//     sycl::memory_scope::system and OpenCL's memory_scope_all_svm_devices are
//     lowered to SPIR-V CrossDevice. SYCL SPEC 2020 for system scope:
//     "applies to any work-item or host thread in the system that is currently permitted to access
//     the memory allocation containing the referenced object".
// 2. Subgroup and Invocation --> LSC_FS_THREAD_GROUP:
//     our HW spec doesn’t have corresponding scope for Invocation or Subgroup, hence mapping to
//     lowest possible scope
#define CONVERT_SCOPE_SPIRV_TO_VISA(scope)            \
    ((scope) == CrossDevice ? LSC_FS_GPU :            \
     (scope) == Device      ? LSC_FS_GPU :            \
     LSC_FS_THREAD_GROUP)

// Adding scope parameter to 'MEMFENCE_IF' macro causes huge compilation time increase
// due to number of calls to __builtin_IB_memfence inside __intel_memfence_optnone increases from
// 16 (2 * 2 * 2 * 2) to <number of possible scope values> * 16.
// So for optnone version keep old behavior: use GPU scope for global memory fence
// and GROUP scope for local memory fence.
void OPTNONE __intel_memfence_optnone(bool flushRW, bool isGlobal, bool invalidateL1, bool evictL1)
{
#define MEMFENCE_IF(V1, V5, V6, V7)                                              \
if (flushRW == V1 && isGlobal == V5 && invalidateL1 == V6 && evictL1 == V7)      \
{                                                                                \
    LSC_FS lsc_scope = (V5 ? LSC_FS_GPU : LSC_FS_THREAD_GROUP);                  \
    __builtin_IB_memfence(true, V1, false, false, false, V5, V6, V7, lsc_scope); \
} else

// Generate combinations for all MEMFENCE_IF cases, e.g.:
// true, true, true
// true, true, false etc.
#define MF_L3(...) MF_L2(__VA_ARGS__,false) MF_L2(__VA_ARGS__,true)
#define MF_L2(...) MF_L1(__VA_ARGS__,false) MF_L1(__VA_ARGS__,true)
#define MF_L1(...) MEMFENCE_IF(__VA_ARGS__,false) MEMFENCE_IF(__VA_ARGS__,true)
MF_L3(false)
MF_L3(true) {}

#undef MEMFENCE_IF
#undef MF_L3
#undef MF_L2
#undef MF_L1
}
void __intel_memfence(bool flushRW, bool isGlobal, bool invalidateL1, bool evictL1, Scope_t scope)
{
    LSC_FS lsc_scope = CONVERT_SCOPE_SPIRV_TO_VISA(scope);
    __builtin_IB_memfence(true, flushRW, false, false, false, isGlobal, invalidateL1, evictL1, lsc_scope);
}

void __intel_memfence_handler(bool flushRW, bool isGlobal, bool invalidateL1, bool evictL1, Scope_t scope)
{
    if (BIF_FLAG_CTRL_GET(OptDisable))
        __intel_memfence_optnone(flushRW, isGlobal, invalidateL1, evictL1);
    else
        __intel_memfence(flushRW, isGlobal, invalidateL1, evictL1, scope);
}

// TYPEDMEMFENCE IMPLEMENTATION

void OPTNONE __intel_typedmemfence_optnone(bool invalidateL1)
{
    if (invalidateL1)
        __builtin_IB_typedmemfence(true);
    else
        __builtin_IB_typedmemfence(false);
}

void __intel_typedmemfence(bool invalidateL1)
{
    __builtin_IB_typedmemfence(invalidateL1);
}

void __intel_typedmemfence_handler(bool invalidateL1)
{
    if (BIF_FLAG_CTRL_GET(OptDisable))
        __intel_typedmemfence_optnone(invalidateL1);
    else
        __intel_typedmemfence(invalidateL1);
}

// Barrier Instructions

static void __intel_atomic_work_item_fence( Scope_t Memory, uint Semantics )
{
    bool fence = Semantics & ( Acquire | Release | AcquireRelease | SequentiallyConsistent );

    bool invalidateL1 = Semantics & ( Acquire | AcquireRelease | SequentiallyConsistent );
    bool evictL1 = Semantics & ( Release | AcquireRelease | SequentiallyConsistent );

    // We always need to 'fence' image memory (aka, flush caches, drain pipelines)
    fence |= ( Semantics & ImageMemory );

    if (fence)
    {
        if (Semantics & ImageMemory)
        {
            // An image fence requires a fence with R/W invalidate (L3 flush)
            // + for pre-DG2 platforms, sampler cache flush
            __intel_typedmemfence_handler(invalidateL1);
        }
        // A global/local memory fence requires a hardware fence in general,
        // although on some platforms they may be elided; platform-specific checks are performed in codegen
        if (Semantics & WorkgroupMemory)
        {
           __intel_memfence_handler(false, false, false, false, Memory);
        }
        if (Semantics & CrossWorkgroupMemory)
        {
           if (Memory == Device || Memory == CrossDevice)
           {
               __intel_memfence_handler(true, true, invalidateL1, evictL1, Memory);
           }
           else
           {
               // Single workgroup executes on one DSS and shares the same L1 cache.
               // If scope doesn't reach outside of workgroup, L1 flush can be skipped.
               __intel_memfence_handler(false, true, false, false, Memory);
           }
        }
    }
}

void __intel_workgroup_barrier(int Memory, int Semantics)
{
    __intel_atomic_work_item_fence( Memory, Semantics );
    __builtin_IB_thread_group_barrier();
}

void SPIRV_OVERLOADABLE SPIRV_BUILTIN(ControlBarrier, _i32_i32_i32, )(int Execution, int Memory, int Semantics)
{
    if (Execution == Device)
    {
        global_barrier();
    }
    else  if( Execution == Subgroup )
    {
        // nothing will be emited but we need to prevent optimization splitting control flow
        __builtin_IB_sub_group_barrier();
    }
    else  if( Execution <= Workgroup )
    {
        __intel_workgroup_barrier(Memory, Semantics);
    }
}

void SPIRV_OVERLOADABLE SPIRV_BUILTIN(ControlBarrierArriveINTEL, _i32_i32_i32, )(int Execution, int Memory, int Semantics)
{
    if( Execution == Workgroup )
    {
        __intel_atomic_work_item_fence( Memory, Semantics );
        __builtin_IB_thread_group_barrier_signal();
    }
}

void SPIRV_OVERLOADABLE SPIRV_BUILTIN(ControlBarrierWaitINTEL, _i32_i32_i32, )(int Execution, int Memory, int Semantics)
{
    if( Execution == Workgroup )
    {
        __intel_atomic_work_item_fence( Memory, Semantics );
        __builtin_IB_thread_group_barrier_wait();
    }
}

void SPIRV_OVERLOADABLE SPIRV_BUILTIN(MemoryBarrier, _i32_i32, )(int Memory, int Semantics)
{
    __intel_atomic_work_item_fence( Memory, Semantics );
}


// Named Barrier

void __intel_getInitializedNamedBarrierArray(local uint* id)
{
    *id = 0;
    SPIRV_BUILTIN(ControlBarrier, _i32_i32_i32, )( Workgroup, 0, SequentiallyConsistent | WorkgroupMemory );
}

bool __intel_is_first_work_group_item( void );

local __namedBarrier* __builtin_spirv_OpNamedBarrierInitialize_i32_p3__namedBarrier_p3i32(int SubGroupCount, local __namedBarrier* nb_array, local uint* id)
{
    local __namedBarrier* NB = &nb_array[*id];
    NB->count = SubGroupCount;
    NB->orig_count = SubGroupCount;
    NB->inc = 0;
    SPIRV_BUILTIN(ControlBarrier, _i32_i32_i32, )( Workgroup, 0, SequentiallyConsistent | WorkgroupMemory );
    if (__intel_is_first_work_group_item())
    {
        (*id)++;
    }
    SPIRV_BUILTIN(ControlBarrier, _i32_i32_i32, )( Workgroup, 0, SequentiallyConsistent | WorkgroupMemory );
    return NB;
}


static INLINE OVERLOADABLE
uint AtomicCompareExchange(local uint *Pointer, uint Scope, uint Equal, uint Unequal, uint Value, uint Comparator)
{
    return SPIRV_BUILTIN(AtomicCompareExchange, _p3i32_i32_i32_i32_i32_i32, )((local int*)Pointer, Scope, Equal, Unequal, Value, Comparator);
}

static INLINE
uint SubgroupLocalId()
{
    return SPIRV_BUILTIN_NO_OP(BuiltInSubgroupLocalInvocationId, , )();
}

static INLINE OVERLOADABLE
uint AtomicLoad(local uint *Pointer, uint Scope, uint Semantics)
{
    return SPIRV_BUILTIN(AtomicLoad, _p3i32_i32_i32, )((local int*)Pointer, Scope, Semantics);
}

static INLINE OVERLOADABLE
void AtomicStore(local uint *Pointer, uint Scope, uint Semantics, uint Value)
{
    SPIRV_BUILTIN(AtomicStore, _p3i32_i32_i32_i32, )((local int*)Pointer, Scope, Semantics, Value);
}

static INLINE OVERLOADABLE
uint AtomicInc(local uint *Pointer, uint Scope, uint Semantics)
{
    return SPIRV_BUILTIN(AtomicIIncrement, _p3i32_i32_i32, )((local int*)Pointer, Scope, Semantics);
}

static INLINE
uint Broadcast(uint Execution, uint Value, uint3 LocalId)
{
    return SPIRV_BUILTIN(GroupBroadcast, _i32_i32_v3i32, )(Execution, as_int(Value), as_int3(LocalId));
}

static INLINE OVERLOADABLE
uint SubgroupAtomicCompareExchange(local uint *Pointer, uint Scope, uint Equal, uint Unequal, uint Value, uint Comparator)
{
    uint result = 0;
    if (SubgroupLocalId() == 0)
        result = AtomicCompareExchange((volatile local uint*)Pointer, Scope, Equal, Unequal, Value, Comparator);
    result = Broadcast(Subgroup, result, (uint3)0);
    return result;
}

static INLINE OVERLOADABLE
uint SubgroupAtomicInc(local uint *Pointer, uint Scope, uint Semantics)
{
    uint result = 0;
    if (SubgroupLocalId() == 0)
        result = AtomicInc((volatile local uint*)Pointer, Scope, Semantics);
    result = Broadcast(Subgroup, result, (uint3)0);
    return result;
}

static void MemoryBarrier(Scope_t Memory, uint Semantics)
{
    SPIRV_BUILTIN(MemoryBarrier, _i32_i32, )(Memory, Semantics);
}

void __builtin_spirv_OpMemoryNamedBarrier_p3__namedBarrier_i32_i32(local __namedBarrier* NB,Scope_t Memory, uint Semantics)
{
    const uint AtomSema = SequentiallyConsistent | WorkgroupMemory;
    while (1)
    {
        const uint cnt = AtomicLoad(&NB->count, Workgroup, AtomSema);
        if (cnt > 0)
        {
            uint before = SubgroupAtomicCompareExchange(&NB->count, Workgroup, AtomSema, AtomSema, cnt - 1, cnt);
            if (before == cnt)
            {
                break;
            }
        }
    }

    while(AtomicLoad(&NB->count, Workgroup, AtomSema) > 0);
    MemoryBarrier(Memory, Semantics);
    uint inc = SubgroupAtomicInc(&NB->inc, Workgroup, AtomSema);
    if(inc == ((NB->orig_count) - 1))
    {
        AtomicStore(&NB->inc, Workgroup, AtomSema, 0);
        AtomicStore(&NB->count, Workgroup, AtomSema, NB->orig_count);
    }
}
void __builtin_spirv_OpMemoryNamedBarrierWrapperOCL_p3__namedBarrier_i32(local __namedBarrier* barrier, cl_mem_fence_flags flags)
{
    __builtin_spirv_OpMemoryNamedBarrier_p3__namedBarrier_i32_i32(barrier, Workgroup, AcquireRelease | get_spirv_mem_fence(flags));
}

void __builtin_spirv_OpMemoryNamedBarrierWrapperOCL_p3__namedBarrier_i32_i32(local __namedBarrier* barrier, cl_mem_fence_flags flags, memory_scope scope)
{
    __builtin_spirv_OpMemoryNamedBarrier_p3__namedBarrier_i32_i32(barrier, get_spirv_mem_scope(scope), AcquireRelease | get_spirv_mem_fence(flags));
}

__global volatile uchar* __builtin_IB_get_sync_buffer();
uint __intel_get_local_linear_id( void );
uint __intel_get_global_linear_id( void );
uint __intel_get_local_size( void );

// Implementation of global_barrier using atomic instructions.
// First two integers in the sync buffer are used for global synchronization, they are used interchangeably:
// first call to global_barrier() will use syncBuffer + 0, second call will use syncBuffer + 1, third call will use syncBuffer + 0 again, etc.
// This is to prevent a race condition, where one of the workgroups returned from the barrier and some other still wait for the value of the synchronization variable.
void __global_barrier_atomic()
{
    __intel_workgroup_barrier(Device, AcquireRelease | CrossWorkgroupMemory);

    bool firstThreadPerWg = (__builtin_IB_get_local_id_x() == 0) && (__builtin_IB_get_local_id_y() == 0) && (__builtin_IB_get_local_id_z() == 0);
    size_t numGroups = __builtin_IB_get_num_groups(0) * __builtin_IB_get_num_groups(1) * __builtin_IB_get_num_groups(2);

    __global volatile int* syncBuffer = (__global volatile int*)__builtin_IB_get_sync_buffer();
    __global volatile int* offsetVar = syncBuffer + 2;

    if (firstThreadPerWg)
    {
        int offset = atomic_or(offsetVar, 0);
        __global volatile int* syncVar = syncBuffer + offset;

        if (__intel_get_global_linear_id() == 0)
        {
            atomic_sub(syncVar, numGroups-1);
        }
        else
        {
            atomic_inc(syncVar);
        }

        while (atomic_or(syncVar, 0) != 0) {}

        if (offset) {
            atomic_and(offsetVar, 0);
        } else {
            atomic_or(offsetVar, 1);
        }
    }

    __intel_workgroup_barrier(Device, AcquireRelease | CrossWorkgroupMemory);
}

// Implementation of global_barrier without using atomic instructions except for fences.
void __global_barrier_nonatomic()
{
    //Make sure each WKG item hit the barrier.
    __intel_workgroup_barrier(Device, AcquireRelease | CrossWorkgroupMemory);

    __global volatile uchar* syncBuffer = __builtin_IB_get_sync_buffer();
    bool firstThreadPerWg = __intel_is_first_work_group_item();
    uint groupLinearId = (__builtin_IB_get_group_id(2) * __builtin_IB_get_num_groups(1) * __builtin_IB_get_num_groups(0)) + (__builtin_IB_get_group_id(1) * __builtin_IB_get_num_groups(0)) + __builtin_IB_get_group_id(0);

    //Now first thread of each wkg writes to designated place in syncBuffer
    if (firstThreadPerWg)
    {
        syncBuffer[groupLinearId] = 1;
        atomic_work_item_fence(CLK_GLOBAL_MEM_FENCE, memory_order_release, memory_scope_device); // == write_mem_fence(CLK_GLOBAL_MEM_FENCE);
    }

    uint numGroups = __builtin_IB_get_num_groups(0) * __builtin_IB_get_num_groups(1) * __builtin_IB_get_num_groups(2);
    //Higher wkg ids tend to not have work to do in all cases, therefore I choose last wkg to wait for the others, as it is most likely it will hit this code sooner.
    if (groupLinearId == (numGroups - 1))
    {
        uint localSize = __intel_get_local_size();
        //24 -48 case
        volatile uchar Value;
        do
        {
            atomic_work_item_fence(CLK_GLOBAL_MEM_FENCE, memory_order_acquire, memory_scope_device); // == read_mem_fence(CLK_GLOBAL_MEM_FENCE);
            Value = 1;
            for (uint i = __intel_get_local_linear_id(); i < numGroups; i += localSize)
            {
                Value = Value & syncBuffer[i];
            }

        } while (Value == 0);
        __intel_workgroup_barrier(Device, AcquireRelease | CrossWorkgroupMemory);

        for (uint i = __intel_get_local_linear_id(); i < numGroups; i += localSize)
        {
            syncBuffer[i] = 0;
        }
        atomic_work_item_fence(CLK_GLOBAL_MEM_FENCE, memory_order_release, memory_scope_device); // == write_mem_fence(CLK_GLOBAL_MEM_FENCE);
    }

    if (firstThreadPerWg)
    {
        while (syncBuffer[groupLinearId] != 0) {
           atomic_work_item_fence(CLK_GLOBAL_MEM_FENCE, memory_order_acquire, memory_scope_device); // == read_mem_fence(CLK_GLOBAL_MEM_FENCE);
        };
    }
    __intel_workgroup_barrier(Device, AcquireRelease | CrossWorkgroupMemory);
}

void global_barrier() {
    __global_barrier_nonatomic();
}

void system_memfence(char fence_typed_memory)
{
    return __builtin_IB_system_memfence(fence_typed_memory);
}



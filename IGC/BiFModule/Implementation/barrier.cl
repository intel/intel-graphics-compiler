/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2022 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../Languages/OpenCL/IBiF_SPIRV_Utils.cl"

extern __constant int __OptDisable;

// EmitVISAPass support for __builtin_IB_memfence/__builtin_IB_typedmemfence requires some arguments to
// be constants as those are used to prepare a message descriptor, so must be known at compile time.
// To assure that all the arguments are constants for O0 path, there is a special function
// marked with __attribute__((optnone)) which implements seperate call instruction created for each
// arguments configuration.

// MEMFENCE IMPLEMENTATION

void OPTNONE __intel_memfence_optnone(bool flushRW, bool isGlobal, bool invalidateL1, bool evictL1)
{
#define MEMFENCE_IF(V1, V5, V6, V7)                                           \
if (flushRW == V1 && isGlobal == V5 && invalidateL1 == V6 && evictL1 == V7)   \
{                                                                             \
    __builtin_IB_memfence(true, V1, false, false, false, V5, V6, V7);         \
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
void __intel_memfence(bool flushRW, bool isGlobal, bool invalidateL1, bool evictL1)
{
    __builtin_IB_memfence(true, flushRW, false, false, false, isGlobal, invalidateL1, evictL1);
}

void __intel_memfence_handler(bool flushRW, bool isGlobal, bool invalidateL1, bool evictL1)
{
    if (__OptDisable)
        __intel_memfence_optnone(flushRW, isGlobal, invalidateL1, evictL1);
    else
        __intel_memfence(flushRW, isGlobal, invalidateL1, evictL1);
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
    if (__OptDisable)
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
            // An image fence requires a fence with R/W invalidate (L3 flush) + a flush
            // of the sampler cache
            __intel_typedmemfence_handler(invalidateL1);
        }
        // A global/local memory fence requires a hardware fence in general,
        // although on some platforms they may be elided; platform-specific checks are performed in codegen
        if (Semantics & WorkgroupMemory)
        {
           __intel_memfence_handler(false, false, false, false);
        }
        if (Semantics & CrossWorkgroupMemory)
        {
           if (Memory == Device || Memory == CrossDevice)
           {
               __intel_memfence_handler(true, true, invalidateL1, evictL1);
           }
           else
           {
               // Single workgroup executes on one DSS and shares the same L1 cache.
               // If scope doesn't reach outside of workgroup, L1 flush can be skipped.
               __intel_memfence_handler(false, true, false, false);
           }
        }
    }
}

void SPIRV_OVERLOADABLE SPIRV_BUILTIN(ControlBarrier, _i32_i32_i32, )(int Execution, int Memory, int Semantics)
{
    if (Execution != Subgroup)
    {
        // sub group barrier requires no fence
        __intel_atomic_work_item_fence( Memory, Semantics );
    }

    if( Execution <= Workgroup )
    {
        __builtin_IB_thread_group_barrier();
    }
    else  if( Execution == Subgroup )
    {
        // nothing will be emited but we need to prevent optimization splitting control flow
        __builtin_IB_sub_group_barrier();
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

void global_barrier()
{
    //Make sure each WKG item hit the barrier.
    barrier(CLK_GLOBAL_MEM_FENCE);

    __global volatile uchar* syncBuffer = __builtin_IB_get_sync_buffer();
    bool firstThreadPerWg = (get_local_id(0) == 0) && (get_local_id(1) == 0) && (get_local_id(2) == 0);
    size_t groupLinearId = (get_group_id(2) * get_num_groups(1) * get_num_groups(0)) + (get_group_id(1) * get_num_groups(0)) + get_group_id(0);

    //Now first thread of each wkg writes to designated place in syncBuffer
    if (firstThreadPerWg)
    {
        syncBuffer[groupLinearId] = 1;
        atomic_work_item_fence(CLK_GLOBAL_MEM_FENCE, memory_order_release, memory_scope_device); // == write_mem_fence(CLK_GLOBAL_MEM_FENCE);
    }

    size_t numGroups = get_num_groups(0) * get_num_groups(1) * get_num_groups(2);
    //Higher wkg ids tend to not have work to do in all cases, therefore I choose last wkg to wait for the others, as it is most likely it will hit this code sooner.
    if (groupLinearId == (numGroups - 1))
    {
        size_t localSize = get_local_size(0) * get_local_size(1) * get_local_size(2);
        //24 -48 case
        volatile uchar Value;
        do
        {
            atomic_work_item_fence(CLK_GLOBAL_MEM_FENCE, memory_order_acquire, memory_scope_device); // == read_mem_fence(CLK_GLOBAL_MEM_FENCE);
            Value = 1;
            for (size_t i = get_local_linear_id(); i < numGroups; i += localSize)
            {
                Value = Value & syncBuffer[i];
            }

        } while (Value == 0);
        barrier(CLK_GLOBAL_MEM_FENCE);

        for (size_t i = get_local_linear_id(); i < numGroups; i += localSize)
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
    barrier(CLK_GLOBAL_MEM_FENCE);
}

void system_memfence(char fence_typed_memory)
{
    return __builtin_IB_system_memfence(fence_typed_memory);
}



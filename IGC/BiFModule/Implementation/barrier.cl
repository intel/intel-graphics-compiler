/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/


extern __constant int __OptDisable;

// EmitVISAPass support for __builtin_IB_memfence/__builtin_IB_typedmemfence requires some arguments to
// be constants as those are used to prepare a message descriptor, so must be known at compile time.
// To assure that all the arguments are constants for O0 path, there is a special function
// marked with __attribute__((optnone)) which implements seperate call instruction created for each
// arguments configuration.

// MEMFENCE IMPLEMENTATION

void __attribute__((optnone)) __intel_memfence_optnone(bool flushRW, bool isGlobal, bool invalidateL1)
{
#define MEMFENCE_IF(V1, V5, V6)                                    \
if (flushRW == V1 && isGlobal == V5 && invalidateL1 == V6)         \
{                                                                  \
    __builtin_IB_memfence(true, V1, false, false, false, V5, V6);  \
}

    MEMFENCE_IF(false, false, false)
    else MEMFENCE_IF(false, false, true)
    else MEMFENCE_IF(false, true, false)
    else MEMFENCE_IF(false, true, true)
    else MEMFENCE_IF(true, false, false)
    else MEMFENCE_IF(true, false, true)
    else MEMFENCE_IF(true, true, false)
    else MEMFENCE_IF(true, true, true)

#undef MEMFENCE_IF
}
void __intel_memfence(bool flushRW, bool isGlobal, bool invalidateL1)
{
    __builtin_IB_memfence(true, flushRW, false, false, false, isGlobal, invalidateL1);
}

void __intel_memfence_handler(bool flushRW, bool isGlobal, bool invalidateL1)
{
    if (__OptDisable)
        __intel_memfence_optnone(flushRW, isGlobal, invalidateL1);
    else
        __intel_memfence(flushRW, isGlobal, invalidateL1);
}

// TYPEDMEMFENCE IMPLEMENTATION

void __attribute__((optnone)) __intel_typedmemfence_optnone(bool invalidateL1)
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

    // We always need to 'fence' image memory (aka, flush caches, drain pipelines)
    fence |= ( Semantics & ImageMemory );

    if( fence )
    {
        if( Semantics & ImageMemory )
        {
            // An image fence requires a fence with R/W invalidate (L3 flush) + a flush
            // of the sampler cache,
            // ImageMemory | WorkgroupMemory should count as global!
            __intel_typedmemfence_handler(invalidateL1);
        }
        else if( Semantics & ( CrossWorkgroupMemory | WorkgroupMemory ) )
        {
            // A global/local opencl fence requires a hardware fence
            // We let the code generation decide whether we can elide local fences or not,
            // so we need to pass the CLK_GLOBAL flag forward
            bool flushL3 = Memory == Device || Memory == CrossDevice;
            __intel_memfence_handler(flushL3, Semantics & CrossWorkgroupMemory, invalidateL1);
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
        // nothing will be emited but we need to prevent optimization spliting control flow
        __builtin_IB_sub_group_barrier();
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
    return __builtin_spirv_OpGroupBroadcast_i32_i32_v3i32(Execution, Value, LocalId);
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



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
            __builtin_IB_typedmemfence(invalidateL1);
        }
        else if( Semantics & ( CrossWorkgroupMemory | WorkgroupMemory ) )
        {
            // A global/local opencl fence requires a hardware fence
            // We let the code generation decide whether we can elide local fences or not,
            // so we need to pass the CLK_GLOBAL flag forward
            bool flushL3 = Memory == Device || Memory == CrossDevice;
            __builtin_IB_memfence(true, flushL3, false, false, false, Semantics & CrossWorkgroupMemory, invalidateL1);
        }
    }
}

void __builtin_spirv_OpControlBarrier_i32_i32_i32(Scope_t Execution, Scope_t Memory, uint Semantics)
{
    __intel_atomic_work_item_fence( Memory, Semantics );

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

void __builtin_spirv_OpMemoryBarrier_i32_i32(Scope_t Memory, uint Semantics)
{
    __intel_atomic_work_item_fence( Memory, Semantics );
}


// Named Barrier

void __intel_getInitializedNamedBarrierArray(local uint* id)
{
    *id = 0;
    __builtin_spirv_OpControlBarrier_i32_i32_i32( Workgroup, 0, SequentiallyConsistent | WorkgroupMemory );
}

bool __intel_is_first_work_group_item( void );

local __namedBarrier* __builtin_spirv_OpNamedBarrierInitialize_i32_p3__namedBarrier_p3i32(int SubGroupCount, local __namedBarrier* nb_array, local uint* id)
{
    local __namedBarrier* NB = &nb_array[*id];
    NB->count = SubGroupCount;
    NB->orig_count = SubGroupCount;
    NB->inc = 0;
    __builtin_spirv_OpControlBarrier_i32_i32_i32( Workgroup, 0, SequentiallyConsistent | WorkgroupMemory );
    if (__intel_is_first_work_group_item())
    {
        (*id)++;
    }
    __builtin_spirv_OpControlBarrier_i32_i32_i32( Workgroup, 0, SequentiallyConsistent | WorkgroupMemory );
    return NB;
}


static INLINE OVERLOADABLE
uint AtomicCompareExchange(local uint *Pointer, uint Scope, uint Equal, uint Unequal, uint Value, uint Comparator)
{
    return __builtin_spirv_OpAtomicCompareExchange_p3i32_i32_i32_i32_i32_i32((volatile local uint*)Pointer, Scope, Equal, Unequal, Value, Comparator);
}

static INLINE
uint SubgroupLocalId()
{
    return __builtin_spirv_BuiltInSubgroupLocalInvocationId();
}

static INLINE OVERLOADABLE
uint AtomicLoad(local uint *Pointer, uint Scope, uint Semantics)
{
    return __builtin_spirv_OpAtomicLoad_p3i32_i32_i32((volatile local uint*)Pointer, Scope, Semantics);
}

static INLINE OVERLOADABLE
void AtomicStore(local uint *Pointer, uint Scope, uint Semantics, uint Value)
{
    __builtin_spirv_OpAtomicStore_p3i32_i32_i32_i32((volatile local uint*)Pointer, Scope, Semantics, Value);
}

static INLINE OVERLOADABLE
uint AtomicInc(local uint *Pointer, uint Scope, uint Semantics)
{
    return __builtin_spirv_OpAtomicIIncrement_p3i32_i32_i32((volatile local uint*)Pointer, Scope, Semantics);
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
    __builtin_spirv_OpMemoryBarrier_i32_i32(Memory, Semantics);
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



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

//*****************************************************************************/
// Generic Header
//*****************************************************************************/
#include "IBiF_Header.cl"

//*****************************************************************************/
// Explicit conversion functions
//*****************************************************************************/
#include "IBiF_Conversions.cl"

//*****************************************************************************/
// Misc. vector functions
//*****************************************************************************/
#include "IBiF_Shuffle.cl"

//*****************************************************************************/
// Device enqueue functions
//*****************************************************************************/
#include "IBiF_Device_Enqueue.cl"

//*****************************************************************************/
// Image Atomics (Intel Vendor Extension)
//*****************************************************************************/

//*****************************************************************************/
// Images
//*****************************************************************************/
#include "IBiF_Images.cl"

//*****************************************************************************/
// Math builtin functions
//*****************************************************************************/
#include "IBiF_Math_Common.cl"

//*****************************************************************************/
// 64bit Math Emulation
//*****************************************************************************/
#ifdef __IGC_BUILD__
//#include "IGCBiF_Math_64bitDiv.cl"
#endif



//*****************************************************************************/
// Sub Group functions
//*****************************************************************************/
#include "IBiF_Sub_Groups.cl"

//*****************************************************************************/
// Extensions (VME, VA)
//*****************************************************************************/
#include "IBiF_VME_VA.cl"

//*****************************************************************************/
// Sub Group and Work Group Reduce and Scan functions
//*****************************************************************************/
#include "IBiF_Reduce_Scan.cl"

//*****************************************************************************/
// Atomic Functions (1.2)
//*****************************************************************************/
#include "IBiF_Atomics.cl"


//*****************************************************************************/
// Work-Item function
//*****************************************************************************/

// All other work-item functions have size_t in their signature so they have
// to be compiled in the size_t .bc files.  This is here so its code is not
// in both the 32-bit and 64-bit .bcs.
INLINE uint OVERLOADABLE get_work_dim() {
  return __builtin_IB_get_work_dim();
}

//*****************************************************************************/
// generic address space function overloads
//*****************************************************************************/

#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

INLINE local void* __to_local(generic void* ptr)
{
    return __builtin_spirv_OpGenericCastToPtrExplicit_p3i8_p4i8_i32(ptr,StorageWorkgroup);
}

INLINE private void* __to_private(generic void* ptr)
{
    return __builtin_spirv_OpGenericCastToPtrExplicit_p0i8_p4i8_i32(ptr,StorageFunction);
}

INLINE global void* __to_global(generic void* ptr)
{
    return __builtin_spirv_OpGenericCastToPtrExplicit_p1i8_p4i8_i32(ptr,StorageCrossWorkgroup);
}

INLINE cl_mem_fence_flags OVERLOADABLE get_fence(generic void* ptr)
{

    if(__builtin_spirv_OpGenericCastToPtrExplicit_p3i8_p4i8_i32(ptr,StorageWorkgroup) != NULL)
    {
        return CLK_LOCAL_MEM_FENCE;
    }
    return CLK_GLOBAL_MEM_FENCE;
}

INLINE cl_mem_fence_flags OVERLOADABLE get_fence(generic const void* ptr)
{
    return get_fence((generic void*)ptr);
}

#endif // __OPENCL_C_VERSION__ >= CL_VERSION_2_0

//*****************************************************************************/
// Functions to convert enum of memory_fence and cl_mem_fence_flags to
// to corresponding SPIRV equivalents
//*****************************************************************************/

#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
static Scope_t get_spirv_mem_scope( memory_scope scope )
{
   switch( scope )
   {
    case memory_scope_work_item:
        return Invocation;
    case memory_scope_sub_group:
        return Subgroup;
    case memory_scope_work_group:
        return Workgroup;
    case memory_scope_device:
        return Device;
    case memory_scope_all_svm_devices:
        return CrossDevice;
    default:
        return CrossDevice;
    }
}
#endif // __OPENCL_C_VERSION__ >= CL_VERSION_2_0

static uint get_spirv_mem_fence( cl_mem_fence_flags flag )
{
    uint result = 0;

    if( flag & CLK_GLOBAL_MEM_FENCE )
    {
        result |= CrossWorkgroupMemory;
    }

    if( flag & CLK_LOCAL_MEM_FENCE )
    {
        result |= WorkgroupMemory;
    }

#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
    if( flag & CLK_IMAGE_MEM_FENCE )
    {
        result |= ImageMemory;
    }
#endif // __OPENCL_C_VERSION__ >= CL_VERSION_2_0

    return result;
}

#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
static uint get_spirv_mem_order( memory_order order )
{
    switch( order )
    {
        case memory_order_relaxed:
            return Relaxed;
        case memory_order_acquire:
            return Acquire;
        case memory_order_release:
            return Release;
        case memory_order_acq_rel:
            return AcquireRelease;
        case memory_order_seq_cst:
            return SequentiallyConsistent;
        default:
            return 0;
    }
}
#endif // __OPENCL_C_VERSION__ >= CL_VERSION_2_0

//*****************************************************************************/
// Synchronization functions
//*****************************************************************************/

INLINE void OVERLOADABLE work_group_barrier(cl_mem_fence_flags flags)
{
    __builtin_spirv_OpControlBarrier_i32_i32_i32(Workgroup, Device, AcquireRelease | get_spirv_mem_fence(flags));
}

INLINE void OVERLOADABLE barrier(cl_mem_fence_flags flags)
{
    // 2.0 barrier is able to accept CLK_IMAGE_MEM_FENCE
    work_group_barrier( flags );
}

#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
INLINE void OVERLOADABLE work_group_barrier(cl_mem_fence_flags flags, memory_scope scope)
{
    __builtin_spirv_OpControlBarrier_i32_i32_i32(get_spirv_mem_scope(scope), Device, AcquireRelease | get_spirv_mem_fence(flags));
}

INLINE void OVERLOADABLE read_mem_fence(cl_mem_fence_flags flags)
{
  atomic_work_item_fence(flags & ( CLK_LOCAL_MEM_FENCE | CLK_GLOBAL_MEM_FENCE ), memory_order_acq_rel, memory_scope_work_group);
}

INLINE void OVERLOADABLE write_mem_fence(cl_mem_fence_flags flags)
{
  atomic_work_item_fence(flags & ( CLK_LOCAL_MEM_FENCE | CLK_GLOBAL_MEM_FENCE ), memory_order_acq_rel, memory_scope_work_group);
}

INLINE void OVERLOADABLE mem_fence(cl_mem_fence_flags flags)
{
  // Notice that per 2.0 CLK_IMAGE_MEM_FENCE is not supported as a flag argument for a mem_fence
  atomic_work_item_fence(flags & ( CLK_LOCAL_MEM_FENCE | CLK_GLOBAL_MEM_FENCE ), memory_order_acq_rel, memory_scope_work_group);
}

// atomic_work_item_fence()
// Caller to this function should understand that for an acq or rel memory
// models you would have to perform additional checks inside a calle,
// ie. look at the INLINE TYPE OVERLOADABLE atomic_fetch_##KEY##_explicit(volatile ADDRSPACE atomic_##TYPE *object, OPTYPE operand, memory_order order, memory_scope scope)
// implementation.
// We need only single fence around an operation for acq/rel as opposed to 2
// for acq+rel/seq_cst. This function does not distinguish between acq/rel.

INLINE void OVERLOADABLE atomic_work_item_fence( cl_mem_fence_flags flags, memory_order order, memory_scope scope )
{
    __builtin_spirv_OpMemoryBarrier_i32_i32( get_spirv_mem_scope( scope ), get_spirv_mem_order( order ) | get_spirv_mem_fence( flags ) );
}

#endif // __OPENCL_C_VERSION__ >= CL_VERSION_2_0

//*****************************************************************************/
// wait_group_events()
//*****************************************************************************/
INLINE void OVERLOADABLE wait_group_events( int num_events, __private event_t *event_list )
{
    barrier( CLK_GLOBAL_MEM_FENCE );
    barrier( CLK_LOCAL_MEM_FENCE );

}

// here for backwards compatibility with clang/llvm 3.0
// wait_group_events().
INLINE void OVERLOADABLE wait_group_events( int num_events, __private uint *event_list )
{
    barrier( CLK_GLOBAL_MEM_FENCE );
    barrier( CLK_LOCAL_MEM_FENCE );
}

///////////////////////////////////////////////////////////////////////////////

//*****************************************************************************/
// c1x Atomic Functions
//*****************************************************************************/

// TODO:
// - implement old atomics in terms of new atomics.
// - disallow 64-bit functions (move size_t funcs to size_t.cl).
// - what to do about atomic_compare_exchange_strong/weak?
// - instruction select everything.

// atomic_init()

#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
#define atomic_init_function_addrspace(TYPE, ADDRSPACE) \
INLINE void OVERLOADABLE atomic_init(volatile ADDRSPACE atomic_##TYPE *object, TYPE value) \
{ \
  volatile ADDRSPACE TYPE *nonatomic = (volatile ADDRSPACE TYPE*)object; \
  *nonatomic = value; \
}

#define atomic_init_function(TYPE) \
atomic_init_function_addrspace(TYPE, generic)

atomic_init_function(int)
atomic_init_function(uint)
atomic_init_function(float)

// atomic_fetch()

#define atomic_fetch_function_addrspace(KEY, TYPE, OPTYPE, FUNC, ADDRSPACE, ABBR_ADDRSPACE, ABBR_TYPE, PTR_TYPE) \
INLINE TYPE OVERLOADABLE atomic_fetch_##KEY(volatile ADDRSPACE atomic_##TYPE *object, OPTYPE operand) \
{ \
  return atomic_fetch_##KEY##_explicit(object, operand, memory_order_seq_cst); \
} \
INLINE TYPE OVERLOADABLE atomic_fetch_##KEY##_explicit(volatile ADDRSPACE atomic_##TYPE *object, OPTYPE operand, memory_order order) \
{ \
  return atomic_fetch_##KEY##_explicit(object, operand, order, memory_scope_device); \
} \
INLINE TYPE OVERLOADABLE atomic_fetch_##KEY##_explicit(volatile ADDRSPACE atomic_##TYPE *object, OPTYPE operand, memory_order order, memory_scope scope) \
{ \
  return __builtin_spirv_##FUNC##_##ABBR_ADDRSPACE##ABBR_TYPE##_i32_i32_##ABBR_TYPE((volatile ADDRSPACE PTR_TYPE*)object, get_spirv_mem_scope(scope), get_spirv_mem_order(order) | get_spirv_mem_fence(get_fence((const ADDRSPACE void*)object)), operand);\
}

#define atomic_fetch_function(KEY, TYPE, OPTYPE, FUNC) \
atomic_fetch_function_addrspace(KEY, TYPE, OPTYPE, FUNC, generic, p4, i32, uint)

#define atomic_fetch_supported_types(KEY, FUNC) \
atomic_fetch_function(KEY, int, int, FUNC) \
atomic_fetch_function(KEY, uint, uint, FUNC) \
atomic_fetch_function(KEY, uint, int, FUNC) // the (size_t, ptrdiff_t) variety for 32-bit

atomic_fetch_supported_types(add, OpAtomicIAdd)
atomic_fetch_supported_types(sub, OpAtomicISub)
atomic_fetch_supported_types(or, OpAtomicOr)
atomic_fetch_supported_types(xor, OpAtomicXor)
atomic_fetch_supported_types(and, OpAtomicAnd)
atomic_fetch_function(max, int, int, OpAtomicSMax)
atomic_fetch_function(max, uint, uint, OpAtomicUMax)
atomic_fetch_function(min, int, int, OpAtomicSMin)
atomic_fetch_function(min, uint, uint, OpAtomicUMin)


#endif // __OPENCL_C_VERSION__ >= CL_VERSION_2_0

// atomic_store()

// TODO: should we use  __c11_atomic_store(object, operand, order) ?

// Also need to force fences on raw stores and loads regardless of memory scope or order.
// Atomically! replace the value pointed to by object with desired.
// Since there is no difference between an ordinary store an an atomic one on
// Gen HW we use the first one.

#define atomic_store_function_addrspace_int(TYPE, ABBR_TYPE, ADDRSPACE, ABBR_ADDRSPACE) \
INLINE void OVERLOADABLE atomic_store(volatile ADDRSPACE atomic_##TYPE *object, TYPE operand) \
{ \
  atomic_store_explicit(object, operand, memory_order_seq_cst); \
} \
INLINE void OVERLOADABLE atomic_store_explicit(volatile ADDRSPACE atomic_##TYPE *object, TYPE operand, memory_order order) \
{ \
  atomic_store_explicit(object, operand, order, memory_scope_device); \
} \
INLINE void OVERLOADABLE atomic_store_explicit(volatile ADDRSPACE atomic_##TYPE *object, TYPE operand, memory_order order, memory_scope scope) \
{ \
  __builtin_spirv_OpAtomicStore_##ABBR_ADDRSPACE##ABBR_TYPE##_i32_i32_##ABBR_TYPE((volatile ADDRSPACE uint*)object, get_spirv_mem_scope(scope), get_spirv_mem_order(order) | get_spirv_mem_fence(get_fence((const ADDRSPACE void*)object)) , operand);\
}

#define atomic_store_function_addrspace(TYPE, ABBR_TYPE, ADDRSPACE, ABBR_ADDRSPACE) \
INLINE void OVERLOADABLE atomic_store(volatile ADDRSPACE atomic_##TYPE *object, TYPE operand) \
{ \
  atomic_store_explicit(object, operand, memory_order_seq_cst); \
} \
INLINE void OVERLOADABLE atomic_store_explicit(volatile ADDRSPACE atomic_##TYPE *object, TYPE operand, memory_order order) \
{ \
  atomic_store_explicit(object, operand, order, memory_scope_device); \
} \
INLINE void OVERLOADABLE atomic_store_explicit(volatile ADDRSPACE atomic_##TYPE *object, TYPE operand, memory_order order, memory_scope scope) \
{ \
  __builtin_spirv_OpAtomicStore_##ABBR_ADDRSPACE##ABBR_TYPE##_i32_i32_##ABBR_TYPE((volatile ADDRSPACE TYPE*)object, get_spirv_mem_scope(scope), get_spirv_mem_order(order) | get_spirv_mem_fence(get_fence((const ADDRSPACE void*)object)), operand);\
}

#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
#define atomic_store_function(TYPE, ABBR_TYPE) \
atomic_store_function_addrspace(TYPE, ABBR_TYPE, generic, p4)

atomic_store_function_addrspace_int(int,i32, generic, p4)
atomic_store_function(uint,i32)
// TODO: are any float types supported?
atomic_store_function(float,f32)

// atomic_load()

// Also need to force fences on raw stores and loads regardless of memory scope or order.

#define atomic_load_function_addrspace_int(TYPE, ABBR_TYPE, ADDRSPACE, ABBR_ADDRSPACE) \
INLINE TYPE OVERLOADABLE atomic_load(volatile ADDRSPACE atomic_##TYPE *object) \
{ \
  return atomic_load_explicit(object, memory_order_seq_cst, memory_scope_device); \
} \
INLINE TYPE OVERLOADABLE atomic_load_explicit(volatile ADDRSPACE atomic_##TYPE *object, memory_order order) \
{ \
  return atomic_load_explicit(object, order, memory_scope_device); \
} \
INLINE TYPE OVERLOADABLE atomic_load_explicit(volatile ADDRSPACE atomic_##TYPE *object, memory_order order, memory_scope scope) \
{ \
  return __builtin_spirv_OpAtomicLoad_##ABBR_ADDRSPACE##ABBR_TYPE##_i32_i32((volatile ADDRSPACE uint*)object, get_spirv_mem_scope(scope), get_spirv_mem_order(order) | get_spirv_mem_fence(get_fence((const ADDRSPACE void*)object)));\
}

#define atomic_load_function_addrspace(TYPE, ABBR_TYPE, ADDRSPACE, ABBR_ADDRSPACE) \
INLINE TYPE OVERLOADABLE atomic_load(volatile ADDRSPACE atomic_##TYPE *object) \
{ \
  return atomic_load_explicit(object, memory_order_seq_cst, memory_scope_device); \
} \
INLINE TYPE OVERLOADABLE atomic_load_explicit(volatile ADDRSPACE atomic_##TYPE *object, memory_order order) \
{ \
  return atomic_load_explicit(object, order, memory_scope_device); \
} \
INLINE TYPE OVERLOADABLE atomic_load_explicit(volatile ADDRSPACE atomic_##TYPE *object, memory_order order, memory_scope scope) \
{ \
  return __builtin_spirv_OpAtomicLoad_##ABBR_ADDRSPACE##ABBR_TYPE##_i32_i32((volatile ADDRSPACE TYPE*)object, get_spirv_mem_scope(scope), get_spirv_mem_order(order) | get_spirv_mem_fence(get_fence((const ADDRSPACE void*)object)));\
}

#define atomic_load_function(TYPE, ABBR_TYPE) \
atomic_load_function_addrspace(TYPE, ABBR_TYPE, generic, p4)

atomic_load_function_addrspace_int(int, i32, generic, p4)
atomic_load_function(uint,i32)
atomic_load_function(float,f32)

// atomic_exchange()

#define atomic_exchange_function_addrspace_int(TYPE, ABBR_TYPE, ADDRSPACE, ABBR_ADDRSPACE) \
INLINE TYPE OVERLOADABLE atomic_exchange(volatile ADDRSPACE atomic_##TYPE *object, TYPE desired) \
{ \
  return atomic_exchange_explicit(object, desired, memory_order_seq_cst, memory_scope_device); \
} \
INLINE TYPE OVERLOADABLE atomic_exchange_explicit(volatile ADDRSPACE atomic_##TYPE *object, TYPE desired, memory_order order) \
{ \
  return atomic_exchange_explicit(object, desired, order, memory_scope_device); \
} \
INLINE TYPE OVERLOADABLE atomic_exchange_explicit(volatile ADDRSPACE atomic_##TYPE *object, TYPE desired, memory_order order, memory_scope scope) \
{ \
  return __builtin_spirv_OpAtomicExchange_##ABBR_ADDRSPACE##ABBR_TYPE##_i32_i32_##ABBR_TYPE((volatile ADDRSPACE uint*)object, get_spirv_mem_scope(scope), get_spirv_mem_order(order) | get_spirv_mem_fence(get_fence((const ADDRSPACE void*)object)), desired);\
}

#define atomic_exchange_function_addrspace(TYPE, ABBR_TYPE, ADDRSPACE, ABBR_ADDRSPACE) \
INLINE TYPE OVERLOADABLE atomic_exchange(volatile ADDRSPACE atomic_##TYPE *object, TYPE desired) \
{ \
  return atomic_exchange_explicit(object, desired, memory_order_seq_cst, memory_scope_device); \
} \
INLINE TYPE OVERLOADABLE atomic_exchange_explicit(volatile ADDRSPACE atomic_##TYPE *object, TYPE desired, memory_order order) \
{ \
  return atomic_exchange_explicit(object, desired, order, memory_scope_device); \
} \
INLINE TYPE OVERLOADABLE atomic_exchange_explicit(volatile ADDRSPACE atomic_##TYPE *object, TYPE desired, memory_order order, memory_scope scope) \
{ \
  return __builtin_spirv_OpAtomicExchange_##ABBR_ADDRSPACE##ABBR_TYPE##_i32_i32_##ABBR_TYPE((volatile ADDRSPACE TYPE*)object, get_spirv_mem_scope(scope), get_spirv_mem_order(order) | get_spirv_mem_fence(get_fence((const ADDRSPACE void*)object)), desired);\
}

#define atomic_exchange_function(TYPE, ABBR_TYPE) \
atomic_exchange_function_addrspace(TYPE, ABBR_TYPE, generic, p4)

atomic_exchange_function_addrspace_int(int,i32, generic, p4)
atomic_exchange_function(uint,i32)
atomic_exchange_function(float,f32)

// atomic_compare_exchange_strong() and atomic_compare_exchange_weak()

#define atomic_compare_exchange_strength_function_addrspace(TYPE, ADDRSPACE, ADDRSPACE2, STRENGTH, FUNC, ABBR_ADDRSPACE) \
INLINE bool OVERLOADABLE atomic_compare_exchange_##STRENGTH(volatile ADDRSPACE atomic_##TYPE *object, ADDRSPACE2 TYPE *expected, TYPE desired) \
{ \
  return atomic_compare_exchange_##STRENGTH##_explicit(object, expected, desired, memory_order_seq_cst, memory_order_seq_cst, memory_scope_device); \
} \
INLINE bool OVERLOADABLE atomic_compare_exchange_##STRENGTH##_explicit(volatile ADDRSPACE atomic_##TYPE *object, ADDRSPACE2 TYPE *expected, \
                                                                       TYPE desired, memory_order success, memory_order failure) \
{ \
  return atomic_compare_exchange_##STRENGTH##_explicit(object, expected, desired, success, failure, memory_scope_device); \
} \
INLINE bool OVERLOADABLE atomic_compare_exchange_##STRENGTH##_explicit(volatile ADDRSPACE atomic_##TYPE *object, ADDRSPACE2 TYPE *expected, \
                                                                       TYPE desired, memory_order success, memory_order failure, memory_scope scope) \
{ \
  TYPE expected_start = (*expected);\
  uint before = __builtin_spirv_##FUNC##_##ABBR_ADDRSPACE##i32_i32_i32_i32_i32_i32((volatile ADDRSPACE uint*)object, get_spirv_mem_scope(scope), success, failure, as_uint(desired),as_uint(expected_start)); \
  bool ret = false; \
  if ( before == as_uint(expected_start)) { \
    ret = true; \
  } \
  else { \
    *expected = as_##TYPE(before); \
    ret = false; \
  } \
  if( success != memory_order_release ) \
  { \
    atomic_work_item_fence( CLK_LOCAL_MEM_FENCE | CLK_GLOBAL_MEM_FENCE, success, scope); \
  } \
  return ret; \
}
// __builtin_IB_atomic_cmpxchg_i32_##ADDRSPACE((ADDRSPACE TYPE*)object, *expected, desired); \
// bool val = __c11_atomic_compare_exchange_##STRENGTH(object, expected, desired, success, failure); \
// TODO: success vs. failure: can we always just use success?  More control flow would
// be necessary to check result.  Also current cmpxchg instruction isn't the same as
// what this function does.  Also, is currently implementation sufficient?

#define atomic_compare_exchange_strength_function(TYPE, STRENGTH, FUNC) \
atomic_compare_exchange_strength_function_addrspace(TYPE, generic, generic, STRENGTH, FUNC, p4)

atomic_compare_exchange_strength_function(int, strong, OpAtomicCompareExchange)
atomic_compare_exchange_strength_function(uint, strong, OpAtomicCompareExchange)
atomic_compare_exchange_strength_function(int, weak, OpAtomicCompareExchangeWeak)
atomic_compare_exchange_strength_function(uint, weak, OpAtomicCompareExchangeWeak)
atomic_compare_exchange_strength_function(float, strong, OpAtomicCompareExchange)
atomic_compare_exchange_strength_function(float, weak, OpAtomicCompareExchangeWeak)

// atomic_flag_test_and_set()

#define ATOMIC_FLAG_TRUE 1
#define ATOMIC_FLAG_FALSE 0

#define atomic_flag_test_and_set_function_addrspace(ADDRSPACE, ABBR_ADDRSPACE) \
bool __attribute__((overloadable)) atomic_flag_test_and_set(volatile ADDRSPACE atomic_flag *object) \
{ \
  return atomic_flag_test_and_set_explicit(object, memory_order_seq_cst); \
} \
bool __attribute__((overloadable)) atomic_flag_test_and_set_explicit(volatile ADDRSPACE atomic_flag *object, memory_order order) \
{ \
  return atomic_flag_test_and_set_explicit(object, order, memory_scope_device); \
} \
bool __attribute__((overloadable)) atomic_flag_test_and_set_explicit(volatile ADDRSPACE atomic_flag *object, memory_order order, memory_scope scope) \
{ \
  return __builtin_spirv_OpAtomicFlagTestAndSet_##ABBR_ADDRSPACE##i32_i32_i32((volatile ADDRSPACE uint*)object, get_spirv_mem_scope(scope), get_spirv_mem_order(order) | get_spirv_mem_fence(get_fence((const ADDRSPACE void*)object))); \
}

#define atomic_flag_test_and_set_function() \
atomic_flag_test_and_set_function_addrspace(generic, p4)

atomic_flag_test_and_set_function()

// atomic_flag_clear()

#define atomic_flag_clear_function_addrspace(ADDRSPACE, ABBR_ADDRSPACE) \
void __attribute__((overloadable)) atomic_flag_clear(volatile ADDRSPACE atomic_flag *object) \
{ \
  atomic_flag_clear_explicit(object, memory_order_seq_cst); \
} \
void __attribute__((overloadable)) atomic_flag_clear_explicit(volatile ADDRSPACE atomic_flag *object, memory_order order) \
{ \
  atomic_flag_clear_explicit(object, order, memory_scope_device); \
} \
void __attribute__((overloadable)) atomic_flag_clear_explicit(volatile ADDRSPACE atomic_flag *object, memory_order order, memory_scope scope) \
{ \
  __builtin_spirv_OpAtomicFlagClear_##ABBR_ADDRSPACE##i32_i32_i32((volatile ADDRSPACE uint*)object, get_spirv_mem_scope(scope), get_spirv_mem_order(order) | get_spirv_mem_fence(get_fence((const ADDRSPACE void*)object))); \
}

#define atomic_flag_clear_function() \
atomic_flag_clear_function_addrspace(generic, p4)

atomic_flag_clear_function()

#undef ATOMIC_FLAG_TRUE
#undef ATOMIC_FLAG_FALSE


#endif // __OPENCL_C_VERSION__ >= CL_VERSION_2_0

INLINE int OVERLOADABLE work_group_any(int predicate)
{
    GET_MEMPOOL_PTR(tmp, int, false, 1)
    *tmp = 0;
        barrier(CLK_LOCAL_MEM_FENCE); // Wait for tmp to be initialized
        atomic_or(tmp, predicate != 0); // Set to true if predicate is non-zero
        barrier(CLK_LOCAL_MEM_FENCE);
        return *tmp; // Return true if any of them passed the test
}

INLINE int OVERLOADABLE work_group_all(int predicate)
{
    GET_MEMPOOL_PTR(tmp, int, false, 1)
    *tmp = 0;
        barrier(CLK_LOCAL_MEM_FENCE); // Wait for tmp to be initialized
        atomic_or(tmp, predicate == 0); // Set to true if predicate is zero
        barrier(CLK_LOCAL_MEM_FENCE); // Wait for threads
        return (*tmp == 0); // Return true if none of them failed the test
}

//*****************************************************************************/
// Pipe functions
//*****************************************************************************/

#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

/////////////////////////////////////////////////////////////////////
// Basic Reads and Writes

int __read_pipe_2( read_only pipe int pipe_, __generic void* data, uint bytes, uint alignment )
{
    return __builtin_spirv_OpReadPipe_i64_p4i8_i32(pipe_, data, bytes);
}

int __write_pipe_2( write_only pipe int pipe_, __generic const void* data, uint bytes, uint alignment)
{
    return __builtin_spirv_OpWritePipe_i64_p4i8_i32(pipe_, data, bytes);
}

/////////////////////////////////////////////////////////////////////
// Reservation Query

bool OVERLOADABLE is_valid_reserve_id(
  reserve_id_t reserve_id )
{
    return __builtin_spirv_OpIsValidReserveId_i64(reserve_id);
}

/////////////////////////////////////////////////////////////////////
// Work Item Reservations

// NOTE: The pipe's packet type doesn't affect mangling.
reserve_id_t __reserve_read_pipe( read_only pipe int pipe_, uint num_packets, uint bytes, uint alignment )
{
    return __builtin_spirv_OpReserveReadPipePackets_i64_i32_i32(pipe_, num_packets, bytes);
}

reserve_id_t __reserve_write_pipe( write_only pipe int pipe_, uint num_packets, uint bytes, uint alignment )
{
    return __builtin_spirv_OpReserveWritePipePackets_i64_i32_i32(pipe_, num_packets, bytes);
}

void __commit_read_pipe( read_only pipe int pipe_, reserve_id_t reserve_id, uint bytes, uint alignment )
{
    __builtin_spirv_OpCommitReadPipe_i64_i64_i32(pipe_, reserve_id, bytes);
}

void __commit_write_pipe( write_only pipe int pipe_, reserve_id_t reserve_id, uint bytes, uint alignment)
{
    __builtin_spirv_OpCommitWritePipe_i64_i64_i32(pipe_, reserve_id, bytes);
}

/////////////////////////////////////////////////////////////////////
// Reads and Writes with Reservations
// The reservation functions lock the _pipe, so we don't need to
// re-lock here.

int __read_pipe_4( read_only pipe int pipe_, reserve_id_t reserve_id, uint index, __generic void* data, uint bytes, uint alignment)
{
    return __builtin_spirv_OpReservedReadPipe_i64_i64_i32_p4i8_i32(pipe_, reserve_id, index, data, bytes);
}

// write_pipe with 4 explicit arguments
int __write_pipe_4( write_only pipe int pipe_, reserve_id_t reserve_id, uint index, __generic const void* data, uint bytes, uint alignment)
{
    return __builtin_spirv_OpReservedWritePipe_i64_i64_i32_p4i8_i32(pipe_, reserve_id, index, data, bytes);
}

/////////////////////////////////////////////////////////////////////
// Work Group Reservations
//
bool __intel_is_first_work_group_item( void );

reserve_id_t __work_group_reserve_read_pipe( read_only pipe int p, uint num_packets, uint bytes, uint alignment )
{
    return __builtin_spirv_OpGroupReserveReadPipePackets_i32_i64_i32_i32(Workgroup, p, num_packets, bytes);
}

reserve_id_t __work_group_reserve_write_pipe( write_only pipe int p, uint num_packets, uint bytes, uint alignment )
{
    return __builtin_spirv_OpGroupReserveWritePipePackets_i32_i64_i32_i32(Workgroup, p, num_packets, bytes);
}

void __work_group_commit_read_pipe( read_only pipe int p, reserve_id_t reserve_id, uint bytes, uint alignment )
{
    __builtin_spirv_OpGroupCommitReadPipe_i32_i64_i64_i32(Workgroup, p, reserve_id, bytes);
}

void __work_group_commit_write_pipe( write_only pipe int p, reserve_id_t reserve_id, uint bytes, uint alignment )
{
    __builtin_spirv_OpGroupCommitWritePipe_i32_i64_i64_i32(Workgroup, p, reserve_id, bytes);
}

/////////////////////////////////////////////////////////////////////
// Sub-Group Reservations
// Note: Not supporting this for intel sub groups (yet?).
reserve_id_t __sub_group_reserve_read_pipe( read_only pipe int p, uint num_packets, uint bytes, uint alignment )
{
    return __builtin_spirv_OpGroupReserveReadPipePackets_i32_i64_i32_i32(Subgroup, p, num_packets, bytes);
}

reserve_id_t __sub_group_reserve_write_pipe( write_only pipe int p, uint num_packets, uint bytes, uint alignment )
{
    return __builtin_spirv_OpGroupReserveWritePipePackets_i32_i64_i32_i32(Subgroup, p, num_packets, bytes);
}

void __sub_group_commit_read_pipe( read_only pipe int p, reserve_id_t reserve_id, uint bytes, uint alignment )
{
    __builtin_spirv_OpGroupCommitReadPipe_i32_i64_i64_i32(Subgroup, p, reserve_id, bytes);
}

void __sub_group_commit_write_pipe( write_only pipe int p, reserve_id_t reserve_id, uint bytes, uint alignment )
{
    __builtin_spirv_OpGroupCommitWritePipe_i32_i64_i64_i32(Subgroup, p, reserve_id, bytes);
}

/////////////////////////////////////////////////////////////////////
// Pipe Queries
//
uint __get_pipe_num_packets(pipe int pipe_, uint bytes, uint alignment)
{
    return __builtin_spirv_OpGetNumPipePackets_i64_i32(pipe_, bytes);
}

uint __get_pipe_max_packets(pipe int pipe_, uint bytes, uint alignment)
{
    return __builtin_spirv_OpGetMaxPipePackets_i64_i32(pipe_, bytes);
}
#endif // __OPENCL_C_VERSION__ >= CL_VERSION_2_0

// Internal Debug Built-Ins (TODO: implement for IGC?)

__attribute__((always_inline))
ulong OVERLOADABLE intel_get_cycle_counter( void )
{
    return __builtin_spirv_OpReadClockKHR_i64(0);
}

__attribute__((always_inline))
uint OVERLOADABLE intel_get_active_channel_mask( void )
{
    return __builtin_IB_WaveBallot(true);
}

__attribute__((always_inline))
void OVERLOADABLE intel_source_value( uchar value )
{
    __builtin_IB_source_value( value );
}

__attribute__((always_inline))
void OVERLOADABLE intel_source_value( ushort value )
{
    __builtin_IB_source_value( value );
}

__attribute__((always_inline))
void OVERLOADABLE intel_source_value( uint value )
{
    __builtin_IB_source_value( value );
}

__attribute__((always_inline))
void OVERLOADABLE intel_source_value( ulong value )
{
    __builtin_IB_source_value( value );
}

// dbg0_0 will be written to dbg0.0
// the return result will be dbg0.1 (start IP of the thread)
__attribute__((always_inline))
uint OVERLOADABLE intel_set_dbg_register(uint dbg0_0)
{
    return __builtin_IB_set_dbg_register(dbg0_0);
}

__attribute__((always_inline))
uint OVERLOADABLE intel_get_grf_register( uint value )
{
    return __builtin_IB_movreg( value );
}

__attribute__((always_inline))
uint OVERLOADABLE intel_get_flag_register( uint flag )
{
    return __builtin_IB_movflag( flag );
}

__attribute__((always_inline))
uint OVERLOADABLE intel_get_control_register( uint value )
{
    return __builtin_IB_movcr( value );
}

__attribute__((always_inline))
uint OVERLOADABLE intel_get_hw_thread_id( void )
{
    return __builtin_IB_hw_thread_id();
}

__attribute__((always_inline))
uint OVERLOADABLE intel_get_slice_id( void )
{
    return __builtin_IB_slice_id();
}

__attribute__((always_inline))
uint OVERLOADABLE intel_get_subslice_id( void )
{
    return __builtin_IB_subslice_id();
}

__attribute__((always_inline))
uint OVERLOADABLE intel_get_eu_id( void )
{
    return __builtin_IB_eu_id();
}

__attribute__((always_inline))
void OVERLOADABLE intel_profile_snapshot( int point_type, int point_index )
{
    __builtin_IB_profile_snapshot( point_type, point_index );
}

__attribute__((always_inline))
void OVERLOADABLE intel_profile_aggregated( int point_type, int point_index )
{
    __builtin_IB_profile_aggregated( point_type, point_index );
}

__attribute__((always_inline))
uint OVERLOADABLE intel_get_eu_thread_id( void )
{
    return __builtin_IB_eu_thread_id();
}

__attribute__((always_inline))
void OVERLOADABLE intel_eu_thread_pause( uint value )
{
    __builtin_IB_eu_thread_pause(value);
}

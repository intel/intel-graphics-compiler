/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2023 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

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
// SPIRV Utils functions
//*****************************************************************************/
#include "IBiF_SPIRV_Utils.cl"

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
// Atomic Functions
//*****************************************************************************/
#include "IBiF_Atomics.cl"

//*****************************************************************************/
// Intel production raytracing extension
//*****************************************************************************/
#include "IBiF_intel_rt_production.cl"


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
    return SPIRV_BUILTIN(GenericCastToPtrExplicit, _p3i8_p4i8_i32, _ToLocal)(__builtin_astype((ptr), __generic char*), StorageWorkgroup);
}

INLINE private void* __to_private(generic void* ptr)
{
    return SPIRV_BUILTIN(GenericCastToPtrExplicit, _p0i8_p4i8_i32, _ToPrivate)(__builtin_astype((ptr), __generic char*), StorageFunction);
}

INLINE global void* __to_global(generic void* ptr)
{
    return SPIRV_BUILTIN(GenericCastToPtrExplicit, _p1i8_p4i8_i32, _ToGlobal)(__builtin_astype((ptr), __generic char*), StorageCrossWorkgroup);
}

INLINE cl_mem_fence_flags OVERLOADABLE get_fence(generic void* ptr)
{

    if(SPIRV_BUILTIN(GenericCastToPtrExplicit, _p3i8_p4i8_i32, _ToLocal)(__builtin_astype((ptr), __generic char*), StorageWorkgroup) != NULL)
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
// Synchronization functions
//*****************************************************************************/

INLINE void OVERLOADABLE work_group_barrier(cl_mem_fence_flags flags)
{
    SPIRV_BUILTIN(ControlBarrier, _i32_i32_i32, )(Workgroup, Device, AcquireRelease | get_spirv_mem_fence(flags));
}

INLINE void OVERLOADABLE barrier(cl_mem_fence_flags flags)
{
    // 2.0 barrier is able to accept CLK_IMAGE_MEM_FENCE
    work_group_barrier( flags );
}

#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
INLINE void OVERLOADABLE work_group_barrier(cl_mem_fence_flags flags, memory_scope scope)
{
    SPIRV_BUILTIN(ControlBarrier, _i32_i32_i32, )(get_spirv_mem_scope(scope), Device, AcquireRelease | get_spirv_mem_fence(flags));
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
    SPIRV_BUILTIN(MemoryBarrier, _i32_i32, )( get_spirv_mem_scope( scope ), get_spirv_mem_order( order ) | get_spirv_mem_fence( flags ) );
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

//*****************************************************************************/
// Workgroup functions
//*****************************************************************************/

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

#define     to_spirv_pipe_ro(p)     __builtin_astype(p, __spirv_Pipe_ro)
#define     to_spirv_pipe_wo(p)     __builtin_astype(p, __spirv_Pipe_wo)
#define     to_spirv_reserveid(p)   __builtin_astype(p, __spirv_ReserveId)
#define     to_ocl_reserveid(p)     __builtin_astype(p, reserve_id_t)

/////////////////////////////////////////////////////////////////////
// Basic Reads and Writes

int __read_pipe_2( read_only pipe int pipe_, __generic void* data, uint bytes, uint alignment )
{
    return SPIRV_BUILTIN(ReadPipe, _Pipe_ro_p4i8_i32, )(to_spirv_pipe_ro(pipe_), data, bytes);
}

int __write_pipe_2( write_only pipe int pipe_, __generic const void* data, uint bytes, uint alignment)
{
    return SPIRV_BUILTIN(WritePipe, _Pipe_wo_p4i8_i32, )(to_spirv_pipe_wo(pipe_), data, bytes);
}

/////////////////////////////////////////////////////////////////////
// Reservation Query

bool OVERLOADABLE is_valid_reserve_id(
  reserve_id_t reserve_id )
{
    return SPIRV_BUILTIN(IsValidReserveId, _ReserveId, )(to_spirv_reserveid(reserve_id));
}

/////////////////////////////////////////////////////////////////////
// Work Item Reservations

// NOTE: The pipe's packet type doesn't affect mangling.
reserve_id_t __reserve_read_pipe( read_only pipe int pipe_, uint num_packets, uint bytes, uint alignment )
{
    return to_ocl_reserveid(SPIRV_BUILTIN(ReserveReadPipePackets, _Pipe_ro_i32_i32, )(to_spirv_pipe_ro(pipe_), num_packets, bytes));
}

reserve_id_t __reserve_write_pipe( write_only pipe int pipe_, uint num_packets, uint bytes, uint alignment )
{
    return to_ocl_reserveid(SPIRV_BUILTIN(ReserveWritePipePackets, _Pipe_wo_i32_i32, )(to_spirv_pipe_wo(pipe_), num_packets, bytes));
}

void __commit_read_pipe( read_only pipe int pipe_, reserve_id_t reserve_id, uint bytes, uint alignment )
{
    SPIRV_BUILTIN(CommitReadPipe, _Pipe_ro_ReserveId_i32, )(to_spirv_pipe_ro(pipe_), to_spirv_reserveid(reserve_id), bytes);
}

void __commit_write_pipe( write_only pipe int pipe_, reserve_id_t reserve_id, uint bytes, uint alignment)
{
    SPIRV_BUILTIN(CommitWritePipe, _Pipe_wo_ReserveId_i32, )(to_spirv_pipe_wo(pipe_), to_spirv_reserveid(reserve_id), bytes);
}

/////////////////////////////////////////////////////////////////////
// Reads and Writes with Reservations
// The reservation functions lock the _pipe, so we don't need to
// re-lock here.

int __read_pipe_4( read_only pipe int pipe_, reserve_id_t reserve_id, uint index, __generic void* data, uint bytes, uint alignment)
{
    return SPIRV_BUILTIN(ReservedReadPipe, _Pipe_ro_ReserveId_i32_p4i8_i32, )(to_spirv_pipe_ro(pipe_), to_spirv_reserveid(reserve_id), index, data, bytes);
}

// write_pipe with 4 explicit arguments
int __write_pipe_4( write_only pipe int pipe_, reserve_id_t reserve_id, uint index, __generic const void* data, uint bytes, uint alignment)
{
    return SPIRV_BUILTIN(ReservedWritePipe, _Pipe_wo_ReserveId_i32_p4i8_i32, )(to_spirv_pipe_wo(pipe_), to_spirv_reserveid(reserve_id), index, data, bytes);
}

/////////////////////////////////////////////////////////////////////
// Work Group Reservations
//
bool __intel_is_first_work_group_item( void );

reserve_id_t __work_group_reserve_read_pipe( read_only pipe int p, uint num_packets, uint bytes, uint alignment )
{
    return to_ocl_reserveid(SPIRV_BUILTIN(GroupReserveReadPipePackets, _i32_Pipe_ro_i32_i32, )(Workgroup, to_spirv_pipe_ro(p), num_packets, bytes));
}

reserve_id_t __work_group_reserve_write_pipe( write_only pipe int p, uint num_packets, uint bytes, uint alignment )
{
    return to_ocl_reserveid(SPIRV_BUILTIN(GroupReserveWritePipePackets, _i32_Pipe_wo_i32_i32, )(Workgroup, to_spirv_pipe_wo(p), num_packets, bytes));
}

void __work_group_commit_read_pipe( read_only pipe int p, reserve_id_t reserve_id, uint bytes, uint alignment )
{
    SPIRV_BUILTIN(GroupCommitReadPipe, _i32_Pipe_ro_ReserveId_i32, )(Workgroup, to_spirv_pipe_ro(p), to_spirv_reserveid(reserve_id), bytes);
}

void __work_group_commit_write_pipe( write_only pipe int p, reserve_id_t reserve_id, uint bytes, uint alignment )
{
    SPIRV_BUILTIN(GroupCommitWritePipe, _i32_Pipe_wo_ReserveId_i32, )(Workgroup, to_spirv_pipe_wo(p), to_spirv_reserveid(reserve_id), bytes);
}

/////////////////////////////////////////////////////////////////////
// Sub-Group Reservations
// Note: Not supporting this for intel sub groups (yet?).
reserve_id_t __sub_group_reserve_read_pipe( read_only pipe int p, uint num_packets, uint bytes, uint alignment )
{
    return to_ocl_reserveid(SPIRV_BUILTIN(GroupReserveReadPipePackets, _i32_Pipe_ro_i32_i32, )(Subgroup, to_spirv_pipe_ro(p), num_packets, bytes));
}

reserve_id_t __sub_group_reserve_write_pipe( write_only pipe int p, uint num_packets, uint bytes, uint alignment )
{
    return to_ocl_reserveid(SPIRV_BUILTIN(GroupReserveWritePipePackets, _i32_Pipe_wo_i32_i32, )(Subgroup, to_spirv_pipe_wo(p), num_packets, bytes));
}

void __sub_group_commit_read_pipe( read_only pipe int p, reserve_id_t reserve_id, uint bytes, uint alignment )
{
    SPIRV_BUILTIN(GroupCommitReadPipe, _i32_Pipe_ro_ReserveId_i32, )(Subgroup, to_spirv_pipe_ro(p), to_spirv_reserveid(reserve_id), bytes);
}

void __sub_group_commit_write_pipe( write_only pipe int p, reserve_id_t reserve_id, uint bytes, uint alignment )
{
    SPIRV_BUILTIN(GroupCommitWritePipe, _i32_Pipe_wo_ReserveId_i32, )(Subgroup, to_spirv_pipe_wo(p), to_spirv_reserveid(reserve_id), bytes);
}

/////////////////////////////////////////////////////////////////////
// Pipe Queries
//
uint __get_pipe_num_packets(pipe int pipe_, uint bytes, uint alignment)
{
    return SPIRV_BUILTIN(GetNumPipePackets, _Pipe_ro_i32, )(to_spirv_pipe_ro(pipe_), bytes);
}

uint __get_pipe_num_packets_ro(pipe int pipe_, uint bytes, uint alignment)
{
    return SPIRV_BUILTIN(GetNumPipePackets, _Pipe_ro_i32, )(to_spirv_pipe_ro(pipe_), bytes);
}

uint __get_pipe_num_packets_wo(write_only pipe int pipe_, uint bytes, uint alignment)
{
    return SPIRV_BUILTIN(GetNumPipePackets, _Pipe_wo_i32, )(to_spirv_pipe_wo(pipe_), bytes);
}

uint __get_pipe_max_packets(pipe int pipe_, uint bytes, uint alignment)
{
    return SPIRV_BUILTIN(GetMaxPipePackets, _Pipe_ro_i32, )(to_spirv_pipe_ro(pipe_), bytes);
}

uint __get_pipe_max_packets_ro(pipe int pipe_, uint bytes, uint alignment)
{
    return SPIRV_BUILTIN(GetMaxPipePackets, _Pipe_ro_i32, )(to_spirv_pipe_ro(pipe_), bytes);
}

uint __get_pipe_max_packets_wo(write_only pipe int pipe_, uint bytes, uint alignment)
{
    return SPIRV_BUILTIN(GetMaxPipePackets, _Pipe_wo_i32, )(to_spirv_pipe_wo(pipe_), bytes);
}

#endif // __OPENCL_C_VERSION__ >= CL_VERSION_2_0

// Internal Debug Built-Ins (TODO: implement for IGC?)

__attribute__((always_inline))
ulong OVERLOADABLE intel_get_cycle_counter( void )
{
    return __builtin_spirv_OpReadClockKHR_i64_i32(0);
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
uint OVERLOADABLE intel_get_dual_subslice_id(void)
{
    return __builtin_IB_dual_subslice_id();
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

__attribute__((always_inline))
uint OVERLOADABLE intel_get_tile_id( void )
{
    return SPIRV_BUILTIN_NO_OP(BuiltInSubDeviceIDINTEL, , )();
}

// Temporary workaround on a test that uses __builtin_spirv_BuiltInSubDeviceIDINTEL
// instead of intel_get_tile_id.
__attribute__((always_inline))
uint __builtin_spirv_BuiltInSubDeviceIDINTEL( void )
{
    return intel_get_tile_id();
}

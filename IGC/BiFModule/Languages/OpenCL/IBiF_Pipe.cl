/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "IBiF_Header.h"
#include "IBiF_SPIRV_Utils.h"

#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
int __read_pipe_2(read_only pipe int pipe_, __generic void *data, uint bytes, uint alignment) {
  long id = (long)__builtin_IB_cast_object_to_generic_ptr(pipe_);
  return __spirv_ReadPipe(__builtin_IB_convert_object_type_to_spirv_pipe_ro((void *)id), data, bytes);
}

int __write_pipe_2(write_only pipe int pipe_, __generic const void *data, uint bytes, uint alignment) {
  long id = (long)__builtin_IB_cast_object_to_generic_ptr(pipe_);
  return __spirv_WritePipe(__builtin_IB_convert_object_type_to_spirv_pipe_wo((void *)id), data, bytes);
}

bool OVERLOADABLE is_valid_reserve_id(reserve_id_t reserve_id) {
  return __spirv_IsValidReserveId(
      __builtin_IB_convert_object_type_to_spirv_reserveid(__builtin_IB_cast_object_to_generic_ptr(reserve_id)));
}

// NOTE: The pipe's packet type doesn't affect mangling.
reserve_id_t __reserve_read_pipe(read_only pipe int pipe_, uint num_packets, uint bytes, uint alignment) {
  long id = (long)__builtin_IB_cast_object_to_generic_ptr(pipe_);
  return __builtin_IB_convert_object_type_to_ocl_reserveid(__spirv_ReserveReadPipePackets(
      __builtin_IB_convert_object_type_to_spirv_pipe_ro((void *)id), num_packets, bytes));
}

reserve_id_t __reserve_write_pipe(write_only pipe int pipe_, uint num_packets, uint bytes, uint alignment) {
  long id = (long)__builtin_IB_cast_object_to_generic_ptr(pipe_);
  return __builtin_IB_convert_object_type_to_ocl_reserveid(__spirv_ReserveWritePipePackets(
      __builtin_IB_convert_object_type_to_spirv_pipe_wo((void *)id), num_packets, bytes));
}

void __commit_read_pipe(read_only pipe int pipe_, reserve_id_t reserve_id, uint bytes, uint alignment) {
  long id = (long)__builtin_IB_cast_object_to_generic_ptr(pipe_);
  __spirv_CommitReadPipe(
      __builtin_IB_convert_object_type_to_spirv_pipe_ro((void *)id),
      __builtin_IB_convert_object_type_to_spirv_reserveid(__builtin_IB_cast_object_to_generic_ptr(reserve_id)), bytes);
}

void __commit_write_pipe(write_only pipe int pipe_, reserve_id_t reserve_id, uint bytes, uint alignment) {
  long id = (long)__builtin_IB_cast_object_to_generic_ptr(pipe_);
  __spirv_CommitWritePipe(
      __builtin_IB_convert_object_type_to_spirv_pipe_wo((void *)id),
      __builtin_IB_convert_object_type_to_spirv_reserveid(__builtin_IB_cast_object_to_generic_ptr(reserve_id)), bytes);
}

int __read_pipe_4(read_only pipe int pipe_, reserve_id_t reserve_id, uint index, __generic void *data, uint bytes,
                  uint alignment) {
  long id = (long)__builtin_IB_cast_object_to_generic_ptr(pipe_);
  return __spirv_ReservedReadPipe(
      __builtin_IB_convert_object_type_to_spirv_pipe_ro((void *)id),
      __builtin_IB_convert_object_type_to_spirv_reserveid(__builtin_IB_cast_object_to_generic_ptr(reserve_id)), index,
      data, bytes);
}

// write_pipe with 4 explicit arguments
int __write_pipe_4(write_only pipe int pipe_, reserve_id_t reserve_id, uint index, __generic const void *data,
                   uint bytes, uint alignment) {
  long id = (long)__builtin_IB_cast_object_to_generic_ptr(pipe_);
  return __spirv_ReservedWritePipe(
      __builtin_IB_convert_object_type_to_spirv_pipe_wo((void *)id),
      __builtin_IB_convert_object_type_to_spirv_reserveid(__builtin_IB_cast_object_to_generic_ptr(reserve_id)), index,
      data, bytes);
}

reserve_id_t __work_group_reserve_read_pipe(read_only pipe int p, uint num_packets, uint bytes, uint alignment) {
  long id = (long)__builtin_IB_cast_object_to_generic_ptr(p);
  return __builtin_IB_convert_object_type_to_ocl_reserveid(__spirv_GroupReserveReadPipePackets(
      Workgroup, __builtin_IB_convert_object_type_to_spirv_pipe_ro((void *)id), num_packets, bytes));
}

reserve_id_t __work_group_reserve_write_pipe(write_only pipe int p, uint num_packets, uint bytes, uint alignment) {
  long id = (long)__builtin_IB_cast_object_to_generic_ptr(p);
  return __builtin_IB_convert_object_type_to_ocl_reserveid(__spirv_GroupReserveWritePipePackets(
      Workgroup, __builtin_IB_convert_object_type_to_spirv_pipe_wo((void *)id), num_packets, bytes));
}

void __work_group_commit_read_pipe(read_only pipe int p, reserve_id_t reserve_id, uint bytes, uint alignment) {
  long id = (long)__builtin_IB_cast_object_to_generic_ptr(p);
  __spirv_GroupCommitReadPipe(
      Workgroup, __builtin_IB_convert_object_type_to_spirv_pipe_ro((void *)id),
      __builtin_IB_convert_object_type_to_spirv_reserveid(__builtin_IB_cast_object_to_generic_ptr(reserve_id)), bytes);
}

void __work_group_commit_write_pipe(write_only pipe int p, reserve_id_t reserve_id, uint bytes, uint alignment) {
  long id = (long)__builtin_IB_cast_object_to_generic_ptr(p);
  __spirv_GroupCommitWritePipe(
      Workgroup, __builtin_IB_convert_object_type_to_spirv_pipe_wo((void *)id),
      __builtin_IB_convert_object_type_to_spirv_reserveid(__builtin_IB_cast_object_to_generic_ptr(reserve_id)), bytes);
}

/////////////////////////////////////////////////////////////////////
// Sub-Group Reservations
// Note: Not supporting this for intel sub groups (yet?).
reserve_id_t __sub_group_reserve_read_pipe(read_only pipe int p, uint num_packets, uint bytes, uint alignment) {
  long id = (long)__builtin_IB_cast_object_to_generic_ptr(p);
  return __builtin_IB_convert_object_type_to_ocl_reserveid(__spirv_GroupReserveReadPipePackets(
      Subgroup, __builtin_IB_convert_object_type_to_spirv_pipe_ro((void *)id), num_packets, bytes));
}

reserve_id_t __sub_group_reserve_write_pipe(write_only pipe int p, uint num_packets, uint bytes, uint alignment) {
  long id = (long)__builtin_IB_cast_object_to_generic_ptr(p);
  return __builtin_IB_convert_object_type_to_ocl_reserveid(__spirv_GroupReserveWritePipePackets(
      Subgroup, __builtin_IB_convert_object_type_to_spirv_pipe_wo((void *)id), num_packets, bytes));
}

void __sub_group_commit_read_pipe(read_only pipe int p, reserve_id_t reserve_id, uint bytes, uint alignment) {
  long id = (long)__builtin_IB_cast_object_to_generic_ptr(p);
  __spirv_GroupCommitReadPipe(
      Subgroup, __builtin_IB_convert_object_type_to_spirv_pipe_ro((void *)id),
      __builtin_IB_convert_object_type_to_spirv_reserveid(__builtin_IB_cast_object_to_generic_ptr(reserve_id)), bytes);
}

void __sub_group_commit_write_pipe(write_only pipe int p, reserve_id_t reserve_id, uint bytes, uint alignment) {
  long id = (long)__builtin_IB_cast_object_to_generic_ptr(p);
  __spirv_GroupCommitWritePipe(
      Subgroup, __builtin_IB_convert_object_type_to_spirv_pipe_wo((void *)id),
      __builtin_IB_convert_object_type_to_spirv_reserveid(__builtin_IB_cast_object_to_generic_ptr(reserve_id)), bytes);
}

/////////////////////////////////////////////////////////////////////
// Pipe Queries
//
uint __get_pipe_num_packets(pipe int pipe_, uint bytes, uint alignment) {
  long id = (long)__builtin_IB_cast_object_to_generic_ptr(pipe_);
  return __spirv_GetNumPipePackets(__builtin_IB_convert_object_type_to_spirv_pipe_ro((void *)id), bytes);
}

uint __get_pipe_num_packets_ro(pipe int pipe_, uint bytes, uint alignment) {
  long id = (long)__builtin_IB_cast_object_to_generic_ptr(pipe_);
  return __spirv_GetNumPipePackets(__builtin_IB_convert_object_type_to_spirv_pipe_ro((void *)id), bytes);
}

uint __get_pipe_num_packets_wo(write_only pipe int pipe_, uint bytes, uint alignment) {
  long id = (long)__builtin_IB_cast_object_to_generic_ptr(pipe_);
  return __spirv_GetNumPipePackets(__builtin_IB_convert_object_type_to_spirv_pipe_wo((void *)id), bytes);
}

uint __get_pipe_max_packets(pipe int pipe_, uint bytes, uint alignment) {
  long id = (long)__builtin_IB_cast_object_to_generic_ptr(pipe_);
  return __spirv_GetMaxPipePackets(__builtin_IB_convert_object_type_to_spirv_pipe_ro((void *)id), bytes);
}

uint __get_pipe_max_packets_ro(pipe int pipe_, uint bytes, uint alignment) {
  long id = (long)__builtin_IB_cast_object_to_generic_ptr(pipe_);
  return __spirv_GetMaxPipePackets(__builtin_IB_convert_object_type_to_spirv_pipe_ro((void *)id), bytes);
}

uint __get_pipe_max_packets_wo(write_only pipe int pipe_, uint bytes, uint alignment) {
  long id = (long)__builtin_IB_cast_object_to_generic_ptr(pipe_);
  return __spirv_GetMaxPipePackets(__builtin_IB_convert_object_type_to_spirv_pipe_wo((void *)id), bytes);
}
#endif

/*========================== begin_copyright_notice ============================

Copyright (C) 2024 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

// REQUIRES: cri-supported

// RUN: ocloc compile -file %s -device cri -options "-igc_opts 'DumpVISAASMToConsole=1' -cl-std=CL3.0 \
// RUN: -DADDRSPACE=local -DFUNC_GET_ID=get_local_id \
// RUN: -DATOMIC_TYPE=atomic_ushort -DOPERAND_TYPE=ushort -DDATA_TYPE=ushort -DWORK_GROUP_SIZE=16 -DFUNCTION=intel_atomic_fetch_add_as_bfloat16" \
// RUN: -internal_options "-cl-ext=-all,+cl_khr_fp16,+cl_intel_bfloat16_atomics" | FileCheck %s --check-prefix=CHECK-VISAASM-ADD-BF16-LOCAL

// CHECK-VISAASM-ADD-BF16-LOCAL: lsc_atomic_store.slm (M1, 32)  %null:d16u32
// CHECK-VISAASM-ADD-BF16-LOCAL: lsc_fence.slm.none.group
// CHECK-VISAASM-ADD-BF16-LOCAL: lsc_atomic_bfadd.slm (M1, 32)  V{{[0-9]+}}:d16u32
// CHECK-VISAASM-ADD-BF16-LOCAL: lsc_atomic_or.slm (M1, 32)  V{{[0-9]+}}:d16u32


// RUN: ocloc compile -file %s -device cri -options "-igc_opts 'DumpVISAASMToConsole=1' -cl-std=CL3.0 \
// RUN: -DADDRSPACE=local -DFUNC_GET_ID=get_local_id \
// RUN: -DATOMIC_TYPE=atomic_ushort -DOPERAND_TYPE=ushort -DDATA_TYPE=ushort -DWORK_GROUP_SIZE=16 -DFUNCTION=intel_atomic_fetch_sub_as_bfloat16" \
// RUN: -internal_options "-cl-ext=-all,+cl_khr_fp16,+cl_intel_bfloat16_atomics" | FileCheck %s --check-prefix=CHECK-VISAASM-SUB-BF16-LOCAL

// CHECK-VISAASM-SUB-BF16-LOCAL: lsc_atomic_store.slm (M1, 32)  %null:d16u32
// CHECK-VISAASM-SUB-BF16-LOCAL: lsc_fence.slm.none.group
// CHECK-VISAASM-SUB-BF16-LOCAL: lsc_atomic_bfsub.slm (M1, 32)  V{{[0-9]+}}:d16u32
// CHECK-VISAASM-SUB-BF16-LOCAL: lsc_atomic_or.slm (M1, 32)  V{{[0-9]+}}:d16u32


// RUN: ocloc compile -file %s -device cri -options "-igc_opts 'DumpVISAASMToConsole=1' -cl-std=CL3.0 \
// RUN: -DADDRSPACE=local -DFUNC_GET_ID=get_local_id \
// RUN: -DATOMIC_TYPE=atomic_ushort -DOPERAND_TYPE=ushort -DDATA_TYPE=ushort -DWORK_GROUP_SIZE=16 -DFUNCTION=intel_atomic_fetch_max_as_bfloat16" \
// RUN: -internal_options "-cl-ext=-all,+cl_khr_fp16,+cl_intel_bfloat16_atomics" | FileCheck %s --check-prefix=CHECK-VISAASM-MAX-BF16-LOCAL

// CHECK-VISAASM-MAX-BF16-LOCAL: lsc_atomic_store.slm (M1, 32)  %null:d16u32
// CHECK-VISAASM-MAX-BF16-LOCAL: lsc_fence.slm.none.group
// CHECK-VISAASM-MAX-BF16-LOCAL: lsc_atomic_bfmax.slm (M1, 32)  V{{[0-9]+}}:d16u32
// CHECK-VISAASM-MAX-BF16-LOCAL: lsc_atomic_or.slm (M1, 32)  V{{[0-9]+}}:d16u32


// RUN: ocloc compile -file %s -device cri -options "-igc_opts 'DumpVISAASMToConsole=1' -cl-std=CL3.0 \
// RUN: -DADDRSPACE=local -DFUNC_GET_ID=get_local_id \
// RUN: -DATOMIC_TYPE=atomic_ushort -DOPERAND_TYPE=ushort -DDATA_TYPE=ushort -DWORK_GROUP_SIZE=16 -DFUNCTION=intel_atomic_fetch_min_as_bfloat16" \
// RUN: -internal_options "-cl-ext=-all,+cl_khr_fp16,+cl_intel_bfloat16_atomics" | FileCheck %s --check-prefix=CHECK-VISAASM-MIN-BF16-LOCAL

// CHECK-VISAASM-MIN-BF16-LOCAL: lsc_atomic_store.slm (M1, 32)  %null:d16u32
// CHECK-VISAASM-MIN-BF16-LOCAL: lsc_fence.slm.none.group
// CHECK-VISAASM-MIN-BF16-LOCAL: lsc_atomic_bfmin.slm (M1, 32)  V{{[0-9]+}}:d16u32
// CHECK-VISAASM-MIN-BF16-LOCAL: lsc_atomic_or.slm (M1, 32)  V{{[0-9]+}}:d16u32


__attribute__((reqd_work_group_size(WORK_GROUP_SIZE, 1, 1))) kernel void
test_kernel(global DATA_TYPE *value, const global OPERAND_TYPE *operand,
            global DATA_TYPE *fetched) {
  const size_t global_id = get_global_id(0);
  const size_t local_id = get_local_id(0);

  local ATOMIC_TYPE local_atomic[WORK_GROUP_SIZE];
  atomic_store_explicit(&local_atomic[local_id], value[global_id],
                        memory_order_relaxed, memory_scope_work_group);

#if defined(MEMORY_SCOPE) && defined(MEMORY_ORDER)
  fetched[global_id] = FUNCTION(&local_atomic[local_id], operand[global_id],
                                MEMORY_ORDER, MEMORY_SCOPE);
#elif defined(MEMORY_ORDER)
  fetched[global_id] =
      FUNCTION(&local_atomic[local_id], operand[global_id], MEMORY_ORDER);
#else
  fetched[global_id] = FUNCTION(&local_atomic[local_id], operand[global_id]);
#endif

  value[global_id] = atomic_load_explicit(
&local_atomic[local_id], memory_order_relaxed, memory_scope_work_group);
}

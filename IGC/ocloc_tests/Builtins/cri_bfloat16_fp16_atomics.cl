/*========================== begin_copyright_notice ============================

Copyright (C) 2023 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/
// REQUIRES: cri-supported, fix-checks-due-to-neo-update

// RUN: ocloc compile -file %s -device cri -options "-igc_opts 'DumpVISAASMToConsole=1' -cl-std=CL3.0 \
// RUN: -DADDRSPACE=global -DFUNC_GET_ID=get_global_id \
// RUN: -DATOMIC_TYPE=atomic_ushort -DOPERAND_TYPE=ushort -DDATA_TYPE=ushort -DWORK_GROUP_SIZE=16 -DFUNCTION=intel_atomic_fetch_add_as_bfloat16" \
// RUN: -internal_options "-cl-ext=-all,+cl_khr_fp16,+cl_intel_bfloat16_atomics" | FileCheck %s --check-prefix=CHECK-VISAASM-ADD-BF16-GLOBAL

// CHECK-VISAASM-ADD-BF16-GLOBAL: lsc_fence.ugm.none.gpu
// CHECK-VISAASM-ADD-BF16-GLOBAL: lsc_atomic_bfadd.ugm (M1, 32)  V{{[0-9]+}}:d16u32

// RUN: ocloc compile -file %s -device cri -options "-igc_opts 'DumpVISAASMToConsole=1' -cl-std=CL3.0 \
// RUN: -DADDRSPACE=local -DFUNC_GET_ID=get_local_id \
// RUN: -DATOMIC_TYPE=atomic_ushort -DOPERAND_TYPE=ushort -DDATA_TYPE=ushort -DWORK_GROUP_SIZE=16 -DFUNCTION=intel_atomic_fetch_add_as_bfloat16" \
// RUN: -internal_options "-cl-ext=-all,+cl_khr_fp16,+cl_intel_bfloat16_atomics" | FileCheck %s --check-prefix=CHECK-VISAASM-ADD-BF16-LOCAL

// CHECK-VISAASM-ADD-BF16-LOCAL: lsc_fence.slm.none.group
// CHECK-VISAASM-ADD-BF16-LOCAL: lsc_atomic_bfadd.slm (M1, 32)  V{{[0-9]+}}:d16u32


// RUN: ocloc compile -file %s -device cri -options "-igc_opts 'DumpVISAASMToConsole=1' -cl-std=CL3.0 \
// RUN: -DADDRSPACE=global -DFUNC_GET_ID=get_global_id \
// RUN: -DATOMIC_TYPE=atomic_ushort -DOPERAND_TYPE=ushort -DDATA_TYPE=ushort -DWORK_GROUP_SIZE=16 -DFUNCTION=intel_atomic_fetch_sub_as_bfloat16" \
// RUN: -internal_options "-cl-ext=-all,+cl_khr_fp16,+cl_intel_bfloat16_atomics" | FileCheck %s --check-prefix=CHECK-VISAASM-SUB-BF16-GLOBAL

// CHECK-VISAASM-SUB-BF16-GLOBAL: lsc_fence.ugm.none.gpu
// CHECK-VISAASM-SUB-BF16-GLOBAL: lsc_atomic_bfsub.ugm (M1, 32)  V{{[0-9]+}}:d16u32

// RUN: ocloc compile -file %s -device cri -options "-igc_opts 'DumpVISAASMToConsole=1' -cl-std=CL3.0 \
// RUN: -DADDRSPACE=local -DFUNC_GET_ID=get_local_id \
// RUN: -DATOMIC_TYPE=atomic_ushort -DOPERAND_TYPE=ushort -DDATA_TYPE=ushort -DWORK_GROUP_SIZE=16 -DFUNCTION=intel_atomic_fetch_sub_as_bfloat16" \
// RUN: -internal_options "-cl-ext=-all,+cl_khr_fp16,+cl_intel_bfloat16_atomics" | FileCheck %s --check-prefix=CHECK-VISAASM-SUB-BF16-LOCAL

// CHECK-VISAASM-SUB-BF16-LOCAL: lsc_fence.slm.none.group
// CHECK-VISAASM-SUB-BF16-LOCAL: lsc_atomic_bfsub.slm (M1, 32)  V{{[0-9]+}}:d16u32


// RUN: ocloc compile -file %s -device cri -options "-igc_opts 'DumpVISAASMToConsole=1' -cl-std=CL3.0 \
// RUN: -DADDRSPACE=global -DFUNC_GET_ID=get_global_id \
// RUN: -DATOMIC_TYPE=atomic_ushort -DOPERAND_TYPE=ushort -DDATA_TYPE=ushort -DWORK_GROUP_SIZE=16 -DFUNCTION=intel_atomic_fetch_max_as_bfloat16" \
// RUN: -internal_options "-cl-ext=-all,+cl_khr_fp16,+cl_intel_bfloat16_atomics" | FileCheck %s --check-prefix=CHECK-VISAASM-MAX-BF16-GLOBAL

// CHECK-VISAASM-MAX-BF16-GLOBAL: lsc_fence.ugm.none.gpu
// CHECK-VISAASM-MAX-BF16-GLOBAL: lsc_atomic_bfmax.ugm (M1, 32)  V{{[0-9]+}}:d16u32

// RUN: ocloc compile -file %s -device cri -options "-igc_opts 'DumpVISAASMToConsole=1' -cl-std=CL3.0 \
// RUN: -DADDRSPACE=local -DFUNC_GET_ID=get_local_id \
// RUN: -DATOMIC_TYPE=atomic_ushort -DOPERAND_TYPE=ushort -DDATA_TYPE=ushort -DWORK_GROUP_SIZE=16 -DFUNCTION=intel_atomic_fetch_max_as_bfloat16" \
// RUN: -internal_options "-cl-ext=-all,+cl_khr_fp16,+cl_intel_bfloat16_atomics" | FileCheck %s --check-prefix=CHECK-VISAASM-MAX-BF16-LOCAL

// CHECK-VISAASM-MAX-BF16-LOCAL: lsc_fence.slm.none.group
// CHECK-VISAASM-MAX-BF16-LOCAL: lsc_atomic_bfmax.slm (M1, 32)  V{{[0-9]+}}:d16u32


// RUN: ocloc compile -file %s -device cri -options "-igc_opts 'DumpVISAASMToConsole=1' -cl-std=CL3.0 \
// RUN: -DADDRSPACE=global -DFUNC_GET_ID=get_global_id \
// RUN: -DATOMIC_TYPE=atomic_ushort -DOPERAND_TYPE=ushort -DDATA_TYPE=ushort -DWORK_GROUP_SIZE=16 -DFUNCTION=intel_atomic_fetch_min_as_bfloat16" \
// RUN: -internal_options "-cl-ext=-all,+cl_khr_fp16,+cl_intel_bfloat16_atomics" | FileCheck %s --check-prefix=CHECK-VISAASM-MIN-BF16-GLOBAL

// CHECK-VISAASM-MIN-BF16-GLOBAL: lsc_fence.ugm.none.gpu
// CHECK-VISAASM-MIN-BF16-GLOBAL: lsc_atomic_bfmin.ugm (M1, 32)  V{{[0-9]+}}:d16u32

// RUN: ocloc compile -file %s -device cri -options "-igc_opts 'DumpVISAASMToConsole=1' -cl-std=CL3.0 \
// RUN: -DADDRSPACE=local -DFUNC_GET_ID=get_local_id \
// RUN: -DATOMIC_TYPE=atomic_ushort -DOPERAND_TYPE=ushort -DDATA_TYPE=ushort -DWORK_GROUP_SIZE=16 -DFUNCTION=intel_atomic_fetch_min_as_bfloat16" \
// RUN: -internal_options "-cl-ext=-all,+cl_khr_fp16,+cl_intel_bfloat16_atomics" | FileCheck %s --check-prefix=CHECK-VISAASM-MIN-BF16-LOCAL

// CHECK-VISAASM-MIN-BF16-LOCAL: lsc_fence.slm.none.group
// CHECK-VISAASM-MIN-BF16-LOCAL: lsc_atomic_bfmin.slm (M1, 32)  V{{[0-9]+}}:d16u32


// RUN: ocloc compile -file %s -device cri -options "-igc_opts 'DumpVISAASMToConsole=1' -cl-std=CL3.0 \
// RUN: -DADDRSPACE=global -DFUNC_GET_ID=get_global_id \
// RUN: -DATOMIC_TYPE=atomic_half -DOPERAND_TYPE=half -DDATA_TYPE=half -DWORK_GROUP_SIZE=16 -DFUNCTION=atomic_fetch_add" | \
// RUN: FileCheck %s --check-prefix=CHECK-VISAASM-ADD-HALF-GLOBAL

// CHECK-VISAASM-ADD-HALF-GLOBAL: lsc_fence.ugm.none.gpu
// CHECK-VISAASM-ADD-HALF-GLOBAL: lsc_atomic_fadd.ugm (M1, 32)  V{{[0-9]+}}:d16u32

// RUN: ocloc compile -file %s -device cri -options "-igc_opts 'DumpVISAASMToConsole=1' -cl-std=CL3.0 \
// RUN: -DADDRSPACE=local -DFUNC_GET_ID=get_local_id \
// RUN: -DATOMIC_TYPE=atomic_half -DOPERAND_TYPE=half -DDATA_TYPE=half -DWORK_GROUP_SIZE=16 -DFUNCTION=atomic_fetch_add" | \
// RUN: FileCheck %s --check-prefix=CHECK-VISAASM-ADD-HALF-LOCAL

// CHECK-VISAASM-ADD-HALF-LOCAL: lsc_fence.slm.none.group
// CHECK-VISAASM-ADD-HALF-LOCAL: lsc_atomic_fadd.slm (M1, 32)  V{{[0-9]+}}:d16u32


// RUN: ocloc compile -file %s -device cri -options "-igc_opts 'EnableNativeFP32LocalAtomicAdd=1,DumpVISAASMToConsole=1' -cl-std=CL3.0 \
// RUN: -DADDRSPACE=local -DFUNC_GET_ID=get_local_id \
// RUN: -DATOMIC_TYPE=atomic_float -DOPERAND_TYPE=float -DDATA_TYPE=float -DWORK_GROUP_SIZE=16 -DFUNCTION=atomic_fetch_add" | \
// RUN: FileCheck %s --check-prefix=CHECK-VISAASM-ADD-FLOAT-LOCAL

// CHECK-VISAASM-ADD-FLOAT-LOCAL: lsc_fence.slm.none.group
// CHECK-VISAASM-ADD-FLOAT-LOCAL: lsc_atomic_fadd.slm (M1, 32)  V{{[0-9]+}}:d32

// RUN: ocloc compile -file %s -device cri -options "-igc_opts 'EnableNativeFP32LocalAtomicAdd=0,DumpVISAASMToConsole=1' -cl-std=CL3.0 \
// RUN: -DADDRSPACE=local -DFUNC_GET_ID=get_local_id \
// RUN: -DATOMIC_TYPE=atomic_float -DOPERAND_TYPE=float -DDATA_TYPE=float -DWORK_GROUP_SIZE=16 -DFUNCTION=atomic_fetch_add" | \
// RUN: FileCheck %s --check-prefix=CHECK-VISAASM-ADD-FLOAT-LOCAL-NONNATIVE

// CHECK-VISAASM-ADD-FLOAT-LOCAL-NONNATIVE-NOT: lsc_atomic_fadd.slm (M1, 32)  V{{[0-9]+}}:d32


kernel void test_kernel(ADDRSPACE ATOMIC_TYPE *value,
                        const ADDRSPACE OPERAND_TYPE *operand,
                        ADDRSPACE DATA_TYPE *fetched) {
  const size_t id = FUNC_GET_ID(0);

#if defined(MEMORY_SCOPE) && defined(MEMORY_ORDER)
  fetched[id] = FUNCTION(&value[id], operand[id],
                                MEMORY_ORDER, MEMORY_SCOPE);
#elif defined(MEMORY_ORDER)
  fetched[id] =
      FUNCTION(&value[id], operand[id], MEMORY_ORDER);
#else
  fetched[id] = FUNCTION(&value[id], operand[id]);
#endif
}

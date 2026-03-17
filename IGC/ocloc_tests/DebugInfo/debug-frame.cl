//========================== begin_copyright_notice ============================
//
// Copyright (C) 2023 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
//=========================== end_copyright_notice =============================

// We want to verify CIE and FDE for last item.
// CIE is constant for all elf files IGC emits where stack call is present.

// UNSUPPORTED: sys32, system-windows

#pragma OPENCL EXTENSION cl_khr_int64_base_atomics : enable
#pragma OPENCL EXTENSION cl_khr_int64_extended_atomics : enable

void test_atomic_function(uint tid, uint threadCount, uint numDestItems, volatile __global atomic_ptrdiff_t *destMemory, __global ptrdiff_t *oldValues)
{
  size_t numBits = sizeof(ptrdiff_t) * 8;
  int whichResult = tid / numBits;
  int bitIndex = tid - (whichResult * numBits);

  oldValues[tid] = atomic_fetch_and_explicit(&destMemory[whichResult], ~((ptrdiff_t)1 << bitIndex) , memory_order_acq_rel, memory_scope_all_svm_devices);
}

__kernel void foo(uint threadCount, uint numDestItems, volatile __global atomic_ptrdiff_t *destMemory, __global ptrdiff_t *oldValues)
{
  uint tid = get_global_id(0);

  test_atomic_function(tid, threadCount, numDestItems, destMemory, oldValues);
}

// RUN: %if dg2-supported %{ ocloc compile -file %s -options " -g -cl-opt-disable -cl-std=CL3.0 -igc_opts 'ElfDumpEnable=1, DumpUseShorterName=0, DebugDumpNamePrefix=%t_dg2_'" -device dg2 %}
// RUN: %if dg2-supported %{ llvm-dwarfdump -a %t_dg2_OCL_simd8_foo.elf &> %t_dg2_OCL_simd8_foo.dwarf %}
// RUN: %if dg2-supported %{ FileCheck %s --input-file=%t_dg2_OCL_simd8_foo.dwarf --check-prefixes=CHECK,CHECK-KERNEL %}
// RUN: %if dg2-supported %{ FileCheck %s --input-file=%t_dg2_OCL_simd8_foo.dwarf --check-prefixes=CHECK,CHECK-FUNCTION %}

// RUN: %if cri-supported %{ ocloc compile -file %s -options " -g -cl-opt-disable -cl-std=CL3.0 -igc_opts 'ElfDumpEnable=1, DumpUseShorterName=0, DebugDumpNamePrefix=%t_cri_'" -device cri %}
// RUN: %if cri-supported %{ llvm-dwarfdump -a %t_cri_OCL_simd16_foo.elf &> %t_cri_OCL_simd16_foo.dwarf %}
// RUN: %if cri-supported %{ FileCheck %s --input-file=%t_cri_OCL_simd16_foo.dwarf --check-prefixes=CHECK,CHECK-KERNEL %}
// RUN: %if cri-supported %{ FileCheck %s --input-file=%t_cri_OCL_simd16_foo.dwarf --check-prefixes=CHECK,CHECK-FUNCTION %}

// CHECK: .debug_info contents:

// CHECK-KERNEL: DW_AT_name ("foo")
// CHECK-KERNEL: DW_AT_low_pc (0x[[#%x,LOW_PC:]])
// CHECK-KERNEL: DW_AT_high_pc (0x[[#%x,HIGH_PC:]])
// CHECK-KERNEL: DW_TAG_formal_parameter

// CHECK-FUNCTION: DW_AT_name ("test_atomic_function")
// CHECK-FUNCTION: DW_AT_low_pc (0x[[#%x,LOW_PC:]])
// CHECK-FUNCTION: DW_AT_high_pc (0x[[#%x,HIGH_PC:]])
// CHECK-FUNCTION: DW_TAG_formal_parameter

// CHECK: .debug_frame contents:
// CHECK: [[#%x,CIE_ID:]] 000000000000013{{.+}} ffffffffffffffff CIE
// CHECK-NEXT: Format: DWARF64
// CHECK-NEXT: Version: 4
// CHECK-NEXT: Augmentation: ""
// CHECK-NEXT: Address size: 8
// CHECK-NEXT: Segment desc size: 0
// CHECK-NEXT: Code alignment factor: 1
// CHECK-NEXT: Data alignment factor: 1
// CHECK-NEXT: Return address column: [[#]]

// CHECK-KERNEL: {{.+}} FDE cie=[[#%.8x,CIE_ID]] pc=[[#%.8x,LOW_PC]]...[[#%.8x,HIGH_PC]]
// CHECK-KERNEL-NEXT: Format: DWARF64

// CHECK-FUNCTION: {{.+}} FDE cie=[[#%.8x,CIE_ID]] pc=[[#%.8x,LOW_PC]]...[[#%.8x,HIGH_PC]]
// CHECK-FUNCTION-NEXT: Format: DWARF64

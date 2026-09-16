//========================== begin_copyright_notice ============================
//
// Copyright (C) 2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
//=========================== end_copyright_notice =============================

// A subroutine FDE recovers the return address from the GRF that call wrote, so
// it must read as many bits as %ip is wide.

// UNSUPPORTED: sys32
// REQUIRES: regkeys, oneapi-readelf

extern int extern_func_decl(__global int *in);

__attribute__((noinline)) int helper(__private int *p) { return *p + 5; }

__kernel void foo(__global int *out) {
  __private int tmp[2] = {extern_func_decl(0), 0};
  out[0] = helper(tmp);
}

// RUN: %if cri-supported %{ ocloc compile -file %s -device cri -options "-g -cl-opt-disable -igc_opts 'FunctionControl=2, ElfDumpEnable=1, DumpUseShorterName=0, DebugDumpNamePrefix=%t_cri_'" %}
// RUN: %if cri-supported %{ oneapi-readelf --debug-dump %t_cri_OCL_simd16_foo.elf | FileCheck %s --check-prefix=CHECK-IP64 %}

// RUN: %if dg2-supported %{ ocloc compile -file %s -device dg2 -options "-g -cl-opt-disable -igc_opts 'FunctionControl=2, ElfDumpEnable=1, DumpUseShorterName=0, DebugDumpNamePrefix=%t_dg2_'" %}
// RUN: %if dg2-supported %{ oneapi-readelf --debug-dump %t_dg2_OCL_simd8_foo.elf | FileCheck %s --check-prefix=CHECK-IP32 %}

// CHECK-IP64: Contents of the .debug_frame section:
// CHECK-IP64: DW_CFA_val_expression: r0 {{.*}}(DW_OP_const2u: {{[0-9]+}}; DW_OP_const2u: {{[0-9]+}}; DW_OP_INTEL_regval_bits: 64)
// CHECK-IP64-NEXT: DW_CFA_def_cfa_expression (DW_OP_const2u: 143; DW_OP_const1u: 64; DW_OP_INTEL_regval_bits: 32; DW_OP_breg6{{.*}}: 0; DW_OP_plus)

// CHECK-IP32: Contents of the .debug_frame section:
// CHECK-IP32: DW_CFA_val_expression: r0 {{.*}}(DW_OP_const2u: {{[0-9]+}}; DW_OP_const2u: {{[0-9]+}}; DW_OP_INTEL_regval_bits: 32)
// CHECK-IP32-NEXT: DW_CFA_def_cfa_expression (DW_OP_const2u: 143; DW_OP_const1u: 64; DW_OP_INTEL_regval_bits: 32; DW_OP_breg6{{.*}}: 0; DW_OP_plus)

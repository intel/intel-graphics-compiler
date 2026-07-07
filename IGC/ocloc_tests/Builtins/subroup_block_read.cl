/*========================== begin_copyright_notice ============================

Copyright (C) 2023 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/
// REQUIRES: regkeys, pvc-supported

// RUN: ocloc compile -file %s -device pvc -options "-igc_opts 'DumpVISAASMToConsole=1' \
// RUN: -DINPUT_TYPE=ulong -DOUTPUT_TYPE=ulong4 -DFUNCTION=intel_subgroup_block_read_transpose_u64_k4 -DINPUT_WIDTH=512 -DINPUT_HEIGHT=46 -DPITCH=512" \
// RUN: -internal_options "-cl-ext=-all,+cl_intel_subgroup_extended_block_read" | FileCheck %s --check-prefix=CHECK-VISAASM-U64-K4
// RUN: ocloc compile -file %s -device pvc -options "-igc_opts 'VISAOptions=-asmToConsole' \
// RUN: -DINPUT_TYPE=ulong -DOUTPUT_TYPE=ulong4 -DFUNCTION=intel_subgroup_block_read_transpose_u64_k4 -DINPUT_WIDTH=512 -DINPUT_HEIGHT=46 -DPITCH=512" \
// RUN: -internal_options "-cl-ext=-all,+cl_intel_subgroup_extended_block_read" | FileCheck %s --check-prefix=CHECK-ASM-U64-K4

// CHECK-VISAASM-U64-K4-DAG: lsc_load_block2d.ugm (M1, {{[0-9]+}})  V{{[0-9]+}}:d64.4x8tn  flat{{\[}}[[BASEADDR:.+]],0x1FF,0x2D,0x1FF,[[XBLOCKOFFSET:V[0-9]+]],[[YBLOCKOFFSET:V[0-9]+]]
// CHECK-VISAASM-U64-K4-DAG: add (M1_NM, {{[0-9]+}})  [[ADDDST:V[0-9]+]](0,0)<1> [[YBLOCKOFFSET]](0,{{[0-9]+}})<0;1,0> 0x8
// CHECK-VISAASM-U64-K4-DAG: lsc_load_block2d.ugm (M1, {{[0-9]+}})  V{{[0-9]+}}:d64.4x8tn  flat{{\[}}[[BASEADDR]],0x1FF,0x2D,0x1FF,[[XBLOCKOFFSET]],[[ADDDST]]
// CHECK-ASM-U64-K4-COUNT-2: load_block2d.ugm.d64t.a64 ({{[0-9]+}}|M0)  r{{[0-9]+}}:4 {{.+}} ex_desc:0x0; desc:0x2408603

__attribute__((intel_reqd_sub_group_size(16))) kernel void
test(global INPUT_TYPE *input, const global int2 *coord,
             global OUTPUT_TYPE *output) {
  const int tid = get_global_id(0);
  output[tid] = FUNCTION(input, INPUT_WIDTH, INPUT_HEIGHT, PITCH, *coord);
}

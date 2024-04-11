/*========================== begin_copyright_notice ============================

Copyright (C) 2024 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/
// REQUIRES: regkeys, pvc-supported

// RUN: ocloc compile -file %s -device pvc -options "-igc_opts 'DumpVISAASMToConsole=1' \
// RUN: -DINPUT_TYPE=ushort -DOUTPUT_TYPE=ushort2 -DFUNCTION=intel_subgroup_block_read_cacheopts_u8_m1k32v2 -DINPUT_WIDTH=512 -DINPUT_HEIGHT=46 -DPITCH=512 \
// RUN: -DCACHE_CONTROL=read_cache_control_default_intel" \
// RUN: -internal_options "-cl-ext=-all,+cl_intel_subgroup_extended_block_read_cacheopts" | FileCheck %s --check-prefix=CHECK-DEFAULT

// CHECK-DEFAULT: lsc_load_block2d.ugm {{.*}}

// RUN: ocloc compile -file %s -device pvc -options "-igc_opts 'DumpVISAASMToConsole=1' \
// RUN: -DINPUT_TYPE=ushort -DOUTPUT_TYPE=ushort2 -DFUNCTION=intel_subgroup_block_read_cacheopts_u8_m1k32v2 -DINPUT_WIDTH=512 -DINPUT_HEIGHT=46 -DPITCH=512 \
// RUN: -DCACHE_CONTROL=read_cache_control_l1_uncached_l3_uncached_intel" \
// RUN: -internal_options "-cl-ext=-all,+cl_intel_subgroup_extended_block_read_cacheopts" | FileCheck %s --check-prefix=CHECK-UC-UC

// CHECK-UC-UC: lsc_load_block2d.ugm.uc.uc

// RUN: ocloc compile -file %s -device pvc -options "-igc_opts 'DumpVISAASMToConsole=1' \
// RUN: -DINPUT_TYPE=ushort -DOUTPUT_TYPE=ushort2 -DFUNCTION=intel_subgroup_block_read_cacheopts_u8_m1k32v2 -DINPUT_WIDTH=512 -DINPUT_HEIGHT=46 -DPITCH=512 \
// RUN: -DCACHE_CONTROL=read_cache_control_l1_uncached_l3_cached_intel" \
// RUN: -internal_options "-cl-ext=-all,+cl_intel_subgroup_extended_block_read_cacheopts" | FileCheck %s --check-prefix=CHECK-UC-CA

// CHECK-UC-CA: lsc_load_block2d.ugm.uc.ca

// RUN: ocloc compile -file %s -device pvc -options "-igc_opts 'DumpVISAASMToConsole=1' \
// RUN: -DINPUT_TYPE=ushort -DOUTPUT_TYPE=ushort2 -DFUNCTION=intel_subgroup_block_read_cacheopts_u8_m1k32v2 -DINPUT_WIDTH=512 -DINPUT_HEIGHT=46 -DPITCH=512 \
// RUN: -DCACHE_CONTROL=read_cache_control_l1_cached_l3_uncached_intel" \
// RUN: -internal_options "-cl-ext=-all,+cl_intel_subgroup_extended_block_read_cacheopts" | FileCheck %s --check-prefix=CHECK-CA-UC

// CHECK-CA-UC: lsc_load_block2d.ugm.ca.uc

// RUN: ocloc compile -file %s -device pvc -options "-igc_opts 'DumpVISAASMToConsole=1' \
// RUN: -DINPUT_TYPE=ushort -DOUTPUT_TYPE=ushort2 -DFUNCTION=intel_subgroup_block_read_cacheopts_u8_m1k32v2 -DINPUT_WIDTH=512 -DINPUT_HEIGHT=46 -DPITCH=512 \
// RUN: -DCACHE_CONTROL=read_cache_control_l1_cached_l3_cached_intel" \
// RUN: -internal_options "-cl-ext=-all,+cl_intel_subgroup_extended_block_read_cacheopts" | FileCheck %s --check-prefix=CHECK-CA-CA

// CHECK-CA-CA: lsc_load_block2d.ugm.ca.ca

// RUN: ocloc compile -file %s -device pvc -options "-igc_opts 'DumpVISAASMToConsole=1' \
// RUN: -DINPUT_TYPE=ushort -DOUTPUT_TYPE=ushort2 -DFUNCTION=intel_subgroup_block_read_cacheopts_u8_m1k32v2 -DINPUT_WIDTH=512 -DINPUT_HEIGHT=46 -DPITCH=512 \
// RUN: -DCACHE_CONTROL=read_cache_control_l1_streaming_l3_uncached_intel" \
// RUN: -internal_options "-cl-ext=-all,+cl_intel_subgroup_extended_block_read_cacheopts" | FileCheck %s --check-prefix=CHECK-S-UC

// CHECK-S-UC: lsc_load_block2d.ugm.st.uc

// RUN: ocloc compile -file %s -device pvc -options "-igc_opts 'DumpVISAASMToConsole=1' \
// RUN: -DINPUT_TYPE=ushort -DOUTPUT_TYPE=ushort2 -DFUNCTION=intel_subgroup_block_read_cacheopts_u8_m1k32v2 -DINPUT_WIDTH=512 -DINPUT_HEIGHT=46 -DPITCH=512 \
// RUN: -DCACHE_CONTROL=read_cache_control_l1_streaming_l3_cached_intel" \
// RUN: -internal_options "-cl-ext=-all,+cl_intel_subgroup_extended_block_read_cacheopts" | FileCheck %s --check-prefix=CHECK-S-CA

// CHECK-S-CA: lsc_load_block2d.ugm.st.ca

// RUN: ocloc compile -file %s -device pvc -options "-igc_opts 'DumpVISAASMToConsole=1' \
// RUN: -DINPUT_TYPE=ushort -DOUTPUT_TYPE=ushort2 -DFUNCTION=intel_subgroup_block_read_cacheopts_u8_m1k32v2 -DINPUT_WIDTH=512 -DINPUT_HEIGHT=46 -DPITCH=512 \
// RUN: -DCACHE_CONTROL=read_cache_control_l1_iar_l3_cached_intel" \
// RUN: -internal_options "-cl-ext=-all,+cl_intel_subgroup_extended_block_read_cacheopts" | FileCheck %s --check-prefix=CHECK-IAR-CA

// CHECK-IAR-CA: lsc_load_block2d.ugm.ri.ca

__attribute__((intel_reqd_sub_group_size(16))) kernel void
test(global INPUT_TYPE *input, const global int2 *coord,
             global OUTPUT_TYPE *output) {
  const int tid = get_global_id(0);
  output[tid] = FUNCTION(input, INPUT_WIDTH, INPUT_HEIGHT, PITCH, *coord, CACHE_CONTROL);
}

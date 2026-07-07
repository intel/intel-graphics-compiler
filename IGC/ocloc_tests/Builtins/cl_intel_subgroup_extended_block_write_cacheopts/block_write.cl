/*========================== begin_copyright_notice ============================

Copyright (C) 2024 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/
// REQUIRES: regkeys, pvc-supported

// RUN: ocloc compile -file %s -device pvc -options "-igc_opts 'DumpVISAASMToConsole=1' \
// RUN: -DINPUT_TYPE=ushort -DOUTPUT_TYPE=ushort -DFUNCTION=intel_subgroup_block_write_cacheopts_u8_m1k32v1 -DINPUT_WIDTH=512 -DINPUT_HEIGHT=46 -DPITCH=512 \
// RUN: -DCACHE_CONTROL=write_cache_control_default_intel" \
// RUN: -internal_options "-cl-ext=-all,+cl_intel_subgroup_extended_block_write_cacheopts" | FileCheck %s --check-prefix=CHECK-DEFAULT

// CHECK-DEFAULT: lsc_store_block2d.ugm {{.*}}

// RUN: ocloc compile -file %s -device pvc -options "-igc_opts 'DumpVISAASMToConsole=1' \
// RUN: -DINPUT_TYPE=ushort -DOUTPUT_TYPE=ushort -DFUNCTION=intel_subgroup_block_write_cacheopts_u8_m1k32v1 -DINPUT_WIDTH=512 -DINPUT_HEIGHT=46 -DPITCH=512 \
// RUN: -DCACHE_CONTROL=write_cache_control_l1_uncached_l3_uncached_intel" \
// RUN: -internal_options "-cl-ext=-all,+cl_intel_subgroup_extended_block_write_cacheopts" | FileCheck %s --check-prefix=CHECK-UC-UC

// CHECK-UC-UC: lsc_store_block2d.ugm.uc.uc

// RUN: ocloc compile -file %s -device pvc -options "-igc_opts 'DumpVISAASMToConsole=1' \
// RUN: -DINPUT_TYPE=ushort -DOUTPUT_TYPE=ushort -DFUNCTION=intel_subgroup_block_write_cacheopts_u8_m1k32v1 -DINPUT_WIDTH=512 -DINPUT_HEIGHT=46 -DPITCH=512 \
// RUN: -DCACHE_CONTROL=write_cache_control_l1_uncached_l3_writeback_intel" \
// RUN: -internal_options "-cl-ext=-all,+cl_intel_subgroup_extended_block_write_cacheopts" | FileCheck %s --check-prefix=CHECK-UC-WB

// CHECK-UC-WB: lsc_store_block2d.ugm.uc.wb

// RUN: ocloc compile -file %s -device pvc -options "-igc_opts 'DumpVISAASMToConsole=1' \
// RUN: -DINPUT_TYPE=ushort -DOUTPUT_TYPE=ushort -DFUNCTION=intel_subgroup_block_write_cacheopts_u8_m1k32v1 -DINPUT_WIDTH=512 -DINPUT_HEIGHT=46 -DPITCH=512 \
// RUN: -DCACHE_CONTROL=write_cache_control_l1_writethrough_l3_uncached_intel" \
// RUN: -internal_options "-cl-ext=-all,+cl_intel_subgroup_extended_block_write_cacheopts" | FileCheck %s --check-prefix=CHECK-WT-UC

// CHECK-WT-UC: lsc_store_block2d.ugm.wt.uc

// RUN: ocloc compile -file %s -device pvc -options "-igc_opts 'DumpVISAASMToConsole=1' \
// RUN: -DINPUT_TYPE=ushort -DOUTPUT_TYPE=ushort -DFUNCTION=intel_subgroup_block_write_cacheopts_u8_m1k32v1 -DINPUT_WIDTH=512 -DINPUT_HEIGHT=46 -DPITCH=512 \
// RUN: -DCACHE_CONTROL=write_cache_control_l1_writethrough_l3_writeback_intel" \
// RUN: -internal_options "-cl-ext=-all,+cl_intel_subgroup_extended_block_write_cacheopts" | FileCheck %s --check-prefix=CHECK-WT-WB

// CHECK-WT-WB: lsc_store_block2d.ugm.wt.wb

// RUN: ocloc compile -file %s -device pvc -options "-igc_opts 'DumpVISAASMToConsole=1' \
// RUN: -DINPUT_TYPE=ushort -DOUTPUT_TYPE=ushort -DFUNCTION=intel_subgroup_block_write_cacheopts_u8_m1k32v1 -DINPUT_WIDTH=512 -DINPUT_HEIGHT=46 -DPITCH=512 \
// RUN: -DCACHE_CONTROL=write_cache_control_l1_streaming_l3_uncached_intel" \
// RUN: -internal_options "-cl-ext=-all,+cl_intel_subgroup_extended_block_write_cacheopts" | FileCheck %s --check-prefix=CHECK-ST-UC

// CHECK-ST-UC: lsc_store_block2d.ugm.st.uc

// RUN: ocloc compile -file %s -device pvc -options "-igc_opts 'DumpVISAASMToConsole=1' \
// RUN: -DINPUT_TYPE=ushort -DOUTPUT_TYPE=ushort -DFUNCTION=intel_subgroup_block_write_cacheopts_u8_m1k32v1 -DINPUT_WIDTH=512 -DINPUT_HEIGHT=46 -DPITCH=512 \
// RUN: -DCACHE_CONTROL=write_cache_control_l1_streaming_l3_writeback_intel" \
// RUN: -internal_options "-cl-ext=-all,+cl_intel_subgroup_extended_block_write_cacheopts" | FileCheck %s --check-prefix=CHECK-ST-WB

// CHECK-ST-WB: lsc_store_block2d.ugm.st.wb

// RUN: ocloc compile -file %s -device pvc -options "-igc_opts 'DumpVISAASMToConsole=1' \
// RUN: -DINPUT_TYPE=ushort -DOUTPUT_TYPE=ushort -DFUNCTION=intel_subgroup_block_write_cacheopts_u8_m1k32v1 -DINPUT_WIDTH=512 -DINPUT_HEIGHT=46 -DPITCH=512 \
// RUN: -DCACHE_CONTROL=write_cache_control_l1_writeback_l3_writeback_intel" \
// RUN: -internal_options "-cl-ext=-all,+cl_intel_subgroup_extended_block_write_cacheopts" | FileCheck %s --check-prefix=CHECK-WB-WB

// CHECK-WB-WB: lsc_store_block2d.ugm.wb.wb

__attribute__((intel_reqd_sub_group_size(16))) kernel void
test(global INPUT_TYPE *input, const global int2 *coord, global OUTPUT_TYPE *output) {
  const int tid = get_global_id(0);
  FUNCTION(output, INPUT_WIDTH, INPUT_HEIGHT, PITCH, *coord, input[tid], CACHE_CONTROL);
}

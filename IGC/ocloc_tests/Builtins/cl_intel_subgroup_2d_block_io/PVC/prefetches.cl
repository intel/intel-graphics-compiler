/*========================== begin_copyright_notice ============================

Copyright (C) 2024 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

// REQUIRES: regkeys, pvc-supported

// RUN: ocloc compile -file %s -device pvc -options "-igc_opts 'DumpVISAASMToConsole=1' \
// RUN: -DINPUT_TYPE=ushort -DFUNCTION=intel_sub_group_2d_block_prefetch_8b_1r32x2c" \
// RUN: -internal_options "-cl-ext=-all,+cl_intel_subgroup_2d_block_io" | FileCheck %s --check-prefix=CHECK-VISAASM-8B-1R-32X2C

// CHECK-VISAASM-8B-1R-32X2C: lsc_load_block2d.ugm (M1, 1)  %null:d8.64x1nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}

// RUN: ocloc compile -file %s -device pvc -options "-igc_opts 'DumpVISAASMToConsole=1' \
// RUN: -DINPUT_TYPE=ushort -DFUNCTION=intel_sub_group_2d_block_prefetch_8b_2r32x2c" \
// RUN: -internal_options "-cl-ext=-all,+cl_intel_subgroup_2d_block_io" | FileCheck %s --check-prefix=CHECK-VISAASM-8B-2R-32X2C

// CHECK-VISAASM-8B-2R-32X2C: lsc_load_block2d.ugm (M1, 1)  %null:d8.64x2nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}

// RUN: ocloc compile -file %s -device pvc -options "-igc_opts 'DumpVISAASMToConsole=1' \
// RUN: -DINPUT_TYPE=ushort -DFUNCTION=intel_sub_group_2d_block_prefetch_8b_4r32x2c" \
// RUN: -internal_options "-cl-ext=-all,+cl_intel_subgroup_2d_block_io" | FileCheck %s --check-prefix=CHECK-VISAASM-8B-4R-32X2C

// CHECK-VISAASM-8B-4R-32X2C: lsc_load_block2d.ugm (M1, 1)  %null:d8.64x4nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}

// RUN: ocloc compile -file %s -device pvc -options "-igc_opts 'DumpVISAASMToConsole=1' \
// RUN: -DINPUT_TYPE=ushort -DFUNCTION=intel_sub_group_2d_block_prefetch_8b_8r32x2c" \
// RUN: -internal_options "-cl-ext=-all,+cl_intel_subgroup_2d_block_io" | FileCheck %s --check-prefix=CHECK-VISAASM-8B-8R-32X2C

// CHECK-VISAASM-8B-8R-32X2C: lsc_load_block2d.ugm (M1, 1)  %null:d8.64x8nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}

// RUN: ocloc compile -file %s -device pvc -options "-igc_opts 'DumpVISAASMToConsole=1' \
// RUN: -DINPUT_TYPE=ushort -DFUNCTION=intel_sub_group_2d_block_prefetch_8b_16r32x2c" \
// RUN: -internal_options "-cl-ext=-all,+cl_intel_subgroup_2d_block_io" | FileCheck %s --check-prefix=CHECK-VISAASM-8B-16R-32X2C

// CHECK-VISAASM-8B-16R-32X2C: lsc_load_block2d.ugm (M1, 1)  %null:d8.64x16nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}

// RUN: ocloc compile -file %s -device pvc -options "-igc_opts 'DumpVISAASMToConsole=1' \
// RUN: -DINPUT_TYPE=ushort -DFUNCTION=intel_sub_group_2d_block_prefetch_8b_32r32x2c" \
// RUN: -internal_options "-cl-ext=-all,+cl_intel_subgroup_2d_block_io" | FileCheck %s --check-prefix=CHECK-VISAASM-8B-32R-32X2C

// CHECK-VISAASM-8B-32R-32X2C: lsc_load_block2d.ugm (M1, 1)  %null:d8.64x32nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}

// RUN: ocloc compile -file %s -device pvc -options "-igc_opts 'DumpVISAASMToConsole=1' \
// RUN: -DINPUT_TYPE=ushort -DFUNCTION=intel_sub_group_2d_block_prefetch_16b_1r16x1c" \
// RUN: -internal_options "-cl-ext=-all,+cl_intel_subgroup_2d_block_io" | FileCheck %s --check-prefix=CHECK-VISAASM-16B-1R-16X1C

// CHECK-VISAASM-16B-1R-16X1C: lsc_load_block2d.ugm (M1, 1)  %null:d16.16x1nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}

// RUN: ocloc compile -file %s -device pvc -options "-igc_opts 'DumpVISAASMToConsole=1' \
// RUN: -DINPUT_TYPE=ushort -DFUNCTION=intel_sub_group_2d_block_prefetch_16b_2r16x1c" \
// RUN: -internal_options "-cl-ext=-all,+cl_intel_subgroup_2d_block_io" | FileCheck %s --check-prefix=CHECK-VISAASM-16B-2R-16X1C

// CHECK-VISAASM-16B-2R-16X1C: lsc_load_block2d.ugm (M1, 1)  %null:d16.16x2nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}

// RUN: ocloc compile -file %s -device pvc -options "-igc_opts 'DumpVISAASMToConsole=1' \
// RUN: -DINPUT_TYPE=ushort -DFUNCTION=intel_sub_group_2d_block_prefetch_16b_4r16x1c" \
// RUN: -internal_options "-cl-ext=-all,+cl_intel_subgroup_2d_block_io" | FileCheck %s --check-prefix=CHECK-VISAASM-16B-4R-16X1C

// CHECK-VISAASM-16B-4R-16X1C: lsc_load_block2d.ugm (M1, 1)  %null:d16.16x4nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}

// RUN: ocloc compile -file %s -device pvc -options "-igc_opts 'DumpVISAASMToConsole=1' \
// RUN: -DINPUT_TYPE=ushort -DFUNCTION=intel_sub_group_2d_block_prefetch_16b_8r16x1c" \
// RUN: -internal_options "-cl-ext=-all,+cl_intel_subgroup_2d_block_io" | FileCheck %s --check-prefix=CHECK-VISAASM-16B-8R-16X1C

// CHECK-VISAASM-16B-8R-16X1C: lsc_load_block2d.ugm (M1, 1)  %null:d16.16x8nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}

// RUN: ocloc compile -file %s -device pvc -options "-igc_opts 'DumpVISAASMToConsole=1' \
// RUN: -DINPUT_TYPE=ushort -DFUNCTION=intel_sub_group_2d_block_prefetch_16b_16r16x1c" \
// RUN: -internal_options "-cl-ext=-all,+cl_intel_subgroup_2d_block_io" | FileCheck %s --check-prefix=CHECK-VISAASM-16B-16R-16X1C

// CHECK-VISAASM-16B-16R-16X1C: lsc_load_block2d.ugm (M1, 1)  %null:d16.16x16nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}

// RUN: ocloc compile -file %s -device pvc -options "-igc_opts 'DumpVISAASMToConsole=1' \
// RUN: -DINPUT_TYPE=ushort -DFUNCTION=intel_sub_group_2d_block_prefetch_16b_32r16x1c" \
// RUN: -internal_options "-cl-ext=-all,+cl_intel_subgroup_2d_block_io" | FileCheck %s --check-prefix=CHECK-VISAASM-16B-32R-16X1C

// CHECK-VISAASM-16B-32R-16X1C: lsc_load_block2d.ugm (M1, 1)  %null:d16.16x32nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}

// RUN: ocloc compile -file %s -device pvc -options "-igc_opts 'DumpVISAASMToConsole=1' \
// RUN: -DINPUT_TYPE=ushort -DFUNCTION=intel_sub_group_2d_block_prefetch_16b_1r16x2c" \
// RUN: -internal_options "-cl-ext=-all,+cl_intel_subgroup_2d_block_io" | FileCheck %s --check-prefix=CHECK-VISAASM-16B-1R-16X2C

// CHECK-VISAASM-16B-1R-16X2C: lsc_load_block2d.ugm (M1, 1)  %null:d16.32x1nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}

// RUN: ocloc compile -file %s -device pvc -options "-igc_opts 'DumpVISAASMToConsole=1' \
// RUN: -DINPUT_TYPE=ushort -DFUNCTION=intel_sub_group_2d_block_prefetch_16b_2r16x2c" \
// RUN: -internal_options "-cl-ext=-all,+cl_intel_subgroup_2d_block_io" | FileCheck %s --check-prefix=CHECK-VISAASM-16B-2R-16X2C

// CHECK-VISAASM-16B-2R-16X2C: lsc_load_block2d.ugm (M1, 1)  %null:d16.32x2nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}

// RUN: ocloc compile -file %s -device pvc -options "-igc_opts 'DumpVISAASMToConsole=1' \
// RUN: -DINPUT_TYPE=ushort -DFUNCTION=intel_sub_group_2d_block_prefetch_16b_4r16x2c" \
// RUN: -internal_options "-cl-ext=-all,+cl_intel_subgroup_2d_block_io" | FileCheck %s --check-prefix=CHECK-VISAASM-16B-4R-16X2C

// CHECK-VISAASM-16B-4R-16X2C: lsc_load_block2d.ugm (M1, 1)  %null:d16.32x4nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}

// RUN: ocloc compile -file %s -device pvc -options "-igc_opts 'DumpVISAASMToConsole=1' \
// RUN: -DINPUT_TYPE=ushort -DFUNCTION=intel_sub_group_2d_block_prefetch_16b_8r16x2c" \
// RUN: -internal_options "-cl-ext=-all,+cl_intel_subgroup_2d_block_io" | FileCheck %s --check-prefix=CHECK-VISAASM-16B-8R-16X2C

// CHECK-VISAASM-16B-8R-16X2C: lsc_load_block2d.ugm (M1, 1)  %null:d16.32x8nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}

// RUN: ocloc compile -file %s -device pvc -options "-igc_opts 'DumpVISAASMToConsole=1' \
// RUN: -DINPUT_TYPE=ushort -DFUNCTION=intel_sub_group_2d_block_prefetch_16b_16r16x2c" \
// RUN: -internal_options "-cl-ext=-all,+cl_intel_subgroup_2d_block_io" | FileCheck %s --check-prefix=CHECK-VISAASM-16B-16R-16X2C

// CHECK-VISAASM-16B-16R-16X2C: lsc_load_block2d.ugm (M1, 1)  %null:d16.32x16nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}

// RUN: ocloc compile -file %s -device pvc -options "-igc_opts 'DumpVISAASMToConsole=1' \
// RUN: -DINPUT_TYPE=ushort -DFUNCTION=intel_sub_group_2d_block_prefetch_16b_32r16x2c" \
// RUN: -internal_options "-cl-ext=-all,+cl_intel_subgroup_2d_block_io" | FileCheck %s --check-prefix=CHECK-VISAASM-16B-32R-16X2C

// CHECK-VISAASM-16B-32R-16X2C: lsc_load_block2d.ugm (M1, 1)  %null:d16.32x32nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}

// RUN: ocloc compile -file %s -device pvc -options "-igc_opts 'DumpVISAASMToConsole=1' \
// RUN: -DINPUT_TYPE=ushort -DFUNCTION=intel_sub_group_2d_block_prefetch_8b_32r16x1c" \
// RUN: -internal_options "-cl-ext=-all,+cl_intel_subgroup_2d_block_io" | FileCheck %s --check-prefix=CHECK-VISAASM-8B-32R-16X1C

// CHECK-VISAASM-8B-32R-16X1C: lsc_load_block2d.ugm (M1, 1)  %null:d8.16x32nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}

// RUN: ocloc compile -file %s -device pvc -options "-igc_opts 'DumpVISAASMToConsole=1' \
// RUN: -DINPUT_TYPE=ushort -DFUNCTION=intel_sub_group_2d_block_prefetch_8b_1r32x1c" \
// RUN: -internal_options "-cl-ext=-all,+cl_intel_subgroup_2d_block_io" | FileCheck %s --check-prefix=CHECK-VISAASM-8B-1R-32X1C

// CHECK-VISAASM-8B-1R-32X1C: lsc_load_block2d.ugm (M1, 1)  %null:d8.32x1nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}

// RUN: ocloc compile -file %s -device pvc -options "-igc_opts 'DumpVISAASMToConsole=1' \
// RUN: -DINPUT_TYPE=ushort -DFUNCTION=intel_sub_group_2d_block_prefetch_8b_2r32x1c" \
// RUN: -internal_options "-cl-ext=-all,+cl_intel_subgroup_2d_block_io" | FileCheck %s --check-prefix=CHECK-VISAASM-8B-2R-32X1C

// CHECK-VISAASM-8B-2R-32X1C: lsc_load_block2d.ugm (M1, 1)  %null:d8.32x2nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}

// RUN: ocloc compile -file %s -device pvc -options "-igc_opts 'DumpVISAASMToConsole=1' \
// RUN: -DINPUT_TYPE=ushort -DFUNCTION=intel_sub_group_2d_block_prefetch_8b_4r32x1c" \
// RUN: -internal_options "-cl-ext=-all,+cl_intel_subgroup_2d_block_io" | FileCheck %s --check-prefix=CHECK-VISAASM-8B-4R-32X1C

// CHECK-VISAASM-8B-4R-32X1C: lsc_load_block2d.ugm (M1, 1)  %null:d8.32x4nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}

// RUN: ocloc compile -file %s -device pvc -options "-igc_opts 'DumpVISAASMToConsole=1' \
// RUN: -DINPUT_TYPE=ushort -DFUNCTION=intel_sub_group_2d_block_prefetch_8b_8r32x1c" \
// RUN: -internal_options "-cl-ext=-all,+cl_intel_subgroup_2d_block_io" | FileCheck %s --check-prefix=CHECK-VISAASM-8B-8R-32X1C

// CHECK-VISAASM-8B-8R-32X1C: lsc_load_block2d.ugm (M1, 1)  %null:d8.32x8nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}

// RUN: ocloc compile -file %s -device pvc -options "-igc_opts 'DumpVISAASMToConsole=1' \
// RUN: -DINPUT_TYPE=ushort -DFUNCTION=intel_sub_group_2d_block_prefetch_8b_16r32x1c" \
// RUN: -internal_options "-cl-ext=-all,+cl_intel_subgroup_2d_block_io" | FileCheck %s --check-prefix=CHECK-VISAASM-8B-16R-32X1C

// CHECK-VISAASM-8B-16R-32X1C: lsc_load_block2d.ugm (M1, 1)  %null:d8.32x16nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}

// RUN: ocloc compile -file %s -device pvc -options "-igc_opts 'DumpVISAASMToConsole=1' \
// RUN: -DINPUT_TYPE=ushort -DFUNCTION=intel_sub_group_2d_block_prefetch_8b_32r32x1c" \
// RUN: -internal_options "-cl-ext=-all,+cl_intel_subgroup_2d_block_io" | FileCheck %s --check-prefix=CHECK-VISAASM-8B-32R-32X1C

// CHECK-VISAASM-8B-32R-32X1C: lsc_load_block2d.ugm (M1, 1)  %null:d8.32x32nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}

// RUN: ocloc compile -file %s -device pvc -options "-igc_opts 'DumpVISAASMToConsole=1' \
// RUN: -DINPUT_TYPE=uint -DFUNCTION=intel_sub_group_2d_block_prefetch_32b_1r8x1c" \
// RUN: -internal_options "-cl-ext=-all,+cl_intel_subgroup_2d_block_io" | FileCheck %s --check-prefix=CHECK-VISAASM-32B-1R-8X1C

// CHECK-VISAASM-32B-1R-8X1C: lsc_load_block2d.ugm (M1, 1)  %null:d32.8x1nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}

// RUN: ocloc compile -file %s -device pvc -options "-igc_opts 'DumpVISAASMToConsole=1' \
// RUN: -DINPUT_TYPE=uint -DFUNCTION=intel_sub_group_2d_block_prefetch_32b_2r8x1c" \
// RUN: -internal_options "-cl-ext=-all,+cl_intel_subgroup_2d_block_io" | FileCheck %s --check-prefix=CHECK-VISAASM-32B-2R-8X1C

// CHECK-VISAASM-32B-2R-8X1C: lsc_load_block2d.ugm (M1, 1)  %null:d32.8x2nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}

// RUN: ocloc compile -file %s -device pvc -options "-igc_opts 'DumpVISAASMToConsole=1' \
// RUN: -DINPUT_TYPE=uint -DFUNCTION=intel_sub_group_2d_block_prefetch_32b_4r8x1c" \
// RUN: -internal_options "-cl-ext=-all,+cl_intel_subgroup_2d_block_io" | FileCheck %s --check-prefix=CHECK-VISAASM-32B-4R-8X1C

// CHECK-VISAASM-32B-4R-8X1C: lsc_load_block2d.ugm (M1, 1)  %null:d32.8x4nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}

// RUN: ocloc compile -file %s -device pvc -options "-igc_opts 'DumpVISAASMToConsole=1' \
// RUN: -DINPUT_TYPE=uint -DFUNCTION=intel_sub_group_2d_block_prefetch_32b_8r8x1c" \
// RUN: -internal_options "-cl-ext=-all,+cl_intel_subgroup_2d_block_io" | FileCheck %s --check-prefix=CHECK-VISAASM-32B-8R-8X1C

// CHECK-VISAASM-32B-8R-8X1C: lsc_load_block2d.ugm (M1, 1)  %null:d32.8x8nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}

// RUN: ocloc compile -file %s -device pvc -options "-igc_opts 'DumpVISAASMToConsole=1' \
// RUN: -DINPUT_TYPE=uint -DFUNCTION=intel_sub_group_2d_block_prefetch_32b_16r8x1c" \
// RUN: -internal_options "-cl-ext=-all,+cl_intel_subgroup_2d_block_io" | FileCheck %s --check-prefix=CHECK-VISAASM-32B-16R-8X1C

// CHECK-VISAASM-32B-16R-8X1C: lsc_load_block2d.ugm (M1, 1)  %null:d32.8x16nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}

// RUN: ocloc compile -file %s -device pvc -options "-igc_opts 'DumpVISAASMToConsole=1' \
// RUN: -DINPUT_TYPE=uint -DFUNCTION=intel_sub_group_2d_block_prefetch_32b_32r8x1c" \
// RUN: -internal_options "-cl-ext=-all,+cl_intel_subgroup_2d_block_io" | FileCheck %s --check-prefix=CHECK-VISAASM-32B-32R-8X1C

// CHECK-VISAASM-32B-32R-8X1C: lsc_load_block2d.ugm (M1, 1)  %null:d32.8x32nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}

// RUN: ocloc compile -file %s -device pvc -options "-igc_opts 'DumpVISAASMToConsole=1' \
// RUN: -DINPUT_TYPE=uint -DFUNCTION=intel_sub_group_2d_block_prefetch_32b_1r16x1c" \
// RUN: -internal_options "-cl-ext=-all,+cl_intel_subgroup_2d_block_io" | FileCheck %s --check-prefix=CHECK-VISAASM-32B-1R-16X1C

// CHECK-VISAASM-32B-1R-16X1C: lsc_load_block2d.ugm (M1, 1)  %null:d32.16x1nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}

// RUN: ocloc compile -file %s -device pvc -options "-igc_opts 'DumpVISAASMToConsole=1' \
// RUN: -DINPUT_TYPE=uint -DFUNCTION=intel_sub_group_2d_block_prefetch_32b_2r16x1c" \
// RUN: -internal_options "-cl-ext=-all,+cl_intel_subgroup_2d_block_io" | FileCheck %s --check-prefix=CHECK-VISAASM-32B-2R-16X1C

// CHECK-VISAASM-32B-2R-16X1C: lsc_load_block2d.ugm (M1, 1)  %null:d32.16x2nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}

// RUN: ocloc compile -file %s -device pvc -options "-igc_opts 'DumpVISAASMToConsole=1' \
// RUN: -DINPUT_TYPE=uint -DFUNCTION=intel_sub_group_2d_block_prefetch_32b_4r16x1c" \
// RUN: -internal_options "-cl-ext=-all,+cl_intel_subgroup_2d_block_io" | FileCheck %s --check-prefix=CHECK-VISAASM-32B-4R-16X1C

// CHECK-VISAASM-32B-4R-16X1C: lsc_load_block2d.ugm (M1, 1)  %null:d32.16x4nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}

// RUN: ocloc compile -file %s -device pvc -options "-igc_opts 'DumpVISAASMToConsole=1' \
// RUN: -DINPUT_TYPE=uint -DFUNCTION=intel_sub_group_2d_block_prefetch_32b_8r16x1c" \
// RUN: -internal_options "-cl-ext=-all,+cl_intel_subgroup_2d_block_io" | FileCheck %s --check-prefix=CHECK-VISAASM-32B-8R-16X1C

// CHECK-VISAASM-32B-8R-16X1C: lsc_load_block2d.ugm (M1, 1)  %null:d32.16x8nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}

// RUN: ocloc compile -file %s -device pvc -options "-igc_opts 'DumpVISAASMToConsole=1' \
// RUN: -DINPUT_TYPE=uint -DFUNCTION=intel_sub_group_2d_block_prefetch_32b_16r16x1c" \
// RUN: -internal_options "-cl-ext=-all,+cl_intel_subgroup_2d_block_io" | FileCheck %s --check-prefix=CHECK-VISAASM-32B-16R-16X1C

// CHECK-VISAASM-32B-16R-16X1C: lsc_load_block2d.ugm (M1, 1)  %null:d32.16x16nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}

// RUN: ocloc compile -file %s -device pvc -options "-igc_opts 'DumpVISAASMToConsole=1' \
// RUN: -DINPUT_TYPE=uint -DFUNCTION=intel_sub_group_2d_block_prefetch_32b_32r16x1c" \
// RUN: -internal_options "-cl-ext=-all,+cl_intel_subgroup_2d_block_io" | FileCheck %s --check-prefix=CHECK-VISAASM-32B-32R-16X1C

// CHECK-VISAASM-32B-32R-16X1C: lsc_load_block2d.ugm (M1, 1)  %null:d32.16x32nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}

// RUN: ocloc compile -file %s -device pvc -options "-igc_opts 'DumpVISAASMToConsole=1' \
// RUN: -DINPUT_TYPE=uint -DFUNCTION=intel_sub_group_2d_block_prefetch_32b_1r8x2c" \
// RUN: -internal_options "-cl-ext=-all,+cl_intel_subgroup_2d_block_io" | FileCheck %s --check-prefix=CHECK-VISAASM-32B-1R-8X2C

// CHECK-VISAASM-32B-1R-8X2C: lsc_load_block2d.ugm (M1, 1)  %null:d32.16x1nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}

// RUN: ocloc compile -file %s -device pvc -options "-igc_opts 'DumpVISAASMToConsole=1' \
// RUN: -DINPUT_TYPE=uint -DFUNCTION=intel_sub_group_2d_block_prefetch_32b_2r8x2c" \
// RUN: -internal_options "-cl-ext=-all,+cl_intel_subgroup_2d_block_io" | FileCheck %s --check-prefix=CHECK-VISAASM-32B-2R-8X2C

// CHECK-VISAASM-32B-2R-8X2C: lsc_load_block2d.ugm (M1, 1)  %null:d32.16x2nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}

// RUN: ocloc compile -file %s -device pvc -options "-igc_opts 'DumpVISAASMToConsole=1' \
// RUN: -DINPUT_TYPE=uint -DFUNCTION=intel_sub_group_2d_block_prefetch_32b_4r8x2c" \
// RUN: -internal_options "-cl-ext=-all,+cl_intel_subgroup_2d_block_io" | FileCheck %s --check-prefix=CHECK-VISAASM-32B-4R-8X2C

// CHECK-VISAASM-32B-4R-8X2C: lsc_load_block2d.ugm (M1, 1)  %null:d32.16x4nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}

// RUN: ocloc compile -file %s -device pvc -options "-igc_opts 'DumpVISAASMToConsole=1' \
// RUN: -DINPUT_TYPE=uint -DFUNCTION=intel_sub_group_2d_block_prefetch_32b_8r8x2c" \
// RUN: -internal_options "-cl-ext=-all,+cl_intel_subgroup_2d_block_io" | FileCheck %s --check-prefix=CHECK-VISAASM-32B-8R-8X2C

// CHECK-VISAASM-32B-8R-8X2C: lsc_load_block2d.ugm (M1, 1)  %null:d32.16x8nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}

// RUN: ocloc compile -file %s -device pvc -options "-igc_opts 'DumpVISAASMToConsole=1' \
// RUN: -DINPUT_TYPE=uint -DFUNCTION=intel_sub_group_2d_block_prefetch_32b_16r8x2c" \
// RUN: -internal_options "-cl-ext=-all,+cl_intel_subgroup_2d_block_io" | FileCheck %s --check-prefix=CHECK-VISAASM-32B-16R-8X2C

// CHECK-VISAASM-32B-16R-8X2C: lsc_load_block2d.ugm (M1, 1)  %null:d32.16x16nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}

// RUN: ocloc compile -file %s -device pvc -options "-igc_opts 'DumpVISAASMToConsole=1' \
// RUN: -DINPUT_TYPE=uint -DFUNCTION=intel_sub_group_2d_block_prefetch_32b_32r8x2c" \
// RUN: -internal_options "-cl-ext=-all,+cl_intel_subgroup_2d_block_io" | FileCheck %s --check-prefix=CHECK-VISAASM-32B-32R-8X2C

// CHECK-VISAASM-32B-32R-8X2C: lsc_load_block2d.ugm (M1, 1)  %null:d32.16x32nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}

// RUN: ocloc compile -file %s -device pvc -options "-igc_opts 'DumpVISAASMToConsole=1' \
// RUN: -DINPUT_TYPE=uint -DFUNCTION=intel_sub_group_2d_block_prefetch_8b_32r16x2c" \
// RUN: -internal_options "-cl-ext=-all,+cl_intel_subgroup_2d_block_io" | FileCheck %s --check-prefix=CHECK-VISAASM-8B-32R-16X2C

// CHECK-VISAASM-8B-32R-16X2C: lsc_load_block2d.ugm (M1, 1)  %null:d8.32x32nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}

// RUN: ocloc compile -file %s -device pvc -options "-igc_opts 'DumpVISAASMToConsole=1' \
// RUN: -DINPUT_TYPE=uint -DFUNCTION=intel_sub_group_2d_block_prefetch_8b_32r16x4c" \
// RUN: -internal_options "-cl-ext=-all,+cl_intel_subgroup_2d_block_io" | FileCheck %s --check-prefix=CHECK-VISAASM-8B-32R-16X4C

// CHECK-VISAASM-8B-32R-16X4C: lsc_load_block2d.ugm (M1, 1)  %null:d8.64x32nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}

__attribute__((intel_reqd_sub_group_size(16)))
kernel void test_default(global INPUT_TYPE* input, const global int2* coord) {
    FUNCTION(input, 512, 46, 512, *coord);
}

/*========================== begin_copyright_notice ============================

Copyright (C) 2024 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

// REQUIRES: regkeys, pvc-supported

// RUN: ocloc compile -file %s -device pvc -options "-igc_opts 'DumpVISAASMToConsole=1' \
// RUN: -DINPUT_TYPE=ushort -DOUTPUT_TYPE=ushort -DFUNCTION=intel_sub_group_2d_block_read_8b_1r32x2c -DDST_ARRAY_EL_TYPE=ushort -DDST_ARRAY_EL_NUM=2" \
// RUN: -internal_options "-cl-ext=-all,+cl_intel_subgroup_2d_block_io" | FileCheck %s --check-prefix=CHECK-VISAASM-8B-1R-32X2C

// CHECK-VISAASM-8B-1R-32X2C: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d8.2x32x1nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}

// RUN: ocloc compile -file %s -device pvc -options "-igc_opts 'DumpVISAASMToConsole=1' \
// RUN: -DINPUT_TYPE=ushort -DOUTPUT_TYPE=ushort -DFUNCTION=intel_sub_group_2d_block_read_8b_2r32x2c -DDST_ARRAY_EL_TYPE=ushort2 -DDST_ARRAY_EL_NUM=2" \
// RUN: -internal_options "-cl-ext=-all,+cl_intel_subgroup_2d_block_io" | FileCheck %s --check-prefix=CHECK-VISAASM-8B-2R-32X2C

// CHECK-VISAASM-8B-2R-32X2C: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d8.2x32x2nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}

// RUN: ocloc compile -file %s -device pvc -options "-igc_opts 'DumpVISAASMToConsole=1' \
// RUN: -DINPUT_TYPE=ushort -DOUTPUT_TYPE=ushort -DFUNCTION=intel_sub_group_2d_block_read_8b_4r32x2c -DDST_ARRAY_EL_TYPE=ushort4 -DDST_ARRAY_EL_NUM=2" \
// RUN: -internal_options "-cl-ext=-all,+cl_intel_subgroup_2d_block_io" | FileCheck %s --check-prefix=CHECK-VISAASM-8B-4R-32X2C

// CHECK-VISAASM-8B-4R-32X2C: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d8.2x32x4nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}

// RUN: ocloc compile -file %s -device pvc -options "-igc_opts 'DumpVISAASMToConsole=1' \
// RUN: -DINPUT_TYPE=ushort -DOUTPUT_TYPE=ushort -DFUNCTION=intel_sub_group_2d_block_read_8b_8r32x2c -DDST_ARRAY_EL_TYPE=ushort8 -DDST_ARRAY_EL_NUM=2" \
// RUN: -internal_options "-cl-ext=-all,+cl_intel_subgroup_2d_block_io" | FileCheck %s --check-prefix=CHECK-VISAASM-8B-8R-32X2C

// CHECK-VISAASM-8B-8R-32X2C: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d8.2x32x8nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}

// RUN: ocloc compile -file %s -device pvc -options "-igc_opts 'DumpVISAASMToConsole=1' \
// RUN: -DINPUT_TYPE=ushort -DOUTPUT_TYPE=ushort -DFUNCTION=intel_sub_group_2d_block_read_8b_16r32x2c -DDST_ARRAY_EL_TYPE=ushort8 -DDST_ARRAY_EL_NUM=4" \
// RUN: -internal_options "-cl-ext=-all,+cl_intel_subgroup_2d_block_io" | FileCheck %s --check-prefix=CHECK-VISAASM-8B-16R-32X2C

// CHECK-VISAASM-8B-16R-32X2C: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d8.2x32x16nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}

// RUN: ocloc compile -file %s -device pvc -options "-igc_opts 'DumpVISAASMToConsole=1' \
// RUN: -DINPUT_TYPE=ushort -DOUTPUT_TYPE=ushort -DFUNCTION=intel_sub_group_2d_block_read_8b_32r32x2c -DDST_ARRAY_EL_TYPE=ushort8 -DDST_ARRAY_EL_NUM=8" \
// RUN: -internal_options "-cl-ext=-all,+cl_intel_subgroup_2d_block_io" | FileCheck %s --check-prefix=CHECK-VISAASM-8B-32R-32X2C

// CHECK-VISAASM-8B-32R-32X2C: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d8.2x32x32nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}

// RUN: ocloc compile -file %s -device pvc -options "-igc_opts 'DumpVISAASMToConsole=1' \
// RUN: -DINPUT_TYPE=uchar -DOUTPUT_TYPE=uchar -DFUNCTION=intel_sub_group_2d_block_read_8b_8r16x4c -DDST_ARRAY_EL_TYPE=uchar -DDST_ARRAY_EL_NUM=32" \
// RUN: -internal_options "-cl-ext=-all,+cl_intel_subgroup_2d_block_io" | FileCheck %s --check-prefix=CHECK-VISAASM-8B-8R-16X4C

// CHECK-VISAASM-8B-8R-16X4C: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d8.4x16x8nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}

// RUN: ocloc compile -file %s -device pvc -options "-igc_opts 'DumpVISAASMToConsole=1' \
// RUN: -DINPUT_TYPE=uchar -DOUTPUT_TYPE=uchar -DFUNCTION=intel_sub_group_2d_block_read_8b_16r16x4c -DDST_ARRAY_EL_TYPE=uchar -DDST_ARRAY_EL_NUM=64" \
// RUN: -internal_options "-cl-ext=-all,+cl_intel_subgroup_2d_block_io" | FileCheck %s --check-prefix=CHECK-VISAASM-8B-16R-16X4C

// CHECK-VISAASM-8B-16R-16X4C: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d8.4x16x16nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}

// RUN: ocloc compile -file %s -device pvc -options "-igc_opts 'DumpVISAASMToConsole=1' \
// RUN: -DINPUT_TYPE=uchar -DOUTPUT_TYPE=uchar -DFUNCTION=intel_sub_group_2d_block_read_8b_32r16x4c -DDST_ARRAY_EL_TYPE=uchar -DDST_ARRAY_EL_NUM=128" \
// RUN: -internal_options "-cl-ext=-all,+cl_intel_subgroup_2d_block_io" | FileCheck %s --check-prefix=CHECK-VISAASM-8B-32R-16X4C

// CHECK-VISAASM-8B-32R-16X4C: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d8.4x16x32nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}

// RUN: ocloc compile -file %s -device pvc -options "-igc_opts 'DumpVISAASMToConsole=1' \
// RUN: -DINPUT_TYPE=ushort -DOUTPUT_TYPE=ushort -DFUNCTION=intel_sub_group_2d_block_read_16b_1r16x1c -DDST_ARRAY_EL_TYPE=ushort -DDST_ARRAY_EL_NUM=1" \
// RUN: -internal_options "-cl-ext=-all,+cl_intel_subgroup_2d_block_io" | FileCheck %s --check-prefix=CHECK-VISAASM-16B-1R-16X1C

// CHECK-VISAASM-16B-1R-16X1C: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d16.16x1nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}

// RUN: ocloc compile -file %s -device pvc -options "-igc_opts 'DumpVISAASMToConsole=1' \
// RUN: -DINPUT_TYPE=ushort -DOUTPUT_TYPE=ushort -DFUNCTION=intel_sub_group_2d_block_read_16b_2r16x1c -DDST_ARRAY_EL_TYPE=ushort2 -DDST_ARRAY_EL_NUM=1" \
// RUN: -internal_options "-cl-ext=-all,+cl_intel_subgroup_2d_block_io" | FileCheck %s --check-prefix=CHECK-VISAASM-16B-2R-16X1C

// CHECK-VISAASM-16B-2R-16X1C: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d16.16x2nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}

// RUN: ocloc compile -file %s -device pvc -options "-igc_opts 'DumpVISAASMToConsole=1' \
// RUN: -DINPUT_TYPE=ushort -DOUTPUT_TYPE=ushort -DFUNCTION=intel_sub_group_2d_block_read_16b_4r16x1c -DDST_ARRAY_EL_TYPE=ushort4 -DDST_ARRAY_EL_NUM=1" \
// RUN: -internal_options "-cl-ext=-all,+cl_intel_subgroup_2d_block_io" | FileCheck %s --check-prefix=CHECK-VISAASM-16B-4R-16X1C

// CHECK-VISAASM-16B-4R-16X1C: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d16.16x4nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}

// RUN: ocloc compile -file %s -device pvc -options "-igc_opts 'DumpVISAASMToConsole=1' \
// RUN: -DINPUT_TYPE=ushort -DOUTPUT_TYPE=ushort -DFUNCTION=intel_sub_group_2d_block_read_16b_8r16x1c -DDST_ARRAY_EL_TYPE=ushort8 -DDST_ARRAY_EL_NUM=1" \
// RUN: -internal_options "-cl-ext=-all,+cl_intel_subgroup_2d_block_io" | FileCheck %s --check-prefix=CHECK-VISAASM-16B-8R-16X1C

// CHECK-VISAASM-16B-8R-16X1C: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d16.16x8nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}

// RUN: ocloc compile -file %s -device pvc -options "-igc_opts 'DumpVISAASMToConsole=1' \
// RUN: -DINPUT_TYPE=ushort -DOUTPUT_TYPE=ushort -DFUNCTION=intel_sub_group_2d_block_read_16b_16r16x1c -DDST_ARRAY_EL_TYPE=ushort8 -DDST_ARRAY_EL_NUM=2" \
// RUN: -internal_options "-cl-ext=-all,+cl_intel_subgroup_2d_block_io" | FileCheck %s --check-prefix=CHECK-VISAASM-16B-16R-16X1C

// CHECK-VISAASM-16B-16R-16X1C: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d16.16x16nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}

// RUN: ocloc compile -file %s -device pvc -options "-igc_opts 'DumpVISAASMToConsole=1' \
// RUN: -DINPUT_TYPE=ushort -DOUTPUT_TYPE=ushort -DFUNCTION=intel_sub_group_2d_block_read_16b_32r16x1c -DDST_ARRAY_EL_TYPE=ushort8 -DDST_ARRAY_EL_NUM=4" \
// RUN: -internal_options "-cl-ext=-all,+cl_intel_subgroup_2d_block_io" | FileCheck %s --check-prefix=CHECK-VISAASM-16B-32R-16X1C

// CHECK-VISAASM-16B-32R-16X1C: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d16.16x32nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}

// RUN: ocloc compile -file %s -device pvc -options "-igc_opts 'DumpVISAASMToConsole=1' \
// RUN: -DINPUT_TYPE=ushort -DOUTPUT_TYPE=ushort -DFUNCTION=intel_sub_group_2d_block_read_16b_1r16x2c -DDST_ARRAY_EL_TYPE=ushort -DDST_ARRAY_EL_NUM=2" \
// RUN: -internal_options "-cl-ext=-all,+cl_intel_subgroup_2d_block_io" | FileCheck %s --check-prefix=CHECK-VISAASM-16B-1R-16X2C

// CHECK-VISAASM-16B-1R-16X2C: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d16.2x16x1nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}

// RUN: ocloc compile -file %s -device pvc -options "-igc_opts 'DumpVISAASMToConsole=1' \
// RUN: -DINPUT_TYPE=ushort -DOUTPUT_TYPE=ushort -DFUNCTION=intel_sub_group_2d_block_read_16b_2r16x2c -DDST_ARRAY_EL_TYPE=ushort2 -DDST_ARRAY_EL_NUM=2" \
// RUN: -internal_options "-cl-ext=-all,+cl_intel_subgroup_2d_block_io" | FileCheck %s --check-prefix=CHECK-VISAASM-16B-2R-16X2C

// CHECK-VISAASM-16B-2R-16X2C: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d16.2x16x2nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}

// RUN: ocloc compile -file %s -device pvc -options "-igc_opts 'DumpVISAASMToConsole=1' \
// RUN: -DINPUT_TYPE=ushort -DOUTPUT_TYPE=ushort -DFUNCTION=intel_sub_group_2d_block_read_16b_4r16x2c -DDST_ARRAY_EL_TYPE=ushort4 -DDST_ARRAY_EL_NUM=2" \
// RUN: -internal_options "-cl-ext=-all,+cl_intel_subgroup_2d_block_io" | FileCheck %s --check-prefix=CHECK-VISAASM-16B-4R-16X2C

// CHECK-VISAASM-16B-4R-16X2C: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d16.2x16x4nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}

// RUN: ocloc compile -file %s -device pvc -options "-igc_opts 'DumpVISAASMToConsole=1' \
// RUN: -DINPUT_TYPE=ushort -DOUTPUT_TYPE=ushort -DFUNCTION=intel_sub_group_2d_block_read_16b_8r16x2c -DDST_ARRAY_EL_TYPE=ushort8 -DDST_ARRAY_EL_NUM=2" \
// RUN: -internal_options "-cl-ext=-all,+cl_intel_subgroup_2d_block_io" | FileCheck %s --check-prefix=CHECK-VISAASM-16B-8R-16X2C

// CHECK-VISAASM-16B-8R-16X2C: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d16.2x16x8nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}

// RUN: ocloc compile -file %s -device pvc -options "-igc_opts 'DumpVISAASMToConsole=1' \
// RUN: -DINPUT_TYPE=ushort -DOUTPUT_TYPE=ushort -DFUNCTION=intel_sub_group_2d_block_read_16b_16r16x2c -DDST_ARRAY_EL_TYPE=ushort8 -DDST_ARRAY_EL_NUM=4" \
// RUN: -internal_options "-cl-ext=-all,+cl_intel_subgroup_2d_block_io" | FileCheck %s --check-prefix=CHECK-VISAASM-16B-16R-16X2C

// CHECK-VISAASM-16B-16R-16X2C: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d16.2x16x16nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}

// RUN: ocloc compile -file %s -device pvc -options "-igc_opts 'DumpVISAASMToConsole=1' \
// RUN: -DINPUT_TYPE=ushort -DOUTPUT_TYPE=ushort -DFUNCTION=intel_sub_group_2d_block_read_16b_32r16x2c -DDST_ARRAY_EL_TYPE=ushort8 -DDST_ARRAY_EL_NUM=8" \
// RUN: -internal_options "-cl-ext=-all,+cl_intel_subgroup_2d_block_io" | FileCheck %s --check-prefix=CHECK-VISAASM-16B-32R-16X2C

// CHECK-VISAASM-16B-32R-16X2C: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d16.2x16x32nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}

// RUN: ocloc compile -file %s -device pvc -options "-igc_opts 'DumpVISAASMToConsole=1' \
// RUN: -DINPUT_TYPE=ushort -DOUTPUT_TYPE=uint -DFUNCTION=intel_sub_group_2d_block_read_transform_8b_32r16x1c -DDST_ARRAY_EL_TYPE=uint8 -DDST_ARRAY_EL_NUM=1" \
// RUN: -internal_options "-cl-ext=-all,+cl_intel_subgroup_2d_block_io" | FileCheck %s --check-prefix=CHECK-VISAASM-TRANSFORM-8B-32R-16X1C

// CHECK-VISAASM-TRANSFORM-8B-32R-16X1C: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d8.16x32nt  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}

// RUN: ocloc compile -file %s -device pvc -options "-igc_opts 'DumpVISAASMToConsole=1' \
// RUN: -DINPUT_TYPE=ushort -DOUTPUT_TYPE=uint -DFUNCTION=intel_sub_group_2d_block_read_transform_16b_16r16x1c -DDST_ARRAY_EL_TYPE=uint8 -DDST_ARRAY_EL_NUM=1" \
// RUN: -internal_options "-cl-ext=-all,+cl_intel_subgroup_2d_block_io" | FileCheck %s --check-prefix=CHECK-VISAASM-TRANSFORM-16B-16R-16X1C

// CHECK-VISAASM-TRANSFORM-16B-16R-16X1C: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d16.16x16nt  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}

// RUN: ocloc compile -file %s -device pvc -options "-igc_opts 'DumpVISAASMToConsole=1' \
// RUN: -DINPUT_TYPE=ushort -DOUTPUT_TYPE=uint -DFUNCTION=intel_sub_group_2d_block_read_transpose_32b_16r8x1c -DDST_ARRAY_EL_TYPE=uint8 -DDST_ARRAY_EL_NUM=1" \
// RUN: -internal_options "-cl-ext=-all,+cl_intel_subgroup_2d_block_io" | FileCheck %s --check-prefix=CHECK-VISAASM-TRANSFORM-32B-16R-8X1C

// CHECK-VISAASM-TRANSFORM-32B-16R-8X1C: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d32.8x16tn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}

// RUN: ocloc compile -file %s -device pvc -options "-igc_opts 'DumpVISAASMToConsole=1' \
// RUN: -DINPUT_TYPE=uint -DOUTPUT_TYPE=uint -DFUNCTION=intel_sub_group_2d_block_read_transpose_32b_32r8x1c -DDST_ARRAY_EL_TYPE=uint16 -DDST_ARRAY_EL_NUM=1" \
// RUN: -internal_options "-cl-ext=-all,+cl_intel_subgroup_2d_block_io" | FileCheck %s --check-prefix=CHECK-VISAASM-TRANSFORM-32B-32R-8X1C

// CHECK-VISAASM-TRANSFORM-32B-32R-8X1C: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d32.8x32tn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}

// RUN: ocloc compile -file %s -device pvc -options "-igc_opts 'DumpVISAASMToConsole=1' \
// RUN: -DINPUT_TYPE=ushort -DOUTPUT_TYPE=ushort -DFUNCTION=intel_sub_group_2d_block_read_8b_1r32x1c -DDST_ARRAY_EL_TYPE=ushort -DDST_ARRAY_EL_NUM=1" \
// RUN: -internal_options "-cl-ext=-all,+cl_intel_subgroup_2d_block_io" | FileCheck %s --check-prefix=CHECK-VISAASM-8B-1R-32X1C

// CHECK-VISAASM-8B-1R-32X1C: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d8.32x1nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}

// RUN: ocloc compile -file %s -device pvc -options "-igc_opts 'DumpVISAASMToConsole=1' \
// RUN: -DINPUT_TYPE=ushort -DOUTPUT_TYPE=ushort -DFUNCTION=intel_sub_group_2d_block_read_8b_2r32x1c -DDST_ARRAY_EL_TYPE=ushort2 -DDST_ARRAY_EL_NUM=1" \
// RUN: -internal_options "-cl-ext=-all,+cl_intel_subgroup_2d_block_io" | FileCheck %s --check-prefix=CHECK-VISAASM-8B-2R-32X1C

// CHECK-VISAASM-8B-2R-32X1C: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d8.32x2nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}

// RUN: ocloc compile -file %s -device pvc -options "-igc_opts 'DumpVISAASMToConsole=1' \
// RUN: -DINPUT_TYPE=ushort -DOUTPUT_TYPE=ushort -DFUNCTION=intel_sub_group_2d_block_read_8b_4r32x1c -DDST_ARRAY_EL_TYPE=ushort4 -DDST_ARRAY_EL_NUM=1" \
// RUN: -internal_options "-cl-ext=-all,+cl_intel_subgroup_2d_block_io" | FileCheck %s --check-prefix=CHECK-VISAASM-8B-4R-32X1C

// CHECK-VISAASM-8B-4R-32X1C: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d8.32x4nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}

// RUN: ocloc compile -file %s -device pvc -options "-igc_opts 'DumpVISAASMToConsole=1' \
// RUN: -DINPUT_TYPE=ushort -DOUTPUT_TYPE=ushort -DFUNCTION=intel_sub_group_2d_block_read_8b_8r32x1c -DDST_ARRAY_EL_TYPE=ushort8 -DDST_ARRAY_EL_NUM=1" \
// RUN: -internal_options "-cl-ext=-all,+cl_intel_subgroup_2d_block_io" | FileCheck %s --check-prefix=CHECK-VISAASM-8B-8R-32X1C

// CHECK-VISAASM-8B-8R-32X1C: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d8.32x8nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}

// RUN: ocloc compile -file %s -device pvc -options "-igc_opts 'DumpVISAASMToConsole=1' \
// RUN: -DINPUT_TYPE=ushort -DOUTPUT_TYPE=ushort -DFUNCTION=intel_sub_group_2d_block_read_8b_16r32x1c -DDST_ARRAY_EL_TYPE=ushort8 -DDST_ARRAY_EL_NUM=2" \
// RUN: -internal_options "-cl-ext=-all,+cl_intel_subgroup_2d_block_io" | FileCheck %s --check-prefix=CHECK-VISAASM-8B-16R-32X1C

// CHECK-VISAASM-8B-16R-32X1C: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d8.32x16nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}

// RUN: ocloc compile -file %s -device pvc -options "-igc_opts 'DumpVISAASMToConsole=1' \
// RUN: -DINPUT_TYPE=ushort -DOUTPUT_TYPE=ushort -DFUNCTION=intel_sub_group_2d_block_read_8b_32r32x1c -DDST_ARRAY_EL_TYPE=ushort8 -DDST_ARRAY_EL_NUM=4" \
// RUN: -internal_options "-cl-ext=-all,+cl_intel_subgroup_2d_block_io" | FileCheck %s --check-prefix=CHECK-VISAASM-8B-32R-32X1C

// CHECK-VISAASM-8B-32R-32X1C: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d8.32x32nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}

// RUN: ocloc compile -file %s -device pvc -options "-igc_opts 'DumpVISAASMToConsole=1' \
// RUN: -DINPUT_TYPE=uint -DOUTPUT_TYPE=uint -DFUNCTION=intel_sub_group_2d_block_read_32b_1r8x1c -DDST_ARRAY_EL_TYPE=uint -DDST_ARRAY_EL_NUM=1" \
// RUN: -internal_options "-cl-ext=-all,+cl_intel_subgroup_2d_block_io" | FileCheck %s --check-prefix=CHECK-VISAASM-32B-1R-8X1C

// CHECK-VISAASM-32B-1R-8X1C: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d32.8x1nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}

// RUN: ocloc compile -file %s -device pvc -options "-igc_opts 'DumpVISAASMToConsole=1' \
// RUN: -DINPUT_TYPE=uint -DOUTPUT_TYPE=uint -DFUNCTION=intel_sub_group_2d_block_read_32b_2r8x1c -DDST_ARRAY_EL_TYPE=uint -DDST_ARRAY_EL_NUM=1" \
// RUN: -internal_options "-cl-ext=-all,+cl_intel_subgroup_2d_block_io" | FileCheck %s --check-prefix=CHECK-VISAASM-32B-2R-8X1C

// CHECK-VISAASM-32B-2R-8X1C: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d32.8x2nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}

// RUN: ocloc compile -file %s -device pvc -options "-igc_opts 'DumpVISAASMToConsole=1' \
// RUN: -DINPUT_TYPE=uint -DOUTPUT_TYPE=uint -DFUNCTION=intel_sub_group_2d_block_read_32b_4r8x1c -DDST_ARRAY_EL_TYPE=uint2 -DDST_ARRAY_EL_NUM=1" \
// RUN: -internal_options "-cl-ext=-all,+cl_intel_subgroup_2d_block_io" | FileCheck %s --check-prefix=CHECK-VISAASM-32B-4R-8X1C

// CHECK-VISAASM-32B-4R-8X1C: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d32.8x4nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}

// RUN: ocloc compile -file %s -device pvc -options "-igc_opts 'DumpVISAASMToConsole=1' \
// RUN: -DINPUT_TYPE=uint -DOUTPUT_TYPE=uint -DFUNCTION=intel_sub_group_2d_block_read_32b_8r8x1c -DDST_ARRAY_EL_TYPE=uint4 -DDST_ARRAY_EL_NUM=1" \
// RUN: -internal_options "-cl-ext=-all,+cl_intel_subgroup_2d_block_io" | FileCheck %s --check-prefix=CHECK-VISAASM-32B-8R-8X1C

// CHECK-VISAASM-32B-8R-8X1C: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d32.8x8nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}

// RUN: ocloc compile -file %s -device pvc -options "-igc_opts 'DumpVISAASMToConsole=1' \
// RUN: -DINPUT_TYPE=uint -DOUTPUT_TYPE=uint -DFUNCTION=intel_sub_group_2d_block_read_32b_16r8x1c -DDST_ARRAY_EL_TYPE=uint8 -DDST_ARRAY_EL_NUM=1" \
// RUN: -internal_options "-cl-ext=-all,+cl_intel_subgroup_2d_block_io" | FileCheck %s --check-prefix=CHECK-VISAASM-32B-16R-8X1C

// CHECK-VISAASM-32B-16R-8X1C: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d32.8x16nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}

// RUN: ocloc compile -file %s -device pvc -options "-igc_opts 'DumpVISAASMToConsole=1' \
// RUN: -DINPUT_TYPE=uint -DOUTPUT_TYPE=uint -DFUNCTION=intel_sub_group_2d_block_read_32b_32r8x1c -DDST_ARRAY_EL_TYPE=uint8 -DDST_ARRAY_EL_NUM=2" \
// RUN: -internal_options "-cl-ext=-all,+cl_intel_subgroup_2d_block_io" | FileCheck %s --check-prefix=CHECK-VISAASM-32B-32R-8X1C

// CHECK-VISAASM-32B-32R-8X1C: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d32.8x32nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}

// RUN: ocloc compile -file %s -device pvc -options "-igc_opts 'DumpVISAASMToConsole=1' \
// RUN: -DINPUT_TYPE=uint -DOUTPUT_TYPE=uint -DFUNCTION=intel_sub_group_2d_block_read_32b_1r16x1c -DDST_ARRAY_EL_TYPE=uint -DDST_ARRAY_EL_NUM=1" \
// RUN: -internal_options "-cl-ext=-all,+cl_intel_subgroup_2d_block_io" | FileCheck %s --check-prefix=CHECK-VISAASM-32B-1R-16X1C

// CHECK-VISAASM-32B-1R-16X1C: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d32.16x1nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}

// RUN: ocloc compile -file %s -device pvc -options "-igc_opts 'DumpVISAASMToConsole=1' \
// RUN: -DINPUT_TYPE=uint -DOUTPUT_TYPE=uint -DFUNCTION=intel_sub_group_2d_block_read_32b_2r16x1c -DDST_ARRAY_EL_TYPE=uint2 -DDST_ARRAY_EL_NUM=1" \
// RUN: -internal_options "-cl-ext=-all,+cl_intel_subgroup_2d_block_io" | FileCheck %s --check-prefix=CHECK-VISAASM-32B-2R-16X1C

// CHECK-VISAASM-32B-2R-16X1C: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d32.16x2nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}

// RUN: ocloc compile -file %s -device pvc -options "-igc_opts 'DumpVISAASMToConsole=1' \
// RUN: -DINPUT_TYPE=uint -DOUTPUT_TYPE=uint -DFUNCTION=intel_sub_group_2d_block_read_32b_4r16x1c -DDST_ARRAY_EL_TYPE=uint4 -DDST_ARRAY_EL_NUM=1" \
// RUN: -internal_options "-cl-ext=-all,+cl_intel_subgroup_2d_block_io" | FileCheck %s --check-prefix=CHECK-VISAASM-32B-4R-16X1C

// CHECK-VISAASM-32B-4R-16X1C: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d32.16x4nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}

// RUN: ocloc compile -file %s -device pvc -options "-igc_opts 'DumpVISAASMToConsole=1' \
// RUN: -DINPUT_TYPE=uint -DOUTPUT_TYPE=uint -DFUNCTION=intel_sub_group_2d_block_read_32b_8r16x1c -DDST_ARRAY_EL_TYPE=uint8 -DDST_ARRAY_EL_NUM=1" \
// RUN: -internal_options "-cl-ext=-all,+cl_intel_subgroup_2d_block_io" | FileCheck %s --check-prefix=CHECK-VISAASM-32B-8R-16X1C

// CHECK-VISAASM-32B-8R-16X1C: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d32.16x8nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}

// RUN: ocloc compile -file %s -device pvc -options "-igc_opts 'DumpVISAASMToConsole=1' \
// RUN: -DINPUT_TYPE=uint -DOUTPUT_TYPE=uint -DFUNCTION=intel_sub_group_2d_block_read_32b_16r16x1c -DDST_ARRAY_EL_TYPE=uint8 -DDST_ARRAY_EL_NUM=2" \
// RUN: -internal_options "-cl-ext=-all,+cl_intel_subgroup_2d_block_io" | FileCheck %s --check-prefix=CHECK-VISAASM-32B-16R-16X1C

// CHECK-VISAASM-32B-16R-16X1C: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d32.16x16nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}

// RUN: ocloc compile -file %s -device pvc -options "-igc_opts 'DumpVISAASMToConsole=1' \
// RUN: -DINPUT_TYPE=uint -DOUTPUT_TYPE=uint -DFUNCTION=intel_sub_group_2d_block_read_32b_32r16x1c -DDST_ARRAY_EL_TYPE=uint8 -DDST_ARRAY_EL_NUM=4" \
// RUN: -internal_options "-cl-ext=-all,+cl_intel_subgroup_2d_block_io" | FileCheck %s --check-prefix=CHECK-VISAASM-32B-32R-16X1C

// CHECK-VISAASM-32B-32R-16X1C: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d32.16x32nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}

// RUN: ocloc compile -file %s -device pvc -options "-igc_opts 'DumpVISAASMToConsole=1' \
// RUN: -DINPUT_TYPE=uint -DOUTPUT_TYPE=uint -DFUNCTION=intel_sub_group_2d_block_read_32b_1r8x2c -DDST_ARRAY_EL_TYPE=uint2 -DDST_ARRAY_EL_NUM=1" \
// RUN: -internal_options "-cl-ext=-all,+cl_intel_subgroup_2d_block_io" | FileCheck %s --check-prefix=CHECK-VISAASM-32B-1R-8X2C

// CHECK-VISAASM-32B-1R-8X2C: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d32.2x8x1nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}

// RUN: ocloc compile -file %s -device pvc -options "-igc_opts 'DumpVISAASMToConsole=1' \
// RUN: -DINPUT_TYPE=uint -DOUTPUT_TYPE=uint -DFUNCTION=intel_sub_group_2d_block_read_32b_2r8x2c -DDST_ARRAY_EL_TYPE=uint2 -DDST_ARRAY_EL_NUM=1" \
// RUN: -internal_options "-cl-ext=-all,+cl_intel_subgroup_2d_block_io" | FileCheck %s --check-prefix=CHECK-VISAASM-32B-2R-8X2C

// CHECK-VISAASM-32B-2R-8X2C: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d32.2x8x2nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}

// RUN: ocloc compile -file %s -device pvc -options "-igc_opts 'DumpVISAASMToConsole=1' \
// RUN: -DINPUT_TYPE=uint -DOUTPUT_TYPE=uint -DFUNCTION=intel_sub_group_2d_block_read_32b_4r8x2c -DDST_ARRAY_EL_TYPE=uint4 -DDST_ARRAY_EL_NUM=1" \
// RUN: -internal_options "-cl-ext=-all,+cl_intel_subgroup_2d_block_io" | FileCheck %s --check-prefix=CHECK-VISAASM-32B-4R-8X2C

// CHECK-VISAASM-32B-4R-8X2C: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d32.2x8x4nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}

// RUN: ocloc compile -file %s -device pvc -options "-igc_opts 'DumpVISAASMToConsole=1' \
// RUN: -DINPUT_TYPE=uint -DOUTPUT_TYPE=uint -DFUNCTION=intel_sub_group_2d_block_read_32b_8r8x2c -DDST_ARRAY_EL_TYPE=uint8 -DDST_ARRAY_EL_NUM=1" \
// RUN: -internal_options "-cl-ext=-all,+cl_intel_subgroup_2d_block_io" | FileCheck %s --check-prefix=CHECK-VISAASM-32B-8R-8X2C

// CHECK-VISAASM-32B-8R-8X2C: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d32.2x8x8nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}

// RUN: ocloc compile -file %s -device pvc -options "-igc_opts 'DumpVISAASMToConsole=1' \
// RUN: -DINPUT_TYPE=uint -DOUTPUT_TYPE=uint -DFUNCTION=intel_sub_group_2d_block_read_32b_16r8x2c -DDST_ARRAY_EL_TYPE=uint8 -DDST_ARRAY_EL_NUM=2" \
// RUN: -internal_options "-cl-ext=-all,+cl_intel_subgroup_2d_block_io" | FileCheck %s --check-prefix=CHECK-VISAASM-32B-16R-8X2C

// CHECK-VISAASM-32B-16R-8X2C: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d32.2x8x16nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}

// RUN: ocloc compile -file %s -device pvc -options "-igc_opts 'DumpVISAASMToConsole=1' \
// RUN: -DINPUT_TYPE=uint -DOUTPUT_TYPE=uint -DFUNCTION=intel_sub_group_2d_block_read_32b_32r8x2c -DDST_ARRAY_EL_TYPE=uint8 -DDST_ARRAY_EL_NUM=4" \
// RUN: -internal_options "-cl-ext=-all,+cl_intel_subgroup_2d_block_io" | FileCheck %s --check-prefix=CHECK-VISAASM-32B-32R-8X2C

// CHECK-VISAASM-32B-32R-8X2C: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d32.2x8x32nn  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}

// RUN: ocloc compile -file %s -device pvc -options "-igc_opts 'DumpVISAASMToConsole=1' \
// RUN: -DINPUT_TYPE=ushort -DOUTPUT_TYPE=uint -DFUNCTION=intel_sub_group_2d_block_read_transform_8b_32r16x2c -DDST_ARRAY_EL_TYPE=uint8 -DDST_ARRAY_EL_NUM=2" \
// RUN: -internal_options "-cl-ext=-all,+cl_intel_subgroup_2d_block_io" | FileCheck %s --check-prefix=CHECK-VISAASM-TRANSFORM-8B-32R-16X2C

// CHECK-VISAASM-TRANSFORM-8B-32R-16X2C: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d8.2x16x32nt  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}

// RUN: ocloc compile -file %s -device pvc -options "-igc_opts 'DumpVISAASMToConsole=1' \
// RUN: -DINPUT_TYPE=ushort -DOUTPUT_TYPE=uint -DFUNCTION=intel_sub_group_2d_block_read_transform_8b_32r16x4c -DDST_ARRAY_EL_TYPE=uint8 -DDST_ARRAY_EL_NUM=4" \
// RUN: -internal_options "-cl-ext=-all,+cl_intel_subgroup_2d_block_io" | FileCheck %s --check-prefix=CHECK-VISAASM-TRANSFORM-8B-32R-16X4C

// CHECK-VISAASM-TRANSFORM-8B-32R-16X4C: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d8.4x16x32nt  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}

// RUN: ocloc compile -file %s -device pvc -options "-igc_opts 'DumpVISAASMToConsole=1' \
// RUN: -DINPUT_TYPE=ushort -DOUTPUT_TYPE=uint -DFUNCTION=intel_sub_group_2d_block_read_transform_16b_32r16x1c -DDST_ARRAY_EL_TYPE=uint8 -DDST_ARRAY_EL_NUM=2" \
// RUN: -internal_options "-cl-ext=-all,+cl_intel_subgroup_2d_block_io" | FileCheck %s --check-prefix=CHECK-VISAASM-TRANSFORM-16B-32R-16X1C

// CHECK-VISAASM-TRANSFORM-16B-32R-16X1C: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d16.16x32nt  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}

// RUN: ocloc compile -file %s -device pvc -options "-igc_opts 'DumpVISAASMToConsole=1' \
// RUN: -DINPUT_TYPE=ushort -DOUTPUT_TYPE=uint -DFUNCTION=intel_sub_group_2d_block_read_transform_16b_16r16x2c -DDST_ARRAY_EL_TYPE=uint8 -DDST_ARRAY_EL_NUM=2" \
// RUN: -internal_options "-cl-ext=-all,+cl_intel_subgroup_2d_block_io" | FileCheck %s --check-prefix=CHECK-VISAASM-TRANSFORM-16B-16R-16X2C

// CHECK-VISAASM-TRANSFORM-16B-16R-16X2C: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d16.2x16x16nt  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}

// RUN: ocloc compile -file %s -device pvc -options "-igc_opts 'DumpVISAASMToConsole=1' \
// RUN: -DINPUT_TYPE=ushort -DOUTPUT_TYPE=uint -DFUNCTION=intel_sub_group_2d_block_read_transform_16b_32r16x2c -DDST_ARRAY_EL_TYPE=uint8 -DDST_ARRAY_EL_NUM=4" \
// RUN: -internal_options "-cl-ext=-all,+cl_intel_subgroup_2d_block_io" | FileCheck %s --check-prefix=CHECK-VISAASM-TRANSFORM-16B-32R-16X2C

// CHECK-VISAASM-TRANSFORM-16B-32R-16X2C: lsc_load_block2d.ugm (M1, 1)  V{{[0-9]+}}:d16.2x16x32nt  flat[{{.+}},0x1FF,0x2D,0x1FF,V{{[0-9]+}},V{{[0-9]+}}

__attribute__((intel_reqd_sub_group_size(16)))
kernel void test_default(global INPUT_TYPE* input, const global int2* coord, global OUTPUT_TYPE* output) {
    const int tid = get_global_id(0);
    DST_ARRAY_EL_TYPE dst[DST_ARRAY_EL_NUM];
    FUNCTION(input, 512, 46, 512, *coord, dst);
    output[tid] = *(private OUTPUT_TYPE*)dst;
}

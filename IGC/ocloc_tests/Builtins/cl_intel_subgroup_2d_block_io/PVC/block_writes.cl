/*========================== begin_copyright_notice ============================

Copyright (C) 2024 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

// REQUIRES: regkeys, pvc-supported

// RUN: ocloc compile -file %s -device pvc -options "-igc_opts 'DumpVISAASMToConsole=1' \
// RUN: -DFUNCTION=intel_sub_group_2d_block_write_8b_1r32x1c -DVALUE_TYPE=ushort" \
// RUN: -internal_options "-cl-ext=-all,+cl_intel_subgroup_2d_block_io" | FileCheck %s --check-prefix=CHECK-VISAASM-8B-1R-32X1C

// CHECK-VISAASM-8B-1R-32X1C: lsc_store_block2d.ugm (M1, 1)  flat[{{.*}},0x1FF,0x2D,0x1FF,{{.*}},{{.*}}]  {{.*}}:d8.32x1nn

// RUN: ocloc compile -file %s -device pvc -options "-igc_opts 'DumpVISAASMToConsole=1' \
// RUN: -DFUNCTION=intel_sub_group_2d_block_write_8b_2r32x1c -DVALUE_TYPE=ushort2" \
// RUN: -internal_options "-cl-ext=-all,+cl_intel_subgroup_2d_block_io" | FileCheck %s --check-prefix=CHECK-VISAASM-8B-2R-32X1C

// CHECK-VISAASM-8B-2R-32X1C: lsc_store_block2d.ugm (M1, 1)  flat[{{.*}},0x1FF,0x2D,0x1FF,{{.*}},{{.*}}]  {{.*}}:d8.32x2nn

// RUN: ocloc compile -file %s -device pvc -options "-igc_opts 'DumpVISAASMToConsole=1' \
// RUN: -DFUNCTION=intel_sub_group_2d_block_write_8b_4r32x1c -DVALUE_TYPE=ushort4" \
// RUN: -internal_options "-cl-ext=-all,+cl_intel_subgroup_2d_block_io" | FileCheck %s --check-prefix=CHECK-VISAASM-8B-4R-32X1C

// CHECK-VISAASM-8B-4R-32X1C: lsc_store_block2d.ugm (M1, 1)  flat[{{.*}},0x1FF,0x2D,0x1FF,{{.*}},{{.*}}]  {{.*}}:d8.32x4nn

// RUN: ocloc compile -file %s -device pvc -options "-igc_opts 'DumpVISAASMToConsole=1' \
// RUN: -DFUNCTION=intel_sub_group_2d_block_write_8b_8r32x1c -DVALUE_TYPE=ushort8" \
// RUN: -internal_options "-cl-ext=-all,+cl_intel_subgroup_2d_block_io" | FileCheck %s --check-prefix=CHECK-VISAASM-8B-8R-32X1C

// CHECK-VISAASM-8B-8R-32X1C: lsc_store_block2d.ugm (M1, 1)  flat[{{.*}},0x1FF,0x2D,0x1FF,{{.*}},{{.*}}]  {{.*}}:d8.32x8nn

// RUN: ocloc compile -file %s -device pvc -options "-igc_opts 'DumpVISAASMToConsole=1' \
// RUN: -DFUNCTION=intel_sub_group_2d_block_write_16b_1r16x1c -DVALUE_TYPE=ushort" \
// RUN: -internal_options "-cl-ext=-all,+cl_intel_subgroup_2d_block_io" | FileCheck %s --check-prefix=CHECK-VISAASM-16B-1R-16X1C

// CHECK-VISAASM-16B-1R-16X1C: lsc_store_block2d.ugm (M1, 1)  flat[{{.*}},0x1FF,0x2D,0x1FF,{{.*}},{{.*}}]  {{.*}}:d16.16x1nn

// RUN: ocloc compile -file %s -device pvc -options "-igc_opts 'DumpVISAASMToConsole=1' \
// RUN: -DFUNCTION=intel_sub_group_2d_block_write_16b_2r16x1c -DVALUE_TYPE=ushort2" \
// RUN: -internal_options "-cl-ext=-all,+cl_intel_subgroup_2d_block_io" | FileCheck %s --check-prefix=CHECK-VISAASM-16B-2R-16X1C

// CHECK-VISAASM-16B-2R-16X1C: lsc_store_block2d.ugm (M1, 1)  flat[{{.*}},0x1FF,0x2D,0x1FF,{{.*}},{{.*}}]  {{.*}}:d16.16x2nn

// RUN: ocloc compile -file %s -device pvc -options "-igc_opts 'DumpVISAASMToConsole=1' \
// RUN: -DFUNCTION=intel_sub_group_2d_block_write_16b_4r16x1c -DVALUE_TYPE=ushort4" \
// RUN: -internal_options "-cl-ext=-all,+cl_intel_subgroup_2d_block_io" | FileCheck %s --check-prefix=CHECK-VISAASM-16B-4R-16X1C

// CHECK-VISAASM-16B-4R-16X1C: lsc_store_block2d.ugm (M1, 1)  flat[{{.*}},0x1FF,0x2D,0x1FF,{{.*}},{{.*}}]  {{.*}}:d16.16x4nn

// RUN: ocloc compile -file %s -device pvc -options "-igc_opts 'DumpVISAASMToConsole=1' \
// RUN: -DFUNCTION=intel_sub_group_2d_block_write_16b_8r16x1c -DVALUE_TYPE=ushort8" \
// RUN: -internal_options "-cl-ext=-all,+cl_intel_subgroup_2d_block_io" | FileCheck %s --check-prefix=CHECK-VISAASM-16B-8R-16X1C

// CHECK-VISAASM-16B-8R-16X1C: lsc_store_block2d.ugm (M1, 1)  flat[{{.*}},0x1FF,0x2D,0x1FF,{{.*}},{{.*}}]  {{.*}}:d16.16x8nn

// RUN: ocloc compile -file %s -device pvc -options "-igc_opts 'DumpVISAASMToConsole=1' \
// RUN: -DFUNCTION=intel_sub_group_2d_block_write_8b_1r16x1c -DVALUE_TYPE=uchar" \
// RUN: -internal_options "-cl-ext=-all,+cl_intel_subgroup_2d_block_io" | FileCheck %s --check-prefix=CHECK-VISAASM-8B-1R-16X1C

// CHECK-VISAASM-8B-1R-16X1C: lsc_store_block2d.ugm (M1, 1)  flat[{{.*}},0x1FF,0x2D,0x1FF,{{.*}},{{.*}}]  {{.*}}:d8.16x1nn

// RUN: ocloc compile -file %s -device pvc -options "-igc_opts 'DumpVISAASMToConsole=1' \
// RUN: -DFUNCTION=intel_sub_group_2d_block_write_8b_2r16x1c -DVALUE_TYPE=uchar2" \
// RUN: -internal_options "-cl-ext=-all,+cl_intel_subgroup_2d_block_io" | FileCheck %s --check-prefix=CHECK-VISAASM-8B-2R-16X1C

// CHECK-VISAASM-8B-2R-16X1C: lsc_store_block2d.ugm (M1, 1)  flat[{{.*}},0x1FF,0x2D,0x1FF,{{.*}},{{.*}}]  {{.*}}:d8.16x2nn

// RUN: ocloc compile -file %s -device pvc -options "-igc_opts 'DumpVISAASMToConsole=1' \
// RUN: -DFUNCTION=intel_sub_group_2d_block_write_8b_4r16x1c -DVALUE_TYPE=uchar4" \
// RUN: -internal_options "-cl-ext=-all,+cl_intel_subgroup_2d_block_io" | FileCheck %s --check-prefix=CHECK-VISAASM-8B-4R-16X1C

// CHECK-VISAASM-8B-4R-16X1C: lsc_store_block2d.ugm (M1, 1)  flat[{{.*}},0x1FF,0x2D,0x1FF,{{.*}},{{.*}}]  {{.*}}:d8.16x4nn

// RUN: ocloc compile -file %s -device pvc -options "-igc_opts 'DumpVISAASMToConsole=1' \
// RUN: -DFUNCTION=intel_sub_group_2d_block_write_8b_8r16x1c -DVALUE_TYPE=uchar8" \
// RUN: -internal_options "-cl-ext=-all,+cl_intel_subgroup_2d_block_io" | FileCheck %s --check-prefix=CHECK-VISAASM-8B-8R-16X1C

// CHECK-VISAASM-8B-8R-16X1C: lsc_store_block2d.ugm (M1, 1)  flat[{{.*}},0x1FF,0x2D,0x1FF,{{.*}},{{.*}}]  {{.*}}:d8.16x8nn

// RUN: ocloc compile -file %s -device pvc -options "-igc_opts 'DumpVISAASMToConsole=1' \
// RUN: -DFUNCTION=intel_sub_group_2d_block_write_32b_1r16x1c -DVALUE_TYPE=uint" \
// RUN: -internal_options "-cl-ext=-all,+cl_intel_subgroup_2d_block_io" | FileCheck %s --check-prefix=CHECK-VISAASM-32B-1R-16X1C

// CHECK-VISAASM-32B-1R-16X1C: lsc_store_block2d.ugm (M1, 1)  flat[{{.*}},0x1FF,0x2D,0x1FF,{{.*}},{{.*}}]  {{.*}}:d32.16x1nn

// RUN: ocloc compile -file %s -device pvc -options "-igc_opts 'DumpVISAASMToConsole=1' \
// RUN: -DFUNCTION=intel_sub_group_2d_block_write_32b_2r16x1c -DVALUE_TYPE=uint2" \
// RUN: -internal_options "-cl-ext=-all,+cl_intel_subgroup_2d_block_io" | FileCheck %s --check-prefix=CHECK-VISAASM-32B-2R-16X1C

// CHECK-VISAASM-32B-2R-16X1C: lsc_store_block2d.ugm (M1, 1)  flat[{{.*}},0x1FF,0x2D,0x1FF,{{.*}},{{.*}}]  {{.*}}:d32.16x2nn

// RUN: ocloc compile -file %s -device pvc -options "-igc_opts 'DumpVISAASMToConsole=1' \
// RUN: -DFUNCTION=intel_sub_group_2d_block_write_32b_4r16x1c -DVALUE_TYPE=uint4" \
// RUN: -internal_options "-cl-ext=-all,+cl_intel_subgroup_2d_block_io" | FileCheck %s --check-prefix=CHECK-VISAASM-32B-4R-16X1C

// CHECK-VISAASM-32B-4R-16X1C: lsc_store_block2d.ugm (M1, 1)  flat[{{.*}},0x1FF,0x2D,0x1FF,{{.*}},{{.*}}]  {{.*}}:d32.16x4nn

// RUN: ocloc compile -file %s -device pvc -options "-igc_opts 'DumpVISAASMToConsole=1' \
// RUN: -DFUNCTION=intel_sub_group_2d_block_write_32b_8r16x1c -DVALUE_TYPE=uint8" \
// RUN: -internal_options "-cl-ext=-all,+cl_intel_subgroup_2d_block_io" | FileCheck %s --check-prefix=CHECK-VISAASM-32B-8R-16X1C

// CHECK-VISAASM-32B-8R-16X1C: lsc_store_block2d.ugm (M1, 1)  flat[{{.*}},0x1FF,0x2D,0x1FF,{{.*}},{{.*}}]  {{.*}}:d32.16x8nn

__attribute__((intel_reqd_sub_group_size(16)))
kernel void test_uc_uc(global VALUE_TYPE* output, VALUE_TYPE value, const global int2* coord) {
    FUNCTION(output, 512, 46, 512, *coord, &value);
}

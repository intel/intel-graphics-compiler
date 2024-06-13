/*========================== begin_copyright_notice ============================

Copyright (C) 2024 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

// REQUIRES: regkeys, pvc-supported

// UNSUPPORTED: sys32

// RUN: ocloc compile -file %s -device pvc -options "-igc_opts 'DumpVISAASMToConsole=1'" \
// RUN: -internal_options "-cl-ext=-all,+cl_intel_subgroup_buffer_prefetch" | FileCheck %s

__attribute__((intel_reqd_sub_group_size(16)))
kernel void test_char(const global uchar* buffer) {
// CHECK: lsc_load.ugm (M1_NM, 1) %null:d32x4t  flat[{{.*}}]:a64
    intel_sub_group_block_prefetch_uc(buffer);
// CHECK: lsc_load.ugm (M1_NM, 1) %null:d32x8t  flat[{{.*}}]:a64
    intel_sub_group_block_prefetch_uc2(buffer);
// CHECK: lsc_load.ugm (M1_NM, 1) %null:d32x16t  flat[{{.*}}]:a64
    intel_sub_group_block_prefetch_uc4(buffer);
// CHECK: lsc_load.ugm (M1_NM, 1) %null:d32x32t  flat[{{.*}}]:a64
    intel_sub_group_block_prefetch_uc8(buffer);
// CHECK: lsc_load.ugm (M1_NM, 1) %null:d32x64t  flat[{{.*}}]:a64
    intel_sub_group_block_prefetch_uc16(buffer);
}

__attribute__((intel_reqd_sub_group_size(16)))
kernel void test_short(const global ushort* buffer) {
// CHECK: lsc_load.ugm (M1_NM, 1) %null:d32x8t  flat[{{.*}}]:a64
    intel_sub_group_block_prefetch_us(buffer);
// CHECK: lsc_load.ugm (M1_NM, 1) %null:d32x16t  flat[{{.*}}]:a64
    intel_sub_group_block_prefetch_us2(buffer);
// CHECK: lsc_load.ugm (M1_NM, 1) %null:d32x32t  flat[{{.*}}]:a64
    intel_sub_group_block_prefetch_us4(buffer);
// CHECK: lsc_load.ugm (M1_NM, 1) %null:d32x64t  flat[{{.*}}]:a64
    intel_sub_group_block_prefetch_us8(buffer);
// CHECK: lsc_load.ugm (M1_NM, 1) %null:d32x64t  flat[{{.*}}]:a64
// CHECK: lsc_load.ugm (M1_NM, 1) %null:d32x64t  flat[{{.*}}+0x100]:a64
    intel_sub_group_block_prefetch_us16(buffer);
}

__attribute__((intel_reqd_sub_group_size(16)))
kernel void test_int(const global uint* buffer) {
// CHECK: lsc_load.ugm (M1_NM, 1) %null:d32x16t  flat[{{.*}}]:a64
    intel_sub_group_block_prefetch_ui(buffer);
// CHECK: lsc_load.ugm (M1_NM, 1) %null:d32x32t  flat[{{.*}}]:a64
    intel_sub_group_block_prefetch_ui2(buffer);
// CHECK: lsc_load.ugm (M1_NM, 1) %null:d32x64t  flat[{{.*}}]:a64
    intel_sub_group_block_prefetch_ui4(buffer);
// CHECK: lsc_load.ugm (M1_NM, 1) %null:d32x64t  flat[{{.*}}]:a64
// CHECK: lsc_load.ugm (M1_NM, 1) %null:d32x64t  flat[{{.*}}+0x100]:a64
    intel_sub_group_block_prefetch_ui8(buffer);
}

__attribute__((intel_reqd_sub_group_size(16)))
kernel void test_long(const global ulong* buffer) {
// CHECK: lsc_load.ugm (M1_NM, 1) %null:d32x32t  flat[{{.*}}]:a64
    intel_sub_group_block_prefetch_ul(buffer);
// CHECK: lsc_load.ugm (M1_NM, 1) %null:d32x64t  flat[{{.*}}]:a64
    intel_sub_group_block_prefetch_ul2(buffer);
// CHECK: lsc_load.ugm (M1_NM, 1) %null:d32x64t  flat[{{.*}}]:a64
// CHECK: lsc_load.ugm (M1_NM, 1) %null:d32x64t  flat[{{.*}}+0x100]:a64
    intel_sub_group_block_prefetch_ul4(buffer);
// CHECK: lsc_load.ugm (M1_NM, 1) %null:d32x64t  flat[{{.*}}]:a64
// CHECK: lsc_load.ugm (M1_NM, 1) %null:d32x64t  flat[{{.*}}+0x100]:a64
// CHECK: lsc_load.ugm (M1_NM, 1) %null:d32x64t  flat[{{.*}}+0x200]:a64
// CHECK: lsc_load.ugm (M1_NM, 1) %null:d32x64t  flat[{{.*}}+0x300]:a64
    intel_sub_group_block_prefetch_ul8(buffer);
}

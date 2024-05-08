;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; UNSUPPORTED: system-windows
; REQUIRES: llvm-spirv, regkeys, pvc-supported

; RUN: llvm-as %s -o %t.bc
; RUN: llvm-spirv %t.bc --spirv-ext=+SPV_INTEL_cache_controls,+SPV_INTEL_joint_matrix -o %t.spv
; RUN: ocloc compile -spirv_input -file %t.spv -device pvc -options " -igc_opts 'DumpVISAASMToConsole=1'" 2>&1 | FileCheck %s


target triple = "spir64-unknown-unknown"

; 8 bit prefetch
declare spir_func void @_Z38__spirv_CooperativeMatrixPrefetchINTELPU3AS4ciiiiiil(i8 addrspace(1)*, i32, i32, i32, i32, i32, i32, i64)

; 16 bit prefetch
%"class.sycl::_V1::ext::oneapi::bfloat16" = type { i16 }
declare spir_func void @"_Z38__spirv_CooperativeMatrixPrefetchINTELPU3AS438class.sycl::_V1::ext::oneapi::bfloat16lllliil"(%"class.sycl::_V1::ext::oneapi::bfloat16" addrspace(1)*, i32, i32, i32, i32, i32, i32, i64)

; 32 bit prefetch
declare spir_func void @_Z38__spirv_CooperativeMatrixPrefetchINTELPU3AS4flllliil(float addrspace(1)*, i32, i32, i32, i32, i32, i32, i64)

; 64 bit prefetch
declare spir_func void @_Z38__spirv_CooperativeMatrixPrefetchINTELPU3AS4diiiiiil(double addrspace(1)*, i32, i32, i32, i32, i32, i32, i64)



; 1B * 8 == 8B
; CHECK: .kernel "TestPrefetch_ElementSize1B_1x8"
; CHECK: lsc_load_block2d.ugm.ca.ca (M1, 1)  %null
define spir_kernel void @TestPrefetch_ElementSize1B_1x8(i8 addrspace(1)* %src) {
  entry:
  call spir_func void @_Z38__spirv_CooperativeMatrixPrefetchINTELPU3AS4ciiiiiil(i8 addrspace(1)* %src, i32 0, i32 0, i32 1, i32 8, i32 1, i32 0, i64 64)
  ret void
}


; 1B * 16 == 16B
; CHECK: .kernel "TestPrefetch_ElementSize1B_1x16"
; CHECK: lsc_load_block2d.ugm.ca.ca (M1, 1)  %null
define spir_kernel void @TestPrefetch_ElementSize1B_1x16(i8 addrspace(1)* %src) {
  entry:
  call spir_func void @_Z38__spirv_CooperativeMatrixPrefetchINTELPU3AS4ciiiiiil(i8 addrspace(1)* %src, i32 0, i32 0, i32 1, i32 16, i32 1, i32 0, i64 64)
  ret void
}


; 1B * 32 == 32B
; CHECK: .kernel "TestPrefetch_ElementSize1B_1x32"
; CHECK: lsc_load_block2d.ugm.ca.ca (M1, 1)  %null
define spir_kernel void @TestPrefetch_ElementSize1B_1x32(i8 addrspace(1)* %src) {
  entry:
  call spir_func void @_Z38__spirv_CooperativeMatrixPrefetchINTELPU3AS4ciiiiiil(i8 addrspace(1)* %src, i32 0, i32 0, i32 1, i32 32, i32 1, i32 0, i64 64)
  ret void
}


; 1B * 64 == 64B
; CHECK: .kernel "TestPrefetch_ElementSize1B_1x64"
; CHECK: lsc_load_block2d.ugm.ca.ca (M1, 1)  %null
define spir_kernel void @TestPrefetch_ElementSize1B_1x64(i8 addrspace(1)* %src) {
  entry:
  call spir_func void @_Z38__spirv_CooperativeMatrixPrefetchINTELPU3AS4ciiiiiil(i8 addrspace(1)* %src, i32 0, i32 0, i32 1, i32 64, i32 1, i32 0, i64 64)
  ret void
}


; 1B * 8 == 8B
; CHECK: .kernel "TestPrefetch_ElementSize1B_2x8"
; CHECK: lsc_load_block2d.ugm.ca.ca (M1, 1)  %null
define spir_kernel void @TestPrefetch_ElementSize1B_2x8(i8 addrspace(1)* %src) {
  entry:
  call spir_func void @_Z38__spirv_CooperativeMatrixPrefetchINTELPU3AS4ciiiiiil(i8 addrspace(1)* %src, i32 0, i32 0, i32 2, i32 8, i32 1, i32 0, i64 64)
  ret void
}


; 1B * 16 == 16B
; CHECK: .kernel "TestPrefetch_ElementSize1B_2x16"
; CHECK: lsc_load_block2d.ugm.ca.ca (M1, 1)  %null
define spir_kernel void @TestPrefetch_ElementSize1B_2x16(i8 addrspace(1)* %src) {
  entry:
  call spir_func void @_Z38__spirv_CooperativeMatrixPrefetchINTELPU3AS4ciiiiiil(i8 addrspace(1)* %src, i32 0, i32 0, i32 2, i32 16, i32 1, i32 0, i64 64)
  ret void
}


; 1B * 32 == 32B
; CHECK: .kernel "TestPrefetch_ElementSize1B_2x32"
; CHECK: lsc_load_block2d.ugm.ca.ca (M1, 1)  %null
define spir_kernel void @TestPrefetch_ElementSize1B_2x32(i8 addrspace(1)* %src) {
  entry:
  call spir_func void @_Z38__spirv_CooperativeMatrixPrefetchINTELPU3AS4ciiiiiil(i8 addrspace(1)* %src, i32 0, i32 0, i32 2, i32 32, i32 1, i32 0, i64 64)
  ret void
}


; 1B * 64 == 64B
; CHECK: .kernel "TestPrefetch_ElementSize1B_2x64"
; CHECK: lsc_load_block2d.ugm.ca.ca (M1, 1)  %null
define spir_kernel void @TestPrefetch_ElementSize1B_2x64(i8 addrspace(1)* %src) {
  entry:
  call spir_func void @_Z38__spirv_CooperativeMatrixPrefetchINTELPU3AS4ciiiiiil(i8 addrspace(1)* %src, i32 0, i32 0, i32 2, i32 64, i32 1, i32 0, i64 64)
  ret void
}


; 1B * 8 == 8B
; CHECK: .kernel "TestPrefetch_ElementSize1B_4x8"
; CHECK: lsc_load_block2d.ugm.ca.ca (M1, 1)  %null
define spir_kernel void @TestPrefetch_ElementSize1B_4x8(i8 addrspace(1)* %src) {
  entry:
  call spir_func void @_Z38__spirv_CooperativeMatrixPrefetchINTELPU3AS4ciiiiiil(i8 addrspace(1)* %src, i32 0, i32 0, i32 4, i32 8, i32 1, i32 0, i64 64)
  ret void
}


; 1B * 16 == 16B
; CHECK: .kernel "TestPrefetch_ElementSize1B_4x16"
; CHECK: lsc_load_block2d.ugm.ca.ca (M1, 1)  %null
define spir_kernel void @TestPrefetch_ElementSize1B_4x16(i8 addrspace(1)* %src) {
  entry:
  call spir_func void @_Z38__spirv_CooperativeMatrixPrefetchINTELPU3AS4ciiiiiil(i8 addrspace(1)* %src, i32 0, i32 0, i32 4, i32 16, i32 1, i32 0, i64 64)
  ret void
}


; 1B * 32 == 32B
; CHECK: .kernel "TestPrefetch_ElementSize1B_4x32"
; CHECK: lsc_load_block2d.ugm.ca.ca (M1, 1)  %null
define spir_kernel void @TestPrefetch_ElementSize1B_4x32(i8 addrspace(1)* %src) {
  entry:
  call spir_func void @_Z38__spirv_CooperativeMatrixPrefetchINTELPU3AS4ciiiiiil(i8 addrspace(1)* %src, i32 0, i32 0, i32 4, i32 32, i32 1, i32 0, i64 64)
  ret void
}


; 1B * 64 == 64B
; CHECK: .kernel "TestPrefetch_ElementSize1B_4x64"
; CHECK: lsc_load_block2d.ugm.ca.ca (M1, 1)  %null
define spir_kernel void @TestPrefetch_ElementSize1B_4x64(i8 addrspace(1)* %src) {
  entry:
  call spir_func void @_Z38__spirv_CooperativeMatrixPrefetchINTELPU3AS4ciiiiiil(i8 addrspace(1)* %src, i32 0, i32 0, i32 4, i32 64, i32 1, i32 0, i64 64)
  ret void
}


; 1B * 8 == 8B
; CHECK: .kernel "TestPrefetch_ElementSize1B_8x8"
; CHECK: lsc_load_block2d.ugm.ca.ca (M1, 1)  %null
define spir_kernel void @TestPrefetch_ElementSize1B_8x8(i8 addrspace(1)* %src) {
  entry:
  call spir_func void @_Z38__spirv_CooperativeMatrixPrefetchINTELPU3AS4ciiiiiil(i8 addrspace(1)* %src, i32 0, i32 0, i32 8, i32 8, i32 1, i32 0, i64 64)
  ret void
}


; 1B * 16 == 16B
; CHECK: .kernel "TestPrefetch_ElementSize1B_8x16"
; CHECK: lsc_load_block2d.ugm.ca.ca (M1, 1)  %null
define spir_kernel void @TestPrefetch_ElementSize1B_8x16(i8 addrspace(1)* %src) {
  entry:
  call spir_func void @_Z38__spirv_CooperativeMatrixPrefetchINTELPU3AS4ciiiiiil(i8 addrspace(1)* %src, i32 0, i32 0, i32 8, i32 16, i32 1, i32 0, i64 64)
  ret void
}


; 1B * 32 == 32B
; CHECK: .kernel "TestPrefetch_ElementSize1B_8x32"
; CHECK: lsc_load_block2d.ugm.ca.ca (M1, 1)  %null
define spir_kernel void @TestPrefetch_ElementSize1B_8x32(i8 addrspace(1)* %src) {
  entry:
  call spir_func void @_Z38__spirv_CooperativeMatrixPrefetchINTELPU3AS4ciiiiiil(i8 addrspace(1)* %src, i32 0, i32 0, i32 8, i32 32, i32 1, i32 0, i64 64)
  ret void
}


; 1B * 64 == 64B
; CHECK: .kernel "TestPrefetch_ElementSize1B_8x64"
; CHECK: lsc_load_block2d.ugm.ca.ca (M1, 1)  %null
define spir_kernel void @TestPrefetch_ElementSize1B_8x64(i8 addrspace(1)* %src) {
  entry:
  call spir_func void @_Z38__spirv_CooperativeMatrixPrefetchINTELPU3AS4ciiiiiil(i8 addrspace(1)* %src, i32 0, i32 0, i32 8, i32 64, i32 1, i32 0, i64 64)
  ret void
}


; 1B * 8 == 8B
; CHECK: .kernel "TestPrefetch_ElementSize1B_16x8"
; CHECK: lsc_load_block2d.ugm.ca.ca (M1, 1)  %null
define spir_kernel void @TestPrefetch_ElementSize1B_16x8(i8 addrspace(1)* %src) {
  entry:
  call spir_func void @_Z38__spirv_CooperativeMatrixPrefetchINTELPU3AS4ciiiiiil(i8 addrspace(1)* %src, i32 0, i32 0, i32 16, i32 8, i32 1, i32 0, i64 64)
  ret void
}


; 1B * 16 == 16B
; CHECK: .kernel "TestPrefetch_ElementSize1B_16x16"
; CHECK: lsc_load_block2d.ugm.ca.ca (M1, 1)  %null
define spir_kernel void @TestPrefetch_ElementSize1B_16x16(i8 addrspace(1)* %src) {
  entry:
  call spir_func void @_Z38__spirv_CooperativeMatrixPrefetchINTELPU3AS4ciiiiiil(i8 addrspace(1)* %src, i32 0, i32 0, i32 16, i32 16, i32 1, i32 0, i64 64)
  ret void
}


; 1B * 32 == 32B
; CHECK: .kernel "TestPrefetch_ElementSize1B_16x32"
; CHECK: lsc_load_block2d.ugm.ca.ca (M1, 1)  %null
define spir_kernel void @TestPrefetch_ElementSize1B_16x32(i8 addrspace(1)* %src) {
  entry:
  call spir_func void @_Z38__spirv_CooperativeMatrixPrefetchINTELPU3AS4ciiiiiil(i8 addrspace(1)* %src, i32 0, i32 0, i32 16, i32 32, i32 1, i32 0, i64 64)
  ret void
}


; 1B * 64 == 64B
; CHECK: .kernel "TestPrefetch_ElementSize1B_16x64"
; CHECK: lsc_load_block2d.ugm.ca.ca (M1, 1)  %null
define spir_kernel void @TestPrefetch_ElementSize1B_16x64(i8 addrspace(1)* %src) {
  entry:
  call spir_func void @_Z38__spirv_CooperativeMatrixPrefetchINTELPU3AS4ciiiiiil(i8 addrspace(1)* %src, i32 0, i32 0, i32 16, i32 64, i32 1, i32 0, i64 64)
  ret void
}


; 1B * 8 == 8B
; CHECK: .kernel "TestPrefetch_ElementSize1B_32x8"
; CHECK: lsc_load_block2d.ugm.ca.ca (M1, 1)  %null
define spir_kernel void @TestPrefetch_ElementSize1B_32x8(i8 addrspace(1)* %src) {
  entry:
  call spir_func void @_Z38__spirv_CooperativeMatrixPrefetchINTELPU3AS4ciiiiiil(i8 addrspace(1)* %src, i32 0, i32 0, i32 32, i32 8, i32 1, i32 0, i64 64)
  ret void
}


; 1B * 16 == 16B
; CHECK: .kernel "TestPrefetch_ElementSize1B_32x16"
; CHECK: lsc_load_block2d.ugm.ca.ca (M1, 1)  %null
define spir_kernel void @TestPrefetch_ElementSize1B_32x16(i8 addrspace(1)* %src) {
  entry:
  call spir_func void @_Z38__spirv_CooperativeMatrixPrefetchINTELPU3AS4ciiiiiil(i8 addrspace(1)* %src, i32 0, i32 0, i32 32, i32 16, i32 1, i32 0, i64 64)
  ret void
}


; 1B * 32 == 32B
; CHECK: .kernel "TestPrefetch_ElementSize1B_32x32"
; CHECK: lsc_load_block2d.ugm.ca.ca (M1, 1)  %null
define spir_kernel void @TestPrefetch_ElementSize1B_32x32(i8 addrspace(1)* %src) {
  entry:
  call spir_func void @_Z38__spirv_CooperativeMatrixPrefetchINTELPU3AS4ciiiiiil(i8 addrspace(1)* %src, i32 0, i32 0, i32 32, i32 32, i32 1, i32 0, i64 64)
  ret void
}


; 1B * 64 == 64B
; CHECK: .kernel "TestPrefetch_ElementSize1B_32x64"
; CHECK: lsc_load_block2d.ugm.ca.ca (M1, 1)  %null
define spir_kernel void @TestPrefetch_ElementSize1B_32x64(i8 addrspace(1)* %src) {
  entry:
  call spir_func void @_Z38__spirv_CooperativeMatrixPrefetchINTELPU3AS4ciiiiiil(i8 addrspace(1)* %src, i32 0, i32 0, i32 32, i32 64, i32 1, i32 0, i64 64)
  ret void
}


; 2B * 8 == 16B
; CHECK: .kernel "TestPrefetch_ElementSize2B_1x8"
; CHECK: lsc_load_block2d.ugm.ca.ca (M1, 1)  %null
define spir_kernel void @TestPrefetch_ElementSize2B_1x8(%"class.sycl::_V1::ext::oneapi::bfloat16" addrspace(1)* %src) {
  entry:
  call spir_func void @"_Z38__spirv_CooperativeMatrixPrefetchINTELPU3AS438class.sycl::_V1::ext::oneapi::bfloat16lllliil"(%"class.sycl::_V1::ext::oneapi::bfloat16" addrspace(1)* %src, i32 0, i32 0, i32 1, i32 8, i32 1, i32 0, i64 64)
  ret void
}


; 2B * 16 == 32B
; CHECK: .kernel "TestPrefetch_ElementSize2B_1x16"
; CHECK: lsc_load_block2d.ugm.ca.ca (M1, 1)  %null
define spir_kernel void @TestPrefetch_ElementSize2B_1x16(%"class.sycl::_V1::ext::oneapi::bfloat16" addrspace(1)* %src) {
  entry:
  call spir_func void @"_Z38__spirv_CooperativeMatrixPrefetchINTELPU3AS438class.sycl::_V1::ext::oneapi::bfloat16lllliil"(%"class.sycl::_V1::ext::oneapi::bfloat16" addrspace(1)* %src, i32 0, i32 0, i32 1, i32 16, i32 1, i32 0, i64 64)
  ret void
}


; 2B * 32 == 64B
; CHECK: .kernel "TestPrefetch_ElementSize2B_1x32"
; CHECK: lsc_load_block2d.ugm.ca.ca (M1, 1)  %null
define spir_kernel void @TestPrefetch_ElementSize2B_1x32(%"class.sycl::_V1::ext::oneapi::bfloat16" addrspace(1)* %src) {
  entry:
  call spir_func void @"_Z38__spirv_CooperativeMatrixPrefetchINTELPU3AS438class.sycl::_V1::ext::oneapi::bfloat16lllliil"(%"class.sycl::_V1::ext::oneapi::bfloat16" addrspace(1)* %src, i32 0, i32 0, i32 1, i32 32, i32 1, i32 0, i64 64)
  ret void
}


; 2B * 8 == 16B
; CHECK: .kernel "TestPrefetch_ElementSize2B_2x8"
; CHECK: lsc_load_block2d.ugm.ca.ca (M1, 1)  %null
define spir_kernel void @TestPrefetch_ElementSize2B_2x8(%"class.sycl::_V1::ext::oneapi::bfloat16" addrspace(1)* %src) {
  entry:
  call spir_func void @"_Z38__spirv_CooperativeMatrixPrefetchINTELPU3AS438class.sycl::_V1::ext::oneapi::bfloat16lllliil"(%"class.sycl::_V1::ext::oneapi::bfloat16" addrspace(1)* %src, i32 0, i32 0, i32 2, i32 8, i32 1, i32 0, i64 64)
  ret void
}


; 2B * 16 == 32B
; CHECK: .kernel "TestPrefetch_ElementSize2B_2x16"
; CHECK: lsc_load_block2d.ugm.ca.ca (M1, 1)  %null
define spir_kernel void @TestPrefetch_ElementSize2B_2x16(%"class.sycl::_V1::ext::oneapi::bfloat16" addrspace(1)* %src) {
  entry:
  call spir_func void @"_Z38__spirv_CooperativeMatrixPrefetchINTELPU3AS438class.sycl::_V1::ext::oneapi::bfloat16lllliil"(%"class.sycl::_V1::ext::oneapi::bfloat16" addrspace(1)* %src, i32 0, i32 0, i32 2, i32 16, i32 1, i32 0, i64 64)
  ret void
}


; 2B * 32 == 64B
; CHECK: .kernel "TestPrefetch_ElementSize2B_2x32"
; CHECK: lsc_load_block2d.ugm.ca.ca (M1, 1)  %null
define spir_kernel void @TestPrefetch_ElementSize2B_2x32(%"class.sycl::_V1::ext::oneapi::bfloat16" addrspace(1)* %src) {
  entry:
  call spir_func void @"_Z38__spirv_CooperativeMatrixPrefetchINTELPU3AS438class.sycl::_V1::ext::oneapi::bfloat16lllliil"(%"class.sycl::_V1::ext::oneapi::bfloat16" addrspace(1)* %src, i32 0, i32 0, i32 2, i32 32, i32 1, i32 0, i64 64)
  ret void
}


; 2B * 8 == 16B
; CHECK: .kernel "TestPrefetch_ElementSize2B_4x8"
; CHECK: lsc_load_block2d.ugm.ca.ca (M1, 1)  %null
define spir_kernel void @TestPrefetch_ElementSize2B_4x8(%"class.sycl::_V1::ext::oneapi::bfloat16" addrspace(1)* %src) {
  entry:
  call spir_func void @"_Z38__spirv_CooperativeMatrixPrefetchINTELPU3AS438class.sycl::_V1::ext::oneapi::bfloat16lllliil"(%"class.sycl::_V1::ext::oneapi::bfloat16" addrspace(1)* %src, i32 0, i32 0, i32 4, i32 8, i32 1, i32 0, i64 64)
  ret void
}


; 2B * 16 == 32B
; CHECK: .kernel "TestPrefetch_ElementSize2B_4x16"
; CHECK: lsc_load_block2d.ugm.ca.ca (M1, 1)  %null
define spir_kernel void @TestPrefetch_ElementSize2B_4x16(%"class.sycl::_V1::ext::oneapi::bfloat16" addrspace(1)* %src) {
  entry:
  call spir_func void @"_Z38__spirv_CooperativeMatrixPrefetchINTELPU3AS438class.sycl::_V1::ext::oneapi::bfloat16lllliil"(%"class.sycl::_V1::ext::oneapi::bfloat16" addrspace(1)* %src, i32 0, i32 0, i32 4, i32 16, i32 1, i32 0, i64 64)
  ret void
}


; 2B * 32 == 64B
; CHECK: .kernel "TestPrefetch_ElementSize2B_4x32"
; CHECK: lsc_load_block2d.ugm.ca.ca (M1, 1)  %null
define spir_kernel void @TestPrefetch_ElementSize2B_4x32(%"class.sycl::_V1::ext::oneapi::bfloat16" addrspace(1)* %src) {
  entry:
  call spir_func void @"_Z38__spirv_CooperativeMatrixPrefetchINTELPU3AS438class.sycl::_V1::ext::oneapi::bfloat16lllliil"(%"class.sycl::_V1::ext::oneapi::bfloat16" addrspace(1)* %src, i32 0, i32 0, i32 4, i32 32, i32 1, i32 0, i64 64)
  ret void
}


; 2B * 8 == 16B
; CHECK: .kernel "TestPrefetch_ElementSize2B_8x8"
; CHECK: lsc_load_block2d.ugm.ca.ca (M1, 1)  %null
define spir_kernel void @TestPrefetch_ElementSize2B_8x8(%"class.sycl::_V1::ext::oneapi::bfloat16" addrspace(1)* %src) {
  entry:
  call spir_func void @"_Z38__spirv_CooperativeMatrixPrefetchINTELPU3AS438class.sycl::_V1::ext::oneapi::bfloat16lllliil"(%"class.sycl::_V1::ext::oneapi::bfloat16" addrspace(1)* %src, i32 0, i32 0, i32 8, i32 8, i32 1, i32 0, i64 64)
  ret void
}


; 2B * 16 == 32B
; CHECK: .kernel "TestPrefetch_ElementSize2B_8x16"
; CHECK: lsc_load_block2d.ugm.ca.ca (M1, 1)  %null
define spir_kernel void @TestPrefetch_ElementSize2B_8x16(%"class.sycl::_V1::ext::oneapi::bfloat16" addrspace(1)* %src) {
  entry:
  call spir_func void @"_Z38__spirv_CooperativeMatrixPrefetchINTELPU3AS438class.sycl::_V1::ext::oneapi::bfloat16lllliil"(%"class.sycl::_V1::ext::oneapi::bfloat16" addrspace(1)* %src, i32 0, i32 0, i32 8, i32 16, i32 1, i32 0, i64 64)
  ret void
}


; 2B * 32 == 64B
; CHECK: .kernel "TestPrefetch_ElementSize2B_8x32"
; CHECK: lsc_load_block2d.ugm.ca.ca (M1, 1)  %null
define spir_kernel void @TestPrefetch_ElementSize2B_8x32(%"class.sycl::_V1::ext::oneapi::bfloat16" addrspace(1)* %src) {
  entry:
  call spir_func void @"_Z38__spirv_CooperativeMatrixPrefetchINTELPU3AS438class.sycl::_V1::ext::oneapi::bfloat16lllliil"(%"class.sycl::_V1::ext::oneapi::bfloat16" addrspace(1)* %src, i32 0, i32 0, i32 8, i32 32, i32 1, i32 0, i64 64)
  ret void
}


; 2B * 8 == 16B
; CHECK: .kernel "TestPrefetch_ElementSize2B_16x8"
; CHECK: lsc_load_block2d.ugm.ca.ca (M1, 1)  %null
define spir_kernel void @TestPrefetch_ElementSize2B_16x8(%"class.sycl::_V1::ext::oneapi::bfloat16" addrspace(1)* %src) {
  entry:
  call spir_func void @"_Z38__spirv_CooperativeMatrixPrefetchINTELPU3AS438class.sycl::_V1::ext::oneapi::bfloat16lllliil"(%"class.sycl::_V1::ext::oneapi::bfloat16" addrspace(1)* %src, i32 0, i32 0, i32 16, i32 8, i32 1, i32 0, i64 64)
  ret void
}


; 2B * 16 == 32B
; CHECK: .kernel "TestPrefetch_ElementSize2B_16x16"
; CHECK: lsc_load_block2d.ugm.ca.ca (M1, 1)  %null
define spir_kernel void @TestPrefetch_ElementSize2B_16x16(%"class.sycl::_V1::ext::oneapi::bfloat16" addrspace(1)* %src) {
  entry:
  call spir_func void @"_Z38__spirv_CooperativeMatrixPrefetchINTELPU3AS438class.sycl::_V1::ext::oneapi::bfloat16lllliil"(%"class.sycl::_V1::ext::oneapi::bfloat16" addrspace(1)* %src, i32 0, i32 0, i32 16, i32 16, i32 1, i32 0, i64 64)
  ret void
}


; 2B * 32 == 64B
; CHECK: .kernel "TestPrefetch_ElementSize2B_16x32"
; CHECK: lsc_load_block2d.ugm.ca.ca (M1, 1)  %null
define spir_kernel void @TestPrefetch_ElementSize2B_16x32(%"class.sycl::_V1::ext::oneapi::bfloat16" addrspace(1)* %src) {
  entry:
  call spir_func void @"_Z38__spirv_CooperativeMatrixPrefetchINTELPU3AS438class.sycl::_V1::ext::oneapi::bfloat16lllliil"(%"class.sycl::_V1::ext::oneapi::bfloat16" addrspace(1)* %src, i32 0, i32 0, i32 16, i32 32, i32 1, i32 0, i64 64)
  ret void
}


; 2B * 8 == 16B
; CHECK: .kernel "TestPrefetch_ElementSize2B_32x8"
; CHECK: lsc_load_block2d.ugm.ca.ca (M1, 1)  %null
define spir_kernel void @TestPrefetch_ElementSize2B_32x8(%"class.sycl::_V1::ext::oneapi::bfloat16" addrspace(1)* %src) {
  entry:
  call spir_func void @"_Z38__spirv_CooperativeMatrixPrefetchINTELPU3AS438class.sycl::_V1::ext::oneapi::bfloat16lllliil"(%"class.sycl::_V1::ext::oneapi::bfloat16" addrspace(1)* %src, i32 0, i32 0, i32 32, i32 8, i32 1, i32 0, i64 64)
  ret void
}


; 2B * 16 == 32B
; CHECK: .kernel "TestPrefetch_ElementSize2B_32x16"
; CHECK: lsc_load_block2d.ugm.ca.ca (M1, 1)  %null
define spir_kernel void @TestPrefetch_ElementSize2B_32x16(%"class.sycl::_V1::ext::oneapi::bfloat16" addrspace(1)* %src) {
  entry:
  call spir_func void @"_Z38__spirv_CooperativeMatrixPrefetchINTELPU3AS438class.sycl::_V1::ext::oneapi::bfloat16lllliil"(%"class.sycl::_V1::ext::oneapi::bfloat16" addrspace(1)* %src, i32 0, i32 0, i32 32, i32 16, i32 1, i32 0, i64 64)
  ret void
}


; 2B * 32 == 64B
; CHECK: .kernel "TestPrefetch_ElementSize2B_32x32"
; CHECK: lsc_load_block2d.ugm.ca.ca (M1, 1)  %null
define spir_kernel void @TestPrefetch_ElementSize2B_32x32(%"class.sycl::_V1::ext::oneapi::bfloat16" addrspace(1)* %src) {
  entry:
  call spir_func void @"_Z38__spirv_CooperativeMatrixPrefetchINTELPU3AS438class.sycl::_V1::ext::oneapi::bfloat16lllliil"(%"class.sycl::_V1::ext::oneapi::bfloat16" addrspace(1)* %src, i32 0, i32 0, i32 32, i32 32, i32 1, i32 0, i64 64)
  ret void
}


; 4B * 8 == 32B
; CHECK: .kernel "TestPrefetch_ElementSize4B_1x8"
; CHECK: lsc_load_block2d.ugm.ca.ca (M1, 1)  %null
define spir_kernel void @TestPrefetch_ElementSize4B_1x8(float addrspace(1)* %src) {
  entry:
  call spir_func void @_Z38__spirv_CooperativeMatrixPrefetchINTELPU3AS4flllliil(float addrspace(1)* %src, i32 0, i32 0, i32 1, i32 8, i32 1, i32 0, i64 64)
  ret void
}


; 4B * 16 == 64B
; CHECK: .kernel "TestPrefetch_ElementSize4B_1x16"
; CHECK: lsc_load_block2d.ugm.ca.ca (M1, 1)  %null
define spir_kernel void @TestPrefetch_ElementSize4B_1x16(float addrspace(1)* %src) {
  entry:
  call spir_func void @_Z38__spirv_CooperativeMatrixPrefetchINTELPU3AS4flllliil(float addrspace(1)* %src, i32 0, i32 0, i32 1, i32 16, i32 1, i32 0, i64 64)
  ret void
}


; 4B * 8 == 32B
; CHECK: .kernel "TestPrefetch_ElementSize4B_2x8"
; CHECK: lsc_load_block2d.ugm.ca.ca (M1, 1)  %null
define spir_kernel void @TestPrefetch_ElementSize4B_2x8(float addrspace(1)* %src) {
  entry:
  call spir_func void @_Z38__spirv_CooperativeMatrixPrefetchINTELPU3AS4flllliil(float addrspace(1)* %src, i32 0, i32 0, i32 2, i32 8, i32 1, i32 0, i64 64)
  ret void
}


; 4B * 16 == 64B
; CHECK: .kernel "TestPrefetch_ElementSize4B_2x16"
; CHECK: lsc_load_block2d.ugm.ca.ca (M1, 1)  %null
define spir_kernel void @TestPrefetch_ElementSize4B_2x16(float addrspace(1)* %src) {
  entry:
  call spir_func void @_Z38__spirv_CooperativeMatrixPrefetchINTELPU3AS4flllliil(float addrspace(1)* %src, i32 0, i32 0, i32 2, i32 16, i32 1, i32 0, i64 64)
  ret void
}


; 4B * 8 == 32B
; CHECK: .kernel "TestPrefetch_ElementSize4B_4x8"
; CHECK: lsc_load_block2d.ugm.ca.ca (M1, 1)  %null
define spir_kernel void @TestPrefetch_ElementSize4B_4x8(float addrspace(1)* %src) {
  entry:
  call spir_func void @_Z38__spirv_CooperativeMatrixPrefetchINTELPU3AS4flllliil(float addrspace(1)* %src, i32 0, i32 0, i32 4, i32 8, i32 1, i32 0, i64 64)
  ret void
}


; 4B * 16 == 64B
; CHECK: .kernel "TestPrefetch_ElementSize4B_4x16"
; CHECK: lsc_load_block2d.ugm.ca.ca (M1, 1)  %null
define spir_kernel void @TestPrefetch_ElementSize4B_4x16(float addrspace(1)* %src) {
  entry:
  call spir_func void @_Z38__spirv_CooperativeMatrixPrefetchINTELPU3AS4flllliil(float addrspace(1)* %src, i32 0, i32 0, i32 4, i32 16, i32 1, i32 0, i64 64)
  ret void
}


; 4B * 8 == 32B
; CHECK: .kernel "TestPrefetch_ElementSize4B_8x8"
; CHECK: lsc_load_block2d.ugm.ca.ca (M1, 1)  %null
define spir_kernel void @TestPrefetch_ElementSize4B_8x8(float addrspace(1)* %src) {
  entry:
  call spir_func void @_Z38__spirv_CooperativeMatrixPrefetchINTELPU3AS4flllliil(float addrspace(1)* %src, i32 0, i32 0, i32 8, i32 8, i32 1, i32 0, i64 64)
  ret void
}


; 4B * 16 == 64B
; CHECK: .kernel "TestPrefetch_ElementSize4B_8x16"
; CHECK: lsc_load_block2d.ugm.ca.ca (M1, 1)  %null
define spir_kernel void @TestPrefetch_ElementSize4B_8x16(float addrspace(1)* %src) {
  entry:
  call spir_func void @_Z38__spirv_CooperativeMatrixPrefetchINTELPU3AS4flllliil(float addrspace(1)* %src, i32 0, i32 0, i32 8, i32 16, i32 1, i32 0, i64 64)
  ret void
}


; 4B * 8 == 32B
; CHECK: .kernel "TestPrefetch_ElementSize4B_16x8"
; CHECK: lsc_load_block2d.ugm.ca.ca (M1, 1)  %null
define spir_kernel void @TestPrefetch_ElementSize4B_16x8(float addrspace(1)* %src) {
  entry:
  call spir_func void @_Z38__spirv_CooperativeMatrixPrefetchINTELPU3AS4flllliil(float addrspace(1)* %src, i32 0, i32 0, i32 16, i32 8, i32 1, i32 0, i64 64)
  ret void
}


; 4B * 16 == 64B
; CHECK: .kernel "TestPrefetch_ElementSize4B_16x16"
; CHECK: lsc_load_block2d.ugm.ca.ca (M1, 1)  %null
define spir_kernel void @TestPrefetch_ElementSize4B_16x16(float addrspace(1)* %src) {
  entry:
  call spir_func void @_Z38__spirv_CooperativeMatrixPrefetchINTELPU3AS4flllliil(float addrspace(1)* %src, i32 0, i32 0, i32 16, i32 16, i32 1, i32 0, i64 64)
  ret void
}


; 4B * 8 == 32B
; CHECK: .kernel "TestPrefetch_ElementSize4B_32x8"
; CHECK: lsc_load_block2d.ugm.ca.ca (M1, 1)  %null
define spir_kernel void @TestPrefetch_ElementSize4B_32x8(float addrspace(1)* %src) {
  entry:
  call spir_func void @_Z38__spirv_CooperativeMatrixPrefetchINTELPU3AS4flllliil(float addrspace(1)* %src, i32 0, i32 0, i32 32, i32 8, i32 1, i32 0, i64 64)
  ret void
}


; 4B * 16 == 64B
; CHECK: .kernel "TestPrefetch_ElementSize4B_32x16"
; CHECK: lsc_load_block2d.ugm.ca.ca (M1, 1)  %null
define spir_kernel void @TestPrefetch_ElementSize4B_32x16(float addrspace(1)* %src) {
  entry:
  call spir_func void @_Z38__spirv_CooperativeMatrixPrefetchINTELPU3AS4flllliil(float addrspace(1)* %src, i32 0, i32 0, i32 32, i32 16, i32 1, i32 0, i64 64)
  ret void
}


; 8B * 8 == 64B
; CHECK: .kernel "TestPrefetch_ElementSize8B_1x8"
; CHECK: lsc_load_block2d.ugm.ca.ca (M1, 1)  %null
define spir_kernel void @TestPrefetch_ElementSize8B_1x8(double addrspace(1)* %src) {
  entry:
  call spir_func void @_Z38__spirv_CooperativeMatrixPrefetchINTELPU3AS4diiiiiil(double addrspace(1)* %src, i32 0, i32 0, i32 1, i32 8, i32 1, i32 0, i64 64)
  ret void
}


; 8B * 8 == 64B
; CHECK: .kernel "TestPrefetch_ElementSize8B_2x8"
; CHECK: lsc_load_block2d.ugm.ca.ca (M1, 1)  %null
define spir_kernel void @TestPrefetch_ElementSize8B_2x8(double addrspace(1)* %src) {
  entry:
  call spir_func void @_Z38__spirv_CooperativeMatrixPrefetchINTELPU3AS4diiiiiil(double addrspace(1)* %src, i32 0, i32 0, i32 2, i32 8, i32 1, i32 0, i64 64)
  ret void
}


; 8B * 8 == 64B
; CHECK: .kernel "TestPrefetch_ElementSize8B_4x8"
; CHECK: lsc_load_block2d.ugm.ca.ca (M1, 1)  %null
define spir_kernel void @TestPrefetch_ElementSize8B_4x8(double addrspace(1)* %src) {
  entry:
  call spir_func void @_Z38__spirv_CooperativeMatrixPrefetchINTELPU3AS4diiiiiil(double addrspace(1)* %src, i32 0, i32 0, i32 4, i32 8, i32 1, i32 0, i64 64)
  ret void
}


; 8B * 8 == 64B
; CHECK: .kernel "TestPrefetch_ElementSize8B_8x8"
; CHECK: lsc_load_block2d.ugm.ca.ca (M1, 1)  %null
define spir_kernel void @TestPrefetch_ElementSize8B_8x8(double addrspace(1)* %src) {
  entry:
  call spir_func void @_Z38__spirv_CooperativeMatrixPrefetchINTELPU3AS4diiiiiil(double addrspace(1)* %src, i32 0, i32 0, i32 8, i32 8, i32 1, i32 0, i64 64)
  ret void
}


; 8B * 8 == 64B
; CHECK: .kernel "TestPrefetch_ElementSize8B_16x8"
; CHECK: lsc_load_block2d.ugm.ca.ca (M1, 1)  %null
define spir_kernel void @TestPrefetch_ElementSize8B_16x8(double addrspace(1)* %src) {
  entry:
  call spir_func void @_Z38__spirv_CooperativeMatrixPrefetchINTELPU3AS4diiiiiil(double addrspace(1)* %src, i32 0, i32 0, i32 16, i32 8, i32 1, i32 0, i64 64)
  ret void
}


; 8B * 8 == 64B
; CHECK: .kernel "TestPrefetch_ElementSize8B_32x8"
; CHECK: lsc_load_block2d.ugm.ca.ca (M1, 1)  %null
define spir_kernel void @TestPrefetch_ElementSize8B_32x8(double addrspace(1)* %src) {
  entry:
  call spir_func void @_Z38__spirv_CooperativeMatrixPrefetchINTELPU3AS4diiiiiil(double addrspace(1)* %src, i32 0, i32 0, i32 32, i32 8, i32 1, i32 0, i64 64)
  ret void
}

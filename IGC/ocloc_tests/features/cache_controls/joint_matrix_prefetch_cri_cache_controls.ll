;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; UNSUPPORTED: system-windows
; REQUIRES: llvm-spirv, cri-supported

; LLVM with opaque pointers:
; RUN: llvm-as %OPAQUE_PTR_FLAG% %s -o %t.bc
; RUN: llvm-spirv %t.bc %OPAQUE_PTR_FLAG% --spirv-ext=+SPV_INTEL_cache_controls,+SPV_INTEL_joint_matrix -o %t.spv
; RUN: ocloc compile -spirv_input -file %t.spv -device cri -options " -igc_opts 'EnableOpaquePointersBackend=1, DumpVISAASMToConsole=1'" -internal_options "'-ze-intel-64bit-addressing'" 2>&1 | FileCheck %s


; LLVM with typed pointers/default pointer typing:
; RUN: llvm-as %TYPED_PTR_FLAG% %s -o %t.bc
; RUN: llvm-spirv %t.bc %TYPED_PTR_FLAG% --spirv-ext=+SPV_INTEL_cache_controls,+SPV_INTEL_joint_matrix -o %t.spv
; RUN: ocloc compile -spirv_input -file %t.spv -device cri -options " -igc_opts 'DumpVISAASMToConsole=1'" -internal_options "'-ze-intel-64bit-addressing'" 2>&1 | FileCheck %s

; This tests 2d block prefetches that are loading exactly 256 bytes.
; These prefetches are supported only for XE3P+.


target triple = "spir64-unknown-unknown"

; 8 bit prefetch
declare spir_func void @_Z38__spirv_CooperativeMatrixPrefetchINTELPU3AS4ciiiil(i8 addrspace(1)*, i32, i32, i32, i32, i64)

; 16 bit prefetch
%"class.sycl::_V1::ext::oneapi::bfloat16" = type { i16 }
declare spir_func void @"_Z38__spirv_CooperativeMatrixPrefetchINTELPU3AS438class.sycl::_V1::ext::oneapi::bfloat16iiiil"(%"class.sycl::_V1::ext::oneapi::bfloat16" addrspace(1)*, i32, i32, i32, i32, i64)

; 32 bit prefetch
declare spir_func void @_Z38__spirv_CooperativeMatrixPrefetchINTELPU3AS4fiiiil(float addrspace(1)*, i32, i32, i32, i32, i64)

; 64 bit prefetch
declare spir_func void @_Z38__spirv_CooperativeMatrixPrefetchINTELPU3AS4diiiil(double addrspace(1)*, i32, i32, i32, i32, i64)



; 1B * 256 == 256B
; CHECK: .kernel "TestPrefetch_ElementSize1B_1x256"
; CHECK: lsc_load_block2d.ugm.ca.ca.uc (M1, 1)  %null
define spir_kernel void @TestPrefetch_ElementSize1B_1x256(i8 addrspace(1)* %src) {
  entry:
  %gep = getelementptr i8, i8 addrspace(1)* %src, i64 0
  call spir_func void @_Z38__spirv_CooperativeMatrixPrefetchINTELPU3AS4ciiiil(i8 addrspace(1)* %gep, i32 1, i32 256, i32 1, i32 0, i64 256)
  ret void
}


; 1B * 256 == 256B
; CHECK: .kernel "TestPrefetch_ElementSize1B_2x256"
; CHECK: lsc_load_block2d.ugm.ca.ca.uc (M1, 1)  %null
define spir_kernel void @TestPrefetch_ElementSize1B_2x256(i8 addrspace(1)* %src) {
  entry:
  %gep = getelementptr i8, i8 addrspace(1)* %src, i64 0
  call spir_func void @_Z38__spirv_CooperativeMatrixPrefetchINTELPU3AS4ciiiil(i8 addrspace(1)* %gep, i32 2, i32 256, i32 1, i32 0, i64 256)
  ret void
}


; 1B * 256 == 256B
; CHECK: .kernel "TestPrefetch_ElementSize1B_4x256"
; CHECK: lsc_load_block2d.ugm.ca.ca.uc (M1, 1)  %null
define spir_kernel void @TestPrefetch_ElementSize1B_4x256(i8 addrspace(1)* %src) {
  entry:
  %gep = getelementptr i8, i8 addrspace(1)* %src, i64 0
  call spir_func void @_Z38__spirv_CooperativeMatrixPrefetchINTELPU3AS4ciiiil(i8 addrspace(1)* %gep, i32 4, i32 256, i32 1, i32 0, i64 256)
  ret void
}


; 1B * 256 == 256B
; CHECK: .kernel "TestPrefetch_ElementSize1B_8x256"
; CHECK: lsc_load_block2d.ugm.ca.ca.uc (M1, 1)  %null
define spir_kernel void @TestPrefetch_ElementSize1B_8x256(i8 addrspace(1)* %src) {
  entry:
  %gep = getelementptr i8, i8 addrspace(1)* %src, i64 0
  call spir_func void @_Z38__spirv_CooperativeMatrixPrefetchINTELPU3AS4ciiiil(i8 addrspace(1)* %gep, i32 8, i32 256, i32 1, i32 0, i64 256)
  ret void
}


; 1B * 256 == 256B
; CHECK: .kernel "TestPrefetch_ElementSize1B_16x256"
; CHECK: lsc_load_block2d.ugm.ca.ca.uc (M1, 1)  %null
define spir_kernel void @TestPrefetch_ElementSize1B_16x256(i8 addrspace(1)* %src) {
  entry:
  %gep = getelementptr i8, i8 addrspace(1)* %src, i64 0
  call spir_func void @_Z38__spirv_CooperativeMatrixPrefetchINTELPU3AS4ciiiil(i8 addrspace(1)* %gep, i32 16, i32 256, i32 1, i32 0, i64 256)
  ret void
}


; 1B * 256 == 256B
; CHECK: .kernel "TestPrefetch_ElementSize1B_32x256"
; CHECK: lsc_load_block2d.ugm.ca.ca.uc (M1, 1)  %null
define spir_kernel void @TestPrefetch_ElementSize1B_32x256(i8 addrspace(1)* %src) {
  entry:
  %gep = getelementptr i8, i8 addrspace(1)* %src, i64 0
  call spir_func void @_Z38__spirv_CooperativeMatrixPrefetchINTELPU3AS4ciiiil(i8 addrspace(1)* %gep, i32 32, i32 256, i32 1, i32 0, i64 256)
  ret void
}


; 2B * 128 == 256B
; CHECK: .kernel "TestPrefetch_ElementSize2B_1x128"
; CHECK: lsc_load_block2d.ugm.ca.ca.uc (M1, 1)  %null
define spir_kernel void @TestPrefetch_ElementSize2B_1x128(%"class.sycl::_V1::ext::oneapi::bfloat16" addrspace(1)* %src) {
  entry:
  %gep = getelementptr %"class.sycl::_V1::ext::oneapi::bfloat16", %"class.sycl::_V1::ext::oneapi::bfloat16" addrspace(1)* %src, i64 0
  call spir_func void @"_Z38__spirv_CooperativeMatrixPrefetchINTELPU3AS438class.sycl::_V1::ext::oneapi::bfloat16iiiil"(%"class.sycl::_V1::ext::oneapi::bfloat16" addrspace(1)* %gep, i32 1, i32 128, i32 1, i32 0, i64 256)
  ret void
}


; 2B * 128 == 256B
; CHECK: .kernel "TestPrefetch_ElementSize2B_2x128"
; CHECK: lsc_load_block2d.ugm.ca.ca.uc (M1, 1)  %null
define spir_kernel void @TestPrefetch_ElementSize2B_2x128(%"class.sycl::_V1::ext::oneapi::bfloat16" addrspace(1)* %src) {
  entry:
  %gep = getelementptr %"class.sycl::_V1::ext::oneapi::bfloat16", %"class.sycl::_V1::ext::oneapi::bfloat16" addrspace(1)* %src, i64 0
  call spir_func void @"_Z38__spirv_CooperativeMatrixPrefetchINTELPU3AS438class.sycl::_V1::ext::oneapi::bfloat16iiiil"(%"class.sycl::_V1::ext::oneapi::bfloat16" addrspace(1)* %gep, i32 2, i32 128, i32 1, i32 0, i64 256)
  ret void
}


; 2B * 128 == 256B
; CHECK: .kernel "TestPrefetch_ElementSize2B_4x128"
; CHECK: lsc_load_block2d.ugm.ca.ca.uc (M1, 1)  %null
define spir_kernel void @TestPrefetch_ElementSize2B_4x128(%"class.sycl::_V1::ext::oneapi::bfloat16" addrspace(1)* %src) {
  entry:
  %gep = getelementptr %"class.sycl::_V1::ext::oneapi::bfloat16", %"class.sycl::_V1::ext::oneapi::bfloat16" addrspace(1)* %src, i64 0
  call spir_func void @"_Z38__spirv_CooperativeMatrixPrefetchINTELPU3AS438class.sycl::_V1::ext::oneapi::bfloat16iiiil"(%"class.sycl::_V1::ext::oneapi::bfloat16" addrspace(1)* %gep, i32 4, i32 128, i32 1, i32 0, i64 256)
  ret void
}


; 2B * 128 == 256B
; CHECK: .kernel "TestPrefetch_ElementSize2B_8x128"
; CHECK: lsc_load_block2d.ugm.ca.ca.uc (M1, 1)  %null
define spir_kernel void @TestPrefetch_ElementSize2B_8x128(%"class.sycl::_V1::ext::oneapi::bfloat16" addrspace(1)* %src) {
  entry:
  %gep = getelementptr %"class.sycl::_V1::ext::oneapi::bfloat16", %"class.sycl::_V1::ext::oneapi::bfloat16" addrspace(1)* %src, i64 0
  call spir_func void @"_Z38__spirv_CooperativeMatrixPrefetchINTELPU3AS438class.sycl::_V1::ext::oneapi::bfloat16iiiil"(%"class.sycl::_V1::ext::oneapi::bfloat16" addrspace(1)* %gep, i32 8, i32 128, i32 1, i32 0, i64 256)
  ret void
}


; 2B * 128 == 256B
; CHECK: .kernel "TestPrefetch_ElementSize2B_16x128"
; CHECK: lsc_load_block2d.ugm.ca.ca.uc (M1, 1)  %null
define spir_kernel void @TestPrefetch_ElementSize2B_16x128(%"class.sycl::_V1::ext::oneapi::bfloat16" addrspace(1)* %src) {
  entry:
  %gep = getelementptr %"class.sycl::_V1::ext::oneapi::bfloat16", %"class.sycl::_V1::ext::oneapi::bfloat16" addrspace(1)* %src, i64 0
  call spir_func void @"_Z38__spirv_CooperativeMatrixPrefetchINTELPU3AS438class.sycl::_V1::ext::oneapi::bfloat16iiiil"(%"class.sycl::_V1::ext::oneapi::bfloat16" addrspace(1)* %gep, i32 16, i32 128, i32 1, i32 0, i64 256)
  ret void
}


; 2B * 128 == 256B
; CHECK: .kernel "TestPrefetch_ElementSize2B_32x128"
; CHECK: lsc_load_block2d.ugm.ca.ca.uc (M1, 1)  %null
define spir_kernel void @TestPrefetch_ElementSize2B_32x128(%"class.sycl::_V1::ext::oneapi::bfloat16" addrspace(1)* %src) {
  entry:
  %gep = getelementptr %"class.sycl::_V1::ext::oneapi::bfloat16", %"class.sycl::_V1::ext::oneapi::bfloat16" addrspace(1)* %src, i64 0
  call spir_func void @"_Z38__spirv_CooperativeMatrixPrefetchINTELPU3AS438class.sycl::_V1::ext::oneapi::bfloat16iiiil"(%"class.sycl::_V1::ext::oneapi::bfloat16" addrspace(1)* %gep, i32 32, i32 128, i32 1, i32 0, i64 256)
  ret void
}


; 4B * 64 == 256B
; CHECK: .kernel "TestPrefetch_ElementSize4B_1x64"
; CHECK: lsc_load_block2d.ugm.ca.ca.uc (M1, 1)  %null
define spir_kernel void @TestPrefetch_ElementSize4B_1x64(float addrspace(1)* %src) {
  entry:
  %gep = getelementptr float, float addrspace(1)* %src, i64 0
  call spir_func void @_Z38__spirv_CooperativeMatrixPrefetchINTELPU3AS4fiiiil(float addrspace(1)* %gep, i32 1, i32 64, i32 1, i32 0, i64 256)
  ret void
}


; 4B * 64 == 256B
; CHECK: .kernel "TestPrefetch_ElementSize4B_2x64"
; CHECK: lsc_load_block2d.ugm.ca.ca.uc (M1, 1)  %null
define spir_kernel void @TestPrefetch_ElementSize4B_2x64(float addrspace(1)* %src) {
  entry:
  %gep = getelementptr float, float addrspace(1)* %src, i64 0
  call spir_func void @_Z38__spirv_CooperativeMatrixPrefetchINTELPU3AS4fiiiil(float addrspace(1)* %gep, i32 2, i32 64, i32 1, i32 0, i64 256)
  ret void
}


; 4B * 64 == 256B
; CHECK: .kernel "TestPrefetch_ElementSize4B_4x64"
; CHECK: lsc_load_block2d.ugm.ca.ca.uc (M1, 1)  %null
define spir_kernel void @TestPrefetch_ElementSize4B_4x64(float addrspace(1)* %src) {
  entry:
  %gep = getelementptr float, float addrspace(1)* %src, i64 0
  call spir_func void @_Z38__spirv_CooperativeMatrixPrefetchINTELPU3AS4fiiiil(float addrspace(1)* %gep, i32 4, i32 64, i32 1, i32 0, i64 256)
  ret void
}


; 4B * 64 == 256B
; CHECK: .kernel "TestPrefetch_ElementSize4B_8x64"
; CHECK: lsc_load_block2d.ugm.ca.ca.uc (M1, 1)  %null
define spir_kernel void @TestPrefetch_ElementSize4B_8x64(float addrspace(1)* %src) {
  entry:
  %gep = getelementptr float, float addrspace(1)* %src, i64 0
  call spir_func void @_Z38__spirv_CooperativeMatrixPrefetchINTELPU3AS4fiiiil(float addrspace(1)* %gep, i32 8, i32 64, i32 1, i32 0, i64 256)
  ret void
}


; 4B * 64 == 256B
; CHECK: .kernel "TestPrefetch_ElementSize4B_16x64"
; CHECK: lsc_load_block2d.ugm.ca.ca.uc (M1, 1)  %null
define spir_kernel void @TestPrefetch_ElementSize4B_16x64(float addrspace(1)* %src) {
  entry:
  %gep = getelementptr float, float addrspace(1)* %src, i64 0
  call spir_func void @_Z38__spirv_CooperativeMatrixPrefetchINTELPU3AS4fiiiil(float addrspace(1)* %gep, i32 16, i32 64, i32 1, i32 0, i64 256)
  ret void
}


; 4B * 64 == 256B
; CHECK: .kernel "TestPrefetch_ElementSize4B_32x64"
; CHECK: lsc_load_block2d.ugm.ca.ca.uc (M1, 1)  %null
define spir_kernel void @TestPrefetch_ElementSize4B_32x64(float addrspace(1)* %src) {
  entry:
  %gep = getelementptr float, float addrspace(1)* %src, i64 0
  call spir_func void @_Z38__spirv_CooperativeMatrixPrefetchINTELPU3AS4fiiiil(float addrspace(1)* %gep, i32 32, i32 64, i32 1, i32 0, i64 256)
  ret void
}


; 8B * 32 == 256B
; CHECK: .kernel "TestPrefetch_ElementSize8B_1x32"
; CHECK: lsc_load_block2d.ugm.ca.ca.uc (M1, 1)  %null
define spir_kernel void @TestPrefetch_ElementSize8B_1x32(double addrspace(1)* %src) {
  entry:
  %gep = getelementptr double, double addrspace(1)* %src, i64 0
  call spir_func void @_Z38__spirv_CooperativeMatrixPrefetchINTELPU3AS4diiiil(double addrspace(1)* %gep, i32 1, i32 32, i32 1, i32 0, i64 256)
  ret void
}


; 8B * 32 == 256B
; CHECK: .kernel "TestPrefetch_ElementSize8B_2x32"
; CHECK: lsc_load_block2d.ugm.ca.ca.uc (M1, 1)  %null
define spir_kernel void @TestPrefetch_ElementSize8B_2x32(double addrspace(1)* %src) {
  entry:
  %gep = getelementptr double, double addrspace(1)* %src, i64 0
  call spir_func void @_Z38__spirv_CooperativeMatrixPrefetchINTELPU3AS4diiiil(double addrspace(1)* %gep, i32 2, i32 32, i32 1, i32 0, i64 256)
  ret void
}


; 8B * 32 == 256B
; CHECK: .kernel "TestPrefetch_ElementSize8B_4x32"
; CHECK: lsc_load_block2d.ugm.ca.ca.uc (M1, 1)  %null
define spir_kernel void @TestPrefetch_ElementSize8B_4x32(double addrspace(1)* %src) {
  entry:
  %gep = getelementptr double, double addrspace(1)* %src, i64 0
  call spir_func void @_Z38__spirv_CooperativeMatrixPrefetchINTELPU3AS4diiiil(double addrspace(1)* %gep, i32 4, i32 32, i32 1, i32 0, i64 256)
  ret void
}


; 8B * 32 == 256B
; CHECK: .kernel "TestPrefetch_ElementSize8B_8x32"
; CHECK: lsc_load_block2d.ugm.ca.ca.uc (M1, 1)  %null
define spir_kernel void @TestPrefetch_ElementSize8B_8x32(double addrspace(1)* %src) {
  entry:
  %gep = getelementptr double, double addrspace(1)* %src, i64 0
  call spir_func void @_Z38__spirv_CooperativeMatrixPrefetchINTELPU3AS4diiiil(double addrspace(1)* %gep, i32 8, i32 32, i32 1, i32 0, i64 256)
  ret void
}


; 8B * 32 == 256B
; CHECK: .kernel "TestPrefetch_ElementSize8B_16x32"
; CHECK: lsc_load_block2d.ugm.ca.ca.uc (M1, 1)  %null
define spir_kernel void @TestPrefetch_ElementSize8B_16x32(double addrspace(1)* %src) {
  entry:
  %gep = getelementptr double, double addrspace(1)* %src, i64 0
  call spir_func void @_Z38__spirv_CooperativeMatrixPrefetchINTELPU3AS4diiiil(double addrspace(1)* %gep, i32 16, i32 32, i32 1, i32 0, i64 256)
  ret void
}


; 8B * 32 == 256B
; CHECK: .kernel "TestPrefetch_ElementSize8B_32x32"
; CHECK: lsc_load_block2d.ugm.ca.ca.uc (M1, 1)  %null
define spir_kernel void @TestPrefetch_ElementSize8B_32x32(double addrspace(1)* %src) {
  entry:
  %gep = getelementptr double, double addrspace(1)* %src, i64 0
  call spir_func void @_Z38__spirv_CooperativeMatrixPrefetchINTELPU3AS4diiiil(double addrspace(1)* %gep, i32 32, i32 32, i32 1, i32 0, i64 256)
  ret void
}

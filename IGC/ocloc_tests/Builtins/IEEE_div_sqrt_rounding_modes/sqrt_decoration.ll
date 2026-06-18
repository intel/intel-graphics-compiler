;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: cri-supported

; RUN: llvm-as %s -o %t.bc
; RUN: ocloc compile -llvm_input -file %t.bc -device cri -options " -igc_opts 'DumpVISAASMToConsole=1'" | FileCheck %s


target triple = "spir64-unknown-unknown"

declare half @_Z16__spirv_ocl_sqrtDh(half)
declare float @_Z16__spirv_ocl_sqrtf(float)
declare double @_Z16__spirv_ocl_sqrtd(double)

; fp16 tests: half-precision sqrt is promoted to float (sqrtm in RNE),
; then truncated back to half with the specified rounding mode.

; CHECK-LABEL:      .kernel "test_sqrt_fp16_rne"
; CHECK:            mov (M1_NM, 1) [[ext:.*]](0,0)<1> {{.*}}
; CHECK:            sqrtm (M1_NM, 1) [[sqrt:.*]](0,0)<1> [[ext]](0,0)<0;1,0>
; CHECK-NOT:        xor (M1_NM, 1) %cr0(0,0)<1> %cr0(0,0)<0;1,0> {{.*}}
; CHECK:            mov (M1, {{16|32}}) {{.*}}(0,0)<1> [[sqrt]](0,0)<0;1,0>
; CHECK-NOT:        xor (M1_NM, 1) %cr0(0,0)<1> %cr0(0,0)<0;1,0> {{.*}}
; CHECK:            ret {{.*}}
define spir_kernel void @test_sqrt_fp16_rne(half %a, half addrspace(1)* align 2 %b) {
entry:
  %sqrt = call half @_Z16__spirv_ocl_sqrtDh(half %a), !spirv.Decorations !1
  store half %sqrt, half addrspace(1)* %b, align 2
  ret void
}

; CHECK-LABEL:      .kernel "test_sqrt_fp16_rtz"
; CHECK:            mov (M1_NM, 1) [[ext:.*]](0,0)<1> {{.*}}
; CHECK:            sqrtm (M1_NM, 1) [[sqrt:.*]](0,0)<1> [[ext]](0,0)<0;1,0>
; CHECK:            xor (M1_NM, 1) %cr0(0,0)<1> %cr0(0,0)<0;1,0> 0x30:ud
; CHECK:            mov (M1, {{16|32}}) {{.*}}(0,0)<1> [[sqrt]](0,0)<0;1,0>
; CHECK:            xor (M1_NM, 1) %cr0(0,0)<1> %cr0(0,0)<0;1,0> 0x30:ud
; CHECK:            ret {{.*}}
define spir_kernel void @test_sqrt_fp16_rtz(half %a, half addrspace(1)* align 2 %b) {
entry:
  %sqrt = call half @_Z16__spirv_ocl_sqrtDh(half %a), !spirv.Decorations !2
  store half %sqrt, half addrspace(1)* %b, align 2
  ret void
}

; CHECK-LABEL:      .kernel "test_sqrt_fp16_rtp"
; CHECK:            mov (M1_NM, 1) [[ext:.*]](0,0)<1> {{.*}}
; CHECK:            sqrtm (M1_NM, 1) [[sqrt:.*]](0,0)<1> [[ext]](0,0)<0;1,0>
; CHECK:            xor (M1_NM, 1) %cr0(0,0)<1> %cr0(0,0)<0;1,0> 0x10:ud
; CHECK:            mov (M1, {{16|32}}) {{.*}}(0,0)<1> [[sqrt]](0,0)<0;1,0>
; CHECK:            xor (M1_NM, 1) %cr0(0,0)<1> %cr0(0,0)<0;1,0> 0x10:ud
; CHECK:            ret {{.*}}
define spir_kernel void @test_sqrt_fp16_rtp(half %a, half addrspace(1)* align 2 %b) {
entry:
  %sqrt = call half @_Z16__spirv_ocl_sqrtDh(half %a), !spirv.Decorations !3
  store half %sqrt, half addrspace(1)* %b, align 2
  ret void
}

; CHECK-LABEL:      .kernel "test_sqrt_fp16_rtn"
; CHECK:            mov (M1_NM, 1) [[ext:.*]](0,0)<1> {{.*}}
; CHECK:            sqrtm (M1_NM, 1) [[sqrt:.*]](0,0)<1> [[ext]](0,0)<0;1,0>
; CHECK:            xor (M1_NM, 1) %cr0(0,0)<1> %cr0(0,0)<0;1,0> 0x20:ud
; CHECK:            mov (M1, {{16|32}}) {{.*}}(0,0)<1> [[sqrt]](0,0)<0;1,0>
; CHECK:            xor (M1_NM, 1) %cr0(0,0)<1> %cr0(0,0)<0;1,0> 0x20:ud
; CHECK:            ret {{.*}}
define spir_kernel void @test_sqrt_fp16_rtn(half %a, half addrspace(1)* align 2 %b) {
entry:
  %sqrt = call half @_Z16__spirv_ocl_sqrtDh(half %a), !spirv.Decorations !4
  store half %sqrt, half addrspace(1)* %b, align 2
  ret void
}

; CHECK-LABEL:      .kernel "test_sqrt_fp32_rne"
; CHECK-NOT:        xor (M1_NM, 1) %cr0(0,0)<1> %cr0(0,0)<0;1,0> {{.*}}
; CHECK:            sqrtm (M1_NM, 1) {{.*}} {{.*}}
; CHECK-NOT:        xor (M1_NM, 1) %cr0(0,0)<1> %cr0(0,0)<0;1,0> {{.*}}
; CHECK:            ret {{.*}}
define spir_kernel void @test_sqrt_fp32_rne(float %a, float addrspace(1)* align 4 %b) {
entry:
  %sqrt = call float @_Z16__spirv_ocl_sqrtf(float %a), !spirv.Decorations !1
  store float %sqrt, float addrspace(1)* %b, align 4
  ret void
}

; CHECK-LABEL:      .kernel "test_sqrt_fp32_rtz"
; CHECK:            xor (M1_NM, 1) %cr0(0,0)<1> %cr0(0,0)<0;1,0> 0x30:ud {{.*}}
; CHECK:            sqrtm (M1_NM, 1) {{.*}} {{.*}}
; CHECK:            xor (M1_NM, 1) %cr0(0,0)<1> %cr0(0,0)<0;1,0> 0x30:ud {{.*}}
; CHECK:            ret {{.*}}
define spir_kernel void @test_sqrt_fp32_rtz(float %a, float addrspace(1)* align 4 %b) {
entry:
  %sqrt = call float @_Z16__spirv_ocl_sqrtf(float %a), !spirv.Decorations !2
  store float %sqrt, float addrspace(1)* %b, align 4
  ret void
}

; CHECK-LABEL:      .kernel "test_sqrt_fp32_rtp"
; CHECK:            xor (M1_NM, 1) %cr0(0,0)<1> %cr0(0,0)<0;1,0> 0x10:ud {{.*}}
; CHECK:            sqrtm (M1_NM, 1) {{.*}} {{.*}}
; CHECK:            xor (M1_NM, 1) %cr0(0,0)<1> %cr0(0,0)<0;1,0> 0x10:ud {{.*}}
; CHECK:            ret {{.*}}
define spir_kernel void @test_sqrt_fp32_rtp(float %a, float addrspace(1)* align 4 %b) {
entry:
  %sqrt = call float @_Z16__spirv_ocl_sqrtf(float %a), !spirv.Decorations !3
  store float %sqrt, float addrspace(1)* %b, align 4
  ret void
}

; CHECK-LABEL:      .kernel "test_sqrt_fp32_rtn"
; CHECK:            xor (M1_NM, 1) %cr0(0,0)<1> %cr0(0,0)<0;1,0> 0x20:ud {{.*}}
; CHECK:            sqrtm (M1_NM, 1) {{.*}} {{.*}}
; CHECK:            xor (M1_NM, 1) %cr0(0,0)<1> %cr0(0,0)<0;1,0> 0x20:ud {{.*}}
; CHECK:            ret {{.*}}
define spir_kernel void @test_sqrt_fp32_rtn(float %a, float addrspace(1)* align 4 %b) {
entry:
  %sqrt = call float @_Z16__spirv_ocl_sqrtf(float %a), !spirv.Decorations !4
  store float %sqrt, float addrspace(1)* %b, align 4
  ret void
}

; CHECK-LABEL:      .kernel "test_sqrt_fp64_rne"
; CHECK-NOT:        xor (M1_NM, 1) %cr0(0,0)<1> %cr0(0,0)<0;1,0> {{.*}}
; CHECK:            sqrtm (M1_NM, 1) {{.*}} {{.*}}
; CHECK-NOT:        xor (M1_NM, 1) %cr0(0,0)<1> %cr0(0,0)<0;1,0> {{.*}}
; CHECK:            ret {{.*}}
define spir_kernel void @test_sqrt_fp64_rne(double %a, double addrspace(1)* align 8 %b) {
entry:
  %sqrt = call double @_Z16__spirv_ocl_sqrtd(double %a), !spirv.Decorations !1
  store double %sqrt, double addrspace(1)* %b, align 8
  ret void
}

; CHECK-LABEL:      .kernel "test_sqrt_fp64_rtz"
; CHECK:            xor (M1_NM, 1) %cr0(0,0)<1> %cr0(0,0)<0;1,0> 0x30:ud {{.*}}
; CHECK:            sqrtm (M1_NM, 1) {{.*}} {{.*}}
; CHECK:            xor (M1_NM, 1) %cr0(0,0)<1> %cr0(0,0)<0;1,0> 0x30:ud {{.*}}
; CHECK:            ret {{.*}}
define spir_kernel void @test_sqrt_fp64_rtz(double %a, double addrspace(1)* align 8 %b) {
entry:
  %sqrt = call double @_Z16__spirv_ocl_sqrtd(double %a), !spirv.Decorations !2
  store double %sqrt, double addrspace(1)* %b, align 8
  ret void
}

; CHECK-LABEL:      .kernel "test_sqrt_fp64_rtp"
; CHECK:            xor (M1_NM, 1) %cr0(0,0)<1> %cr0(0,0)<0;1,0> 0x10:ud {{.*}}
; CHECK:            sqrtm (M1_NM, 1) {{.*}} {{.*}}
; CHECK:            xor (M1_NM, 1) %cr0(0,0)<1> %cr0(0,0)<0;1,0> 0x10:ud {{.*}}
; CHECK:            ret {{.*}}
define spir_kernel void @test_sqrt_fp64_rtp(double %a, double addrspace(1)* align 8 %b) {
entry:
  %sqrt = call double @_Z16__spirv_ocl_sqrtd(double %a), !spirv.Decorations !3
  store double %sqrt, double addrspace(1)* %b, align 8
  ret void
}

; CHECK-LABEL:      .kernel "test_sqrt_fp64_rtn"
; CHECK:            xor (M1_NM, 1) %cr0(0,0)<1> %cr0(0,0)<0;1,0> 0x20:ud {{.*}}
; CHECK:            sqrtm (M1_NM, 1) {{.*}} {{.*}}
; CHECK:            xor (M1_NM, 1) %cr0(0,0)<1> %cr0(0,0)<0;1,0> 0x20:ud {{.*}}
; CHECK:            ret {{.*}}
define spir_kernel void @test_sqrt_fp64_rtn(double %a, double addrspace(1)* align 8 %b) {
entry:
  %sqrt = call double @_Z16__spirv_ocl_sqrtd(double %a), !spirv.Decorations !4
  store double %sqrt, double addrspace(1)* %b, align 8
  ret void
}

!1 = !{!5}
!2 = !{!6}
!3 = !{!7}
!4 = !{!8}
!5 = !{i32 39, i32 0} ; RNE
!6 = !{i32 39, i32 1} ; RTZ
!7 = !{i32 39, i32 2} ; RTP
!8 = !{i32 39, i32 3} ; RTN

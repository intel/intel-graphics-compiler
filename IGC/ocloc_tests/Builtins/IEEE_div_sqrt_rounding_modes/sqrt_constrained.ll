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

declare float @llvm.experimental.constrained.sqrt.f32(float, metadata, metadata)
declare double @llvm.experimental.constrained.sqrt.f64(double, metadata, metadata)
declare half @llvm.experimental.constrained.sqrt.f16(half, metadata, metadata)

; CHECK-LABEL:      .kernel "test_sqrt_fp32_rne"
; CHECK-NOT:        xor (M1_NM, 1) %cr0(0,0)<1> %cr0(0,0)<0;1,0> {{.*}}
; CHECK:            sqrtm (M1_NM, 1) {{.*}} {{.*}}
; CHECK-NOT:        xor (M1_NM, 1) %cr0(0,0)<1> %cr0(0,0)<0;1,0> {{.*}}
; CHECK:            ret {{.*}}
define spir_kernel void @test_sqrt_fp32_rne(float %a, float addrspace(1)* align 4 %c) {
entry:
  %res = call float @llvm.experimental.constrained.sqrt.f32(float %a, metadata !"round.tonearest", metadata !"fpexcept.ignore")
  store float %res, float addrspace(1)* %c, align 4
  ret void
}

; CHECK-LABEL:      .kernel "test_sqrt_fp32_rtz"
; CHECK:            xor (M1_NM, 1) %cr0(0,0)<1> %cr0(0,0)<0;1,0> 0x30:ud {{.*}}
; CHECK:            sqrtm (M1_NM, 1) {{.*}} {{.*}}
; CHECK:            xor (M1_NM, 1) %cr0(0,0)<1> %cr0(0,0)<0;1,0> 0x30:ud {{.*}}
; CHECK:            ret {{.*}}
define spir_kernel void @test_sqrt_fp32_rtz(float %a, float addrspace(1)* align 4 %c) {
entry:
  %res = call float @llvm.experimental.constrained.sqrt.f32(float %a, metadata !"round.towardzero", metadata !"fpexcept.ignore")
  store float %res, float addrspace(1)* %c, align 4
  ret void
}

; CHECK-LABEL:      .kernel "test_sqrt_fp32_rtp"
; CHECK:            xor (M1_NM, 1) %cr0(0,0)<1> %cr0(0,0)<0;1,0> 0x10:ud {{.*}}
; CHECK:            sqrtm (M1_NM, 1) {{.*}} {{.*}}
; CHECK:            xor (M1_NM, 1) %cr0(0,0)<1> %cr0(0,0)<0;1,0> 0x10:ud {{.*}}
; CHECK:            ret {{.*}}
define spir_kernel void @test_sqrt_fp32_rtp(float %a, float addrspace(1)* align 4 %c) {
entry:
  %res = call float @llvm.experimental.constrained.sqrt.f32(float %a, metadata !"round.upward", metadata !"fpexcept.ignore")
  store float %res, float addrspace(1)* %c, align 4
  ret void
}

; CHECK-LABEL:      .kernel "test_sqrt_fp32_rtn"
; CHECK:            xor (M1_NM, 1) %cr0(0,0)<1> %cr0(0,0)<0;1,0> 0x20:ud {{.*}}
; CHECK:            sqrtm (M1_NM, 1) {{.*}} {{.*}}
; CHECK:            xor (M1_NM, 1) %cr0(0,0)<1> %cr0(0,0)<0;1,0> 0x20:ud {{.*}}
; CHECK:            ret {{.*}}
define spir_kernel void @test_sqrt_fp32_rtn(float %a, float addrspace(1)* align 4 %c) {
entry:
  %res = call float @llvm.experimental.constrained.sqrt.f32(float %a, metadata !"round.downward", metadata !"fpexcept.ignore")
  store float %res, float addrspace(1)* %c, align 4
  ret void
}

; CHECK-LABEL:      .kernel "test_sqrt_fp64_rne"
; CHECK-NOT:        xor (M1_NM, 1) %cr0(0,0)<1> %cr0(0,0)<0;1,0> {{.*}}
; CHECK:            sqrtm (M1_NM, 1) {{.*}} {{.*}}
; CHECK-NOT:        xor (M1_NM, 1) %cr0(0,0)<1> %cr0(0,0)<0;1,0> {{.*}}
; CHECK:            ret {{.*}}
define spir_kernel void @test_sqrt_fp64_rne(double %a, double addrspace(1)* align 8 %c) {
entry:
  %res = call double @llvm.experimental.constrained.sqrt.f64(double %a, metadata !"round.tonearest", metadata !"fpexcept.ignore")
  store double %res, double addrspace(1)* %c, align 8
  ret void
}

; CHECK-LABEL:      .kernel "test_sqrt_fp64_rtz"
; CHECK:            xor (M1_NM, 1) %cr0(0,0)<1> %cr0(0,0)<0;1,0> 0x30:ud {{.*}}
; CHECK:            sqrtm (M1_NM, 1) {{.*}} {{.*}}
; CHECK:            xor (M1_NM, 1) %cr0(0,0)<1> %cr0(0,0)<0;1,0> 0x30:ud {{.*}}
; CHECK:            ret {{.*}}
define spir_kernel void @test_sqrt_fp64_rtz(double %a, double addrspace(1)* align 8 %c) {
entry:
  %res = call double @llvm.experimental.constrained.sqrt.f64(double %a, metadata !"round.towardzero", metadata !"fpexcept.ignore")
  store double %res, double addrspace(1)* %c, align 8
  ret void
}

; CHECK-LABEL:      .kernel "test_sqrt_fp64_rtp"
; CHECK:            xor (M1_NM, 1) %cr0(0,0)<1> %cr0(0,0)<0;1,0> 0x10:ud {{.*}}
; CHECK:            sqrtm (M1_NM, 1) {{.*}} {{.*}}
; CHECK:            xor (M1_NM, 1) %cr0(0,0)<1> %cr0(0,0)<0;1,0> 0x10:ud {{.*}}
; CHECK:            ret {{.*}}
define spir_kernel void @test_sqrt_fp64_rtp(double %a, double addrspace(1)* align 8 %c) {
entry:
  %res = call double @llvm.experimental.constrained.sqrt.f64(double %a, metadata !"round.upward", metadata !"fpexcept.ignore")
  store double %res, double addrspace(1)* %c, align 8
  ret void
}

; CHECK-LABEL:      .kernel "test_sqrt_fp64_rtn"
; CHECK:            xor (M1_NM, 1) %cr0(0,0)<1> %cr0(0,0)<0;1,0> 0x20:ud {{.*}}
; CHECK:            sqrtm (M1_NM, 1) {{.*}} {{.*}}
; CHECK:            xor (M1_NM, 1) %cr0(0,0)<1> %cr0(0,0)<0;1,0> 0x20:ud {{.*}}
; CHECK:            ret {{.*}}
define spir_kernel void @test_sqrt_fp64_rtn(double %a, double addrspace(1)* align 8 %c) {
entry:
  %res = call double @llvm.experimental.constrained.sqrt.f64(double %a, metadata !"round.downward", metadata !"fpexcept.ignore")
  store double %res, double addrspace(1)* %c, align 8
  ret void
}

; fp16 tests: half-precision sqrt is promoted to float (sqrtm in RNE),
; then truncated back to half with the specified rounding mode.

; CHECK-LABEL:      .kernel "test_sqrt_fp16_rne"
; CHECK:            mov (M1_NM, 1) [[ext:.*]](0,0)<1> {{.*}}
; CHECK:            sqrtm (M1_NM, 1) [[sqrt:.*]](0,0)<1> [[ext]](0,0)<0;1,0>
; CHECK-NOT:        xor (M1_NM, 1) %cr0(0,0)<1> %cr0(0,0)<0;1,0> {{.*}}
; CHECK:            mov (M1, {{16|32}}) {{.*}}(0,0)<1> [[sqrt]](0,0)<0;1,0>
; CHECK-NOT:        xor (M1_NM, 1) %cr0(0,0)<1> %cr0(0,0)<0;1,0> {{.*}}
; CHECK:            ret {{.*}}
define spir_kernel void @test_sqrt_fp16_rne(half %a, half addrspace(1)* align 2 %c) {
entry:
  %res = call half @llvm.experimental.constrained.sqrt.f16(half %a, metadata !"round.tonearest", metadata !"fpexcept.ignore")
  store half %res, half addrspace(1)* %c, align 2
  ret void
}

; CHECK-LABEL:      .kernel "test_sqrt_fp16_rtz"
; CHECK:            mov (M1_NM, 1) [[ext:.*]](0,0)<1> {{.*}}
; CHECK:            sqrtm (M1_NM, 1) [[sqrt:.*]](0,0)<1> [[ext]](0,0)<0;1,0>
; CHECK:            xor (M1_NM, 1) %cr0(0,0)<1> %cr0(0,0)<0;1,0> 0x30:ud
; CHECK:            mov (M1, {{16|32}}) {{.*}}(0,0)<1> [[sqrt]](0,0)<0;1,0>
; CHECK:            xor (M1_NM, 1) %cr0(0,0)<1> %cr0(0,0)<0;1,0> 0x30:ud
; CHECK:            ret {{.*}}
define spir_kernel void @test_sqrt_fp16_rtz(half %a, half addrspace(1)* align 2 %c) {
entry:
  %res = call half @llvm.experimental.constrained.sqrt.f16(half %a, metadata !"round.towardzero", metadata !"fpexcept.ignore")
  store half %res, half addrspace(1)* %c, align 2
  ret void
}

; CHECK-LABEL:      .kernel "test_sqrt_fp16_rtp"
; CHECK:            mov (M1_NM, 1) [[ext:.*]](0,0)<1> {{.*}}
; CHECK:            sqrtm (M1_NM, 1) [[sqrt:.*]](0,0)<1> [[ext]](0,0)<0;1,0>
; CHECK:            xor (M1_NM, 1) %cr0(0,0)<1> %cr0(0,0)<0;1,0> 0x10:ud
; CHECK:            mov (M1, {{16|32}}) {{.*}}(0,0)<1> [[sqrt]](0,0)<0;1,0>
; CHECK:            xor (M1_NM, 1) %cr0(0,0)<1> %cr0(0,0)<0;1,0> 0x10:ud
; CHECK:            ret {{.*}}
define spir_kernel void @test_sqrt_fp16_rtp(half %a, half addrspace(1)* align 2 %c) {
entry:
  %res = call half @llvm.experimental.constrained.sqrt.f16(half %a, metadata !"round.upward", metadata !"fpexcept.ignore")
  store half %res, half addrspace(1)* %c, align 2
  ret void
}

; CHECK-LABEL:      .kernel "test_sqrt_fp16_rtn"
; CHECK:            mov (M1_NM, 1) [[ext:.*]](0,0)<1> {{.*}}
; CHECK:            sqrtm (M1_NM, 1) [[sqrt:.*]](0,0)<1> [[ext]](0,0)<0;1,0>
; CHECK:            xor (M1_NM, 1) %cr0(0,0)<1> %cr0(0,0)<0;1,0> 0x20:ud
; CHECK:            mov (M1, {{16|32}}) {{.*}}(0,0)<1> [[sqrt]](0,0)<0;1,0>
; CHECK:            xor (M1_NM, 1) %cr0(0,0)<1> %cr0(0,0)<0;1,0> 0x20:ud
; CHECK:            ret {{.*}}
define spir_kernel void @test_sqrt_fp16_rtn(half %a, half addrspace(1)* align 2 %c) {
entry:
  %res = call half @llvm.experimental.constrained.sqrt.f16(half %a, metadata !"round.downward", metadata !"fpexcept.ignore")
  store half %res, half addrspace(1)* %c, align 2
  ret void
}

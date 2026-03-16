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

declare float @llvm.experimental.constrained.fdiv.f32(float, float, metadata, metadata)
declare double @llvm.experimental.constrained.fdiv.f64(double, double, metadata, metadata)
declare half @llvm.experimental.constrained.fdiv.f16(half, half, metadata, metadata)

; CHECK-LABEL:      .kernel "test_div_fp32_rne"
; CHECK-NOT:        xor (M1_NM, 1) %cr0(0,0)<1> %cr0(0,0)<0;1,0> {{.*}}
; CHECK:            divm (M1_NM, 1) {{.*}} {{.*}} {{.*}}
; CHECK-NOT:        xor (M1_NM, 1) %cr0(0,0)<1> %cr0(0,0)<0;1,0> {{.*}}
; CHECK:            ret {{.*}}
define spir_kernel void @test_div_fp32_rne(float %a, float %b, float addrspace(1)* align 4 %c) {
entry:
  %div = call float @llvm.experimental.constrained.fdiv.f32(float %a, float %b, metadata !"round.tonearest", metadata !"fpexcept.ignore")
  store float %div, float addrspace(1)* %c, align 4
  ret void
}

; CHECK-LABEL:      .kernel "test_div_fp32_rtz"
; CHECK:            xor (M1_NM, 1) %cr0(0,0)<1> %cr0(0,0)<0;1,0> 0x30:ud {{.*}}
; CHECK:            divm (M1_NM, 1) {{.*}} {{.*}} {{.*}}
; CHECK:            xor (M1_NM, 1) %cr0(0,0)<1> %cr0(0,0)<0;1,0> 0x30:ud {{.*}}
; CHECK:            ret {{.*}}
define spir_kernel void @test_div_fp32_rtz(float %a, float %b, float addrspace(1)* align 4 %c) {
entry:
  %div = call float @llvm.experimental.constrained.fdiv.f32(float %a, float %b, metadata !"round.towardzero", metadata !"fpexcept.ignore")
  store float %div, float addrspace(1)* %c, align 4
  ret void
}

; CHECK-LABEL:      .kernel "test_div_fp32_rtp"
; CHECK:            xor (M1_NM, 1) %cr0(0,0)<1> %cr0(0,0)<0;1,0> 0x10:ud {{.*}}
; CHECK:            divm (M1_NM, 1) {{.*}} {{.*}} {{.*}}
; CHECK:            xor (M1_NM, 1) %cr0(0,0)<1> %cr0(0,0)<0;1,0> 0x10:ud {{.*}}
; CHECK:            ret {{.*}}
define spir_kernel void @test_div_fp32_rtp(float %a, float %b, float addrspace(1)* align 4 %c) {
entry:
  %div = call float @llvm.experimental.constrained.fdiv.f32(float %a, float %b, metadata !"round.upward", metadata !"fpexcept.ignore")
  store float %div, float addrspace(1)* %c, align 4
  ret void
}

; CHECK-LABEL:      .kernel "test_div_fp32_rtn"
; CHECK:            xor (M1_NM, 1) %cr0(0,0)<1> %cr0(0,0)<0;1,0> 0x20:ud {{.*}}
; CHECK:            divm (M1_NM, 1) {{.*}} {{.*}} {{.*}}
; CHECK:            xor (M1_NM, 1) %cr0(0,0)<1> %cr0(0,0)<0;1,0> 0x20:ud {{.*}}
; CHECK:            ret {{.*}}
define spir_kernel void @test_div_fp32_rtn(float %a, float %b, float addrspace(1)* align 4 %c) {
entry:
  %div = call float @llvm.experimental.constrained.fdiv.f32(float %a, float %b, metadata !"round.downward", metadata !"fpexcept.ignore")
  store float %div, float addrspace(1)* %c, align 4
  ret void
}

; CHECK-LABEL:      .kernel "test_div_fp64_rne"
; CHECK-NOT:        xor (M1_NM, 1) %cr0(0,0)<1> %cr0(0,0)<0;1,0> {{.*}}
; CHECK:            divm (M1_NM, 1) {{.*}} {{.*}} {{.*}}
; CHECK-NOT:        xor (M1_NM, 1) %cr0(0,0)<1> %cr0(0,0)<0;1,0> {{.*}}
; CHECK:            ret {{.*}}
define spir_kernel void @test_div_fp64_rne(double %a, double %b, double addrspace(1)* align 8 %c) {
entry:
  %div = call double @llvm.experimental.constrained.fdiv.f64(double %a, double %b, metadata !"round.tonearest", metadata !"fpexcept.ignore")
  store double %div, double addrspace(1)* %c, align 8
  ret void
}

; CHECK-LABEL:      .kernel "test_div_fp64_rtz"
; CHECK:            xor (M1_NM, 1) %cr0(0,0)<1> %cr0(0,0)<0;1,0> 0x30:ud {{.*}}
; CHECK:            divm (M1_NM, 1) {{.*}} {{.*}} {{.*}}
; CHECK:            xor (M1_NM, 1) %cr0(0,0)<1> %cr0(0,0)<0;1,0> 0x30:ud {{.*}}
; CHECK:            ret {{.*}}
define spir_kernel void @test_div_fp64_rtz(double %a, double %b, double addrspace(1)* align 8 %c) {
entry:
  %div = call double @llvm.experimental.constrained.fdiv.f64(double %a, double %b, metadata !"round.towardzero", metadata !"fpexcept.ignore")
  store double %div, double addrspace(1)* %c, align 8
  ret void
}

; CHECK-LABEL:      .kernel "test_div_fp64_rtp"
; CHECK:            xor (M1_NM, 1) %cr0(0,0)<1> %cr0(0,0)<0;1,0> 0x10:ud {{.*}}
; CHECK:            divm (M1_NM, 1) {{.*}} {{.*}} {{.*}}
; CHECK:            xor (M1_NM, 1) %cr0(0,0)<1> %cr0(0,0)<0;1,0> 0x10:ud {{.*}}
; CHECK:            ret {{.*}}
define spir_kernel void @test_div_fp64_rtp(double %a, double %b, double addrspace(1)* align 8 %c) {
entry:
  %div = call double @llvm.experimental.constrained.fdiv.f64(double %a, double %b, metadata !"round.upward", metadata !"fpexcept.ignore")
  store double %div, double addrspace(1)* %c, align 8
  ret void
}

; CHECK-LABEL:      .kernel "test_div_fp64_rtn"
; CHECK:            xor (M1_NM, 1) %cr0(0,0)<1> %cr0(0,0)<0;1,0> 0x20:ud {{.*}}
; CHECK:            divm (M1_NM, 1) {{.*}} {{.*}} {{.*}}
; CHECK:            xor (M1_NM, 1) %cr0(0,0)<1> %cr0(0,0)<0;1,0> 0x20:ud {{.*}}
; CHECK:            ret {{.*}}
define spir_kernel void @test_div_fp64_rtn(double %a, double %b, double addrspace(1)* align 8 %c) {
entry:
  %div = call double @llvm.experimental.constrained.fdiv.f64(double %a, double %b, metadata !"round.downward", metadata !"fpexcept.ignore")
  store double %div, double addrspace(1)* %c, align 8
  ret void
}

; fp16 tests: half-precision fdiv is promoted to float (divm in RNE),
; then truncated back to half with the specified rounding mode.

; CHECK-LABEL:      .kernel "test_div_fp16_rne"
; CHECK:            mov (M1_NM, 1) [[lhs:.*]](0,0)<1> {{.*}}
; CHECK:            mov (M1_NM, 1) [[rhs:.*]](0,0)<1> {{.*}}
; CHECK:            divm (M1_NM, 1) [[div:.*]](0,0)<1> [[lhs]](0,0)<0;1,0> [[rhs]](0,0)<0;1,0>
; CHECK-NOT:        xor (M1_NM, 1) %cr0(0,0)<1> %cr0(0,0)<0;1,0> {{.*}}
; CHECK:            mov (M1, {{16|32}}) {{.*}}(0,0)<1> [[div]](0,0)<0;1,0>
; CHECK-NOT:        xor (M1_NM, 1) %cr0(0,0)<1> %cr0(0,0)<0;1,0> {{.*}}
; CHECK:            ret {{.*}}
define spir_kernel void @test_div_fp16_rne(half %a, half %b, half addrspace(1)* align 2 %c) {
entry:
  %div = call half @llvm.experimental.constrained.fdiv.f16(half %a, half %b, metadata !"round.tonearest", metadata !"fpexcept.ignore")
  store half %div, half addrspace(1)* %c, align 2
  ret void
}

; CHECK-LABEL:      .kernel "test_div_fp16_rtz"
; CHECK:            mov (M1_NM, 1) [[lhs:.*]](0,0)<1> {{.*}}
; CHECK:            mov (M1_NM, 1) [[rhs:.*]](0,0)<1> {{.*}}
; CHECK:            divm (M1_NM, 1) [[div:.*]](0,0)<1> [[lhs]](0,0)<0;1,0> [[rhs]](0,0)<0;1,0>
; CHECK:            xor (M1_NM, 1) %cr0(0,0)<1> %cr0(0,0)<0;1,0> 0x30:ud
; CHECK:            mov (M1, {{16|32}}) {{.*}}(0,0)<1> [[div]](0,0)<0;1,0>
; CHECK:            xor (M1_NM, 1) %cr0(0,0)<1> %cr0(0,0)<0;1,0> 0x30:ud
; CHECK:            ret {{.*}}
define spir_kernel void @test_div_fp16_rtz(half %a, half %b, half addrspace(1)* align 2 %c) {
entry:
  %div = call half @llvm.experimental.constrained.fdiv.f16(half %a, half %b, metadata !"round.towardzero", metadata !"fpexcept.ignore")
  store half %div, half addrspace(1)* %c, align 2
  ret void
}

; CHECK-LABEL:      .kernel "test_div_fp16_rtp"
; CHECK:            mov (M1_NM, 1) [[lhs:.*]](0,0)<1> {{.*}}
; CHECK:            mov (M1_NM, 1) [[rhs:.*]](0,0)<1> {{.*}}
; CHECK:            divm (M1_NM, 1) [[div:.*]](0,0)<1> [[lhs]](0,0)<0;1,0> [[rhs]](0,0)<0;1,0>
; CHECK:            xor (M1_NM, 1) %cr0(0,0)<1> %cr0(0,0)<0;1,0> 0x10:ud
; CHECK:            mov (M1, {{16|32}}) {{.*}}(0,0)<1> [[div]](0,0)<0;1,0>
; CHECK:            xor (M1_NM, 1) %cr0(0,0)<1> %cr0(0,0)<0;1,0> 0x10:ud
; CHECK:            ret {{.*}}
define spir_kernel void @test_div_fp16_rtp(half %a, half %b, half addrspace(1)* align 2 %c) {
entry:
  %div = call half @llvm.experimental.constrained.fdiv.f16(half %a, half %b, metadata !"round.upward", metadata !"fpexcept.ignore")
  store half %div, half addrspace(1)* %c, align 2
  ret void
}

; CHECK-LABEL:      .kernel "test_div_fp16_rtn"
; CHECK:            mov (M1_NM, 1) [[lhs:.*]](0,0)<1> {{.*}}
; CHECK:            mov (M1_NM, 1) [[rhs:.*]](0,0)<1> {{.*}}
; CHECK:            divm (M1_NM, 1) [[div:.*]](0,0)<1> [[lhs]](0,0)<0;1,0> [[rhs]](0,0)<0;1,0>
; CHECK:            xor (M1_NM, 1) %cr0(0,0)<1> %cr0(0,0)<0;1,0> 0x20:ud
; CHECK:            mov (M1, {{16|32}}) {{.*}}(0,0)<1> [[div]](0,0)<0;1,0>
; CHECK:            xor (M1_NM, 1) %cr0(0,0)<1> %cr0(0,0)<0;1,0> 0x20:ud
; CHECK:            ret {{.*}}
define spir_kernel void @test_div_fp16_rtn(half %a, half %b, half addrspace(1)* align 2 %c) {
entry:
  %div = call half @llvm.experimental.constrained.fdiv.f16(half %a, half %b, metadata !"round.downward", metadata !"fpexcept.ignore")
  store half %div, half addrspace(1)* %c, align 2
  ret void
}

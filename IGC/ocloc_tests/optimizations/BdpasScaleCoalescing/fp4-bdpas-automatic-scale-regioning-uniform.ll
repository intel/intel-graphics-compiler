;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: cri-supported, regkeys, llvm-16-plus
; RUN: llvm-as %s -o %t.bc
; RUN: ocloc compile -llvm_input -file %t.bc -device cri -output_no_suffix -output %t.bin -options "-igc_opts 'EnableOpaquePointersBackend=1,EnableFP4Dpas=1,EnableBdpasScaleCoalescing=1,DumpVISAASMToConsole=1'" 2>&1 | FileCheck %s --check-prefix=CHECK-ASM

; Uniform scale loads are legal ordinary input. Dependency analysis keeps them
; out of automatic reconstruction, so they retain per-instruction scale
; materialization instead of building an unprofitable shared region.
; Each call must copy both +0/+32 parts into its own B and A temporaries; the
; checks capture those operands without relying on debug-only VISA names.

; CHECK-ASM: .kernel "automatic_scale_regioning_uniform"
; CHECK-ASM: mov (M1, 16) [[B00:[A-Za-z0-9_]+]](0,0)<1> {{.*}}<1;1,0>
; CHECK-ASM: mov (M1, 16) [[B00]](0,32)<1> {{.*}}<1;1,0>
; CHECK-ASM: mov (M1, 8) [[A00:[A-Za-z0-9_]+]](0,0)<1> {{.*}}<1;1,0>
; CHECK-ASM: mov (M1, 8) [[A00]](0,32)<1> {{.*}}<1;1,0>
; CHECK-ASM: bdpas.e2m1.e2m1.8.8 {{.*}} [[B00]](0,0) [[A00]](0,0)
; CHECK-ASM: mov (M1, 16) [[B01:[A-Za-z0-9_]+]](0,0)<1> {{.*}}<1;1,0>
; CHECK-ASM: mov (M1, 16) [[B01]](0,32)<1> {{.*}}<1;1,0>
; CHECK-ASM: mov (M1, 8) [[A01:[A-Za-z0-9_]+]](0,0)<1> {{.*}}<1;1,0>
; CHECK-ASM: mov (M1, 8) [[A01]](0,32)<1> {{.*}}<1;1,0>
; CHECK-ASM: bdpas.e2m1.e2m1.8.8 {{.*}} [[B01]](0,0) [[A01]](0,0)
; CHECK-ASM: mov (M1, 16) [[B10:[A-Za-z0-9_]+]](0,0)<1> {{.*}}<1;1,0>
; CHECK-ASM: mov (M1, 16) [[B10]](0,32)<1> {{.*}}<1;1,0>
; CHECK-ASM: mov (M1, 8) [[A10:[A-Za-z0-9_]+]](0,0)<1> {{.*}}<1;1,0>
; CHECK-ASM: mov (M1, 8) [[A10]](0,32)<1> {{.*}}<1;1,0>
; CHECK-ASM: bdpas.e2m1.e2m1.8.8 {{.*}} [[B10]](0,0) [[A10]](0,0)
; CHECK-ASM: mov (M1, 16) [[B11:[A-Za-z0-9_]+]](0,0)<1> {{.*}}<1;1,0>
; CHECK-ASM: mov (M1, 16) [[B11]](0,32)<1> {{.*}}<1;1,0>
; CHECK-ASM: mov (M1, 8) [[A11:[A-Za-z0-9_]+]](0,0)<1> {{.*}}<1;1,0>
; CHECK-ASM: mov (M1, 8) [[A11]](0,32)<1> {{.*}}<1;1,0>
; CHECK-ASM: bdpas.e2m1.e2m1.8.8 {{.*}} [[B11]](0,0) [[A11]](0,0)

target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v512:512:512-v1024:1024:1024"
target triple = "spir64-unknown-unknown"

define spir_kernel void @automatic_scale_regioning_uniform(
    ptr addrspace(1) nocapture readonly %a_ptr,
    ptr addrspace(1) nocapture readonly %b_ptr,
    ptr addrspace(1) nocapture readonly %scale_a0_ptr,
    ptr addrspace(1) nocapture readonly %scale_a1_ptr,
    ptr addrspace(1) nocapture readonly %scale_b0_ptr,
    ptr addrspace(1) nocapture readonly %scale_b1_ptr,
    ptr addrspace(1) nocapture %out) #0 !intel_reqd_sub_group_size !4 {
entry:
  %a = load <8 x i16>, ptr addrspace(1) %a_ptr, align 16
  %b = load <8 x i32>, ptr addrspace(1) %b_ptr, align 32
  %scale_a0 = load <2 x i8>, ptr addrspace(1) %scale_a0_ptr, align 2
  %scale_a1 = load <2 x i8>, ptr addrspace(1) %scale_a1_ptr, align 2
  %scale_b0 = load <2 x i8>, ptr addrspace(1) %scale_b0_ptr, align 2
  %scale_b1 = load <2 x i8>, ptr addrspace(1) %scale_b1_ptr, align 2
  %r00 = call <8 x float> @llvm.genx.GenISA.sub.group.bdpas.v8f32.v8f32.v8i16.v8i32.v2i8.v2i8(
      <8 x float> zeroinitializer, <8 x i16> %a, <8 x i32> %b,
      <2 x i8> %scale_a0, <2 x i8> %scale_b0, i32 13, i32 13
  )
  %r01 = call <8 x float> @llvm.genx.GenISA.sub.group.bdpas.v8f32.v8f32.v8i16.v8i32.v2i8.v2i8(
      <8 x float> zeroinitializer, <8 x i16> %a, <8 x i32> %b,
      <2 x i8> %scale_a0, <2 x i8> %scale_b1, i32 13, i32 13
  )
  %r10 = call <8 x float> @llvm.genx.GenISA.sub.group.bdpas.v8f32.v8f32.v8i16.v8i32.v2i8.v2i8(
      <8 x float> zeroinitializer, <8 x i16> %a, <8 x i32> %b,
      <2 x i8> %scale_a1, <2 x i8> %scale_b0, i32 13, i32 13
  )
  %r11 = call <8 x float> @llvm.genx.GenISA.sub.group.bdpas.v8f32.v8f32.v8i16.v8i32.v2i8.v2i8(
      <8 x float> zeroinitializer, <8 x i16> %a, <8 x i32> %b,
      <2 x i8> %scale_a1, <2 x i8> %scale_b1, i32 13, i32 13
  )
  store <8 x float> %r00, ptr addrspace(1) %out, align 32
  %out1 = getelementptr <8 x float>, ptr addrspace(1) %out, i64 1
  store <8 x float> %r01, ptr addrspace(1) %out1, align 32
  %out2 = getelementptr <8 x float>, ptr addrspace(1) %out, i64 2
  store <8 x float> %r10, ptr addrspace(1) %out2, align 32
  %out3 = getelementptr <8 x float>, ptr addrspace(1) %out, i64 3
  store <8 x float> %r11, ptr addrspace(1) %out3, align 32
  ret void
}

declare <8 x float> @llvm.genx.GenISA.sub.group.bdpas.v8f32.v8f32.v8i16.v8i32.v2i8.v2i8(
    <8 x float>, <8 x i16>, <8 x i32>, <2 x i8>, <2 x i8>, i32, i32
)

attributes #0 = { nounwind }

!0 = !{i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1}
!1 = !{!"none", !"none", !"none", !"none", !"none", !"none", !"none"}
!2 = !{!"short8*", !"int8*", !"uchar2*", !"uchar2*", !"uchar2*", !"uchar2*", !"float8*"}
!3 = !{!"", !"", !"", !"", !"", !"", !""}
!4 = !{i32 16}

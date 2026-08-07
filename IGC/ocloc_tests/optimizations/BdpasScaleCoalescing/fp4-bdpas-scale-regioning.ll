;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: cri-supported, regkeys, llvm-16-plus
; RUN: llvm-as %s -o %t.bc
; RUN: ocloc compile -llvm_input -file %t.bc -device cri -output_no_suffix -output %t.bin -options "-igc_opts 'EnableOpaquePointersBackend=1,EnableFP4Dpas=1,DumpVISAASMToConsole=1'" 2>&1 | FileCheck %s --check-prefix=CHECK-ASM

; An intrinsic form of a 2x2 FP4 block-scaled BDPAS tile. The source has
; three preconstructed scale regions: A, B for the first K=32 block, and B for
; the second K=32 block.  Each packed BDPAS receives the complete producer
; region and explicit A/B byte offsets.  The
; extractelement/insertelement chains are deliberately kept: their moves are
; setup work, outside this test's packed-emitter scope.

; CHECK-ASM: .kernel "fp4_bdpas_scale_regioning"
; CHECK-ASM-NOT: StridedScale
; CHECK-ASM: bdpas.e2m1.e2m1.8.8 {{.*}} [[B0:V[0-9]+]](0,0) [[A:V[0-9]+]](0,0)
; CHECK-ASM: bdpas.e2m1.e2m1.8.8 {{.*}} [[B0]](0,0) [[A]](0,16)
; CHECK-ASM: bdpas.e2m1.e2m1.8.8 {{.*}} [[B0]](0,16) [[A]](0,0)
; CHECK-ASM: bdpas.e2m1.e2m1.8.8 {{.*}} [[B0]](0,16) [[A]](0,16)
; CHECK-ASM: bdpas.e2m1.e2m1.8.8 {{.*}} [[B1:V[0-9]+]](0,0) [[A]](0,8)
; CHECK-ASM: bdpas.e2m1.e2m1.8.8 {{.*}} [[B1]](0,0) [[A]](0,24)
; CHECK-ASM: bdpas.e2m1.e2m1.8.8 {{.*}} [[B1]](0,16) [[A]](0,8)
; CHECK-ASM: bdpas.e2m1.e2m1.8.8 {{.*}} [[B1]](0,16) [[A]](0,24)

target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v512:512:512-v1024:1024:1024"
target triple = "spir64-unknown-unknown"

define spir_kernel void @fp4_bdpas_scale_regioning(
    ptr addrspace(1) nocapture readonly %a00_ptr,
    ptr addrspace(1) nocapture readonly %a01_ptr,
    ptr addrspace(1) nocapture readonly %a10_ptr,
    ptr addrspace(1) nocapture readonly %a11_ptr,
    ptr addrspace(1) nocapture readonly %b00_ptr,
    ptr addrspace(1) nocapture readonly %b01_ptr,
    ptr addrspace(1) nocapture readonly %b10_ptr,
    ptr addrspace(1) nocapture readonly %b11_ptr,
    ptr addrspace(1) nocapture readonly %scale_a_ptr,
    ptr addrspace(1) nocapture readonly %scale_b0_ptr,
    ptr addrspace(1) nocapture readonly %scale_b1_ptr,
    ptr addrspace(1) nocapture %out) #0 !kernel_arg_addr_space !0 !kernel_arg_access_qual !1 !kernel_arg_type !2 !kernel_arg_type_qual !3 !kernel_arg_base_type !2 !intel_reqd_sub_group_size !4 {
entry:
  %lane16 = call i16 @llvm.genx.GenISA.simdLaneId()
  %lane = zext i16 %lane16 to i64
  %a00 = load <8 x i16>, ptr addrspace(1) %a00_ptr, align 16
  %a01 = load <8 x i16>, ptr addrspace(1) %a01_ptr, align 16
  %a10 = load <8 x i16>, ptr addrspace(1) %a10_ptr, align 16
  %a11 = load <8 x i16>, ptr addrspace(1) %a11_ptr, align 16
  %b00 = load <8 x i32>, ptr addrspace(1) %b00_ptr, align 32
  %b01 = load <8 x i32>, ptr addrspace(1) %b01_ptr, align 32
  %b10 = load <8 x i32>, ptr addrspace(1) %b10_ptr, align 32
  %b11 = load <8 x i32>, ptr addrspace(1) %b11_ptr, align 32

  ; These are preconstructed shared regions for direct intrinsic-input coverage.
  %scale_a_lane = getelementptr <4 x i8>, ptr addrspace(1) %scale_a_ptr, i64 %lane
  %scale_b0_lane = getelementptr <4 x i8>, ptr addrspace(1) %scale_b0_ptr, i64 %lane
  %scale_b1_lane = getelementptr <4 x i8>, ptr addrspace(1) %scale_b1_ptr, i64 %lane
  %scale_a = load <4 x i8>, ptr addrspace(1) %scale_a_lane, align 4
  %scale_b0 = load <4 x i8>, ptr addrspace(1) %scale_b0_lane, align 4
  %scale_b1 = load <4 x i8>, ptr addrspace(1) %scale_b1_lane, align 4

  %a.0 = extractelement <4 x i8> %scale_a, i32 0
  %a.1 = extractelement <4 x i8> %scale_a, i32 1
  %a.2 = extractelement <4 x i8> %scale_a, i32 2
  %a.3 = extractelement <4 x i8> %scale_a, i32 3
  %packed_a.0 = insertelement <4 x i8> poison, i8 %a.0, i32 0
  %packed_a.1 = insertelement <4 x i8> %packed_a.0, i8 %a.1, i32 1
  %packed_a.2 = insertelement <4 x i8> %packed_a.1, i8 %a.2, i32 2
  %packed_a = insertelement <4 x i8> %packed_a.2, i8 %a.3, i32 3
  %b0.0 = extractelement <4 x i8> %scale_b0, i32 0
  %b0.1 = extractelement <4 x i8> %scale_b0, i32 1
  %b0.2 = extractelement <4 x i8> %scale_b0, i32 2
  %b0.3 = extractelement <4 x i8> %scale_b0, i32 3
  %packed_b0.0 = insertelement <4 x i8> poison, i8 %b0.0, i32 0
  %packed_b0.1 = insertelement <4 x i8> %packed_b0.0, i8 %b0.1, i32 1
  %packed_b0.2 = insertelement <4 x i8> %packed_b0.1, i8 %b0.2, i32 2
  %packed_b0 = insertelement <4 x i8> %packed_b0.2, i8 %b0.3, i32 3
  %b1.0 = extractelement <4 x i8> %scale_b1, i32 0
  %b1.1 = extractelement <4 x i8> %scale_b1, i32 1
  %b1.2 = extractelement <4 x i8> %scale_b1, i32 2
  %b1.3 = extractelement <4 x i8> %scale_b1, i32 3
  %packed_b1.0 = insertelement <4 x i8> poison, i8 %b1.0, i32 0
  %packed_b1.1 = insertelement <4 x i8> %packed_b1.0, i8 %b1.1, i32 1
  %packed_b1.2 = insertelement <4 x i8> %packed_b1.1, i8 %b1.2, i32 2
  %packed_b1 = insertelement <4 x i8> %packed_b1.2, i8 %b1.3, i32 3

  %r00.0 = call <8 x float> @llvm.genx.GenISA.sub.group.bdpas.packed.v8f32.v8f32.v8i16.v8i32.v4i8.v4i8(
      <8 x float> zeroinitializer, <8 x i16> %a00, <8 x i32> %b00,
      <4 x i8> %packed_a, <4 x i8> %packed_b0, i32 13, i32 13,
      i32 0, i32 0)
  %r10.0 = call <8 x float> @llvm.genx.GenISA.sub.group.bdpas.packed.v8f32.v8f32.v8i16.v8i32.v4i8.v4i8(
      <8 x float> zeroinitializer, <8 x i16> %a10, <8 x i32> %b00,
      <4 x i8> %packed_a, <4 x i8> %packed_b0, i32 13, i32 13,
      i32 16, i32 0)
  %r01.0 = call <8 x float> @llvm.genx.GenISA.sub.group.bdpas.packed.v8f32.v8f32.v8i16.v8i32.v4i8.v4i8(
      <8 x float> zeroinitializer, <8 x i16> %a00, <8 x i32> %b01,
      <4 x i8> %packed_a, <4 x i8> %packed_b0, i32 13, i32 13,
      i32 0, i32 16)
  %r11.0 = call <8 x float> @llvm.genx.GenISA.sub.group.bdpas.packed.v8f32.v8f32.v8i16.v8i32.v4i8.v4i8(
      <8 x float> zeroinitializer, <8 x i16> %a10, <8 x i32> %b01,
      <4 x i8> %packed_a, <4 x i8> %packed_b0, i32 13, i32 13,
      i32 16, i32 16)
  %r00 = call <8 x float> @llvm.genx.GenISA.sub.group.bdpas.packed.v8f32.v8f32.v8i16.v8i32.v4i8.v4i8(
      <8 x float> %r00.0, <8 x i16> %a01, <8 x i32> %b10,
      <4 x i8> %packed_a, <4 x i8> %packed_b1, i32 13, i32 13,
      i32 8, i32 0)
  %r10 = call <8 x float> @llvm.genx.GenISA.sub.group.bdpas.packed.v8f32.v8f32.v8i16.v8i32.v4i8.v4i8(
      <8 x float> %r10.0, <8 x i16> %a11, <8 x i32> %b10,
      <4 x i8> %packed_a, <4 x i8> %packed_b1, i32 13, i32 13,
      i32 24, i32 0)
  %r01 = call <8 x float> @llvm.genx.GenISA.sub.group.bdpas.packed.v8f32.v8f32.v8i16.v8i32.v4i8.v4i8(
      <8 x float> %r01.0, <8 x i16> %a01, <8 x i32> %b11,
      <4 x i8> %packed_a, <4 x i8> %packed_b1, i32 13, i32 13,
      i32 8, i32 16)
  %r11 = call <8 x float> @llvm.genx.GenISA.sub.group.bdpas.packed.v8f32.v8f32.v8i16.v8i32.v4i8.v4i8(
      <8 x float> %r11.0, <8 x i16> %a11, <8 x i32> %b11,
      <4 x i8> %packed_a, <4 x i8> %packed_b1, i32 13, i32 13,
      i32 24, i32 16)

  store <8 x float> %r00, ptr addrspace(1) %out, align 32
  %out1 = getelementptr <8 x float>, ptr addrspace(1) %out, i64 1
  store <8 x float> %r01, ptr addrspace(1) %out1, align 32
  %out2 = getelementptr <8 x float>, ptr addrspace(1) %out, i64 2
  store <8 x float> %r10, ptr addrspace(1) %out2, align 32
  %out3 = getelementptr <8 x float>, ptr addrspace(1) %out, i64 3
  store <8 x float> %r11, ptr addrspace(1) %out3, align 32
  ret void
}

declare <8 x float> @llvm.genx.GenISA.sub.group.bdpas.packed.v8f32.v8f32.v8i16.v8i32.v4i8.v4i8(
    <8 x float>, <8 x i16>, <8 x i32>, <4 x i8>, <4 x i8>, i32, i32,
    i32, i32)
declare i16 @llvm.genx.GenISA.simdLaneId()

attributes #0 = { nounwind }

!0 = !{i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1}
!1 = !{!"none", !"none", !"none", !"none", !"none", !"none", !"none", !"none", !"none", !"none", !"none", !"none"}
!2 = !{!"short8*", !"short8*", !"short8*", !"short8*", !"int8*", !"int8*", !"int8*", !"int8*", !"uchar4*", !"uchar4*", !"uchar4*", !"float8*"}
!3 = !{!"", !"", !"", !"", !"", !"", !"", !"", !"", !"", !"", !""}
!4 = !{i32 16}

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

; A complete two-A by two-B rectangle is reconstructed for each canonical
; <2 x i8> scale producer form: an i16 split into two bytes and two independently
; loaded bytes. The matrix operands are loaded per lane so the assembly checks
; remain focused on scale-region construction.
; Each four-call group must reuse the same scale regions at the four legal
; offset pairs. This checks the lowering without depending on debug-only VISA
; symbol names.

; CHECK-ASM: .kernel "automatic_scale_regioning"
; Each pair of direct i16 loads uses adjacent d16u32 responses. Two no-mask
; SIMD32 moves per region then build both packed scale regions without narrow
; load-result copies or byte-crossbar alignment moves.
; CHECK-ASM: lsc_load.ugm (M1, 16) [[A_PAIR:[A-Za-z0-9_]+]]:d16u32
; CHECK-ASM-NEXT: lsc_load.ugm (M1, 16) [[A_PAIR]].64:d16u32
; CHECK-ASM: lsc_load.ugm (M1, 16) [[B_PAIR:[A-Za-z0-9_]+]]:d16u32
; CHECK-ASM-NEXT: lsc_load.ugm (M1, 16) [[B_PAIR]].64:d16u32
; A second swizzle over the same paired loads has a non-BDPAS consumer. It must
; retain four masked SIMD16 moves even though its sources share the allocation.
; CHECK-ASM: mov (M1, 16) [[GENERIC_SCALE:[A-Za-z0-9_]+]](0,0)<1> [[GENERIC_INPUT:[A-Za-z0-9_]+]](0,0)<4;1,0>
; CHECK-ASM-NEXT: mov (M1, 16) [[GENERIC_SCALE]](0,16)<1> [[GENERIC_INPUT]](1,0)<4;1,0>
; CHECK-ASM-NEXT: mov (M1, 16) [[GENERIC_SCALE]](0,32)<1> [[GENERIC_INPUT]](0,1)<4;1,0>
; CHECK-ASM-NEXT: mov (M1, 16) [[GENERIC_SCALE]](0,48)<1> [[GENERIC_INPUT]](1,1)<4;1,0>
; CHECK-ASM-COUNT-4: mov (M1_NM, 32) {{[A-Za-z0-9_]+}}{{.*}}<1> {{[A-Za-z0-9_]+}}{{.*}}<4;1,0>
; CHECK-ASM: bdpas.e2m1.e2m1.8.8 {{.*}} [[I16B:[A-Za-z0-9_]+]](0,0) [[I16A:[A-Za-z0-9_]+]](0,0)
; CHECK-ASM: bdpas.e2m1.e2m1.8.8 {{.*}} [[I16B]](0,16) [[I16A]](0,0)
; CHECK-ASM: bdpas.e2m1.e2m1.8.8 {{.*}} [[I16B]](0,0) [[I16A]](0,16)
; CHECK-ASM: bdpas.e2m1.e2m1.8.8 {{.*}} [[I16B]](0,16) [[I16A]](0,16)
; CHECK-ASM: bdpas.e2m1.e2m1.8.8 {{.*}} [[I8B:[A-Za-z0-9_]+]](0,0) [[I8A:[A-Za-z0-9_]+]](0,0)
; CHECK-ASM: bdpas.e2m1.e2m1.8.8 {{.*}} [[I8B]](0,16) [[I8A]](0,0)
; CHECK-ASM: bdpas.e2m1.e2m1.8.8 {{.*}} [[I8B]](0,0) [[I8A]](0,16)
; CHECK-ASM: bdpas.e2m1.e2m1.8.8 {{.*}} [[I8B]](0,16) [[I8A]](0,16)

target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v512:512:512-v1024:1024:1024"
target triple = "spir64-unknown-unknown"

define spir_kernel void @automatic_scale_regioning(
    ptr addrspace(1) %a.ptr, ptr addrspace(1) %b.ptr,
    ptr addrspace(1) %i16a0.ptr, ptr addrspace(1) %i16a1.ptr,
    ptr addrspace(1) %i16b0.ptr, ptr addrspace(1) %i16b1.ptr,
    ptr addrspace(1) %i8a00.ptr, ptr addrspace(1) %i8a01.ptr,
    ptr addrspace(1) %i8a10.ptr, ptr addrspace(1) %i8a11.ptr,
    ptr addrspace(1) %i8b00.ptr, ptr addrspace(1) %i8b01.ptr,
    ptr addrspace(1) %i8b10.ptr, ptr addrspace(1) %i8b11.ptr,
    ptr addrspace(1) %out) !intel_reqd_sub_group_size !4 {
entry:
  %lane16 = call i16 @llvm.genx.GenISA.simdLaneId()
  %lane = zext i16 %lane16 to i64

  %a.lane = getelementptr <8 x i16>, ptr addrspace(1) %a.ptr, i64 %lane
  %b.lane = getelementptr <8 x i32>, ptr addrspace(1) %b.ptr, i64 %lane
  %a = load <8 x i16>, ptr addrspace(1) %a.lane, align 16
  %b = load <8 x i32>, ptr addrspace(1) %b.lane, align 32

  %i16a0.lane = getelementptr i16, ptr addrspace(1) %i16a0.ptr, i64 %lane
  %i16a1.lane = getelementptr i16, ptr addrspace(1) %i16a1.ptr, i64 %lane
  %i16b0.lane = getelementptr i16, ptr addrspace(1) %i16b0.ptr, i64 %lane
  %i16b1.lane = getelementptr i16, ptr addrspace(1) %i16b1.ptr, i64 %lane
  %i16a0.raw = load i16, ptr addrspace(1) %i16a0.lane, align 2
  %i16a1.raw = load i16, ptr addrspace(1) %i16a1.lane, align 2
  %i16b0.raw = load i16, ptr addrspace(1) %i16b0.lane, align 2
  %i16b1.raw = load i16, ptr addrspace(1) %i16b1.lane, align 2
  %generic.swizzled = call <4 x i8> @llvm.genx.GenISA.byte.swizzle(i16 %i16a0.raw, i16 %i16a1.raw, <4 x i32> <i32 0, i32 2, i32 1, i32 3>)
  %generic.packed = bitcast <4 x i8> %generic.swizzled to i32
  %generic.out = getelementptr i32, ptr addrspace(1) %out, i64 1024
  store i32 %generic.packed, ptr addrspace(1) %generic.out, align 4
  %i16a0 = bitcast i16 %i16a0.raw to <2 x i8>
  %i16a1 = bitcast i16 %i16a1.raw to <2 x i8>
  %i16b0 = bitcast i16 %i16b0.raw to <2 x i8>
  %i16b1 = bitcast i16 %i16b1.raw to <2 x i8>

  %i8a00.lane = getelementptr i8, ptr addrspace(1) %i8a00.ptr, i64 %lane
  %i8a01.lane = getelementptr i8, ptr addrspace(1) %i8a01.ptr, i64 %lane
  %i8a10.lane = getelementptr i8, ptr addrspace(1) %i8a10.ptr, i64 %lane
  %i8a11.lane = getelementptr i8, ptr addrspace(1) %i8a11.ptr, i64 %lane
  %i8b00.lane = getelementptr i8, ptr addrspace(1) %i8b00.ptr, i64 %lane
  %i8b01.lane = getelementptr i8, ptr addrspace(1) %i8b01.ptr, i64 %lane
  %i8b10.lane = getelementptr i8, ptr addrspace(1) %i8b10.ptr, i64 %lane
  %i8b11.lane = getelementptr i8, ptr addrspace(1) %i8b11.ptr, i64 %lane
  %i8a00 = load i8, ptr addrspace(1) %i8a00.lane, align 1
  %i8a01 = load i8, ptr addrspace(1) %i8a01.lane, align 1
  %i8a10 = load i8, ptr addrspace(1) %i8a10.lane, align 1
  %i8a11 = load i8, ptr addrspace(1) %i8a11.lane, align 1
  %i8b00 = load i8, ptr addrspace(1) %i8b00.lane, align 1
  %i8b01 = load i8, ptr addrspace(1) %i8b01.lane, align 1
  %i8b10 = load i8, ptr addrspace(1) %i8b10.lane, align 1
  %i8b11 = load i8, ptr addrspace(1) %i8b11.lane, align 1
  %i8a0.0 = insertelement <2 x i8> poison, i8 %i8a00, i32 0
  %i8a0 = insertelement <2 x i8> %i8a0.0, i8 %i8a01, i32 1
  %i8a1.0 = insertelement <2 x i8> poison, i8 %i8a10, i32 0
  %i8a1 = insertelement <2 x i8> %i8a1.0, i8 %i8a11, i32 1
  %i8b0.0 = insertelement <2 x i8> poison, i8 %i8b00, i32 0
  %i8b0 = insertelement <2 x i8> %i8b0.0, i8 %i8b01, i32 1
  %i8b1.0 = insertelement <2 x i8> poison, i8 %i8b10, i32 0
  %i8b1 = insertelement <2 x i8> %i8b1.0, i8 %i8b11, i32 1

  %r00.1 = call <8 x float> @llvm.genx.GenISA.sub.group.bdpas.v8f32.v8f32.v8i16.v8i32.v2i8.v2i8(
      <8 x float> zeroinitializer, <8 x i16> %a, <8 x i32> %b,
      <2 x i8> %i16a0, <2 x i8> %i16b0, i32 13, i32 13
  )
  %r01.1 = call <8 x float> @llvm.genx.GenISA.sub.group.bdpas.v8f32.v8f32.v8i16.v8i32.v2i8.v2i8(
      <8 x float> zeroinitializer, <8 x i16> %a, <8 x i32> %b,
      <2 x i8> %i16a0, <2 x i8> %i16b1, i32 13, i32 13
  )
  %r10.1 = call <8 x float> @llvm.genx.GenISA.sub.group.bdpas.v8f32.v8f32.v8i16.v8i32.v2i8.v2i8(
      <8 x float> zeroinitializer, <8 x i16> %a, <8 x i32> %b,
      <2 x i8> %i16a1, <2 x i8> %i16b0, i32 13, i32 13
  )
  %r11.1 = call <8 x float> @llvm.genx.GenISA.sub.group.bdpas.v8f32.v8f32.v8i16.v8i32.v2i8.v2i8(
      <8 x float> zeroinitializer, <8 x i16> %a, <8 x i32> %b,
      <2 x i8> %i16a1, <2 x i8> %i16b1, i32 13, i32 13
  )
  %r00 = call <8 x float> @llvm.genx.GenISA.sub.group.bdpas.v8f32.v8f32.v8i16.v8i32.v2i8.v2i8(
      <8 x float> %r00.1, <8 x i16> %a, <8 x i32> %b,
      <2 x i8> %i8a0, <2 x i8> %i8b0, i32 13, i32 13
  )
  %r01 = call <8 x float> @llvm.genx.GenISA.sub.group.bdpas.v8f32.v8f32.v8i16.v8i32.v2i8.v2i8(
      <8 x float> %r01.1, <8 x i16> %a, <8 x i32> %b,
      <2 x i8> %i8a0, <2 x i8> %i8b1, i32 13, i32 13
  )
  %r10 = call <8 x float> @llvm.genx.GenISA.sub.group.bdpas.v8f32.v8f32.v8i16.v8i32.v2i8.v2i8(
      <8 x float> %r10.1, <8 x i16> %a, <8 x i32> %b,
      <2 x i8> %i8a1, <2 x i8> %i8b0, i32 13, i32 13
  )
  %r11 = call <8 x float> @llvm.genx.GenISA.sub.group.bdpas.v8f32.v8f32.v8i16.v8i32.v2i8.v2i8(
      <8 x float> %r11.1, <8 x i16> %a, <8 x i32> %b,
      <2 x i8> %i8a1, <2 x i8> %i8b1, i32 13, i32 13
  )

  %out1 = getelementptr <8 x float>, ptr addrspace(1) %out, i64 1
  %out2 = getelementptr <8 x float>, ptr addrspace(1) %out, i64 2
  %out3 = getelementptr <8 x float>, ptr addrspace(1) %out, i64 3
  store <8 x float> %r00, ptr addrspace(1) %out, align 32
  store <8 x float> %r01, ptr addrspace(1) %out1, align 32
  store <8 x float> %r10, ptr addrspace(1) %out2, align 32
  store <8 x float> %r11, ptr addrspace(1) %out3, align 32
  ret void
}

declare <8 x float> @llvm.genx.GenISA.sub.group.bdpas.v8f32.v8f32.v8i16.v8i32.v2i8.v2i8(
    <8 x float>, <8 x i16>, <8 x i32>, <2 x i8>, <2 x i8>, i32, i32
)
declare <4 x i8> @llvm.genx.GenISA.byte.swizzle(i16, i16, <4 x i32>)
declare i16 @llvm.genx.GenISA.simdLaneId()

!igc.functions = !{!0}
!0 = !{ptr @automatic_scale_regioning, !1}
!1 = !{!2, !3}
!2 = !{!"function_type", i32 0}
!3 = !{!"sub_group_size", i32 16}
!4 = !{i32 16}

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

; A semantically valid packed call may still miss the physical alias fast path.
; These scale loads are uniform, so the selected source ranges are copied into
; safe 0/+32 temporaries. Capture the operands structurally without relying on
; debug-only variable names.

; CHECK-ASM: .kernel "fp4_bdpas_scale_regioning_fallback"
; CHECK-ASM: mov (M1, 16) [[B_SCALE:[A-Za-z0-9_]+]](0,0)<1> {{.*}}<1;1,0>
; CHECK-ASM: mov (M1, 16) [[B_SCALE]](0,32)<1> {{.*}}<1;1,0>
; CHECK-ASM: mov (M1, 8) [[A_SCALE:[A-Za-z0-9_]+]](0,0)<1> {{.*}}<1;1,0>
; CHECK-ASM: mov (M1, 8) [[A_SCALE]](0,32)<1> {{.*}}<1;1,0>
; CHECK-ASM: bdpas.e2m1.e2m1.8.8 {{.*}} [[B_SCALE]](0,0) [[A_SCALE]](0,0)

target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v512:512:512-v1024:1024:1024"
target triple = "spir64-unknown-unknown"

define spir_kernel void @fp4_bdpas_scale_regioning_fallback(
    ptr addrspace(1) nocapture readonly %a_ptr,
    ptr addrspace(1) nocapture readonly %b_ptr,
    ptr addrspace(1) nocapture readonly %scale_a_ptr,
    ptr addrspace(1) nocapture readonly %scale_b_ptr,
    ptr addrspace(1) nocapture %out) #0 !kernel_arg_addr_space !0 !kernel_arg_access_qual !1 !kernel_arg_type !2 !kernel_arg_type_qual !3 !kernel_arg_base_type !2 !intel_reqd_sub_group_size !4 {
entry:
  %a = load <8 x i16>, ptr addrspace(1) %a_ptr, align 16
  %b = load <8 x i32>, ptr addrspace(1) %b_ptr, align 32
  %scale_a = load <4 x i8>, ptr addrspace(1) %scale_a_ptr, align 4
  %scale_b = load <4 x i8>, ptr addrspace(1) %scale_b_ptr, align 4
  %result = call <8 x float> @llvm.genx.GenISA.sub.group.bdpas.packed.v8f32.v8f32.v8i16.v8i32.v4i8.v4i8(
      <8 x float> zeroinitializer, <8 x i16> %a, <8 x i32> %b,
      <4 x i8> %scale_a, <4 x i8> %scale_b, i32 13, i32 13,
      i32 8, i32 16)
  store <8 x float> %result, ptr addrspace(1) %out, align 32
  ret void
}

declare <8 x float> @llvm.genx.GenISA.sub.group.bdpas.packed.v8f32.v8f32.v8i16.v8i32.v4i8.v4i8(
    <8 x float>, <8 x i16>, <8 x i32>, <4 x i8>, <4 x i8>, i32, i32,
    i32, i32)

attributes #0 = { nounwind }

!0 = !{i32 1, i32 1, i32 1, i32 1, i32 1}
!1 = !{!"none", !"none", !"none", !"none", !"none"}
!2 = !{!"short8*", !"int8*", !"uchar4*", !"uchar4*", !"float8*"}
!3 = !{!"", !"", !"", !"", !""}
!4 = !{i32 16}

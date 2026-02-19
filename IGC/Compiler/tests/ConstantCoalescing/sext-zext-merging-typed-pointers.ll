;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; UNSUPPORTED: llvm-17-plus
; RUN: igc_opt --typed-pointers --igc-constant-coalescing -dce -S < %s | FileCheck %s


define void @test_sext_zext(i32 addrspace(2)* %ca, i16 %offset) {
  ; CHECK-LABEL: @test_sext_zext
  ; CHECK: [[S1:%.*]] = sext i16 %offset to i32
  ; CHECK: [[Z1:%.*]] = zext i16 %offset to i32
  ; CHECK: [[Z2:%.*]] = add i32 [[Z1]], 4
  ; CHECK: [[PTR:%.*]] = ptrtoint i32 addrspace(2)* %ca to i32
  ; CHECK: [[I1:%.*]] = add i32 [[PTR]], [[S1]]
  ; CHECK: [[I2:%.*]] = add i32 [[PTR]], [[Z2]]
  ; CHECK: [[PTR1:%.*]] = inttoptr i32 {{%.*}} to <1 x i32> addrspace(2)*
  ; CHECK: {{%.*}} = load <1 x i32>, <1 x i32> addrspace(2)* [[PTR1]], align 4
  ; CHECK: [[PTR2:%.*]] = inttoptr i32 {{%.*}} to <1 x i32> addrspace(2)*
  ; CHECK: {{%.*}} = load <1 x i32>, <1 x i32> addrspace(2)* [[PTR2]], align 4
  %s1 = sext i16 %offset to i32
  %z1 = zext i16 %offset to i32
  %z2 = add i32 %z1, 4
  %p1 = ptrtoint i32 addrspace(2)* %ca to i32
  %i1 = add i32 %p1, %s1
  %i2 = add i32 %p1, %z2
  %a1 = inttoptr i32 %i1 to i32 addrspace(2)*
  %a2 = inttoptr i32 %i2 to i32 addrspace(2)*
  %1 = load i32, i32 addrspace(2)* %a2, align 4
  %2 = load i32, i32 addrspace(2)* %a1, align 4
  call void @use.i32(i32 %1)
  call void @use.i32(i32 %2)
  ret void
}

define void @test_zext_coalescing(i32 addrspace(2)* %ca, i16 %offset) {
  ; CHECK-LABEL: @test_zext_coalescing
  ; CHECK: {{%.*}} = load <2 x i32>, <2 x i32> addrspace(2)* {{%.*}}, align 4
  ; CHECK-NOT: load
  %z1 = zext i16 %offset to i32
  %z2 = add i32 %z1, 4
  %p1 = ptrtoint i32 addrspace(2)* %ca to i32
  %i1 = add i32 %p1, %z1
  %i2 = add i32 %p1, %z2
  %a1 = inttoptr i32 %i1 to i32 addrspace(2)*
  %a2 = inttoptr i32 %i2 to i32 addrspace(2)*
  %1 = load i32, i32 addrspace(2)* %a2, align 4
  %2 = load i32, i32 addrspace(2)* %a1, align 4
  call void @use.i32(i32 %1)
  call void @use.i32(i32 %2)
  ret void
}

declare void @use.i32(i32)

!igc.functions = !{!0, !4}
!0 = !{void (i32 addrspace(2)*, i16)* @test_sext_zext, !1}
!1 = !{!2, !3}
!2 = !{!"function_type", i32 0}
!3 = !{!"implicit_arg_desc"}
!4 = !{void (i32 addrspace(2)*, i16)* @test_zext_coalescing, !1}
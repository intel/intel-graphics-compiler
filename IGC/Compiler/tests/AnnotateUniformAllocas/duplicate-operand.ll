;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; REQUIRES: llvm-16-plus
; RUN: igc_opt --opaque-pointers -annotate_uniform_allocas -S < %s | FileCheck %s
; ------------------------------------------------
; AnnotateUniformAllocas
; ------------------------------------------------
;
; The address operand of assume.uniform is computed from an instruction that
; is used twice by the same user (mul %m, %m). Erasing the dead address chain
; must not visit %m twice (use-after-free).

define void @test(ptr addrspace(1) %out, i32 %n) {
; CHECK-LABEL: @test(
; CHECK:         [[A:%.*]] = alloca [8 x i32], align 4
; CHECK-NOT:     add i32
; CHECK-NOT:     mul i32
; CHECK-NOT:     assume.uniform
; CHECK:         [[N7:%.*]] = and i32 %n, 7
; CHECK:         [[P:%.*]] = getelementptr inbounds [8 x i32], ptr [[A]], i32 0, i32 [[N7]]
; CHECK:         [[V:%.*]] = load i32, ptr [[P]]
; CHECK:         store i32 [[V]], ptr addrspace(1) %out
; CHECK:         ret void
;
  %a = alloca [8 x i32], align 4
  %m = add i32 %n, 1
  %mm = mul i32 %m, %m
  %i = and i32 %mm, 7
  %p1 = getelementptr inbounds [8 x i32], ptr %a, i32 0, i32 %i
  call void @llvm.genx.GenISA.assume.uniform.p0(ptr %p1)
  %n7 = and i32 %n, 7
  %p2 = getelementptr inbounds [8 x i32], ptr %a, i32 0, i32 %n7
  %v = load i32, ptr %p2
  store i32 %v, ptr addrspace(1) %out
  ret void
}

declare void @llvm.genx.GenISA.assume.uniform.p0(ptr)

!igc.functions = !{!1}
!1 = !{ptr @test, !2}
!2 = !{!3}
!3 = !{!"function_type", i32 0}

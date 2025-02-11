;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: llvm-14-plus
; RUN: igc_opt --opaque-pointers %s -S -o - --dx12 --inputcs --igc-constant-coalescing -dce | FileCheck %s

define void @test_ldraw_merge() {
entry:
  %a = call i16 @llvm.genx.GenISA.DCL.SystemValue.i16(i32 7)
  %src = sext i16 %a to i32
  %0 = inttoptr i32 %src to ptr addrspace(2555909)
  %1 = call fast float @llvm.genx.GenISA.ldraw.indexed.f32.p2555909(ptr addrspace(2555909) %0, i32 %src, i32 4, i1 false)
  %2 = add i32 %src, 4
  %3 = call fast float @llvm.genx.GenISA.ldraw.indexed.f32.p2555909(ptr addrspace(2555909) %0, i32 %2, i32 4, i1 false)
; CHECK: %2 = call <2 x float> @llvm.genx.GenISA.ldrawvector.indexed.v2f32.p2555909(ptr addrspace(2555909) %0, i32 %1, i32 4, i1 false)
  call void @use.f32(float %1)
  call void @use.f32(float %3)
  ret void
}

declare void @use.f32(float)

declare i16 @llvm.genx.GenISA.DCL.SystemValue.i16(i32)

; Function Attrs: argmemonly nounwind readonly
declare float @llvm.genx.GenISA.ldraw.indexed.f32.p2555909(ptr addrspace(2555909), i32, i32, i1) #0

attributes #0 = { argmemonly nounwind readonly }

!igc.functions = !{!0}

!0 = !{ptr @test_ldraw_merge, !1}
!1 = !{!2, !3}
!2 = !{!"function_type", i32 0}
!3 = !{!"implicit_arg_desc"}

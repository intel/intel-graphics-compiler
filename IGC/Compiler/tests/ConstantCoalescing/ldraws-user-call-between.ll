;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: llvm-14-plus
; RUN: igc_opt --opaque-pointers %s -S -o - -igc-constant-coalescing | FileCheck %s

target datalayout = "e-p:32:32:32-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f16:16:16-f32:32:32-f64:64:64-f80:128:128-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024-a:64:64-f80:128:128-n8:16:32:64"

define <2 x float> @test_regular_function_call(ptr addrspace(1) %arg0, i32 %bindlessOffset_arg0) {
entry:
  %0 = inttoptr i32 %bindlessOffset_arg0 to ptr addrspace(2490373)
  %1 = call fast float @llvm.genx.GenISA.ldraw.indexed.f32.p2490373(ptr addrspace(2490373) %0, i32 0, i32 4, i1 false)
  call void @callee(ptr addrspace(1) %arg0)
  %2 = call fast float @llvm.genx.GenISA.ldraw.indexed.f32.p2490373(ptr addrspace(2490373) %0, i32 4, i32 4, i1 false)
  %3 = insertelement <2 x float> undef, float %1, i32 0
  %4 = insertelement <2 x float> %3, float %2, i32 1
  ret <2 x float> %4
}

define void @callee(ptr addrspace(1) %ptr) {
entry:
  store i32 1, ptr addrspace(1) %ptr
  ret  void
}

 ; CHECK-LABEL: define <2 x float> @test_regular_function_call
 ; CHECK-NOT: call <2 x float> @llvm.genx.GenISA.ldrawvector.indexed.v2f32.p2490373

define <2 x float> @test_indirect_function_call(ptr %fptr, ptr addrspace(1) %arg0, i32 %bindlessOffset_arg0) {
entry:
  %0 = inttoptr i32 %bindlessOffset_arg0 to ptr addrspace(2490373)
  %1 = call fast float @llvm.genx.GenISA.ldraw.indexed.f32.p2490373(ptr addrspace(2490373) %0, i32 0, i32 4, i1 false)
  call void %fptr(ptr addrspace(1) %arg0)
  %2 = call fast float @llvm.genx.GenISA.ldraw.indexed.f32.p2490373(ptr addrspace(2490373) %0, i32 4, i32 4, i1 false)
  %3 = insertelement <2 x float> undef, float %1, i32 0
  %4 = insertelement <2 x float> %3, float %2, i32 1
  ret <2 x float> %4
}

 ; CHECK-LABEL: define <2 x float> @test_indirect_function_call
 ; CHECK-NOT: call <2 x float> @llvm.genx.GenISA.ldrawvector.indexed.v2f32.p2490373

; Function Attrs: argmemonly nounwind readonly
declare float @llvm.genx.GenISA.ldraw.indexed.f32.p2490373(ptr addrspace(2490373), i32, i32, i1) argmemonly nounwind readonly

; Function Attrs: argmemonly nounwind writeonly
declare void @llvm.genx.GenISA.storeraw.indexed.p2490368.f32(ptr addrspace(2490373), i32, float, i32, i1) argmemonly nounwind writeonly


!igc.functions = !{!0, !3, !4}

!0 = !{ptr @test_regular_function_call, !1}
!3 = !{ptr @callee, !1}
!4 = !{ptr @test_indirect_function_call, !1}

!1 = !{!2}
!2 = !{!"function_type", i32 0}

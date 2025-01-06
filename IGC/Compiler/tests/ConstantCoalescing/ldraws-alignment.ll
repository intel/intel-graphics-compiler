;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022-2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: igc_opt --opaque-pointers %s -S -o - -igc-constant-coalescing -instcombine -dce | FileCheck %s

target datalayout = "e-p:32:32:32-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f16:16:16-f32:32:32-f64:64:64-f80:128:128-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024-a:64:64-f80:128:128-n8:16:32:64"

define <4 x half> @f0(i32 %src) {
entry:
  %bso = inttoptr i32 %src to ptr addrspace(2490373)
  %ox = shl i32 %src, 2
  %x = call half @llvm.genx.GenISA.ldraw.indexed.f16.p2490373i8(ptr addrspace(2490373) %bso, i32 %ox, i32 2, i1 false)
  %oy = add i32 %ox, 2
  %y = call half @llvm.genx.GenISA.ldraw.indexed.f16.p2490373i8(ptr addrspace(2490373) %bso, i32 %oy, i32 2, i1 false)
  %oz = add i32 %ox, 4
  %z = call half @llvm.genx.GenISA.ldraw.indexed.f16.p2490373i8(ptr addrspace(2490373) %bso, i32 %oz, i32 2, i1 false)
  %ow = add i32 %ox, 6
  %w = call half @llvm.genx.GenISA.ldraw.indexed.f16.p2490373i8(ptr addrspace(2490373) %bso, i32 %ow, i32 2, i1 false)
  %ret.x = insertelement <4 x half> undef, half %x, i32 0
  %ret.xy = insertelement <4 x half> %ret.x, half %y, i32 1
  %ret.xyz = insertelement <4 x half> %ret.xy, half %z, i32 2
  %ret.xyzw = insertelement <4 x half> %ret.xyz, half %w, i32 3
  ret <4 x half> %ret.xyzw
}
; CHECK-LABEL: define <4 x half> @f0
; CHECK: [[BSO:%.*]] = inttoptr i32 %src to ptr addrspace(2490373)
; CHECK: [[OFF:%.*]] = shl i32 %src, 2
; CHECK: [[RET:%.*]] = call <4 x half> @llvm.genx.GenISA.ldrawvector.indexed.v4f16.p2490373(ptr addrspace(2490373) [[BSO]], i32 [[OFF]], i32 4, i1 false)
; CHECK: ret <4 x half> [[RET]]

; Function Attrs: argmemonly nounwind readonly
declare float @llvm.genx.GenISA.ldraw.indexed.f32.p2490373i8(ptr addrspace(2490373), i32, i32, i1) argmemonly nounwind readonly
declare half @llvm.genx.GenISA.ldraw.indexed.f16.p2490373i8(ptr addrspace(2490373), i32, i32, i1) argmemonly nounwind readonly

; Function Attrs: argmemonly nounwind writeonly
declare void @llvm.genx.GenISA.storeraw.indexed.p2490368i8.f32(ptr addrspace(2490373), i32, float, i32, i1) argmemonly nounwind writeonly


!igc.functions = !{!0}

!0 = !{<4 x half> (i32)* @f0, !1}

!1 = !{!2}
!2 = !{!"function_type", i32 0}


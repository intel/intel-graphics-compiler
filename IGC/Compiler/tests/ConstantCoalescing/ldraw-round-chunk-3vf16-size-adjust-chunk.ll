;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: llvm-14-plus
; RUN: igc_opt --opaque-pointers %s -S -o - -igc-constant-coalescing -instcombine -dce | FileCheck %s --check-prefixes=CHECK

target datalayout = "e-p:32:32:32-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f16:16:16-f32:32:32-f64:64:64-f80:128:128-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024-a:64:64-f80:128:128-n8:16:32:64"

define <3 x half> @f0(i32 %src) {
entry:
  %buf = inttoptr i32 %src to ptr addrspace(2490373)
  %off = add i32 %src, 4
  %z = call half @llvm.genx.GenISA.ldraw.indexed.f16.p2490373(ptr addrspace(2490373) %buf, i32 %off, i32 4, i1 false)
  %xy = call <2 x half> @llvm.genx.GenISA.ldraw.indexed.v2f16.p2490373(ptr addrspace(2490373) %buf, i32 %src, i32 4, i1 false)
  %x = extractelement <2 x half> %xy, i32 0
  %y = extractelement <2 x half> %xy, i32 1
  %res.x = insertelement <3 x half> undef, half %x, i32 0
  %res.xy = insertelement <3 x half> %res.x, half %y, i32 1
  %res.xyz = insertelement <3 x half> %res.xy, half %z, i32 2
  ret <3 x half> %res.xyz
}

 ; CHECK-LABEL: define <3 x half> @f0
 ; CHECK: [[PTR:%.*]] = inttoptr i32 %src to ptr addrspace(2490373)
 ; CHECK: [[CHUNK:%.*]] = call <4 x half> @llvm.genx.GenISA.ldrawvector.indexed.v4f16.p2490373(ptr addrspace(2490373) [[PTR]], i32 %src, i32 4, i1 false)
 ; CHECK: [[RESULT:%.*]] = shufflevector <4 x half> [[CHUNK]], <4 x half> undef, <3 x i32> <i32 0, i32 1, i32 2>
 ; CHECK: ret <3 x half> [[RESULT]]


; Function Attrs: argmemonly nounwind readonly willreturn
declare half @llvm.genx.GenISA.ldraw.indexed.f16.p2490373(ptr addrspace(2490373), i32, i32, i1) argmemonly nounwind readonly willreturn
declare <2 x half> @llvm.genx.GenISA.ldraw.indexed.v2f16.p2490373(ptr addrspace(2490373), i32, i32, i1) argmemonly nounwind readonly willreturn


!igc.functions = !{!0}

!0 = !{ptr @f0, !1}


!1 = !{!2}
!2 = !{!"function_type", i32 0}


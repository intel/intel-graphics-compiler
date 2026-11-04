;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: igc_opt --opaque-pointers %s -S -o - -igc-constant-coalescing -dce | FileCheck %s

; Uniform loads of 32-byte records indexed by a loop induction variable.
; Reduced from a closest-hit light loop that was runtime-unrolled by 2.

target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024-n8:16:32"

; After unrolling by 2, %iv starts at 0 and steps by 2, so %rec0.off = %iv << 5
; is a multiple of 64 and the unroller addresses record 2i+1 as
; `or %rec0.off, 32`. That `or` is an `add`, so the record 2i+1 loads in %rec1
; must join the record 2i chunk loaded in the loop header.

; CHECK-LABEL: define void @unrolled_by_2(
; CHECK:       rec0:
; CHECK:         [[CHUNK:%.*]] = call <16 x float> @llvm.genx.GenISA.ldrawvector.indexed.v16f32.p2555904(ptr addrspace(2555904) %lights, i32 %rec0.addr, i32 4, i1 false)
; CHECK-NOT:     @llvm.genx.GenISA.ldraw
; CHECK:         %rec0.pos = extractelement <16 x float> [[CHUNK]], i64 0
; CHECK:         %rec0.radius = extractelement <16 x float> [[CHUNK]], i64 3
; CHECK:       rec0.shade:
; CHECK:         %rec0.color = extractelement <16 x float> [[CHUNK]], i32 4
; CHECK:       rec1:
; CHECK:         %rec1.pos = extractelement <16 x float> [[CHUNK]], i32 8
; CHECK:         %rec1.radius = extractelement <16 x float> [[CHUNK]], i32 11
; CHECK:       rec1.shade:
; CHECK:         %rec1.color = extractelement <16 x float> [[CHUNK]], i32 12
; CHECK:       exit:

define void @unrolled_by_2(ptr addrspace(2555904) %lights, i32 %lightsBase, i32 %lightCount, float %px, ptr addrspace(1) %out) {
entry:
  br label %rec0

rec0:                                             ; light 2i
  %iv = phi i32 [ 0, %entry ], [ %iv.next, %rec1.join ]
  %acc = phi float [ 0.000000e+00, %entry ], [ %acc.next, %rec1.join ]
  %rec0.off = shl i32 %iv, 5
  %rec0.addr = add i32 %lightsBase, %rec0.off
  %rec0.lo = call <4 x float> @llvm.genx.GenISA.ldrawvector.indexed.v4f32.p2555904(ptr addrspace(2555904) %lights, i32 %rec0.addr, i32 4, i1 false)
  %rec0.hi.addr = add i32 %rec0.addr, 16
  %rec0.hi = call <3 x float> @llvm.genx.GenISA.ldrawvector.indexed.v3f32.p2555904(ptr addrspace(2555904) %lights, i32 %rec0.hi.addr, i32 4, i1 false)
  %rec0.pos = extractelement <4 x float> %rec0.lo, i64 0
  %rec0.radius = extractelement <4 x float> %rec0.lo, i64 3
  %rec0.dist = fsub fast float %rec0.pos, %px
  %rec0.hit = fcmp fast olt float %rec0.dist, %rec0.radius
  br i1 %rec0.hit, label %rec0.shade, label %rec1

rec0.shade:
  %rec0.color = extractelement <3 x float> %rec0.hi, i64 0
  %rec0.light = fmul fast float %rec0.color, %rec0.dist
  br label %rec1

rec1:                                             ; light 2i+1
  %rec0.contrib = phi float [ %rec0.light, %rec0.shade ], [ 0.000000e+00, %rec0 ]
  %acc.0 = fadd fast float %acc, %rec0.contrib
  %rec1.off = or i32 %rec0.off, 32
  %rec1.addr = add i32 %lightsBase, %rec1.off
  %rec1.lo = call <4 x float> @llvm.genx.GenISA.ldrawvector.indexed.v4f32.p2555904(ptr addrspace(2555904) %lights, i32 %rec1.addr, i32 4, i1 false)
  %rec1.hi.addr = add i32 %rec1.addr, 16
  %rec1.hi = call <3 x float> @llvm.genx.GenISA.ldrawvector.indexed.v3f32.p2555904(ptr addrspace(2555904) %lights, i32 %rec1.hi.addr, i32 4, i1 false)
  %rec1.pos = extractelement <4 x float> %rec1.lo, i64 0
  %rec1.radius = extractelement <4 x float> %rec1.lo, i64 3
  %rec1.dist = fsub fast float %rec1.pos, %px
  %rec1.hit = fcmp fast olt float %rec1.dist, %rec1.radius
  br i1 %rec1.hit, label %rec1.shade, label %rec1.join

rec1.shade:
  %rec1.color = extractelement <3 x float> %rec1.hi, i64 0
  %rec1.light = fmul fast float %rec1.color, %rec1.dist
  br label %rec1.join

rec1.join:
  %rec1.contrib = phi float [ %rec1.light, %rec1.shade ], [ 0.000000e+00, %rec1 ]
  %acc.next = fadd fast float %acc.0, %rec1.contrib
  %iv.next = add nuw i32 %iv, 2
  %done = icmp eq i32 %iv.next, %lightCount
  br i1 %done, label %exit, label %rec0

exit:
  store float %acc.next, ptr addrspace(1) %out, align 4
  ret void
}

; Same loads, but %iv steps by 1. Bit 5 of %rec0.off may be set, so the `or`
; is not an `add` and the two records must stay separate.

; CHECK-LABEL: define void @step_by_1(
; CHECK:         call <1 x float> @llvm.genx.GenISA.ldrawvector.indexed.v1f32.p2555904(ptr addrspace(2555904) %lights, i32 %rec0.addr, i32 4, i1 false)
; CHECK:         call <1 x float> @llvm.genx.GenISA.ldrawvector.indexed.v1f32.p2555904(ptr addrspace(2555904) %lights, i32 %rec1.addr, i32 4, i1 false)
; CHECK:       exit:

define void @step_by_1(ptr addrspace(2555904) %lights, i32 %lightsBase, i32 %lightCount, ptr addrspace(1) %out) {
entry:
  br label %loop

loop:
  %iv = phi i32 [ 0, %entry ], [ %iv.next, %loop ]
  %acc = phi float [ 0.000000e+00, %entry ], [ %acc.next, %loop ]
  %rec0.off = shl i32 %iv, 5
  %rec0.addr = add i32 %lightsBase, %rec0.off
  %rec0 = call <4 x float> @llvm.genx.GenISA.ldrawvector.indexed.v4f32.p2555904(ptr addrspace(2555904) %lights, i32 %rec0.addr, i32 4, i1 false)
  %rec1.off = or i32 %rec0.off, 32
  %rec1.addr = add i32 %lightsBase, %rec1.off
  %rec1 = call <4 x float> @llvm.genx.GenISA.ldrawvector.indexed.v4f32.p2555904(ptr addrspace(2555904) %lights, i32 %rec1.addr, i32 4, i1 false)
  %rec0.x = extractelement <4 x float> %rec0, i64 0
  %rec1.x = extractelement <4 x float> %rec1, i64 0
  %sum = fadd fast float %rec0.x, %rec1.x
  %acc.next = fadd fast float %acc, %sum
  %iv.next = add nuw i32 %iv, 1
  %done = icmp eq i32 %iv.next, %lightCount
  br i1 %done, label %exit, label %loop

exit:
  store float %acc.next, ptr addrspace(1) %out, align 4
  ret void
}

declare <4 x float> @llvm.genx.GenISA.ldrawvector.indexed.v4f32.p2555904(ptr addrspace(2555904), i32, i32, i1)
declare <3 x float> @llvm.genx.GenISA.ldrawvector.indexed.v3f32.p2555904(ptr addrspace(2555904), i32, i32, i1)

!igc.functions = !{!0, !3}

!0 = !{ptr @unrolled_by_2, !1}
!3 = !{ptr @step_by_1, !1}

!1 = !{!2}
!2 = !{!"function_type", i32 0}

;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; REQUIRES: regkeys
; RUN: igc_opt --opaque-pointers --regkey HoistImmutableBufferLoadsMaxPerLoop=0 -igc-hoist-immutable-buffer-loads -S < %s | FileCheck %s --check-prefix=LIFT
; RUN: igc_opt --opaque-pointers --regkey HoistImmutableBufferLoadsMaxPerLoop=3 -igc-hoist-immutable-buffer-loads -S < %s | FileCheck %s --check-prefix=LIFT
; RUN: igc_opt --opaque-pointers --regkey HoistImmutableBufferLoadsMaxPerLoop=2 -igc-hoist-immutable-buffer-loads -S < %s | FileCheck %s --check-prefix=KEEP
; ------------------------------------------------
; HoistImmutableBufferLoads: HoistImmutableBufferLoadsMaxPerLoop
; ------------------------------------------------

; COM: Three loads: lifted with no limit or a budget of three, all kept with two.
; LIFT-LABEL: @three_loads(
; LIFT:       entry:
; LIFT-NEXT:    %v0 = call float @llvm.genx.GenISA.ldraw.indexed.f32.p2555904(
; LIFT-NEXT:    %v1 = call float @llvm.genx.GenISA.ldraw.indexed.f32.p2555904(
; LIFT-NEXT:    %v2 = call float @llvm.genx.GenISA.ldraw.indexed.f32.p2555904(
; LIFT-NEXT:    br label %loop
; KEEP-LABEL: @three_loads(
; KEEP:       entry:
; KEEP-NEXT:    br label %loop
; KEEP:       loop:
; KEEP:         %v0 = call float @llvm.genx.GenISA.ldraw.indexed.f32.p2555904(
define void @three_loads(ptr addrspace(2555904) %cb, ptr addrspace(10903552) %uav, i32 %n) {
entry:
  br label %loop

loop:
  %i = phi i32 [ 0, %entry ], [ %i.next, %loop ]
  %off = shl i32 %i, 2
  %v0 = call float @llvm.genx.GenISA.ldraw.indexed.f32.p2555904(ptr addrspace(2555904) %cb, i32 16, i32 4, i1 false)
  %v1 = call float @llvm.genx.GenISA.ldraw.indexed.f32.p2555904(ptr addrspace(2555904) %cb, i32 20, i32 4, i1 false)
  %v2 = call float @llvm.genx.GenISA.ldraw.indexed.f32.p2555904(ptr addrspace(2555904) %cb, i32 24, i32 4, i1 false)
  %sum1 = fadd float %v0, %v1
  %sum2 = fadd float %sum1, %v2
  %bits = bitcast float %sum2 to i32
  %old = call i32 @llvm.genx.GenISA.intatomictyped.i32.p10903552(ptr addrspace(10903552) %uav, i32 %off, i32 0, i32 undef, i32 %bits, i32 0)
  %i.next = add i32 %i, 1
  %done = icmp eq i32 %i.next, %n
  br i1 %done, label %exit, label %loop

exit:
  ret void
}

declare float @llvm.genx.GenISA.ldraw.indexed.f32.p2555904(ptr addrspace(2555904), i32, i32, i1)
declare <4 x float> @llvm.genx.GenISA.ldrawvector.indexed.v4f32.p2555904(ptr addrspace(2555904), i32, i32, i1)
declare float @llvm.genx.GenISA.ldraw.indexed.f32.p131072(ptr addrspace(131072), i32, i32, i1)
declare void @llvm.genx.GenISA.storeraw.indexed.p131072.f32(ptr addrspace(131072), i32, float, i32, i1)
declare i32 @llvm.genx.GenISA.intatomictyped.i32.p10903552(ptr addrspace(10903552), i32, i32, i32, i32, i32)

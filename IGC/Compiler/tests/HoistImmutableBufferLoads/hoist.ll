;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: igc_opt --opaque-pointers -igc-hoist-immutable-buffer-loads -S < %s | FileCheck %s
; ------------------------------------------------
; HoistImmutableBufferLoads
; ------------------------------------------------

; COM: LICM leaves a read-only buffer read in a loop with an atomic: the atomic
; COM: may also read, and MemorySSA counts that as a clobber. The pass lifts it.
; CHECK-LABEL: @atomic_loop(
; CHECK:       entry:
; CHECK-NEXT:    %v = call float @llvm.genx.GenISA.ldraw.indexed.f32.p2555904(
; CHECK-NEXT:    br label %loop
; CHECK:       loop:
; CHECK-NOT:     ldraw
; CHECK:       exit:
define void @atomic_loop(ptr addrspace(2555904) %cb, ptr addrspace(10903552) %uav, i32 %n) {
entry:
  br label %loop

loop:
  %i = phi i32 [ 0, %entry ], [ %i.next, %loop ]
  %off = shl i32 %i, 2
  %v = call float @llvm.genx.GenISA.ldraw.indexed.f32.p2555904(ptr addrspace(2555904) %cb, i32 16, i32 4, i1 false)
  %bits = bitcast float %v to i32
  %old = call i32 @llvm.genx.GenISA.intatomictyped.i32.p10903552(ptr addrspace(10903552) %uav, i32 %off, i32 0, i32 undef, i32 %bits, i32 0)
  %i.next = add i32 %i, 1
  %done = icmp eq i32 %i.next, %n
  br i1 %done, label %exit, label %loop

exit:
  ret void
}

; COM: Above the default budget of one distinct load per loop, every load
; COM: stays: the lifted values would all be live across the loop body.
; CHECK-LABEL: @two_loads(
; CHECK:       entry:
; CHECK-NEXT:    br label %loop
; CHECK:       loop:
; CHECK:         %v0 = call float @llvm.genx.GenISA.ldraw.indexed.f32.p2555904(
; CHECK:         %vec = call <4 x float> @llvm.genx.GenISA.ldrawvector.indexed.v4f32.p2555904(
define void @two_loads(ptr addrspace(2555904) %cb, ptr addrspace(10903552) %uav, i32 %n) {
entry:
  br label %loop

loop:
  %i = phi i32 [ 0, %entry ], [ %i.next, %loop ]
  %off = shl i32 %i, 2
  %v0 = call float @llvm.genx.GenISA.ldraw.indexed.f32.p2555904(ptr addrspace(2555904) %cb, i32 16, i32 4, i1 false)
  %vec = call <4 x float> @llvm.genx.GenISA.ldrawvector.indexed.v4f32.p2555904(ptr addrspace(2555904) %cb, i32 32, i32 4, i1 false)
  %v1 = extractelement <4 x float> %vec, i32 0
  %sum = fadd float %v0, %v1
  %bits = bitcast float %sum to i32
  %old = call i32 @llvm.genx.GenISA.intatomictyped.i32.p10903552(ptr addrspace(10903552) %uav, i32 %off, i32 0, i32 undef, i32 %bits, i32 0)
  %i.next = add i32 %i, 1
  %done = icmp eq i32 %i.next, %n
  br i1 %done, label %exit, label %loop

exit:
  ret void
}

; COM: The same holds for more loads.
; CHECK-LABEL: @over_budget(
; CHECK:       entry:
; CHECK-NEXT:    br label %loop
; CHECK:       loop:
; CHECK:         %v0 = call float @llvm.genx.GenISA.ldraw.indexed.f32.p2555904(
; CHECK:         %v1 = call float @llvm.genx.GenISA.ldraw.indexed.f32.p2555904(
; CHECK:         %v2 = call float @llvm.genx.GenISA.ldraw.indexed.f32.p2555904(
define void @over_budget(ptr addrspace(2555904) %cb, ptr addrspace(10903552) %uav, i32 %n) {
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

; COM: Identical reads, as in an unrolled loop, count once against the budget.
; CHECK-LABEL: @unrolled(
; CHECK:       entry:
; CHECK-NEXT:    %v0 = call float @llvm.genx.GenISA.ldraw.indexed.f32.p2555904(
; CHECK-NEXT:    %v1 = call float @llvm.genx.GenISA.ldraw.indexed.f32.p2555904(
; CHECK-NEXT:    %v2 = call float @llvm.genx.GenISA.ldraw.indexed.f32.p2555904(
; CHECK-NEXT:    br label %loop
; CHECK:       loop:
; CHECK-NOT:     ldraw
; CHECK:       exit:
define void @unrolled(ptr addrspace(2555904) %cb, ptr addrspace(10903552) %uav, i32 %n) {
entry:
  br label %loop

loop:
  %i = phi i32 [ 0, %entry ], [ %i.next, %loop ]
  %off = shl i32 %i, 2
  %v0 = call float @llvm.genx.GenISA.ldraw.indexed.f32.p2555904(ptr addrspace(2555904) %cb, i32 16, i32 4, i1 false)
  %v1 = call float @llvm.genx.GenISA.ldraw.indexed.f32.p2555904(ptr addrspace(2555904) %cb, i32 16, i32 4, i1 false)
  %v2 = call float @llvm.genx.GenISA.ldraw.indexed.f32.p2555904(ptr addrspace(2555904) %cb, i32 16, i32 4, i1 false)
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

; COM: A read that does not run on every trip, a writable buffer, a volatile
; COM: read and an offset that changes per trip all stay.
; CHECK-LABEL: @conditional(
; CHECK:       then:
; CHECK-NEXT:    %v = call float @llvm.genx.GenISA.ldraw.indexed.f32.p2555904(
define void @conditional(ptr addrspace(2555904) %cb, ptr addrspace(10903552) %uav, i32 %n, i32 %k) {
entry:
  br label %loop

loop:
  %i = phi i32 [ 0, %entry ], [ %i.next, %latch ]
  %off = shl i32 %i, 2
  %c = icmp eq i32 %i, %k
  br i1 %c, label %then, label %latch

then:
  %v = call float @llvm.genx.GenISA.ldraw.indexed.f32.p2555904(ptr addrspace(2555904) %cb, i32 16, i32 4, i1 false)
  %bits = bitcast float %v to i32
  %old = call i32 @llvm.genx.GenISA.intatomictyped.i32.p10903552(ptr addrspace(10903552) %uav, i32 %off, i32 0, i32 undef, i32 %bits, i32 0)
  br label %latch

latch:
  %i.next = add i32 %i, 1
  %done = icmp eq i32 %i.next, %n
  br i1 %done, label %exit, label %loop

exit:
  ret void
}

; CHECK-LABEL: @uav_read(
; CHECK:       loop:
; CHECK:         %v = call float @llvm.genx.GenISA.ldraw.indexed.f32.p131072(
define void @uav_read(ptr addrspace(131072) %src, ptr addrspace(10903552) %uav, i32 %n) {
entry:
  br label %loop

loop:
  %i = phi i32 [ 0, %entry ], [ %i.next, %loop ]
  %off = shl i32 %i, 2
  %v = call float @llvm.genx.GenISA.ldraw.indexed.f32.p131072(ptr addrspace(131072) %src, i32 16, i32 4, i1 false)
  %bits = bitcast float %v to i32
  %old = call i32 @llvm.genx.GenISA.intatomictyped.i32.p10903552(ptr addrspace(10903552) %uav, i32 %off, i32 0, i32 undef, i32 %bits, i32 0)
  %i.next = add i32 %i, 1
  %done = icmp eq i32 %i.next, %n
  br i1 %done, label %exit, label %loop

exit:
  ret void
}

; CHECK-LABEL: @volatile_read(
; CHECK:       loop:
; CHECK:         %v = call float @llvm.genx.GenISA.ldraw.indexed.f32.p2555904({{.*}}, i1 true)
define void @volatile_read(ptr addrspace(2555904) %cb, ptr addrspace(10903552) %uav, i32 %n) {
entry:
  br label %loop

loop:
  %i = phi i32 [ 0, %entry ], [ %i.next, %loop ]
  %off = shl i32 %i, 2
  %v = call float @llvm.genx.GenISA.ldraw.indexed.f32.p2555904(ptr addrspace(2555904) %cb, i32 16, i32 4, i1 true)
  %bits = bitcast float %v to i32
  %old = call i32 @llvm.genx.GenISA.intatomictyped.i32.p10903552(ptr addrspace(10903552) %uav, i32 %off, i32 0, i32 undef, i32 %bits, i32 0)
  %i.next = add i32 %i, 1
  %done = icmp eq i32 %i.next, %n
  br i1 %done, label %exit, label %loop

exit:
  ret void
}

; CHECK-LABEL: @variant_offset(
; CHECK:       loop:
; CHECK:         %v = call float @llvm.genx.GenISA.ldraw.indexed.f32.p2555904(ptr addrspace(2555904) %cb, i32 %off,
define void @variant_offset(ptr addrspace(2555904) %cb, ptr addrspace(10903552) %uav, i32 %n) {
entry:
  br label %loop

loop:
  %i = phi i32 [ 0, %entry ], [ %i.next, %loop ]
  %off = shl i32 %i, 2
  %v = call float @llvm.genx.GenISA.ldraw.indexed.f32.p2555904(ptr addrspace(2555904) %cb, i32 %off, i32 4, i1 false)
  %bits = bitcast float %v to i32
  %old = call i32 @llvm.genx.GenISA.intatomictyped.i32.p10903552(ptr addrspace(10903552) %uav, i32 %off, i32 0, i32 undef, i32 %bits, i32 0)
  %i.next = add i32 %i, 1
  %done = icmp eq i32 %i.next, %n
  br i1 %done, label %exit, label %loop

exit:
  ret void
}

; COM: Inner loops go first, so a read invariant in both loops ends up in the
; COM: outer preheader.
; CHECK-LABEL: @nested(
; CHECK:       entry:
; CHECK-NEXT:    %v = call float @llvm.genx.GenISA.ldraw.indexed.f32.p2555904(
; CHECK-NEXT:    br label %outer
; CHECK:       outer:
; CHECK-NOT:     ldraw
; CHECK:       exit:
define void @nested(ptr addrspace(2555904) %cb, ptr addrspace(10903552) %uav, i32 %n, i32 %m) {
entry:
  br label %outer

outer:
  %j = phi i32 [ 0, %entry ], [ %j.next, %outer.latch ]
  br label %inner

inner:
  %i = phi i32 [ 0, %outer ], [ %i.next, %inner ]
  %off = shl i32 %i, 2
  %v = call float @llvm.genx.GenISA.ldraw.indexed.f32.p2555904(ptr addrspace(2555904) %cb, i32 16, i32 4, i1 false)
  %bits = bitcast float %v to i32
  %old = call i32 @llvm.genx.GenISA.intatomictyped.i32.p10903552(ptr addrspace(10903552) %uav, i32 %off, i32 0, i32 undef, i32 %bits, i32 0)
  %i.next = add i32 %i, 1
  %idone = icmp eq i32 %i.next, %m
  br i1 %idone, label %outer.latch, label %inner

outer.latch:
  %j.next = add i32 %j, 1
  %odone = icmp eq i32 %j.next, %n
  br i1 %odone, label %exit, label %outer

exit:
  ret void
}

declare float @llvm.genx.GenISA.ldraw.indexed.f32.p2555904(ptr addrspace(2555904), i32, i32, i1)
declare <4 x float> @llvm.genx.GenISA.ldrawvector.indexed.v4f32.p2555904(ptr addrspace(2555904), i32, i32, i1)
declare float @llvm.genx.GenISA.ldraw.indexed.f32.p131072(ptr addrspace(131072), i32, i32, i1)
declare void @llvm.genx.GenISA.storeraw.indexed.p131072.f32(ptr addrspace(131072), i32, float, i32, i1)
declare i32 @llvm.genx.GenISA.intatomictyped.i32.p10903552(ptr addrspace(10903552), i32, i32, i32, i32, i32)

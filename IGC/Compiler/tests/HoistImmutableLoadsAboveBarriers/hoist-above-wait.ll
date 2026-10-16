;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: igc_opt --opaque-pointers %s -S -igc-hoist-immutable-loads-above-barriers | FileCheck %s
; ------------------------------------------------
; HoistImmutableLoadsAboveBarriers
; ------------------------------------------------

; COM: The shape of AllMemoryBarrierWithGroupSync after lane 0 writes shared
; COM: memory: the join block opens with a fence and a barrier, then reads a
; COM: root constant. The read and the casts that form its address move
; COM: directly above the fence, not further up into the entry.

define void @join_after_divergent_branch(ptr addrspace(2) %root, i32 %lid, ptr addrspace(131072) %uav) {
; CHECK-LABEL: @join_after_divergent_branch(
; CHECK:       entry:
; CHECK-NEXT:    icmp eq i32 %lid, 0
; CHECK-NEXT:    br i1
; CHECK:       join:
; CHECK-NEXT:    [[I:%.*]] = ptrtoint ptr addrspace(2) %root to i64
; CHECK-NEXT:    [[P:%.*]] = inttoptr i64 [[I]] to ptr addrspace(2)
; CHECK-NEXT:    [[V:%.*]] = load i32, ptr addrspace(2) [[P]], align 4
; CHECK-NEXT:    call void @llvm.genx.GenISA.LSCFence(i32 3, i32 0, i32 0)
; CHECK-NEXT:    call void @llvm.genx.GenISA.threadgroupbarrier()
; CHECK-NEXT:    icmp eq i32 [[V]], 0
entry:
  %lane0 = icmp eq i32 %lid, 0
  br i1 %lane0, label %write, label %join

write:
  store i32 0, ptr addrspace(3) null, align 4
  br label %join

join:
  call void @llvm.genx.GenISA.LSCFence(i32 3, i32 0, i32 0)
  call void @llvm.genx.GenISA.threadgroupbarrier()
  %i = ptrtoint ptr addrspace(2) %root to i64
  %p = inttoptr i64 %i to ptr addrspace(2)
  %v = load i32, ptr addrspace(2) %p, align 4
  %z = icmp eq i32 %v, 0
  br i1 %z, label %exit, label %use

use:
  %f = bitcast i32 %v to float
  call void @llvm.genx.GenISA.storeraw.indexed.p131072.f32(ptr addrspace(131072) %uav, i32 0, float %f, i32 4, i1 false)
  br label %exit

exit:
  ret void
}

; COM: A constant-buffer ldraw moves the same way.

define float @ldraw_constant_buffer(ptr addrspace(65536) %cb, i32 %lid) {
; CHECK-LABEL: @ldraw_constant_buffer(
; CHECK:       join:
; CHECK-NEXT:    call float @llvm.genx.GenISA.ldraw.indexed.f32.p65536(
; CHECK-NEXT:    call void @llvm.genx.GenISA.threadgroupbarrier()
entry:
  %lane0 = icmp eq i32 %lid, 0
  br i1 %lane0, label %write, label %join

write:
  store i32 0, ptr addrspace(3) null, align 4
  br label %join

join:
  call void @llvm.genx.GenISA.threadgroupbarrier()
  %v = call float @llvm.genx.GenISA.ldraw.indexed.f32.p65536(ptr addrspace(65536) %cb, i32 16, i32 4, i1 false)
  ret float %v
}

; COM: With several waits in the block the read goes above the first one, so
; COM: it overlaps all of them.

define i32 @two_waits(ptr addrspace(2) %root) {
; CHECK-LABEL: @two_waits(
; CHECK:       entry:
; CHECK-NEXT:    [[V:%.*]] = load i32, ptr addrspace(2) %root
; CHECK-NEXT:    call void @llvm.genx.GenISA.threadgroupbarrier()
; CHECK-NEXT:    call void @llvm.genx.GenISA.threadgroupbarrier()
entry:
  call void @llvm.genx.GenISA.threadgroupbarrier()
  call void @llvm.genx.GenISA.threadgroupbarrier()
  %v = load i32, ptr addrspace(2) %root, align 4
  ret i32 %v
}

; COM: With two fence+barrier pairs and work between them, the read goes above
; COM: the pair right before it, not above the first pair.

define i32 @two_runs(ptr addrspace(2) %root, i32 %x) {
; CHECK-LABEL: @two_runs(
; CHECK:       entry:
; CHECK-NEXT:    call void @llvm.genx.GenISA.LSCFence(i32 3, i32 0, i32 0)
; CHECK-NEXT:    call void @llvm.genx.GenISA.threadgroupbarrier()
; CHECK-NEXT:    %y = add i32 %x, 1
; CHECK-NEXT:    [[V:%.*]] = load i32, ptr addrspace(2) %root
; CHECK-NEXT:    call void @llvm.genx.GenISA.LSCFence(i32 3, i32 0, i32 0)
; CHECK-NEXT:    call void @llvm.genx.GenISA.threadgroupbarrier()
entry:
  call void @llvm.genx.GenISA.LSCFence(i32 3, i32 0, i32 0)
  call void @llvm.genx.GenISA.threadgroupbarrier()
  %y = add i32 %x, 1
  call void @llvm.genx.GenISA.LSCFence(i32 3, i32 0, i32 0)
  call void @llvm.genx.GenISA.threadgroupbarrier()
  %v = load i32, ptr addrspace(2) %root, align 4
  %r = add i32 %v, %y
  ret i32 %r
}

declare void @llvm.genx.GenISA.LSCFence(i32, i32, i32)
declare void @llvm.genx.GenISA.threadgroupbarrier()
declare void @llvm.genx.GenISA.storeraw.indexed.p131072.f32(ptr addrspace(131072), i32, float, i32, i1)
declare float @llvm.genx.GenISA.ldraw.indexed.f32.p65536(ptr addrspace(65536), i32, i32, i1)

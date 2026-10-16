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

; COM: A read in a block that does not always run moves above the wait inside
; COM: its own block.

define i32 @conditional_block(ptr addrspace(2) %root, i1 %c) {
; CHECK-LABEL: @conditional_block(
; CHECK:       then:
; CHECK-NEXT:    [[V:%.*]] = load i32, ptr addrspace(2) %root
; CHECK-NEXT:    call void @llvm.genx.GenISA.threadgroupbarrier()
entry:
  br i1 %c, label %then, label %exit

then:
  call void @llvm.genx.GenISA.threadgroupbarrier()
  %v = load i32, ptr addrspace(2) %root, align 4
  %w = add i32 %v, 1
  br label %exit

exit:
  %r = phi i32 [ %w, %then ], [ 0, %entry ]
  ret i32 %r
}

; COM: A read inside a loop is not lifted out of it; that is LICM's job.

define i32 @loop_body(ptr addrspace(2) %root, i32 %n) {
; CHECK-LABEL: @loop_body(
; CHECK:       loop:
; CHECK:         %acc = phi
; CHECK-NEXT:    [[V:%.*]] = load i32, ptr addrspace(2) %root
; CHECK-NEXT:    call void @llvm.genx.GenISA.threadgroupbarrier()
entry:
  br label %loop

loop:
  %i = phi i32 [ 0, %entry ], [ %i.next, %loop ]
  %acc = phi i32 [ 0, %entry ], [ %acc.next, %loop ]
  call void @llvm.genx.GenISA.threadgroupbarrier()
  %v = load i32, ptr addrspace(2) %root, align 4
  %acc.next = add i32 %acc, %v
  %i.next = add i32 %i, 1
  %done = icmp eq i32 %i.next, %n
  br i1 %done, label %exit, label %loop

exit:
  ret i32 %acc.next
}

; COM: Writable memory, volatile reads, and reads with no wait before them
; COM: are left alone.

define float @uav_after_barrier(ptr addrspace(131072) %uav) {
; CHECK-LABEL: @uav_after_barrier(
; CHECK:         call void @llvm.genx.GenISA.threadgroupbarrier()
; CHECK-NEXT:    load float, ptr addrspace(131072) %uav
entry:
  call void @llvm.genx.GenISA.threadgroupbarrier()
  %v = load float, ptr addrspace(131072) %uav, align 4
  ret float %v
}

define i32 @volatile_after_barrier(ptr addrspace(2) %root) {
; CHECK-LABEL: @volatile_after_barrier(
; CHECK:         call void @llvm.genx.GenISA.threadgroupbarrier()
; CHECK-NEXT:    load volatile i32, ptr addrspace(2) %root
entry:
  call void @llvm.genx.GenISA.threadgroupbarrier()
  %v = load volatile i32, ptr addrspace(2) %root, align 4
  ret i32 %v
}

; COM: A read whose value is needed only far below the wait already overlaps
; COM: the work in between, so it stays.

define i32 @used_late(ptr addrspace(2) %root, i32 %x) {
; CHECK-LABEL: @used_late(
; CHECK:         call void @llvm.genx.GenISA.threadgroupbarrier()
; CHECK-NEXT:    load i32, ptr addrspace(2) %root
entry:
  call void @llvm.genx.GenISA.threadgroupbarrier()
  %v = load i32, ptr addrspace(2) %root, align 4
  %a1 = add i32 %x, 1
  %a2 = add i32 %a1, 2
  %a3 = add i32 %a2, 3
  %a4 = add i32 %a3, 4
  %a5 = add i32 %a4, 5
  %a6 = add i32 %a5, 6
  %a7 = add i32 %a6, 7
  %a8 = add i32 %a7, 8
  %a9 = add i32 %a8, 9
  %r = add i32 %a9, %v
  ret i32 %r
}

define i32 @no_wait(ptr addrspace(2) %root, i32 %x) {
; CHECK-LABEL: @no_wait(
; CHECK:         %y = add i32 %x, 1
; CHECK-NEXT:    load i32, ptr addrspace(2) %root
entry:
  %y = add i32 %x, 1
  %v = load i32, ptr addrspace(2) %root, align 4
  %r = add i32 %v, %y
  ret i32 %r
}

; COM: A wave barrier waits on nothing, so it is not a place to move a read
; COM: above, and it ends a run of waits instead of extending it.

define i32 @after_wavebarrier(ptr addrspace(2) %root, i32 %x) {
; CHECK-LABEL: @after_wavebarrier(
; CHECK:         call void @llvm.genx.GenISA.wavebarrier()
; CHECK-NEXT:    load i32, ptr addrspace(2) %root
entry:
  call void @llvm.genx.GenISA.wavebarrier()
  %v = load i32, ptr addrspace(2) %root, align 4
  %r = add i32 %v, %x
  ret i32 %r
}

define i32 @wavebarrier_ends_run(ptr addrspace(2) %root) {
; CHECK-LABEL: @wavebarrier_ends_run(
; CHECK:         call void @llvm.genx.GenISA.wavebarrier()
; CHECK-NEXT:    load i32, ptr addrspace(2) %root
; CHECK-NEXT:    call void @llvm.genx.GenISA.threadgroupbarrier()
entry:
  call void @llvm.genx.GenISA.threadgroupbarrier()
  call void @llvm.genx.GenISA.wavebarrier()
  call void @llvm.genx.GenISA.threadgroupbarrier()
  %v = load i32, ptr addrspace(2) %root, align 4
  %r = add i32 %v, 1
  ret i32 %r
}

declare void @llvm.genx.GenISA.LSCFence(i32, i32, i32)
declare void @llvm.genx.GenISA.threadgroupbarrier()
declare void @llvm.genx.GenISA.wavebarrier()
declare void @llvm.genx.GenISA.storeraw.indexed.p131072.f32(ptr addrspace(131072), i32, float, i32, i1)
declare float @llvm.genx.GenISA.ldraw.indexed.f32.p65536(ptr addrspace(65536), i32, i32, i1)

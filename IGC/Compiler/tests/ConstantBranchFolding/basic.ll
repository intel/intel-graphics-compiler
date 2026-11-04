;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
; RUN: igc_opt --opaque-pointers -igc-constant-branch-folding -S < %s | FileCheck %s
; ------------------------------------------------
; ConstantBranchFolding: a branch on a constant becomes unconditional and the
; blocks it made unreachable are deleted. Nothing else in the CFG changes: no
; block merging, no switch formation, no speculation into selects, and no
; instruction rewritten into `unreachable` (as removeUnreachableBlocks does).
; ------------------------------------------------

declare void @use(i32)
declare void @llvm.assume(i1)
declare void @noret() noreturn

; br i1 true: the false successor and its phi input go away.
define void @fold_true(i32 %x) {
; CHECK-LABEL: @fold_true(
; CHECK:         br label %then
; CHECK-NOT:     else:
; CHECK:       then:
; CHECK:       join:
; CHECK-NOT:     phi
; CHECK:         call void @use(i32 %a)
entry:
  br i1 true, label %then, label %else
then:
  %a = add i32 %x, 1
  br label %join
else:
  %b = mul i32 %x, 3
  br label %join
join:
  %p = phi i32 [ %a, %then ], [ %b, %else ]
  call void @use(i32 %p)
  ret void
}

; br i1 false into a chain of blocks reachable only through the dead edge: all of them go.
define void @fold_false_chain(i32 %x) {
; CHECK-LABEL: @fold_false_chain(
; CHECK:         br label %live
; CHECK-NOT:     dead1:
; CHECK-NOT:     dead2:
; CHECK:       live:
entry:
  br i1 false, label %dead1, label %live
dead1:
  call void @use(i32 1)
  br label %dead2
dead2:
  call void @use(i32 2)
  br label %live
live:
  call void @use(i32 %x)
  ret void
}

; Removing the dead edge collapses %c to true, which decides the second branch too.
define void @phi_becomes_constant(i32 %x) {
; CHECK-LABEL: @phi_becomes_constant(
; CHECK-NOT:     br i1
; CHECK-NOT:     no:
; CHECK:         call void @use(i32 10)
entry:
  br i1 true, label %a, label %b
a:
  br label %merge
b:
  br label %merge
merge:
  %c = phi i1 [ true, %a ], [ false, %b ]
  br i1 %c, label %yes, label %no
yes:
  call void @use(i32 10)
  ret void
no:
  call void @use(i32 20)
  ret void
}

; A switch on a constant goes straight to its case.
define void @switch_on_constant() {
; CHECK-LABEL: @switch_on_constant(
; CHECK-NOT:     switch
; CHECK:         br label %two
; CHECK-NOT:     one:
; CHECK-NOT:     other:
entry:
  switch i32 2, label %other [
    i32 1, label %one
    i32 2, label %two
  ]
one:
  call void @use(i32 1)
  ret void
two:
  call void @use(i32 2)
  ret void
other:
  call void @use(i32 0)
  ret void
}

; Nothing constant: an empty forwarding block and a block with a single predecessor stay, the diamond is not turned
; into a select.
define void @non_constant_untouched(i1 %c, i32 %x) {
; CHECK-LABEL: @non_constant_untouched(
; CHECK:         br i1 %c, label %fwd, label %side
; CHECK:       fwd:
; CHECK-NEXT:    br label %join
; CHECK:       side:
; CHECK:       join:
; CHECK-NEXT:    %p = phi i32
; CHECK-NOT:     select
entry:
  br i1 %c, label %fwd, label %side
fwd:
  br label %join
side:
  %y = add i32 %x, 1
  br label %join
join:
  %p = phi i32 [ 0, %fwd ], [ %y, %side ]
  call void @use(i32 %p)
  ret void
}

; A lowered switch (compare chain) is not re-formed into a switch, unlike with SimplifyCFG.
define void @compare_chain_stays(i32 %x) {
; CHECK-LABEL: @compare_chain_stays(
; CHECK-NOT:     switch
; CHECK:         icmp eq i32 %x, 1
; CHECK:         icmp eq i32 %x, 2
entry:
  %is1 = icmp eq i32 %x, 1
  br i1 %is1, label %one, label %leaf
leaf:
  %is2 = icmp eq i32 %x, 2
  br i1 %is2, label %two, label %other
one:
  call void @use(i32 1)
  ret void
two:
  call void @use(i32 2)
  ret void
other:
  call void @use(i32 0)
  ret void
}

; llvm::removeUnreachableBlocks (markAliveBlocks) would turn the stores to null and
; undef, the false assume and the code after the noreturn call into `unreachable`,
; which codegen does not handle this late. Only the branch is folded here.
define void @no_unreachable_rewrites(i32 %x) {
; CHECK-LABEL: @no_unreachable_rewrites(
; CHECK-NOT:     unreachable
; CHECK:         br label %live
; CHECK:       live:
; CHECK-NEXT:    store i32 %x, ptr null
; CHECK-NEXT:    br label %b1
; CHECK:       b1:
; CHECK-NEXT:    store i32 %x, ptr undef
; CHECK-NEXT:    br label %b2
; CHECK:       b2:
; CHECK-NEXT:    call void @llvm.assume(i1 false)
; CHECK-NEXT:    br label %b3
; CHECK:       b3:
; CHECK-NEXT:    call void @noret()
; CHECK-NEXT:    ret void
; CHECK-NOT:     unreachable
; CHECK:       }
entry:
  br i1 true, label %live, label %dead
dead:
  call void @use(i32 0)
  ret void
live:
  store i32 %x, ptr null
  br label %b1
b1:
  store i32 %x, ptr undef
  br label %b2
b2:
  call void @llvm.assume(i1 false)
  br label %b3
b3:
  call void @noret()
  ret void
}

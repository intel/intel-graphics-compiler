;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
; REQUIRES: regkeys
; RUN: igc_opt --opaque-pointers -igc-constant-branch-folding -S < %s | FileCheck %s --check-prefix=NOMERGE
; RUN: igc_opt --opaque-pointers --regkey ConstantBranchFoldingMask=7 -igc-constant-branch-folding -S < %s \
; RUN:   | FileCheck %s --check-prefix=MERGE
; ------------------------------------------------
; ConstantBranchFolding with ConstantBranchFoldingMask bit 2 (a behaviour bit):
; after folding, each block is merged into its single predecessor when that
; predecessor branches only to it. Without the bit the chain is left as is.
; Critical-edge splitters and loop headers are never merged.
; ------------------------------------------------

declare void @use(i32)

; The chain the fold leaves behind: A -> B -> D collapses into one block.
define void @fold_then_merge(i32 %y, ptr %addr) {
; NOMERGE-LABEL: @fold_then_merge(
; NOMERGE:         br label %B
; NOMERGE:       B:
; NOMERGE-NEXT:    br label %D
; NOMERGE:       D:
; NOMERGE-NEXT:    store i32 %y, ptr null
;
; MERGE-LABEL: @fold_then_merge(
; MERGE-NEXT:  A:
; MERGE-NEXT:    store i32 %y, ptr null
; MERGE-NEXT:    ret void
A:
  br i1 true, label %B, label %C
B:
  br label %D
C:
  br label %D
D:
  %x = phi ptr [ null, %B ], [ %addr, %C ]
  store i32 %y, ptr %x
  ret void
}

; A chain that existed before the fold is merged too: the bit applies to the whole function.
define void @existing_chain(i32 %y) {
; NOMERGE-LABEL: @existing_chain(
; NOMERGE:       next:
;
; MERGE-LABEL: @existing_chain(
; MERGE-NEXT:  entry:
; MERGE-NEXT:    call void @use(i32 %y)
; MERGE-NEXT:    call void @use(i32 1)
; MERGE-NEXT:    ret void
entry:
  call void @use(i32 %y)
  br label %next
next:
  call void @use(i32 1)
  ret void
}

; The splitter of the critical edge entry -> join stays: its predecessor has two successors.
define void @critical_edge_splitter(i1 %c, i32 %y) {
; MERGE-LABEL: @critical_edge_splitter(
; MERGE:         br i1 %c, label %split, label %side
; MERGE:       split:
; MERGE-NEXT:    br label %join
; MERGE:       join:
; MERGE-NEXT:    phi i32 [ 0, %split ], [ %y, %side ]
entry:
  br i1 %c, label %split, label %side
split:
  br label %join
side:
  br i1 %c, label %join, label %exit
join:
  %p = phi i32 [ 0, %split ], [ %y, %side ]
  call void @use(i32 %p)
  br label %exit
exit:
  ret void
}

; The loop header has two predecessors (preheader and latch), so it is not merged into the preheader.
define void @loop_header_kept(i32 %n) {
; MERGE-LABEL: @loop_header_kept(
; MERGE:       header:
; MERGE-NEXT:    %i = phi i32 [ 0, %entry ], [ %inc, %header ]
; MERGE:         br i1 %done, label %exit, label %header
entry:
  br label %header
header:
  %i = phi i32 [ 0, %entry ], [ %inc, %header ]
  call void @use(i32 %i)
  %inc = add i32 %i, 1
  %done = icmp eq i32 %inc, %n
  br i1 %done, label %exit, label %header
exit:
  ret void
}

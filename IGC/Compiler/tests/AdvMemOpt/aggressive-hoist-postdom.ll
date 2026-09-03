;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: llvm-14-plus, regkeys
;
; RUN: igc_opt --opaque-pointers -igc-advmemopt -S \
; RUN:   --regkey=AdvMemOptAggressiveHoist=0 < %s | FileCheck %s --check-prefix=DEFAULT
;
; RUN: igc_opt --opaque-pointers -igc-advmemopt -S \
; RUN:   --regkey=AdvMemOptAggressiveHoist=1 < %s | FileCheck %s --check-prefix=HOIST

; Test: AdvMemOpt's Line-building walk over a loop containing a diamond.
;
; The default walk follows direct successors, requiring each to be dominated by
; and post-dominate the current block. Neither %then nor %else post-dominates
; %loop, so the walk stops at the top of the diamond and never considers %l1.
;
; AdvMemOptAggressiveHoist steps to the immediate post-dominator instead - %loop
; straight to %join - so %l1 joins %l0's Line and hoists into the header.

define spir_kernel void @diamond(i32 %a, ptr addrspace(1) %b, ptr addrspace(1) %b2) {
entry:
  %c0 = icmp slt i32 %a, 13
  br i1 %c0, label %loop, label %end

loop:
  %iv = phi i32 [ 0, %entry ], [ %ivnext, %join ]
  %l0 = load i32, ptr addrspace(1) %b, align 4
  %cd = icmp slt i32 %iv, 5
  br i1 %cd, label %then, label %else

then:
  br label %join

else:
  br label %join

join:
  %l1 = load i32, ptr addrspace(1) %b2, align 4
  %s = add i32 %l0, %l1
  %ivnext = add i32 %iv, %s
  %cc = icmp slt i32 %ivnext, %a
  br i1 %cc, label %loop, label %end

end:
  %r = phi i32 [ %a, %entry ], [ %ivnext, %join ]
  store i32 %r, ptr addrspace(1) %b, align 4
  ret void
}

; The walk dies at the diamond: %l1 stays where it was.
;
; DEFAULT-LABEL: loop:
; DEFAULT-NEXT:    %iv = phi i32 [ 0, %entry ], [ %ivnext, %join ]
; DEFAULT-NEXT:    %l0 = load i32, ptr addrspace(1) %b, align 4
; DEFAULT-NEXT:    %cd = icmp slt i32 %iv, 5
; DEFAULT-NEXT:    br i1 %cd, label %then, label %else
;
; DEFAULT-LABEL: join:
; DEFAULT-NEXT:    %l1 = load i32, ptr addrspace(1) %b2, align 4
; DEFAULT-NEXT:    %s = add i32 %l0, %l1

; Following the immediate post-dominator hops over the diamond and %l1 lands
; next to %l0 in the header.
;
; HOIST-LABEL: loop:
; HOIST-NEXT:    %iv = phi i32 [ 0, %entry ], [ %ivnext, %join ]
; HOIST-NEXT:    %l0 = load i32, ptr addrspace(1) %b, align 4
; HOIST-NEXT:    %cd = icmp slt i32 %iv, 5
; HOIST-NEXT:    %l1 = load i32, ptr addrspace(1) %b2, align 4
; HOIST-NEXT:    br i1 %cd, label %then, label %else
;
; HOIST-LABEL: join:
; HOIST-NEXT:    %s = add i32 %l0, %l1

!igc.functions = !{!0}

!0 = !{ptr @diamond, !1}
!1 = !{!2}
!2 = !{!"function_type", i32 0}

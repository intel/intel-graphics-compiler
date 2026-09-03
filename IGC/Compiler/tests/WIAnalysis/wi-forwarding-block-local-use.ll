;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: llvm-14-plus, regkeys
;
; RUN: igc_opt --opaque-pointers -S -igc-propagate-cmp-uniformity -print-wia-check --disable-output \
; RUN:   --regkey=PrintToConsole=1 --regkey=EnableWIForwardingBlockLocalUse=0 < %s 2>&1 \
; RUN:   | FileCheck %s --check-prefix=NOFWD
;
; RUN: igc_opt --opaque-pointers -S -igc-propagate-cmp-uniformity -print-wia-check --disable-output \
; RUN:   --regkey=PrintToConsole=1 --regkey=EnableWIForwardingBlockLocalUse=1 < %s 2>&1 \
; RUN:   | FileCheck %s --check-prefix=FWD

; Test: a uniform def inside a divergent branch's influence region, used from a
; block outside that region but reachable only through single-predecessor edges.
;
; %divcond comes from LocalID_X, so the branch in %header is divergent and its
; influence region covers %body/%t/%f/%m (full join %latch). %p is a phi, so
; isRegionInvariant() bails and update_cf_dep() inspects its uses. Its only
; out-of-block use is %use in %earlyexit, outside the region - which without
; the forwarding check demotes %p to RANDOM. But %earlyexit's unique predecessor
; is %m, so entering it implies %m's lane mask: the LCSSA-dedicated-exit shape.

@ThreadGroupSize_X = constant i32 64
@ThreadGroupSize_Y = constant i32 1
@ThreadGroupSize_Z = constant i32 1

define spir_kernel void @fwd(i32 %K1, i32 %K2, ptr addrspace(1) %out) {
entry:
  %LocalID_X = call i32 @llvm.genx.GenISA.DCL.SystemValue.i32(i32 17)
  br label %header

header:
  %iv = phi i32 [ 0, %entry ], [ %ivnext, %latch ]
  %divcond = icmp slt i32 %LocalID_X, 8
  br i1 %divcond, label %body, label %latch

body:
  %sel = icmp sgt i32 %iv, %K1
  br i1 %sel, label %t, label %f

t:
  br label %m

f:
  br label %m

; NOFWD:      random   %p = phi i32 [ %K1, %t ], [ %K2, %f ]
; NOFWD-NEXT: random   %ec = icmp sgt i32 %p, 100
;
; FWD:      uniform_global  %p = phi i32 [ %K1, %t ], [ %K2, %f ]
; FWD-NEXT: uniform_global  %ec = icmp sgt i32 %p, 100
m:
  %p = phi i32 [ %K1, %t ], [ %K2, %f ]
  %ec = icmp sgt i32 %p, 100
  br i1 %ec, label %earlyexit, label %latch

latch:
  %ivnext = add i32 %iv, 1
  %lc = icmp slt i32 %ivnext, %K2
  br i1 %lc, label %header, label %done

; NOFWD:      random   %use = add i32 %p, 7
; FWD:        uniform_global  %use = add i32 %p, 7
earlyexit:
  %use = add i32 %p, 7
  store i32 %use, ptr addrspace(1) %out, align 4
  br label %done

done:
  ret void
}

declare i32 @llvm.genx.GenISA.DCL.SystemValue.i32(i32) #0

attributes #0 = { nounwind readnone }

!IGCMetadata = !{!0}
!igc.functions = !{!1}

!0 = !{!"ModuleMD", !2}
!1 = !{ptr @fwd, !3}
!2 = !{!"FuncMD", !4, !5}
!3 = !{!6}
!4 = !{!"FuncMDMap[0]", ptr @fwd}
!5 = !{!"FuncMDValue[0]", !7, !8, !9, !10}
!6 = !{!"function_type", i32 0}
!7 = !{!"localOffsets"}
!8 = !{!"workGroupWalkOrder", !11, !12, !13}
!9 = !{!"funcArgs"}
!10 = !{!"functionType", !"KernelFunction"}
!11 = !{!"dim0", i32 0}
!12 = !{!"dim1", i32 1}
!13 = !{!"dim2", i32 2}

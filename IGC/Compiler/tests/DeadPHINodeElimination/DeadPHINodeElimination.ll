;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2021 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: igc_opt -igc-phielimination -S %s -o %t.ll
; RUN: FileCheck %s --input-file=%t.ll

target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024-n8:16:32"
target triple = "spir64-unknown-unknown"

; Function Attrs: nounwind readnone speculatable willreturn
declare void @llvm.dbg.value(metadata, metadata, metadata)

; Function Attrs: alwaysinline convergent nounwind
define spir_kernel void @igcphieliminationtest0(float addrspace(3)* %arg) {
; 2 duplicate phi node loops made of ai and xi
entry:
  %index = getelementptr inbounds float, float addrspace(3)* %arg, i64 0
  br i1 1, label %outer.loop.preheader, label %outer.loop.cleanup

outer.loop.preheader:                             ; preds = %entry
  br label %outer.loop

outer.loop:                                       ; preds = %outer.loop.preheader, %if.end2
  %i = phi i32 [ 0, %outer.loop.preheader ], [ %ii, %if.end2 ]
  %a0 = phi float [ 0.0, %outer.loop.preheader ], [ %a4, %if.end2 ]
  %x0 = phi float [ 0.0, %outer.loop.preheader ], [ %x4, %if.end2 ]
  br i1 0, label %if.then0, label %if.end2

if.then0:                                         ; preds = %outer.loop
  br i1 1, label %inner.loop.preheader, label %inner.loop.cleanup

inner.loop.preheader:                             ; preds = %if.then0
  br label %inner.loop

inner.loop:                                       ; preds = %inner.loop.preheader, %if.end1
  %j = phi i32 [ 0, %inner.loop.preheader ], [ %jj, %if.end1 ]
  %a2 = phi float [ %a0, %inner.loop.preheader ], [ %a3, %if.end1 ]
  %x2 = phi float [ %x0, %inner.loop.preheader ], [ %x3, %if.end1 ]
  br label %if.end0

if.end0:                                          ; preds = %inner.loop
  br i1 0, label %if.then1, label %if.end1

if.then1:                                         ; preds = %if.end0
  br i1 1, label %if.then2, label %if.end1

if.then2:                                         ; preds = %if.then1
  %t = fadd float %a2, 1.0
  br label %if.end1

if.end1:                                          ; preds = %if.then1, %if.end0, %if.then2
  %a3 = phi float [ %t, %if.then2 ], [ %a2, %if.end0 ], [ %a2, %if.then1 ]
  %x3 = phi float [ %t, %if.then2 ], [ %x2, %if.end0 ], [ %x2, %if.then1 ]
  %jj = add nuw nsw i32 %j, 1
  %inner.cmp = icmp slt i32 %jj, 16
  br i1 %inner.cmp, label %inner.loop, label %inner.loop.cleanup

inner.loop.cleanup:                               ; preds = %if.end1, %if.then0
  %a1 = phi float [ %a0, %if.then0 ], [ %a3, %if.end1 ]
  %x1 = phi float [ %x0, %if.then0 ], [ %x3, %if.end1 ]
  br label %if.end2

if.end2:                                          ; preds = %outer.loop, %inner.loop.cleanup
  %a4 = phi float [ %a1, %inner.loop.cleanup ], [ %a0, %outer.loop ]
  %x4 = phi float [ %x1, %inner.loop.cleanup ], [ %x0, %outer.loop ]
  %ii = add nuw nsw i32 %i, 1
  %outer.cmp = icmp slt i32 %ii, 16
  br i1 %outer.cmp, label %outer.loop, label %outer.loop.cleanup

outer.loop.cleanup:                               ; preds = %if.end2, %entry
  %x5 = phi float [ 0.0, %entry ], [ %x4, %if.end2 ]
  store float %x5, float addrspace(3)* %index, align 4
  br label %exit

exit:                                             ; preds = %outer.loop.cleanup
  ret void
}

!1 = !{}

; CHECK-NOT: %a0
; CHECK: %x0 = phi float

; CHECK-NOT: %a2
; CHECK: %x2 = phi float

; CHECK: %t = fadd float %x2

; CHECK-NOT: %a3
; CHECK: %x3 = phi float

; CHECK-NOT: %a1
; CHECK: %x1 = phi float

; CHECK-NOT: %a4
; CHECK: %x4 = phi float

; CHECK: %x5 = phi float

; Function Attrs: alwaysinline convergent nounwind
define spir_kernel void @igcphieliminationtest1(float addrspace(3)* %arg) {
; 4 duplicate phi node loops made of ai, bi, xi, and yi
entry:
  %index = getelementptr inbounds float, float addrspace(3)* %arg, i64 0
  %s = load float, float addrspace(3)* %index, align 4
  br i1 1, label %outer.loop.preheader, label %outer.loop.cleanup

outer.loop.preheader:                             ; preds = %entry
  br label %outer.loop

outer.loop:                                       ; preds = %outer.loop.preheader, %if.end2
  %i = phi i32 [ 0, %outer.loop.preheader ], [ %ii, %if.end2 ]
  %a0 = phi float [ %s, %outer.loop.preheader ], [ %a4, %if.end2 ]
  %b0 = phi float [ %s, %outer.loop.preheader ], [ %b4, %if.end2 ]
  %x0 = phi float [ %s, %outer.loop.preheader ], [ %x4, %if.end2 ]
  %y0 = phi float [ %s, %outer.loop.preheader ], [ %y4, %if.end2 ]
  br i1 0, label %if.then0, label %if.end2

if.then0:                                         ; preds = %outer.loop
  br i1 1, label %inner.loop.preheader, label %inner.loop.cleanup

inner.loop.preheader:                             ; preds = %if.then0
  br label %inner.loop

inner.loop:                                       ; preds = %inner.loop.preheader, %if.end1
  %j = phi i32 [ 0, %inner.loop.preheader ], [ %jj, %if.end1 ]
  %a2 = phi float [ %a0, %inner.loop.preheader ], [ %a3, %if.end1 ]
  %b2 = phi float [ %b0, %inner.loop.preheader ], [ %b3, %if.end1 ]
  %x2 = phi float [ %x0, %inner.loop.preheader ], [ %x3, %if.end1 ]
  %y2 = phi float [ %y0, %inner.loop.preheader ], [ %y3, %if.end1 ]
  br label %if.end0

if.end0:                                          ; preds = %inner.loop
  br i1 0, label %if.then1, label %if.end1

if.then1:                                         ; preds = %if.end0
  br i1 1, label %if.then2, label %if.end1

if.then2:                                         ; preds = %if.then1
  %t = fadd float %a2, 1.0
  br label %if.end1

if.end1:                                          ; preds = %if.then1, %if.end0, %if.then2
  %a3 = phi float [ %t, %if.then2 ], [ %a2, %if.end0 ], [ %a2, %if.then1 ]
  %b3 = phi float [ %t, %if.then2 ], [ %b2, %if.end0 ], [ %b2, %if.then1 ]
  %x3 = phi float [ %t, %if.then2 ], [ %x2, %if.end0 ], [ %x2, %if.then1 ]
  %y3 = phi float [ %t, %if.then2 ], [ %y2, %if.end0 ], [ %y2, %if.then1 ]
  %jj = add nuw nsw i32 %j, 1
  %inner.cmp = icmp slt i32 %jj, 16
  br i1 %inner.cmp, label %inner.loop, label %inner.loop.cleanup

inner.loop.cleanup:                               ; preds = %if.end1, %if.then0
  %a1 = phi float [ %a0, %if.then0 ], [ %a3, %if.end1 ]
  %b1 = phi float [ %b0, %if.then0 ], [ %b3, %if.end1 ]
  %x1 = phi float [ %x0, %if.then0 ], [ %x3, %if.end1 ]
  %y1 = phi float [ %y0, %if.then0 ], [ %y3, %if.end1 ]
  br label %if.end2

if.end2:                                          ; preds = %outer.loop, %inner.loop.cleanup
  %a4 = phi float [ %a1, %inner.loop.cleanup ], [ %a0, %outer.loop ]
  %b4 = phi float [ %b1, %inner.loop.cleanup ], [ %b0, %outer.loop ]
  %x4 = phi float [ %x1, %inner.loop.cleanup ], [ %x0, %outer.loop ]
  %y4 = phi float [ %y1, %inner.loop.cleanup ], [ %y0, %outer.loop ]
  %ii = add nuw nsw i32 %i, 1
  %outer.cmp = icmp slt i32 %ii, 16
  br i1 %outer.cmp, label %outer.loop, label %outer.loop.cleanup

outer.loop.cleanup:                               ; preds = %if.end2, %entry
  %x5 = phi float [ %s, %entry ], [ %x4, %if.end2 ]
  %y5 = phi float [ %s, %entry ], [ %y4, %if.end2 ]
  store float %x5, float addrspace(3)* %index, align 4
  br label %exit

exit:                                             ; preds = %outer.loop.cleanup
  ret void
}

; CHECK-NOT: %a0
; CHECK-NOT: %b0
; CHECK: %x0 = phi float
; CHECK-NOT: %y0

; CHECK-NOT: %a2
; CHECK-NOT: %b2
; CHECK: %x2 = phi float
; CHECK-NOT: %y3

; CHECK: %t = fadd float %x2

; CHECK-NOT: %a3
; CHECK-NOT: %b3
; CHECK: %x3 = phi float
; CHECK-NOT: %y3

; CHECK-NOT: %a1
; CHECK-NOT: %b1
; CHECK: %x1 = phi float
; CHECK-NOT: %y1

; CHECK-NOT: %a4
; CHECK-NOT: %b4
; CHECK: %x4 = phi float
; CHECK-NOT: %y4

; CHECK: %x5 = phi float
; CHECK-NOT: %y5

; Function Attrs: alwaysinline convergent nounwind
define spir_kernel void @igcphieliminationtest2(float addrspace(3)* %arg) {
; 2 duplicate phi node loops made of ai and xi (outer1-inner1), bi and yi (outer0-inner0)
entry:
  %index = getelementptr inbounds float, float addrspace(3)* %arg, i64 0
  br i1 1, label %outer1.loop.preheader, label %outer1.loop.cleanup

outer1.loop.preheader:                             ; preds = %entry
  br label %outer1.loop

outer1.loop:                                       ; preds = %outer1.loop.preheader, %if1.end2
  %i = phi i32 [ 0, %outer1.loop.preheader ], [ %ii, %if1.end2 ]
  %a0 = phi float [ 0.0, %outer1.loop.preheader ], [ %a4, %if1.end2 ]
  %x0 = phi float [ 0.0, %outer1.loop.preheader ], [ %x4, %if1.end2 ]
  br i1 0, label %if1.then0, label %if1.end2

if1.then0:                                         ; preds = %outer1.loop
  br i1 1, label %inner1.loop.preheader, label %inner1.loop.cleanup

inner1.loop.preheader:                             ; preds = %if1.then0
  br label %inner1.loop

inner1.loop:                                       ; preds = %inner1.loop.preheader, %if1.end1
  %j = phi i32 [ 0, %inner1.loop.preheader ], [ %jj, %if1.end1 ]
  %a2 = phi float [ %a0, %inner1.loop.preheader ], [ %a3, %if1.end1 ]
  %x2 = phi float [ %x0, %inner1.loop.preheader ], [ %x3, %if1.end1 ]
  br i1 1, label %outer0.loop.preheader, label %outer0.loop.cleanup

outer0.loop.preheader:                             ; preds = %entry
  br label %outer0.loop

outer0.loop:                                       ; preds = %outer0.loop.preheader, %if0.end2
  %k = phi i32 [ 0, %outer0.loop.preheader ], [ %kk, %if0.end2 ]
  %b0 = phi float [ 0.0, %outer0.loop.preheader ], [ %b4, %if0.end2 ]
  %y0 = phi float [ 0.0, %outer0.loop.preheader ], [ %y4, %if0.end2 ]
  br i1 0, label %if0.then0, label %if0.end2

if0.then0:                                         ; preds = %outer0.loop
  br i1 1, label %inner0.loop.preheader, label %inner0.loop.cleanup

inner0.loop.preheader:                             ; preds = %if0.then0
  br label %inner0.loop

inner0.loop:                                       ; preds = %inner0.loop.preheader, %if0.end1
  %l = phi i32 [ 0, %inner0.loop.preheader ], [ %ll, %if0.end1 ]
  %b2 = phi float [ %b0, %inner0.loop.preheader ], [ %b3, %if0.end1 ]
  %y2 = phi float [ %y0, %inner0.loop.preheader ], [ %y3, %if0.end1 ]
  br label %if0.end0

if0.end0:                                          ; preds = %inner0.loop
  br i1 0, label %if0.then1, label %if0.end1

if0.then1:                                         ; preds = %if0.end0
  br i1 1, label %if0.then2, label %if0.end1

if0.then2:                                         ; preds = %if0.then1
  %t = fadd float %b2, 1.0
  br label %if0.end1

if0.end1:                                          ; preds = %if0.then1, %if0.end0, %if0.then2
  %b3 = phi float [ %t, %if0.then2 ], [ %b2, %if0.end0 ], [ %b2, %if0.then1 ]
  %y3 = phi float [ %t, %if0.then2 ], [ %y2, %if0.end0 ], [ %y2, %if0.then1 ]
  %ll = add nuw nsw i32 %l, 1
  %inner0.cmp = icmp slt i32 %ll, 16
  br i1 %inner0.cmp, label %inner0.loop, label %inner0.loop.cleanup

inner0.loop.cleanup:                               ; preds = %if0.end1, %if0.then0
  %b1 = phi float [ %b0, %if0.then0 ], [ %b3, %if0.end1 ]
  %y1 = phi float [ %y0, %if0.then0 ], [ %y3, %if0.end1 ]
  br label %if0.end2

if0.end2:                                          ; preds = %outer0.loop, %inner0.loop.cleanup
  %b4 = phi float [ %b1, %inner0.loop.cleanup ], [ %b0, %outer0.loop ]
  %y4 = phi float [ %y1, %inner0.loop.cleanup ], [ %y0, %outer0.loop ]
  %kk = add nuw nsw i32 %k, 1
  %outer0.cmp = icmp slt i32 %kk, 16
  br i1 %outer0.cmp, label %outer0.loop, label %outer0.loop.cleanup

outer0.loop.cleanup:                               ; preds = %if0.end2, %entry
  %y5 = phi float [ %x0, %inner1.loop], [ %y4, %if0.end2 ]
  br label %if1.end0

if1.end0:                                          ; preds = %inner1.loop
  br i1 0, label %if1.then1, label %if1.end1

if1.then1:                                         ; preds = %if1.end0
  br i1 1, label %if1.then2, label %if1.end1

if1.then2:                                         ; preds = %if1.then1
  %tt = fadd float %a2, %y5
  br label %if1.end1

if1.end1:                                          ; preds = %if1.then1, %if1.end0, %if1.then2
  %a3 = phi float [ %tt, %if1.then2 ], [ %a2, %if1.end0 ], [ %a2, %if1.then1 ]
  %x3 = phi float [ %tt, %if1.then2 ], [ %x2, %if1.end0 ], [ %x2, %if1.then1 ]
  %jj = add nuw nsw i32 %j, 1
  %inner1.cmp = icmp slt i32 %jj, 16
  br i1 %inner1.cmp, label %inner1.loop, label %inner1.loop.cleanup

inner1.loop.cleanup:                               ; preds = %if1.end1, %if1.then0
  %a1 = phi float [ %a0, %if1.then0 ], [ %a3, %if1.end1 ]
  %x1 = phi float [ %x0, %if1.then0 ], [ %x3, %if1.end1 ]
  br label %if1.end2

if1.end2:                                          ; preds = %outer1.loop, %inner1.loop.cleanup
  %a4 = phi float [ %a1, %inner1.loop.cleanup ], [ %a0, %outer1.loop ]
  %x4 = phi float [ %x1, %inner1.loop.cleanup ], [ %x0, %outer1.loop ]
  %ii = add nuw nsw i32 %i, 1
  %outer1.cmp = icmp slt i32 %ii, 16
  br i1 %outer1.cmp, label %outer1.loop, label %outer1.loop.cleanup

outer1.loop.cleanup:                               ; preds = %if1.end2, %entry
  %x5 = phi float [ 0.0, %entry ], [ %x4, %if1.end2 ]
  store float %x5, float addrspace(3)* %index, align 4
  br label %exit

exit:                                             ; preds = %outer1.loop.cleanup
  ret void
}

; Outer

; CHECK-NOT: %a0
; CHECK: %x0 = phi float

; CHECK-NOT: %a2
; CHECK: %x2 = phi float


; Inner

; CHECK-NOT: %b0
; CHECK: %y0 = phi float

; CHECK-NOT: %b2
; CHECK: %y2 = phi float

; CHECK: %t = fadd float %y2

; CHECK-NOT: %b3
; CHECK: %y3 = phi float

; CHECK-NOT: %b1
; CHECK: %y1 = phi float

; CHECK-NOT: %b4
; CHECK: %y4 = phi float

; CHECK: %y5 = phi float


; Outer

; CHECK: %tt = fadd float %x2, %y5

; CHECK-NOT: %a3
; CHECK: %x3 = phi float

; CHECK-NOT: %a1
; CHECK: %x1 = phi float

; CHECK-NOT: %a4
; CHECK: %x4 = phi float

; CHECK: %x5 = phi float

; Function Attrs: alwaysinline convergent nounwind
define spir_kernel void @igcphieliminationtest3(float addrspace(3)* %arg) {
; 2 duplicate phi node loops made of ai and xi within one basic block
entry:
  %index = getelementptr inbounds float, float addrspace(3)* %arg, i64 0
  br label %outer.loop

outer.loop:                                       ; preds = %outer.loop.preheader, %if.end2
  %i = phi i32 [ 0, %entry ], [ %ii, %outer.loop ]
  %a0 = phi float [ 0.0, %entry ], [ %tt, %outer.loop ]
  %x0 = phi float [ 0.0, %entry ], [ %tt, %outer.loop ]
  %tt = fadd float %a0, 1.0
  %ii = add nuw nsw i32 %i, 1
  %outer.cmp = icmp slt i32 %ii, 16
  br i1 %outer.cmp, label %outer.loop, label %outer.loop.cleanup

outer.loop.cleanup:                               ; preds = %if.end2, %entry
  store float %x0, float addrspace(3)* %index, align 4
  br label %exit

exit:                                             ; preds = %outer.loop.cleanup
  ret void
}

; CHECK-NOT: %a0
; CHECK: %x0 = phi float
; CHECK: %tt = fadd float %x0

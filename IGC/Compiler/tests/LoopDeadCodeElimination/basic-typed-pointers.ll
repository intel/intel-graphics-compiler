;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022-2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; UNSUPPORTED: llvm-17-plus
; RUN: igc_opt --typed-pointers -igc-loop-dce -dce -S < %s | FileCheck %s
; ------------------------------------------------
; LoopDeadCodeElimination
; ------------------------------------------------


define spir_kernel void @test_loopdce(i32 %a, i32 %b, i32* %c) {
; CHECK-LABEL: @test_loopdce(
; CHECK-NEXT:  entry:
; CHECK-NEXT:    [[TMP0:%.*]] = add i32 %a, %b
; CHECK-NEXT:    br label [[BB1:%.*]]
; CHECK:       bb1:
; CHECK-NEXT:    [[TMP1:%.*]] = phi i32 [ %a, [[ENTRY:%.*]] ], [ [[TMP3:%.*]], [[BB1]] ]
; CHECK-NEXT:    [[TMP2:%.*]] = icmp sgt i32 [[TMP1]], 13
; CHECK-NEXT:    [[TMP3]] = add i32 [[TMP1]], 13
; CHECK-NEXT:    br i1 [[TMP2]], label [[BB1]], label [[END:%.*]]
; CHECK:       end:
; CHECK-NEXT:    [[TMP4:%.*]] = add i32 [[TMP0]], %b
; CHECK-NEXT:    store i32 [[TMP4]], i32* [[C:%.*]], align 4
; CHECK-NEXT:    ret void
;
entry:
  %0 = add i32 %a, %b
  br label %bb1

bb1:
  %1 = phi i32 [ %a, %entry ], [ %4, %bb1 ]
  %2 = icmp sgt i32 %1, 13
  %3 = add i32 %1, 13
  %4 = select i1 %2, i32 %3, i32 %0
  br i1 %2, label %bb1, label %end

end:
  %5 = add i32 %4, %b
  store i32 %5, i32* %c, align 4
  ret void
}

!igc.functions = !{!0}

!0 = !{void (i32, i32, i32*)* @test_loopdce, !1}
!1 = !{!2}
!2 = !{!"function_type", i32 0}

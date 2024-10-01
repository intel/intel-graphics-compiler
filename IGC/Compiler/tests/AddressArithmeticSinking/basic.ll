;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2023-2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; REQUIRES: llvm-14-plus
; RUN: igc_opt --opaque-pointers -igc-address-arith-sinking  -S < %s | FileCheck %s
; ------------------------------------------------
; AddressArithmeticSinking
; ------------------------------------------------

; Test checks address calculation sinking

define spir_func void @test_asink(i1 %src1, ptr %src2) {
; CHECK-LABEL: @test_asink(
; CHECK-NEXT:  entry:
; CHECK-NEXT:    br i1 %src1, label %bb1, label %bb2
; CHECK:       bb1:
; CHECK-NEXT:    [[TMP0:%.*]] = getelementptr i32, ptr %src2, i32 4
; CHECK-NEXT:    [[TMP1:%.*]] = ptrtoint ptr [[TMP0]] to i32
; CHECK-NEXT:    [[TMP2:%.*]] = add i32 [[TMP1]], 14
; CHECK-NEXT:    [[TMP3:%.*]] = inttoptr i32 [[TMP2]] to ptr
; CHECK-NEXT:    br label %end
; CHECK:       bb2:
; CHECK-NEXT:    [[TMP4:%.*]] = getelementptr i32, ptr %src2, i32 4
; CHECK-NEXT:    [[TMP5:%.*]] = ptrtoint ptr [[TMP4]] to i32
; CHECK-NEXT:    [[TMP6:%.*]] = add i32 [[TMP5]], 14
; CHECK-NEXT:    [[TMP7:%.*]] = inttoptr i32 [[TMP6]] to ptr
; CHECK-NEXT:    br label %end
; CHECK:       end:
; CHECK-NEXT:    [[TMP8:%.*]] = phi ptr [ [[TMP3]], %bb1 ], [ [[TMP7]], %bb2 ]
; CHECK-NEXT:    [[TMP9:%.*]] = getelementptr i32, ptr %src2, i32 5
; CHECK-NEXT:    [[TMP10:%.*]] = ptrtoint ptr [[TMP9]] to i32
; CHECK-NEXT:    [[TMP11:%.*]] = add i32 [[TMP10]], 14
; CHECK-NEXT:    [[TMP12:%.*]] = inttoptr i32 [[TMP11]] to ptr
; CHECK-NEXT:    store i32 1, ptr [[TMP12]], align 4
; CHECK-NEXT:    store i32 2, ptr [[TMP8]], align 4
; CHECK-NEXT:    ret void
;
entry:
  %0 = getelementptr i32, ptr %src2, i32 5
  %1 = ptrtoint ptr %0 to i32
  %2 = add i32 %1, 14
  %3 = inttoptr i32 %2 to ptr
  br i1 %src1, label %bb1, label %bb2

bb1:
  %4 = getelementptr i32, ptr %src2, i32 4
  %5 = ptrtoint ptr %4 to i32
  %6 = add i32 %5, 14
  %7 = inttoptr i32 %6 to ptr
  br label %end

bb2:
  %8 = getelementptr i32, ptr %src2, i32 4
  %9 = ptrtoint ptr %8 to i32
  %10 = add i32 %9, 14
  %11 = inttoptr i32 %10 to ptr
  br label %end

end:
  %12 = phi ptr [ %7, %bb1 ], [ %11, %bb2 ]
  store i32 1, ptr %3, align 4
  store i32 2, ptr %12, align 4
  ret void
}

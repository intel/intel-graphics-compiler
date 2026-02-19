;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2023-2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; UNSUPPORTED: llvm-17-plus
; RUN: igc_opt --typed-pointers -igc-address-arith-sinking  -S < %s | FileCheck %s
; ------------------------------------------------
; AddressArithmeticSinking
; ------------------------------------------------

; Test checks address calculation sinking

define spir_func void @test_asink(i1 %src1, i32* %src2) {
; CHECK-LABEL: @test_asink(
; CHECK-NEXT:  entry:
; CHECK-NEXT:    br i1 %src1, label %bb1, label %bb2
; CHECK:       bb1:
; CHECK-NEXT:    [[TMP0:%.*]] = getelementptr i32, i32* %src2, i32 4
; CHECK-NEXT:    [[TMP1:%.*]] = ptrtoint i32* [[TMP0]] to i32
; CHECK-NEXT:    [[TMP2:%.*]] = add i32 [[TMP1]], 14
; CHECK-NEXT:    [[TMP3:%.*]] = inttoptr i32 [[TMP2]] to i32*
; CHECK-NEXT:    br label %end
; CHECK:       bb2:
; CHECK-NEXT:    [[TMP4:%.*]] = getelementptr i32, i32* %src2, i32 4
; CHECK-NEXT:    [[TMP5:%.*]] = ptrtoint i32* [[TMP4]] to i32
; CHECK-NEXT:    [[TMP6:%.*]] = add i32 [[TMP5]], 14
; CHECK-NEXT:    [[TMP7:%.*]] = inttoptr i32 [[TMP6]] to i32*
; CHECK-NEXT:    br label %end
; CHECK:       end:
; CHECK-NEXT:    [[TMP8:%.*]] = phi i32* [ [[TMP3]], %bb1 ], [ [[TMP7]], %bb2 ]
; CHECK-NEXT:    [[TMP9:%.*]] = getelementptr i32, i32* %src2, i32 5
; CHECK-NEXT:    [[TMP10:%.*]] = ptrtoint i32* [[TMP9]] to i32
; CHECK-NEXT:    [[TMP11:%.*]] = add i32 [[TMP10]], 14
; CHECK-NEXT:    [[TMP12:%.*]] = inttoptr i32 [[TMP11]] to i32*
; CHECK-NEXT:    store i32 1, i32* [[TMP12]], align 4
; CHECK-NEXT:    store i32 2, i32* [[TMP8]], align 4
; CHECK-NEXT:    ret void
;
entry:
  %0 = getelementptr i32, i32* %src2, i32 5
  %1 = ptrtoint i32* %0 to i32
  %2 = add i32 %1, 14
  %3 = inttoptr i32 %2 to i32*
  br i1 %src1, label %bb1, label %bb2

bb1:
  %4 = getelementptr i32, i32* %src2, i32 4
  %5 = ptrtoint i32* %4 to i32
  %6 = add i32 %5, 14
  %7 = inttoptr i32 %6 to i32*
  br label %end

bb2:
  %8 = getelementptr i32, i32* %src2, i32 4
  %9 = ptrtoint i32* %8 to i32
  %10 = add i32 %9, 14
  %11 = inttoptr i32 %10 to i32*
  br label %end

end:
  %12 = phi i32* [ %7, %bb1 ], [ %11, %bb2 ]
  store i32 1, i32* %3, align 4
  store i32 2, i32* %12, align 4
  ret void
}

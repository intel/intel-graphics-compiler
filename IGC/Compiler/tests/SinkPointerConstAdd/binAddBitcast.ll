;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; REQUIRES: llvm-14-plus
; RUN: igc_opt --opaque-pointers -platformbmg --igc-sink-ptr-const-add -S < %s | FileCheck %s
; ------------------------------------------------
; SinkPointerConstAdd
; ------------------------------------------------
;
; `add i32 %rt, %bc`: RuntimeValue leg gives KEEP_REASON_ABI, bitcast leg
; gives KEEP_REASON_UNKNOWN. Pins the combiner's ABI-dominates rule -- a
; stricter rule would flip zext to sext and reintroduce the OOB bug.

; Function Attrs: alwaysinline null_pointer_is_valid
define void @main(<8 x i32> %r0, i8* %privateBase) #0 {
GlobalScopeInitialization:
  %0 = call i32 @llvm.genx.GenISA.RuntimeValue.i32(i32 6)
  %1 = call i64 @llvm.genx.GenISA.RuntimeValue.i64(i32 2)
  %2 = bitcast float 1.000000e+00 to i32
; CHECK: %2 = bitcast float 1.000000e+00 to i32
  %3 = add i32 %0, %2
; CHECK-NEXT: %3 = add i32 %0, %2
  %4 = add i32 %3, 128
  %5 = zext i32 %4 to i64
; CHECK-NEXT: %4 = zext i32 %3 to i64
; CHECK-NOT: sext
  %6 = add i64 %1, %5
; CHECK-NEXT: %5 = add i64 %1, %4
; CHECK-NEXT: %6 = add i64 %5, 128
  %ptr = inttoptr i64 %6 to i32 addrspace(1)*
; CHECK-NEXT: %7 = inttoptr i64 %6 to ptr addrspace(1)
  ret void
}

; Function Attrs: nounwind readnone willreturn
declare i64 @llvm.genx.GenISA.RuntimeValue.i64(i32) #1

; Function Attrs: nounwind readnone willreturn
declare i32 @llvm.genx.GenISA.RuntimeValue.i32(i32) #1

attributes #0 = { alwaysinline null_pointer_is_valid }
attributes #1 = { nounwind readnone willreturn }


!igc.functions = !{!0}

!0 = !{void (<8 x i32>, i8*)* @main, !1}
!1 = !{!2, !3}
!2 = !{!"function_type", i32 0}
!3 = !{!"implicit_arg_desc", !4}
!4 = !{i32 0}

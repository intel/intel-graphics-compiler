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
; Regression: the i32 add behind the zext carries no nuw flag, so the pass
; cannot prove +128 survives the zero-extension and must not sink it into
; the i64 chain. (With nuw the sink IS allowed -- see crossZextHighBit.ll.)
; The entire input must round-trip unchanged.

define void @main(<8 x i32> %r0, i8* %privateBase) #0 {
entry:
  %rv32 = call i32 @llvm.genx.GenISA.RuntimeValue.i32(i32 6)
  %rv64 = call i64 @llvm.genx.GenISA.RuntimeValue.i64(i32 2)
  %a32 = add i32 %rv32, 128
  %a64 = zext i32 %a32 to i64
  %sum = add i64 %rv64, %a64
  %ptr = inttoptr i64 %sum to i32 addrspace(1)*
; CHECK: %a32 = add i32 %rv32, 128
; CHECK-NEXT: %a64 = zext i32 %a32 to i64
; CHECK-NEXT: %sum = add i64 %rv64, %a64
; CHECK-NEXT: %ptr = inttoptr i64 %sum to ptr addrspace(1)
  ret void
}

; Function Attrs: nounwind readnone willreturn
declare i32 @llvm.genx.GenISA.RuntimeValue.i32(i32) #1

; Function Attrs: nounwind readnone willreturn
declare i64 @llvm.genx.GenISA.RuntimeValue.i64(i32) #1

attributes #0 = { alwaysinline null_pointer_is_valid }
attributes #1 = { nounwind readnone willreturn }

!igc.functions = !{!0}

!0 = !{void (<8 x i32>, i8*)* @main, !1}
!1 = !{!2, !3}
!2 = !{!"function_type", i32 0}
!3 = !{!"implicit_arg_desc", !4}
!4 = !{i32 0}

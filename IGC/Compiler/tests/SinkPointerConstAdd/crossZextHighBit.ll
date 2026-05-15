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
; When sinking a constant from below a zext into the i64 offset, the constant
; must be zero-extended to i64.  Sign-extending an i32 constant with bit 31
; set would corrupt the offset by 2^32 (here: 0x80000000 -> 0xFFFFFFFF80000000
; instead of 0x0000000080000000).
;
; Input constant: 0x80000000 == 2147483648.
; Correct sunken offset: 2147483648 (zero-extended).
; Incorrect sunken offset: -2147483648 (sign-extended).

define void @main(<8 x i32> %r0, i8* %privateBase) #0 {
entry:
  %rv32 = call i32 @llvm.genx.GenISA.RuntimeValue.i32(i32 6)
  %rv64 = call i64 @llvm.genx.GenISA.RuntimeValue.i64(i32 2)
  %a32 = add nuw i32 %rv32, -2147483648
  %a64 = zext i32 %a32 to i64
  %sum = add i64 %rv64, %a64
  %ptr = inttoptr i64 %sum to i32 addrspace(1)*
; CHECK: %a64 = zext i32 %rv32 to i64
; CHECK-NEXT: %sum = add i64 %rv64, %a64
; CHECK-NEXT: %[[SUNK:[0-9]+]] = add i64 %sum, 2147483648
; CHECK-NEXT: %{{[0-9]+}} = inttoptr i64 %[[SUNK]] to ptr addrspace(1)
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

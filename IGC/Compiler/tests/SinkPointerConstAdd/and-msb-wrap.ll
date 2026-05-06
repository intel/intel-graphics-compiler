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
; Counterpart to andClearMSB.ll. Mask 0x7FFFFFFF still allows bit 30 to be
; set, so add(masked, 128) can carry into bit 31 (max: 0x7FFFFFFF+128 =
; 0x8000007F). computeKnownBits cannot prove bit 31 zero at the zext operand,
; so the pass refuses to cross the zext. The i32-level +128 stays put.

; Function Attrs: alwaysinline null_pointer_is_valid
define void @main(<8 x i32> %r0, i8* %privateBase) #0 {
GlobalScopeInitialization:
  %0 = call i32 @llvm.genx.GenISA.RuntimeValue.i32(i32 6)
  %1 = call i64 @llvm.genx.GenISA.RuntimeValue.i64(i32 2)
  %2 = and i32 %0, 2147483647
; CHECK: %2 = and i32 %0, 2147483647
  %3 = add i32 %2, 128
; CHECK-NEXT: %3 = add i32 %2, 128
  %4 = zext i32 %3 to i64
; CHECK-NEXT: %4 = zext i32 %3 to i64
; CHECK-NOT: sext
  %5 = add i64 %1, %4
; CHECK-NEXT: %5 = add i64 %1, %4
  %ptr = inttoptr i64 %5 to i32 addrspace(1)*
; CHECK-NEXT: %ptr = inttoptr i64 %5 to ptr addrspace(1)
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

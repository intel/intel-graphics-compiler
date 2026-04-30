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
; Like bitcastChain.ll but with no RuntimeValue / Argument / Load anywhere
; in the function -- exercises KEEP_REASON_UNKNOWN with no ABI proof in
; scope. zext is preserved.

; Function Attrs: alwaysinline null_pointer_is_valid
define void @main(<8 x i32> %r0, i8* %privateBase) #0 {
GlobalScopeInitialization:
  %0 = bitcast float 1.000000e+00 to i32
; CHECK: %0 = bitcast float 1.000000e+00 to i32
  %1 = add i32 %0, 128
  %2 = zext i32 %1 to i64
; CHECK-NEXT: %1 = zext i32 %0 to i64
; CHECK-NOT: sext
  %ptr = inttoptr i64 %2 to i32 addrspace(1)*
; CHECK-NEXT: %2 = add i64 %1, 128
; CHECK-NEXT: %3 = inttoptr i64 %2 to ptr addrspace(1)
  ret void
}

attributes #0 = { alwaysinline null_pointer_is_valid }


!igc.functions = !{!0}

!0 = !{void (<8 x i32>, i8*)* @main, !1}
!1 = !{!2, !3}
!2 = !{!"function_type", i32 0}
!3 = !{!"implicit_arg_desc", !4}
!4 = !{i32 0}

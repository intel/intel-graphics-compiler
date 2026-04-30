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
; Bitcast-rooted i32 chain: no whitelisted source and no provably-negative
; component, so getZextDecision returns KEEP_REASON_UNKNOWN. Sinking still
; proceeds; zext is preserved.

; Function Attrs: alwaysinline null_pointer_is_valid
define void @main(<8 x i32> %r0, i8* %privateBase) #0 {
GlobalScopeInitialization:
  %0 = call i64 @llvm.genx.GenISA.RuntimeValue.i64(i32 2)
  %1 = bitcast float 1.000000e+00 to i32
; CHECK: %1 = bitcast float 1.000000e+00 to i32
  %2 = add i32 %1, 128
  %3 = zext i32 %2 to i64
; CHECK-NEXT: %2 = zext i32 %1 to i64
; CHECK-NOT: sext
  %4 = add i64 %0, %3
; CHECK-NEXT: %3 = add i64 %0, %2
; CHECK-NEXT: %4 = add i64 %3, 128
  %ptr = inttoptr i64 %4 to i32 addrspace(1)*
; CHECK-NEXT: %5 = inttoptr i64 %4 to ptr addrspace(1)
  ret void
}

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

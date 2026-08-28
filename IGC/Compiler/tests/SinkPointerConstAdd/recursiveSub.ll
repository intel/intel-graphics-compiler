;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2025-2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: igc_opt --opaque-pointers -platformbmg --igc-sink-ptr-const-add -S < %s | FileCheck %s
; ------------------------------------------------
; SinkPointerConstAdd
; ------------------------------------------------
;
; A chain of i64 const adds and subs collapses recursively into a single
; sunken constant (+128 - 32 + 8 == +104). The recursion stops at the zext
; that begins the descriptor base, so the i32 RuntimeValue and its
; i64-extension are left untouched.

define void @main(<8 x i32> %r0, i8* %privateBase) #0 {
entry:
  %rv32 = call i32 @llvm.genx.GenISA.RuntimeValue.i32(i32 6)
  %rv64 = call i64 @llvm.genx.GenISA.RuntimeValue.i64(i32 2)
  %base = zext i32 %rv32 to i64
  %sum = add i64 %rv64, %base
  %c1 = add i64 %sum, 128
  %c2 = sub i64 %c1, 32
  %c3 = add i64 %c2, 8
  %ptr = inttoptr i64 %c3 to i32 addrspace(1)*
; CHECK: %base = zext i32 %rv32 to i64
; CHECK-NEXT: %sum = add i64 %rv64, %base
; CHECK-NEXT: %[[SUNK:[0-9]+]] = add i64 %sum, 104
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
!1 = !{!2}
!2 = !{!"function_type", i32 0}
!5 = !{!"argId", i32 0}
!6 = !{!"implicitArgInfoListVec[0]", !5}
!7 = !{!"implicitArgInfoList", !6}
!8 = !{!"FuncMDMap[0]", void (<8 x i32>, i8*)* @main}
!9 = !{!"FuncMDValue[0]", !7}
!10 = !{!"FuncMD", !8, !9}
!11 = !{!"ModuleMD", !10}
!IGCMetadata = !{!11}

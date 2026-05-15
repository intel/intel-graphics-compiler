;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2025-2026 Intel Corporation
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
; A single i64 constant add buried inside a descriptor-pointer chain is sunk
; past the dynamic (base+offset) sum to a single add immediately before the
; inttoptr.

define void @main(<8 x i32> %r0, i8* %privateBase) #0 {
entry:
  %rv_base32 = call i32 @llvm.genx.GenISA.RuntimeValue.i32(i32 6)
  %rv_off32  = call i32 @llvm.genx.GenISA.RuntimeValue.i32(i32 8)
  %base = zext i32 %rv_base32 to i64
  %off  = sext i32 %rv_off32  to i64
  %c1 = add i64 %base, 128
  %c2 = add i64 %c1, %off
  %ptr = inttoptr i64 %c2 to i32 addrspace(1)*
; CHECK: %base = zext i32 %rv_base32 to i64
; CHECK-NEXT: %off = sext i32 %rv_off32 to i64
; CHECK-NEXT: %c2 = add i64 %base, %off
; CHECK-NEXT: %[[SUNK:[0-9]+]] = add i64 %c2, 128
; CHECK-NEXT: %{{[0-9]+}} = inttoptr i64 %[[SUNK]] to ptr addrspace(1)
  ret void
}

; Function Attrs: nounwind readnone willreturn
declare i32 @llvm.genx.GenISA.RuntimeValue.i32(i32) #1

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

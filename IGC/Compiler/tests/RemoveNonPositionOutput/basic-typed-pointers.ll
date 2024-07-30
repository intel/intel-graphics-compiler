;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: igc_opt -enable-debugify --igc-remove-nonposition-output -S < %s 2>&1 | FileCheck %s
; ------------------------------------------------
; RemoveNonPositionOutput
; ------------------------------------------------

; Debug-info related check
;
; CHECK-COUNT-1: WARNING
; CHECK-SAME: Missing line 2
; CHECK-NOT: WARNING
; CHECK: CheckModuleDebugify: PASS

define void @test_remove(i32 %src) {
; CHECK-LABEL: @test_remove(
; CHECK:  entry:
; CHECK:    call void @llvm.genx.GenISA.OUTPUT.f32(float 1.500000e+00, float 2.500000e+00, float 3.500000e+00, float 4.500000e+00, i32 1, i32 1, i32 15)
; CHECK:    call void @llvm.genx.GenISA.OUTPUT.f32(float 1.250000e+00, float 2.250000e+00, float 3.250000e+00, float 4.250000e+00, i32 8, i32 1, i32 15)
; CHECK:    ret void
;
entry:
  call void @llvm.genx.GenISA.OUTPUT.f32(float 1.500000e+00, float 2.500000e+00, float 3.500000e+00, float 4.500000e+00, i32 1, i32 1, i32 15)
  call void @llvm.genx.GenISA.OUTPUT.f32(float 1.000000e+00, float 2.000000e+00, float 3.000000e+00, float 4.000000e+00, i32 0, i32 1, i32 15)
  call void @llvm.genx.GenISA.OUTPUT.f32(float 1.250000e+00, float 2.250000e+00, float 3.250000e+00, float 4.250000e+00, i32 8, i32 1, i32 15)
  ret void
}

declare void @llvm.genx.GenISA.OUTPUT.f32(float, float, float, float, i32, i32, i32)

!igc.functions = !{!0}
!IGCMetadata = !{!4}

!0 = !{void (i32)* @test_remove, !1}
!1 = !{!2, !3}
!2 = !{!"function_type", i32 0}
!3 = !{!"implicit_arg_desc"}
!4 = !{!"ModuleMD", !5}
!5 = !{!"FuncMD", !6, !7}
!6 = distinct !{!"FuncMDMap[0]", void (i32)* @test_remove}
!7 = !{!"FuncMDValue[0]", !8, !9}
!8 = !{!"funcArgs"}
!9 = !{!"functionType", !"KernelFunction"}

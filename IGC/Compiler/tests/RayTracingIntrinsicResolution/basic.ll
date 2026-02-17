;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: igc_opt --typed-pointers -enable-debugify --raytracing-intrinsic-resolution -S < %s 2>&1 | FileCheck %s
; ------------------------------------------------
; RayTracingIntrinsicResolution
; ------------------------------------------------

; Test checks that intrinsics are substituted by implicit args

; Debug-info related check
; CHECK: CheckModuleDebugify: PASS

define spir_kernel void @test_rti(i32 %src, i8 addrspace(1)* %globalPointer, i8 addrspace(1)* %localPointer, i16 %stackID, <2 x i8 addrspace(1)*> %inlinedData) {
; CHECK-LABEL: @test_rti(
; CHECK:    [[TMP1:%.*]] = bitcast i8 addrspace(1)* [[LOCALPOINTER:%.*]] to i32 addrspace(1)*
; CHECK:    [[TMP2:%.*]] = extractelement <2 x i8 addrspace(1)*> [[INLINEDDATA:%.*]], i16 [[STACKID:%.*]]
; CHECK:    [[TMP3:%.*]] = bitcast i8 addrspace(1)* [[TMP2]] to i32 addrspace(1)*
; CHECK:    store i8 13, i8 addrspace(1)* [[GLOBALPOINTER:%.*]], align 1
; CHECK:    store i32 [[SRC:%.*]], i32 addrspace(1)* [[TMP1]], align 4
; CHECK:    store i32 [[SRC]], i32 addrspace(1)* [[TMP3]], align 4
; CHECK:    ret void
;
  %1 = call i8 addrspace(1)* @llvm.genx.GenISA.GlobalBufferPointer()
  %2 = call i32 addrspace(1)* @llvm.genx.GenISA.LocalBufferPointer()
  %3 = call i16 @llvm.genx.GenISA.AsyncStackID()
  %4 = call i32 addrspace(1)* @llvm.genx.GenISA.InlinedData(i16 %3)
  store i8 13, i8 addrspace(1)* %1, align 1
  store i32 %src, i32 addrspace(1)* %2, align 4
  store i32 %src, i32 addrspace(1)* %4, align 4
  ret void
}

declare i8 addrspace(1)* @llvm.genx.GenISA.GlobalBufferPointer()
declare i32 addrspace(1)* @llvm.genx.GenISA.LocalBufferPointer()
declare i16 @llvm.genx.GenISA.AsyncStackID()
declare i32 addrspace(1)* @llvm.genx.GenISA.InlinedData(i16)

!igc.functions = !{!0}

!0 = !{void (i32, i8 addrspace(1)*, i8 addrspace(1)*, i16, <2 x i8 addrspace(1)*>)* @test_rti, !1}
!1 = !{!2, !3}
!2 = !{!"function_type", i32 0}
!3 = !{!"implicit_arg_desc", !4, !5, !6, !7}
!4 = !{i32 51}
!5 = !{i32 52}
!6 = !{i32 54}
!7 = !{i32 53}

;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; UNSUPPORTED: llvm-17-plus
; RUN: igc_opt --typed-pointers --raytracing-intrinsic-analysis -igc-serialize-metadata -S %s -o %t.ll
; RUN: FileCheck %s --input-file=%t.ll
; ------------------------------------------------
; RayTracingIntrinsicAnalysis
; ------------------------------------------------

define spir_kernel void @test_rti() {
  call spir_func void @foo()
  ret void
}

define spir_func void @foo() {
  call spir_func void @rti()
  ret void
}

define spir_func void @rti() {
  %1 = call i8 addrspace(1)* @llvm.genx.GenISA.GlobalBufferPointer()
  %2 = call i32 addrspace(1)* @llvm.genx.GenISA.LocalBufferPointer()
  %3 = call i16 @llvm.genx.GenISA.AsyncStackID()
  %stackid_ext = zext i16 %3 to i32
  %4 = call i32 addrspace(1)* @llvm.genx.GenISA.InlinedData(i32 %stackid_ext)
  call spir_func void @foo()
  ret void
}

declare i8 addrspace(1)* @llvm.genx.GenISA.GlobalBufferPointer()
declare i32 addrspace(1)* @llvm.genx.GenISA.LocalBufferPointer()
declare i16 @llvm.genx.GenISA.AsyncStackID()
declare i32 addrspace(1)* @llvm.genx.GenISA.InlinedData(i32)

!igc.functions = !{!0, !3, !6}

!0 = !{void ()* @test_rti, !1}
!1 = !{!2}
!2 = !{!"function_type", i32 0}
!3 = !{void ()* @rti, !4}
!4 = !{!5}
!5 = !{!"function_type", i32 0}
!6 = !{void ()* @foo, !7}
!7 = !{!8}
!8 = !{!"function_type", i32 0}

; RayTracingIntrinsicAnalysis records the detected ray-tracing implicit args
; (argId 50, 51, 53, 52) through FuncMD implicitArgInfoList rather than the legacy
; igc.functions implicit_arg_desc.
; CHECK: !{!"implicitArgInfoList"
; CHECK: !{!"argId", i32 50}
; CHECK: !{!"argId", i32 51}
; CHECK: !{!"argId", i32 53}
; CHECK: !{!"argId", i32 52}

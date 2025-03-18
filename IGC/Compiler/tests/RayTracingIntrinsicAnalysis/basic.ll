;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: igc_opt --typed-pointers --raytracing-intrinsic-analysis -S %s -o %t.ll
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
  %4 = call i32 addrspace(1)* @llvm.genx.GenISA.InlinedData(i16 %3)
  call spir_func void @foo()
  ret void
}

declare i8 addrspace(1)* @llvm.genx.GenISA.GlobalBufferPointer()
declare i32 addrspace(1)* @llvm.genx.GenISA.LocalBufferPointer()
declare i16 @llvm.genx.GenISA.AsyncStackID()
declare i32 addrspace(1)* @llvm.genx.GenISA.InlinedData(i16)

!igc.functions = !{!0, !3, !6}

!0 = !{void ()* @test_rti, !1}
!1 = !{!2, !9}
!2 = !{!"function_type", i32 0}
!3 = !{void ()* @rti, !4}
!4 = !{!5, !9}
!5 = !{!"function_type", i32 0}
!6 = !{void ()* @foo, !7}
!7 = !{!8, !9}
!8 = !{!"function_type", i32 0}
!9 = !{!"implicit_arg_desc"}

; CHECK-LABEL: !igc.functions = !{!0, !8, !9}
; CHECK: !0 = !{void ()* @test_rti, !1}
; CHECK: !1 = !{!2, !3}
; CHECK: !2 = !{!"function_type", i32 0}
; CHECK: !3 = !{!"implicit_arg_desc", !4, !5, !6, !7}
; CHECK: !4 = !{i32 55}
; CHECK: !5 = !{i32 56}
; CHECK: !6 = !{i32 58}
; CHECK: !7 = !{i32 57}
; CHECK: !8 = !{void ()* @rti, !1}
; CHECK: !9 = !{void ()* @foo, !1}

;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022-2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: llvm-14-plus
; RUN: igc_opt --opaque-pointers -igc-add-implicit-args -S %s -o %t.ll
; RUN: FileCheck %s --input-file=%t.ll

; CHECK:     define spir_kernel void @test_rti(ptr addrspace(1) %globalPointer, ptr addrspace(1) %localPointer, i16 %stackID, <2 x ptr addrspace(1)> %inlinedData)
; CHECK-NOT: define spir_kernel void @test_rti()
; CHECK:     call spir_func void @foo(ptr addrspace(1) %globalPointer, ptr addrspace(1) %localPointer, i16 %stackID, <2 x ptr addrspace(1)> %inlinedData)

define spir_kernel void @test_rti() {
  call spir_func void @foo()
  ret void
}

; CHECK:     define spir_func void @foo(ptr addrspace(1) %globalPointer, ptr addrspace(1) %localPointer, i16 %stackID, <2 x ptr addrspace(1)> %inlinedData)
; CHECK-NOT: define spir_func void @foo()
; CHECK:     call spir_func void @rti(ptr addrspace(1) %globalPointer, ptr addrspace(1) %localPointer, i16 %stackID, <2 x ptr addrspace(1)> %inlinedData)

define spir_func void @foo() {
  call spir_func void @rti()
  ret void
}

; CHECK:     define spir_func void @rti(ptr addrspace(1) %globalPointer, ptr addrspace(1) %localPointer, i16 %stackID, <2 x ptr addrspace(1)> %inlinedData)
; CHECK-NOT: define spir_func void @rti()

define spir_func void @rti() {
  %1 = call ptr addrspace(1) @llvm.genx.GenISA.GlobalBufferPointer()
  %2 = call ptr addrspace(1) @llvm.genx.GenISA.LocalBufferPointer()
  %3 = call i16 @llvm.genx.GenISA.AsyncStackID()
  %4 = call ptr addrspace(1) @llvm.genx.GenISA.InlinedData(i16 %3)
  ret void
}

declare ptr addrspace(1) @llvm.genx.GenISA.GlobalBufferPointer()

declare ptr addrspace(1) @llvm.genx.GenISA.LocalBufferPointer()

declare i16 @llvm.genx.GenISA.AsyncStackID()

declare ptr addrspace(1) @llvm.genx.GenISA.InlinedData(i16)

!igc.functions = !{!0, !8, !9}

!0 = !{ptr @test_rti, !1}
!1 = !{!2, !3}
!2 = !{!"function_type", i32 0}
!3 = !{!"implicit_arg_desc", !4, !5, !6, !7}
!4 = !{i32 55}
!5 = !{i32 56}
!6 = !{i32 58}
!7 = !{i32 57}
!8 = !{ptr @rti, !1}
!9 = !{ptr @foo, !1}

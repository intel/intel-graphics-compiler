;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022-2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; UNSUPPORTED: llvm-17-plus
; RUN: igc_opt --typed-pointers -igc-add-implicit-args -S %s -o %t.ll
; RUN: FileCheck %s --input-file=%t.ll

; CHECK:     define spir_kernel void @test_rti(i8 addrspace(1)* %globalPointer, i8 addrspace(1)* %localPointer, i16 %stackID, <2 x i8 addrspace(1)*> %inlinedData)
; CHECK-NOT: define spir_kernel void @test_rti()
; CHECK:     call spir_func void @foo(i8 addrspace(1)* %globalPointer, i8 addrspace(1)* %localPointer, i16 %stackID, <2 x i8 addrspace(1)*> %inlinedData)

define spir_kernel void @test_rti() {
  call spir_func void @foo()
  ret void
}

; CHECK:     define spir_func void @foo(i8 addrspace(1)* %globalPointer, i8 addrspace(1)* %localPointer, i16 %stackID, <2 x i8 addrspace(1)*> %inlinedData)
; CHECK-NOT: define spir_func void @foo()
; CHECK:     call spir_func void @rti(i8 addrspace(1)* %globalPointer, i8 addrspace(1)* %localPointer, i16 %stackID, <2 x i8 addrspace(1)*> %inlinedData)

define spir_func void @foo() {
  call spir_func void @rti()
  ret void
}

; CHECK:     define spir_func void @rti(i8 addrspace(1)* %globalPointer, i8 addrspace(1)* %localPointer, i16 %stackID, <2 x i8 addrspace(1)*> %inlinedData)
; CHECK-NOT: define spir_func void @rti()

define spir_func void @rti() {
  %1 = call i8 addrspace(1)* @llvm.genx.GenISA.GlobalBufferPointer()
  %2 = call i32 addrspace(1)* @llvm.genx.GenISA.LocalBufferPointer()
  %3 = call i16 @llvm.genx.GenISA.AsyncStackID()
  %4 = call i32 addrspace(1)* @llvm.genx.GenISA.InlinedData(i16 %3)
  ret void
}

declare i8 addrspace(1)* @llvm.genx.GenISA.GlobalBufferPointer()

declare i32 addrspace(1)* @llvm.genx.GenISA.LocalBufferPointer()

declare i16 @llvm.genx.GenISA.AsyncStackID()

declare i32 addrspace(1)* @llvm.genx.GenISA.InlinedData(i16)

!igc.functions = !{!0, !8, !9}

!0 = !{void ()* @test_rti, !1}
!1 = !{!2, !3}
!2 = !{!"function_type", i32 0}
!3 = !{!"implicit_arg_desc", !4, !5, !6, !7}
!4 = !{i32 50}
!5 = !{i32 51}
!6 = !{i32 53}
!7 = !{i32 52}
!8 = !{void ()* @rti, !1}
!9 = !{void ()* @foo, !1}

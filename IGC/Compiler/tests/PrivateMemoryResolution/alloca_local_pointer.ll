;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; REQUIRES: regkeys
; RUN: igc_opt --opaque-pointers --regkey EnablePrivMemNewSOATranspose=0 --igc-private-mem-resolution -S < %s 2>&1 | FileCheck %s

; This tests checks that private memory allocation for local pointer is 4B

; CHECK-LABEL: @testallocalocal
; CHECK-NEXT: entry:
; CHECK-NEXT: [[SIMDLANEID16:%.*]] = call i16 @llvm.genx.GenISA.simdLaneId()
; CHECK-NEXT: [[SIMDLANEID:%.*]] = zext i16 [[SIMDLANEID16]] to i32
; CHECK-NEXT: [[SIMDSIZE:%.*]] = call i32 @llvm.genx.GenISA.simdSize()
; CHECK-NEXT: [[SECTIONOFFSET:%.*]] = mul i32 [[SIMDSIZE]], 0
; CHECK-NEXT: [[BUFFEROFFSET:%.*]] = add i32 0, [[SECTIONOFFSET]]
; CHECK-NEXT: [[PERLANEOFFSET:%.*]] = mul i32 [[SIMDLANEID]], 4
; CHECK-NEXT: [[SIMDBUFFEROFFSET:%.*]] = add i32 [[BUFFEROFFSET]], [[PERLANEOFFSET]]
; CHECK-NEXT: [[STACKALLOCA:%.*]] = call ptr @llvm.genx.GenISA.StackAlloca(i32 [[SIMDBUFFEROFFSET]])

target datalayout = "e-p3:32:32:32"

define spir_kernel void @testallocalocal(ptr %privateBase) #0 {
entry:
  %0 = alloca ptr addrspace(3)
  ret void
}

attributes #0 = { convergent noinline nounwind "visaStackCall" }

!IGCMetadata = !{!0}
!igc.functions = !{!1}

!0 = !{!"ModuleMD", !7, !8}
!1 = !{ptr @testallocalocal, !2}
!2 = !{!3, !4}
!3 = !{!"function_type", i32 0}
!4 = !{!"implicit_arg_desc", !5}
!5 = !{i32 13}
!7 = !{!"FuncMD", !9, !10}
!8 = !{!"privateMemoryPerWI", i32 0}
!9 = !{!"FuncMDMap[0]", ptr @testallocalocal}
!10 = !{!"FuncMDValue[0]", !8}

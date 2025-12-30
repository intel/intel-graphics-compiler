;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024-2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; Check if image kernel argument is successfully tracked when it is stored
; via an explicit GEP to the first struct member, but loaded implicitly
; (without a GEP) from the struct pointer. This tests the ValueTracker's
; ability to match implicit offset-0 access with explicit [0, 0] GEP.

; REQUIRES: llvm-14-plus
; RUN: igc_opt --opaque-pointers %s -S -o - -igc-conv-ocl-to-common | FileCheck %s

; CHECK-NOT: __builtin_IB_write_2d_u4i
; CHECK: call void @llvm.genx.GenISA.typedwrite.p131072(

%struct.wrapper = type { ptr addrspace(1) }

define spir_kernel void @test_implicit_gep_access(ptr addrspace(1) %image_arg) {
entry:
  ; Allocate the wrapper struct
  %wrapper = alloca %struct.wrapper, align 8
  ; Store the image pointer using explicit GEP [0, 0]
  %gep = getelementptr inbounds %struct.wrapper, ptr %wrapper, i64 0, i32 0
  store ptr addrspace(1) %image_arg, ptr %gep, align 8
  ; Load the image pointer implicitly (without GEP - accesses offset 0)
  ; This is equivalent to loading from %gep but with implicit offset-0 access
  %loaded_img = load ptr addrspace(1), ptr %wrapper, align 8
  ; Convert to i64 for the builtin call
  %img_as_int = ptrtoint ptr addrspace(1) %loaded_img to i64
  call spir_func void @__builtin_IB_write_2d_u4i(i64 %img_as_int, <2 x i32> zeroinitializer, <4 x i32> zeroinitializer, i32 0)
  ret void
}

declare spir_func void @__builtin_IB_write_2d_u4i(i64, <2 x i32>, <4 x i32>, i32)

!igc.functions = !{!0}
!IGCMetadata = !{!3}

!0 = !{ptr @test_implicit_gep_access, !1}
!1 = !{!2}
!2 = !{!"function_type", i32 0}
!3 = !{!"ModuleMD", !4}
!4 = !{!"FuncMD", !5, !6}
!5 = distinct !{!"FuncMDMap[0]", ptr @test_implicit_gep_access}
!6 = !{!"FuncMDValue[0]", !7, !8, !9}
!7 = !{!"funcArgs"}
!8 = !{!"functionType", !"KernelFunction"}
!9 = !{!"resAllocMD", !10}
!10 = !{!"argAllocMDList", !11}
!11 = !{!"argAllocMDListVec[0]", !12, !13, !14}
!12 = !{!"type", i32 1}
!13 = !{!"extensionType", i32 0}
!14 = !{!"indexType", i32 0}

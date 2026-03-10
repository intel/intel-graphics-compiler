;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: llvm-14-plus
; RUN: igc_opt --opaque-pointers %s -S -o - -igc-conv-ocl-to-common | FileCheck %s

target datalayout = "e-p:32:32:32-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f16:16:16-f32:32:32-f64:64:64-f80:128:128-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024-a:64:64-f80:128:128-n8:16:32:64"

%spirv.Image._void_0_0_0_0_0_0_1 = type opaque
%spirv.Image._void_0_0_1_0_0_0_1 = type opaque
%spirv.Image._void_1_0_0_0_0_0_1 = type opaque
%spirv.Image._void_1_0_1_0_0_0_1 = type opaque
%spirv.Image._void_2_0_0_0_0_0_1 = type opaque

define spir_kernel void @test(ptr addrspace(1) %img_2darr) {
  %img_as_int = ptrtoint ptr addrspace(1) %img_2darr to i64

  ; CHECK-LABEL: @test(

  ; CHECK-NOT: __builtin_IB_OCL_2darr_ldmcs
  ; CHECK: GenISA.ldmcsptr
  %a = call spir_func <4 x float> @__builtin_IB_OCL_2darr_ldmcs(i64 %img_as_int, <4 x i32> zeroinitializer)

  ; CHECK-NOT: __builtin_IB_OCL_2darr_ld2dms
  ; CHECK: GenISA.ldmsptr
  %b = call spir_func <4 x float> @__builtin_IB_OCL_2darr_ld2dms(i64 %img_as_int, <4 x i32> zeroinitializer, i32 0, <4 x float> zeroinitializer)

  ret void
}

declare spir_func <4 x float> @__builtin_IB_OCL_2darr_ldmcs(i64, <4 x i32>)
declare spir_func <4 x float> @__builtin_IB_OCL_2darr_ld2dms(i64, <4 x i32>, i32, <4 x float>)

!igc.functions = !{!0}
!IGCMetadata = !{!2}

!0 = !{ptr @test, !1}
!1 = !{}
!2 = !{!"ModuleMD", !3}
!3 = !{!"FuncMD", !4, !5}
!4 = distinct !{!"FuncMDMap[0]", ptr @test}
!5 = !{!"FuncMDValue[0]", !6}
!6 = !{!"resAllocMD", !7}
!7 = !{!"argAllocMDList", !8}
!8 = !{!"argAllocMDListVec[0]", !9, !10, !11}
!9 = !{!"type", i32 2}
!10 = !{!"extensionType", i32 0}
!11 = !{!"indexType", i32 0}

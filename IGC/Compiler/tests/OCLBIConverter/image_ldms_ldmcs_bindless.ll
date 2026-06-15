;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; Check that under bindless addressing the MSAA load built-ins (ldmcs, ld2dms)
; map the bindless image pointer directly into the resource operand instead of
; using a null BTI constant pointer.

; REQUIRES: llvm-14-plus
; RUN: igc_opt --opaque-pointers %s -S -o - -igc-conv-ocl-to-common | FileCheck %s

target datalayout = "e-p:32:32:32-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f16:16:16-f32:32:32-f64:64:64-f80:128:128-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024-a:64:64-f80:128:128-n8:16:32:64"

%spirv.Image._void_1_0_1_0_0_0_1 = type opaque

define spir_kernel void @test(ptr addrspace(1) %img_2darr) {
  %img_as_int = ptrtoint ptr addrspace(1) %img_2darr to i64
  %conv = trunc i64 %img_as_int to i32

  ; CHECK-LABEL: @test(

  ; CHECK-NOT: __builtin_IB_OCL_2darr_ldmcs
  ; CHECK: %[[MCS_IMG:.*]] = inttoptr i32 %conv to ptr addrspace(393468)
  ; CHECK: call <4 x float> @llvm.genx.GenISA.ldmcsptr.v4f32.i32.p393468(i32 %CoordX, i32 %CoordY, i32 %CoordZ, i32 0, ptr addrspace(393468) %[[MCS_IMG]], i32 0, i32 0, i32 0)
  %a = call spir_func <4 x float> @__builtin_IB_OCL_2darr_ldmcs(i32 %conv, <4 x i32> zeroinitializer)

  ; CHECK-NOT: __builtin_IB_OCL_2darr_ld2dms
  ; CHECK: %[[MS_IMG:.*]] = inttoptr i32 %conv to ptr addrspace(393468)
  ; CHECK: call <4 x float> @llvm.genx.GenISA.ldmsptr.v4f32.p393468(i32 0, i32 %imcsl, i32 %imcsh, i32 %{{.*}}, i32 %{{.*}}, i32 %{{.*}}, i32 0, ptr addrspace(393468) %[[MS_IMG]], i32 0, i32 0, i32 0)
  %b = call spir_func <4 x float> @__builtin_IB_OCL_2darr_ld2dms(i32 %conv, <4 x i32> zeroinitializer, i32 0, <4 x float> zeroinitializer)

  ret void
}

declare spir_func <4 x float> @__builtin_IB_OCL_2darr_ldmcs(i32, <4 x i32>)
declare spir_func <4 x float> @__builtin_IB_OCL_2darr_ld2dms(i32, <4 x i32>, i32, <4 x float>)

!igc.functions = !{!0}
!IGCMetadata = !{!2}

!0 = !{ptr @test, !1}
!1 = !{}
!2 = !{!"ModuleMD", !3, !12}
!3 = !{!"FuncMD", !4, !5}
!4 = distinct !{!"FuncMDMap[0]", ptr @test}
!5 = !{!"FuncMDValue[0]", !6}
!6 = !{!"resAllocMD", !7}
!7 = !{!"argAllocMDList", !8}
!8 = !{!"argAllocMDListVec[0]", !9, !10, !11}
!9 = !{!"type", i32 2}
!10 = !{!"extensionType", i32 0}
!11 = !{!"indexType", i32 0}
!12 = !{!"UseBindlessImage", i1 true}

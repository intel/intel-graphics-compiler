;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; Check bindless inline sampler is passed via implicit kernel argument.

; RUN: igc_opt --typed-pointers -igc-conv-ocl-to-common -S < %s -o - | FileCheck %s

target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024-n8:16:32"
target triple = "spir64-unknown-unknown"

%spirv.Image._void_1_0_0_0_0_0_0 = type opaque
%spirv.Sampler = type opaque

define spir_kernel void @test(%spirv.Image._void_1_0_0_0_0_0_0 addrspace(1)* %srcImg, i32 %inlineSampler) {
entry:
; CHECK: [[ZEXT:%[0-9]+]] = zext i32 %inlineSampler to i64
; CHECK: [[OR:%[0-9]+]] = or i64 [[ZEXT]], 1
; CHECK: [[TRUNC:%[0-9]+]] = trunc i64 [[OR]] to i32
; CHECK: [[TRUNC_IMG:%[0-9]+]] = trunc i64 %1 to i32
; CHECK: %bindless_img = inttoptr i32 [[TRUNC_IMG]] to float addrspace(393468)*
; CHECK: %bindless_sampler = inttoptr i32 [[TRUNC]] to float addrspace(655360)*
; CHECK: = call <4 x float> @llvm.genx.GenISA.sampleLptr.v4f32.f32.p196610f32.p393468f32.p655360f32(float 0.000000e+00, float %CoordX, float %CoordY, float 0.000000e+00, float 0.000000e+00, float addrspace(196610)* undef, float addrspace(393468)* %bindless_img, float addrspace(655360)* %bindless_sampler, i32 0, i32 0, i32 0)

  %0 = inttoptr i64 16 to %spirv.Sampler addrspace(2)*
  %1 = ptrtoint %spirv.Image._void_1_0_0_0_0_0_0 addrspace(1)* %srcImg to i64
  %2 = ptrtoint %spirv.Sampler addrspace(2)* %0 to i64
  %3 = or i64 %2, 1
  %4 = trunc i64 %3 to i32
  %5 = trunc i64 %1 to i32
  %6 = call spir_func <4 x float> @__builtin_IB_OCL_2d_sample_l(i32 %5, i32 %4, <2 x float> zeroinitializer, float 0.000000e+00)
  ret void
}

declare spir_func <4 x float> @__builtin_IB_OCL_2d_sample_l(i32, i32, <2 x float>, float)

!igc.functions = !{!0}
!IGCMetadata = !{!5}

!0 = !{void (%spirv.Image._void_1_0_0_0_0_0_0 addrspace(1)*, i32)* @test, !1}
!1 = !{!2}
!2 = !{!"implicit_arg_desc", !3}
!3 = !{i32 33, !4}
!4 = !{!"explicit_arg_num", i32 16}
!5 = !{!"ModuleMD", !6, !20}
!6 = !{!"FuncMD", !7, !8}
!7 = !{!"FuncMDMap[0]", void (%spirv.Image._void_1_0_0_0_0_0_0 addrspace(1)*, i32)* @test}
!8 = !{!"FuncMDValue[0]", !9}
!9 = !{!"resAllocMD", !10, !19}
!10 = !{!"argAllocMDList", !11, !15}
!11 = !{!"argAllocMDListVec[0]", !12, !13, !14}
!12 = !{!"type", i32 4}
!13 = !{!"extensionType", i32 0}
!14 = !{!"indexType", i32 2}
!15 = !{!"argAllocMDListVec[1]", !16, !17, !18}
!16 = !{!"type", i32 0}
!17 = !{!"extensionType", i32 -1}
!18 = !{!"indexType", i32 -1}
!19 = !{!"inlineSamplersMD"}
!20 = !{!"UseBindlessImage", i1 true}

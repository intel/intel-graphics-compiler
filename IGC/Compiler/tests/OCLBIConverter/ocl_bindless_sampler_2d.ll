;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; Check that bindless image and sampler are passed via kernel arguments.

; REQUIRES: llvm-14-plus
; RUN: igc_opt --opaque-pointers %s -S -o - -igc-conv-ocl-to-common | FileCheck %s

%spirv.Image._void_1_0_0_0_0_0_0 = type opaque
%spirv.Sampler = type opaque

define spir_kernel void @image_read_sampler(ptr addrspace(1) %img, ptr addrspace(2) %sampler) {
entry:
  %0 = ptrtoint ptr addrspace(1) %img to i64
  %1 = ptrtoint ptr addrspace(2) %sampler to i64
  %conv = trunc i64 %0 to i32
  %conv2 = trunc i64 %1 to i32

; CHECK: %bindless_img = inttoptr i32 %conv to ptr addrspace(393468)
; CHECK-NEXT: %bindless_sampler = inttoptr i32 %conv2 to ptr addrspace(655612)
; CHECK-NEXT: call <4 x float> @llvm.genx.GenISA.sampleLptr.v4f32.f32.p196860.p393468.p655612(float 0.000000e+00, float %CoordX, float %CoordY, float 0.000000e+00, float 0.000000e+00, ptr addrspace(196860) undef, ptr addrspace(393468) %bindless_img, ptr addrspace(655612) %bindless_sampler, i32 0, i32 0, i32 0)

  %call = call spir_func <4 x float> @__builtin_IB_OCL_2d_sample_l(i32 %conv, i32 %conv2, <2 x float> zeroinitializer, float 0.000000e+00)
  ret void
}

declare spir_func <4 x float> @__builtin_IB_OCL_2d_sample_l(i32, i32, <2 x float>, float)

!igc.functions = !{!0}
!IGCMetadata = !{!2}

!0 = !{ptr @image_read_sampler, !1}
!1 = !{}
!2 = !{!"ModuleMD", !3, !19}
!3 = !{!"FuncMD", !4, !5}
!4 = !{!"FuncMDMap[0]", ptr @image_read_sampler}
!5 = !{!"FuncMDValue[0]", !6}
!6 = !{!"resAllocMD", !7, !8, !9, !10}
!7 = !{!"uavsNumType", i32 3}
!8 = !{!"srvsNumType", i32 0}
!9 = !{!"samplersNumType", i32 1}
!10 = !{!"argAllocMDList", !11, !15}
!11 = !{!"argAllocMDListVec[0]", !12, !13, !14}
!12 = !{!"type", i32 4}
!13 = !{!"extensionType", i32 0}
!14 = !{!"indexType", i32 2}
!15 = !{!"argAllocMDListVec[1]", !16, !17, !18}
!16 = !{!"type", i32 5}
!17 = !{!"extensionType", i32 -1}
!18 = !{!"indexType", i32 0}
!19 = !{!"UseBindlessImage", i1 true}

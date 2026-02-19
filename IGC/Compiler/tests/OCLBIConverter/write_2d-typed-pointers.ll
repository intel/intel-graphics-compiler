;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022-2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; UNSUPPORTED: llvm-17-plus
; RUN: igc_opt --typed-pointers %s -S -o - -igc-conv-ocl-to-common | FileCheck %s

target datalayout = "e-p:32:32:32-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f16:16:16-f32:32:32-f64:64:64-f80:128:128-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024-a:64:64-f80:128:128-n8:16:32:64"

%spirv.Image._void_1_0_0_0_0_0_1 = type opaque
%class = type { i16 addrspace(4)*, i32, i8, %"accessor" }
%"accessor" = type { %"image_accessor" }
%"image_accessor" = type { %spirv.Image._void_1_0_0_0_0_0_1 addrspace(1)*, [24 x i8] }

define spir_kernel void @kernel(%spirv.Image._void_1_0_0_0_0_0_1 addrspace(1)* %image_2d) {
  %data = alloca %class, align 8
  %gep = getelementptr inbounds %class, %class* %data, i64 0, i32 3, i32 0, i32 0
  store %spirv.Image._void_1_0_0_0_0_0_1 addrspace(1)* %image_2d, %spirv.Image._void_1_0_0_0_0_0_1 addrspace(1)** %gep, align 8
  %img = load %spirv.Image._void_1_0_0_0_0_0_1 addrspace(1)*, %spirv.Image._void_1_0_0_0_0_0_1 addrspace(1)** %gep, align 8
  %img_as_int = ptrtoint %spirv.Image._void_1_0_0_0_0_0_1 addrspace(1)* %img to i64
  %trunc = trunc i64 %img_as_int to i32
  ; CHECK-NOT: __builtin_IB_write_2d_ui
  ; CHECK: %[[IMG:.*]] = call float addrspace(196608)* @llvm.genx.GenISA.GetBufferPtr.p196608f32(i32 0, i32 2)
  ; CHECK: call void @llvm.genx.GenISA.typedwrite.p196608f32(float addrspace(196608)* %[[IMG]], i32 %{{.*}}, i32 %{{.*}}, i32 0, i32 0, float %{{.*}}, float %{{.*}}, float %{{.*}}, float %{{.*}})
  call spir_func void @__builtin_IB_write_2d_u4i(i32 %trunc, <2 x i32> zeroinitializer, <4 x i32> zeroinitializer, i32 0)

  ret void
}

declare spir_func void @__builtin_IB_write_2d_u4i(i32, <2 x i32>, <4 x i32>, i32)

!igc.functions = !{!0}
!IGCMetadata = !{!3}

!0 = !{void (%spirv.Image._void_1_0_0_0_0_0_1 addrspace(1)*)* @kernel, !1}
!1 = !{!2}
!2 = !{!"function_type", i32 0}
!3 = !{!"ModuleMD", !4}
!4 = !{!"FuncMD", !5, !6}
!5 = distinct !{!"FuncMDMap[0]", void (%spirv.Image._void_1_0_0_0_0_0_1 addrspace(1)*)* @kernel}
!6 = !{!"FuncMDValue[0]", !7, !8, !9}
!7 = !{!"funcArgs"}
!8 = !{!"functionType", !"KernelFunction"}
!9 = !{!"resAllocMD", !10}
!10 = !{!"argAllocMDList", !11}
!11 = !{!"argAllocMDListVec[0]", !12, !13, !14}
!12 = !{!"type", i32 2}
!13 = !{!"extensionType", i32 0}
!14 = !{!"indexType", i32 0}

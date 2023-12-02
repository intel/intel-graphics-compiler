;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2023 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; Test checks that built-ins like __builtin_IB_get_image_width
; are replaced with resinfoptr usage for bindless images

; RUN: igc_opt %s -S -o - -igc-conv-ocl-to-common | FileCheck %s

%spirv.Image._void_1_0_0_0_0_0_0 = type opaque

define spir_kernel void @kernel(%spirv.Image._void_1_0_0_0_0_0_0 addrspace(1)* %img) {
; CHECK-LABEL: define spir_kernel void @kernel(
; CHECK-SAME: [[SPIRV_IMAGE__VOID_1_0_0_0_0_0_0:%.*]] addrspace(1)* [[IMG:%.*]]) {
  %data = ptrtoint %spirv.Image._void_1_0_0_0_0_0_0 addrspace(1)* %img to i64
  %1 = trunc i64 %data to i32
; CHECK-NOT: __builtin_IB_get_image_width
; CHECK-NOT: __builtin_IB_get_image_height
; CHECK:    [[BINDLESS_IMG:%.*]] = addrspacecast [[SPIRV_IMAGE__VOID_1_0_0_0_0_0_0]] addrspace(1)* [[IMG]] to float addrspace(393216)*
; CHECK-NEXT:    [[TMP2:%.*]] = call <4 x i32> @llvm.genx.GenISA.resinfoptr.p393216f32(float addrspace(393216)* [[BINDLESS_IMG]], i32 0)
; CHECK-NEXT:    [[TMP3:%.*]] = extractelement <4 x i32> [[TMP2]], i32 0
; CHECK-NEXT:    [[BINDLESS_IMG1:%.*]] = addrspacecast [[SPIRV_IMAGE__VOID_1_0_0_0_0_0_0]] addrspace(1)* [[IMG]] to float addrspace(393216)*
; CHECK-NEXT:    [[TMP4:%.*]] = call <4 x i32> @llvm.genx.GenISA.resinfoptr.p393216f32(float addrspace(393216)* [[BINDLESS_IMG1]], i32 0)
; CHECK-NEXT:    [[TMP5:%.*]] = extractelement <4 x i32> [[TMP4]], i32 1
; CHECK-NEXT:    [[VECINIT_I_I_I:%.*]] = insertelement <2 x i32> undef, i32 [[TMP3]], i64 0
; CHECK-NEXT:    [[VECINIT2_I_I_I:%.*]] = insertelement <2 x i32> [[VECINIT_I_I_I]], i32 [[TMP5]], i64 1
  %call.i.i.i = call spir_func i32 @__builtin_IB_get_image_width(i32 %1)
  %call1.i.i.i = call spir_func i32 @__builtin_IB_get_image_height(i32 %1)
  %vecinit.i.i.i = insertelement <2 x i32> undef, i32 %call.i.i.i, i64 0
  %vecinit2.i.i.i = insertelement <2 x i32> %vecinit.i.i.i, i32 %call1.i.i.i, i64 1
  ret void
}

declare spir_func i32 @__builtin_IB_get_image_width(i32)
declare spir_func i32 @__builtin_IB_get_image_height(i32)

!igc.functions = !{!0}
!IGCMetadata = !{!3}

!0 = !{void (%spirv.Image._void_1_0_0_0_0_0_0 addrspace(1)*)* @kernel, !1}
!1 = !{!2}
!2 = !{!"function_type", i32 0}
!3 = !{!"ModuleMD", !4, !15, !18}
!4 = !{!"FuncMD", !5, !6}
!5 = distinct !{!"FuncMDMap[0]", void (%spirv.Image._void_1_0_0_0_0_0_0 addrspace(1)*)* @kernel}
!6 = !{!"FuncMDValue[0]", !7, !8, !9}
!7 = !{!"funcArgs"}
!8 = !{!"functionType", !"KernelFunction"}
!9 = !{!"resAllocMD", !10}
!10 = !{!"argAllocMDList", !11}
!11 = !{!"argAllocMDListVec[0]", !12, !13, !14}
!12 = !{!"type", i32 4}
!13 = !{!"extensionType", i32 0}
!14 = !{!"indexType", i32 0}
!15 = !{!"compOpt", !16, !17}
!16 = !{!"UseBindlessMode", i1 true}
!17 = !{!"UseLegacyBindlessMode", i1 false}
!18 = !{!"UseBindlessImage", i1 true}

;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2023-2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; Test checks that built-ins like __builtin_IB_get_image_width
; are replaced with resinfoptr usage for bindless images

; REQUIRES: llvm-14-plus
; RUN: igc_opt --opaque-pointers %s -S -o - -igc-conv-ocl-to-common | FileCheck %s

%spirv.Image._void_0_0_1_0_0_0_0 = type opaque
%spirv.Image._void_1_0_1_0_0_0_0 = type opaque
%spirv.Image._void_2_0_0_0_0_0_0 = type opaque

define spir_kernel void @kernel_1d_array(ptr addrspace(1) %img) {
; CHECK-LABEL: define spir_kernel void @kernel_1d_array(
; CHECK-NOT:   __builtin_IB_get_image1d_array_size
; CHECK:       [[BINDLESS_IMG_1D:%.*]] = inttoptr i32 %1 to ptr addrspace(393468)
; CHECK-NEXT:  [[TMP_1D:%.*]] = call <4 x i32> @llvm.genx.GenISA.resinfoptr.p393468(ptr addrspace(393468) [[BINDLESS_IMG_1D]], i32 0)
; CHECK-NEXT:  extractelement <4 x i32> [[TMP_1D]], i32 1
  %data = ptrtoint ptr addrspace(1) %img to i64
  %1 = trunc i64 %data to i32
  %call = call i32 @__builtin_IB_get_image1d_array_size(i32 %1)
  ret void
}

define spir_kernel void @kernel_2d_array(ptr addrspace(1) %img) {
; CHECK-LABEL: define spir_kernel void @kernel_2d_array(
; CHECK-NOT:   __builtin_IB_get_image1d_array_size
; CHECK:       [[BINDLESS_IMG_1D:%.*]] = inttoptr i32 %1 to ptr addrspace(393468)
; CHECK-NEXT:  [[TMP_1D:%.*]] = call <4 x i32> @llvm.genx.GenISA.resinfoptr.p393468(ptr addrspace(393468) [[BINDLESS_IMG_1D]], i32 0)
; CHECK-NEXT:  extractelement <4 x i32> [[TMP_1D]], i32 2
  %data = ptrtoint ptr addrspace(1) %img to i64
  %1 = trunc i64 %data to i32
  %call = call i32 @__builtin_IB_get_image2d_array_size(i32 %1)
  ret void
}

define spir_kernel void @kernel_3d(ptr addrspace(1) %img) {
; CHECK-LABEL: define spir_kernel void @kernel_3d(
  %data = ptrtoint ptr addrspace(1) %img to i64
  %1 = trunc i64 %data to i32
; CHECK-NOT: __builtin_IB_get_image_width
; CHECK-NOT: __builtin_IB_get_image_height
; CHECK-NOT: __builtin_IB_get_image_height
; CHECK:    [[BINDLESS_IMG:%.*]] = inttoptr i32 %1 to ptr addrspace(393468)
; CHECK-NEXT:    [[TMP1:%.*]] = call <4 x i32> @llvm.genx.GenISA.resinfoptr.p393468(ptr addrspace(393468) [[BINDLESS_IMG]], i32 0)
; CHECK-NEXT:    extractelement <4 x i32> [[TMP1]], i32 0
; CHECK-NEXT:    [[BINDLESS_IMG1:%.*]] = inttoptr i32 %1 to ptr addrspace(393468)
; CHECK-NEXT:    [[TMP2:%.*]] = call <4 x i32> @llvm.genx.GenISA.resinfoptr.p393468(ptr addrspace(393468) [[BINDLESS_IMG1]], i32 0)
; CHECK-NEXT:    extractelement <4 x i32> [[TMP2]], i32 1
; CHECK-NEXT:    [[BINDLESS_IMG2:%.*]] = inttoptr i32 %1 to ptr addrspace(393468)
; CHECK-NEXT:    [[TMP3:%.*]] = call <4 x i32> @llvm.genx.GenISA.resinfoptr.p393468(ptr addrspace(393468) [[BINDLESS_IMG2]], i32 0)
; CHECK-NEXT:    extractelement <4 x i32> [[TMP3]], i32 2
  %call = call spir_func i32 @__builtin_IB_get_image_width(i32 %1)
  %call1 = call spir_func i32 @__builtin_IB_get_image_height(i32 %1)
  %call2 = call spir_func i32 @__builtin_IB_get_image_depth(i32 %1)
  ret void
}

declare spir_func i32 @__builtin_IB_get_image_width(i32)
declare spir_func i32 @__builtin_IB_get_image_height(i32)
declare spir_func i32 @__builtin_IB_get_image_depth(i32)
declare spir_func i32 @__builtin_IB_get_image1d_array_size(i32 %img)
declare spir_func i32 @__builtin_IB_get_image2d_array_size(i32 %img)


!igc.functions = !{!0, !19, !23}
!IGCMetadata = !{!3}

!0 = !{ptr @kernel_3d, !1}
!1 = !{!2}
!2 = !{!"function_type", i32 0}
!3 = !{!"ModuleMD", !4, !20, !24, !15, !18}
!4 = !{!"FuncMD", !5, !6, !21, !22, !25, !26}
!5 = distinct !{!"FuncMDMap[0]", ptr @kernel_3d}
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
!19 = !{ptr @kernel_1d_array, !1}
!20 = !{!"FuncMD", !21, !22}
!21 = distinct !{!"FuncMDMap[0]", ptr @kernel_1d_array}
!22 = !{!"FuncMDValue[0]", !7, !8, !9}
!23 = !{ptr @kernel_2d_array, !1}
!24 = !{!"FuncMD", !25, !26}
!25 = distinct !{!"FuncMDMap[0]", ptr @kernel_2d_array}
!26 = !{!"FuncMDValue[0]", !7, !8, !9}

;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2017-2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: igc_opt --typed-pointers -igc-image-func-analysis -igc-serialize-metadata -S %s -o %t.ll
; RUN: FileCheck %s --input-file=%t.ll

%spirv.Image._void_1_0_0_0_0_0_0 = type opaque

declare i32 @__builtin_IB_get_image_width(i32)

define i32 @foo(%spirv.Image._void_1_0_0_0_0_0_0 addrspace(1)* %img1, %spirv.Image._void_1_0_0_0_0_0_0 addrspace(1)* %img2) nounwind {
  %1 = ptrtoint %spirv.Image._void_1_0_0_0_0_0_0 addrspace(1)* %img1 to i64
  %2 = trunc i64 %1 to i32
  %3 = ptrtoint %spirv.Image._void_1_0_0_0_0_0_0 addrspace(1)* %img2 to i64
  %4 = trunc i64 %3 to i32
  %id1 = call i32 @__builtin_IB_get_image_width(i32 %2)
  %id2 = call i32 @__builtin_IB_get_image_width(i32 %4)
  %res = add i32 %id1, %id2
  ret i32 %res
}

!igc.functions = !{!0}
!0 = !{i32 (%spirv.Image._void_1_0_0_0_0_0_0 addrspace(1)*, %spirv.Image._void_1_0_0_0_0_0_0 addrspace(1)*)* @foo, !1}
!1 = !{!2}
!2 = !{!"function_type", i32 0}

; Both images need an IMAGE_WIDTH implicit arg (argId 22); the two list entries
; share the single deduplicated argId node.
;CHECK: !{!"implicitArgInfoList", ![[V0:[0-9]+]], ![[V1:[0-9]+]]}
;CHECK: ![[V0]] = !{!"implicitArgInfoListVec[0]", ![[ARGID:[0-9]+]],
;CHECK: ![[ARGID]] = !{!"argId", i32 22}
;CHECK: ![[V1]] = !{!"implicitArgInfoListVec[1]", ![[ARGID]],

; The following metadata are needed to recognize functions using image/sampler arguments:
!IGCMetadata = !{!4}
!4 = !{!"ModuleMD", !5}
!5 = !{!"FuncMD", !6, !7}
!6 = !{!"FuncMDMap[0]", i32 (%spirv.Image._void_1_0_0_0_0_0_0 addrspace(1)*, %spirv.Image._void_1_0_0_0_0_0_0 addrspace(1)*)* @foo}
!7 = !{!"FuncMDValue[0]", !8}
!8 = !{!"m_OpenCLArgBaseTypes", !9, !10}
!9 = !{!"m_OpenCLArgBaseTypesVec[0]", !"image2d_t"}
!10 = !{!"m_OpenCLArgBaseTypesVec[1]", !"image2d_t"}

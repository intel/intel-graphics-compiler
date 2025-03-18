;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2017-2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: igc_opt --typed-pointers -igc-image-func-analysis -S %s -o %t.ll
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
!1 = !{!2, !3}
!2 = !{!"function_type", i32 0}
!3 = !{!"implicit_arg_desc"}

;CHECK: !{!"implicit_arg_desc", ![[A1:[0-9]+]], ![[A3:[0-9]+]]}
;CHECK: ![[A1]] = !{i32 22, ![[A2:[0-9]+]]}
;CHECK: ![[A2]] = !{!"explicit_arg_num", i32 0}
;CHECK: ![[A3]] = !{i32 22, ![[A4:[0-9]+]]}
;CHECK: ![[A4]] = !{!"explicit_arg_num", i32 1}



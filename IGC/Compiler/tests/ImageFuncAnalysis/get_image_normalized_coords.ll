;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2017-2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: igc_opt --typed-pointers -igc-image-func-analysis -S %s -o %t.ll
; RUN: FileCheck %s --input-file=%t.ll

%spirv.Sampler = type opaque

declare i32 @__builtin_IB_is_normalized_coords(i32)

define i32 @foo(%spirv.Sampler addrspace(2)* %sampler) nounwind {
  %1 = ptrtoint %spirv.Sampler addrspace(2)* %sampler to i64
  %2 = trunc i64 %1 to i32
  %id = call i32 @__builtin_IB_is_normalized_coords(i32 %2)
  ret i32 %id
}

!igc.functions = !{!0}
!0 = !{i32 (%spirv.Sampler addrspace(2)*)* @foo, !1}
!1 = !{!2, !3}
!2 = !{!"function_type", i32 0}
!3 = !{!"implicit_arg_desc"}

;CHECK: !{!"implicit_arg_desc", ![[A1:[0-9]+]]}
;CHECK: ![[A1]] = !{i32 31, ![[A2:[0-9]+]]}
;CHECK: ![[A2]] = !{!"explicit_arg_num", i32 0}



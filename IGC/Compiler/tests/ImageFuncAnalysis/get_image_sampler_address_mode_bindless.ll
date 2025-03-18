;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; Check new implicit arg is added for __builtin_IB_get_address_mode when
; sampler is bindless (offset is or'ed with 1).

; REQUIRES: opaque-ptr-fix, llvm-14-plus
; RUN: igc_opt --opaque-pointers -igc-image-func-analysis -S %s -o %t.ll
; RUN: FileCheck %s --input-file=%t.ll

%spirv.Sampler = type opaque

declare i32 @__builtin_IB_get_address_mode(i32)

define i32 @foo(%spirv.Sampler addrspace(2)* %sampler) nounwind {
  %1 = ptrtoint %spirv.Sampler addrspace(2)* %sampler to i64
  %2 = or i64 %1, 1
  %3 = trunc i64 %2 to i32
  %id = call i32 @__builtin_IB_get_address_mode(i32 %3)
  ret i32 %id
}

!igc.functions = !{!0}
!0 = !{i32 (%spirv.Sampler addrspace(2)*)* @foo, !1}
!1 = !{!2, !3}
!2 = !{!"function_type", i32 0}
!3 = !{!"implicit_arg_desc"}

;CHECK: !{!"implicit_arg_desc", ![[A1:[0-9]+]]}
;CHECK: ![[A1]] = !{i32 30, ![[A2:[0-9]+]]}
;CHECK: ![[A2]] = !{!"explicit_arg_num", i32 0}

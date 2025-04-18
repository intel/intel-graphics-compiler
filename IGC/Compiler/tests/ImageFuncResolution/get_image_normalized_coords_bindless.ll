;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; Check __builtin_IB_is_normalized_coords is replaced with implicit arg %smpNormalized
; for bindless sampler (offset is or'ed with 1).

; REQUIRES: llvm-14-plus
; RUN: igc_opt --opaque-pointers -igc-image-func-resolution -S %s -o %t.ll
; RUN: FileCheck %s --input-file=%t.ll

%opencl.image2d_t = type opaque
%spirv.Sampler = type opaque

declare i32 @__builtin_IB_is_normalized_coords(i32 %sampler)

define i32 @foo(ptr addrspace(2) %sampler, i32 %smpNormalized) nounwind {
  %1 = ptrtoint ptr addrspace(2) %sampler to i64
  %2 = or i64 %1, 1
  %3 = trunc i64 %2 to i32
  %id = call i32 @__builtin_IB_is_normalized_coords(i32 %3)
  ret i32 %id
}

!igc.input.ir = !{!100}
!100 = !{!"ocl", i32 1, i32 2}

!igc.functions = !{!0}
!0 = !{ptr @foo, !1}
!1 = !{!2, !3}
!2 = !{!"function_type", i32 0}
!3 = !{!"implicit_arg_desc", !4}
!4 = !{i32 31, !5}
!5 = !{!"explicit_arg_num", i32 0}

; CHECK:         ret i32 %smpNormalized

; CHECK-NOT:     call i32 @__builtin_IB_is_normalized_coords(i32 %sampler)

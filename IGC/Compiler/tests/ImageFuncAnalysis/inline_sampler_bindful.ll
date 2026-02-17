;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; Check new implicit arg is not added for bindful inline sampler.

; REQUIRES: llvm-14-plus
; RUN: igc_opt --opaque-pointers -igc-image-func-analysis -S %s -o - | FileCheck %s

%spirv.Sampler = type opaque

define spir_kernel void @test() {
entry:
  %0 = trunc i64 16 to i32
  %1 = call spir_func i32 @__builtin_IB_get_address_mode(i32 %0)
  ret void
}

declare spir_func i32 @__builtin_IB_get_address_mode(i32)

; CHECK-NOT: = !{i32 32, !{{[0-9]+}}}
; CHECK-NOT: = !{!"explicit_arg_num", i32 16}

!igc.functions = !{!0}

!0 = !{ptr @test, !1}
!1 = !{}

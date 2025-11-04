;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022-2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; REQUIRES: llvm-14-plus
; RUN: igc_opt --opaque-pointers -igc-priv-mem-to-reg -mem2reg -S < %s 2>&1 | FileCheck %s
; ------------------------------------------------
; LowerGEPForPrivMem
; ------------------------------------------------
; Test checks that LowerGEPForPrivMem applied to folded vector types

; CHECK-LABEL: @test(
; CHECK-NOT: alloca

%"class.sycl::_V1::vec" = type { %"class.sycl::_V1::detail::vec_base" }
%"class.sycl::_V1::detail::vec_base" = type { [4 x float] }

define void @test(<4 x i32> %a, ptr %b) {
  %1 = alloca %"class.sycl::_V1::vec", align 4, !uniform !5
  store <4 x i32> %a, ptr %1, align 16
  %2 = load <4 x i32>, ptr %1, align 16
  store <4 x i32> %2, ptr %b, align 16
  ret void
}

!igc.functions = !{!0}

!0 = !{ptr @test, !1}
!1 = !{!2, !3}
!2 = !{!"function_type", i32 0}
!3 = !{!"implicit_arg_desc"}
!4 = !{i32 8}
!5 = !{i1 true}


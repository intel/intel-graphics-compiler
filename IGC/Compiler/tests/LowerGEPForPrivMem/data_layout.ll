;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: igc_opt -igc-priv-mem-to-reg -S < %s 2>&1 | FileCheck %s
; ------------------------------------------------
; LowerGEPForPrivMem
; ------------------------------------------------
target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024-n8:16:32"

define void @test(<4 x i32> %a, <4 x i32>* %b) {
; CHECK-LABEL: @test(
; CHECK: insertelement <20 x float> {{.*}}, i{{[0-9]*}} 12
; CHECK: insertelement <20 x float> {{.*}}, i{{[0-9]*}} 13
; CHECK: insertelement <20 x float> {{.*}}, i{{[0-9]*}} 14
; CHECK: insertelement <20 x float> {{.*}}, i{{[0-9]*}} 15
; CHECK-NOT: insertelement <20 x float> {{.*}}, i{{[0-9]*}} 9
; CHECK-NOT: insertelement <20 x float> {{.*}}, i{{[0-9]*}} 10
; CHECK-NOT: insertelement <20 x float> {{.*}}, i{{[0-9]*}} 11
  %data = alloca [5 x <3 x float>]
  %gepf3 = getelementptr inbounds [5 x <3 x float>], [5 x <3 x float>]* %data, i64 0, i64 3
  %bcf4 = bitcast <3 x float>* %gepf3 to <4 x float>*
  %v0 = insertelement <4 x float> undef, float 1.000000e+00, i64 0
  %v1 = insertelement <4 x float> %v0, float 2.000000e+00, i64 1
  %v2 = insertelement <4 x float> %v1, float 3.000000e+00, i64 2
  store <4 x float> %v2, <4 x float>* %bcf4
  ret void
}

!igc.functions = !{!0}

!0 = !{void (<4 x i32>, <4 x i32>*)* @test, !1}
!1 = !{!2, !3}
!2 = !{!"function_type", i32 0}
!3 = !{!"implicit_arg_desc"}
!4 = !{i32 8}
!5 = !{i1 true}

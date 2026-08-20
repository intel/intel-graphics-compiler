;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: regkeys

; RUN: igc_opt -S -dce -platformbmg -igc-emit-visa --regkey=DumpVISAASMToConsole=1 -simd-mode 16 < %s | FileCheck %s

; CHECK-LABEL: .kernel "bitcast_i32_phi_to_float"
; CHECK: .decl vectorized_phi v_type=G type=d num_elts=128 align=wordx32
; CHECK: .decl vectorized_phi_0v v_type=G type=f num_elts=128 align=wordx32 alias=<vectorized_phi, 0>
; CHECK: mad (M1, 16) vectorized_phi_0v(0,0)<1> vectorized_phi_0v(0,0)<1;1,0> vector(0,0)<0;1,0> vectorized_phi_0v(0,0)<1;1,0>
; CHECK: mad (M1, 16) vectorized_phi_0v(1,0)<1> vectorized_phi_0v(1,0)<1;1,0> vector(0,1)<0;1,0> vectorized_phi_0v(1,0)<1;1,0>
; CHECK: mad (M1, 16) vectorized_phi_0v(2,0)<1> vectorized_phi_0v(2,0)<1;1,0> vector(0,2)<0;1,0> vectorized_phi_0v(2,0)<1;1,0>
; CHECK: mad (M1, 16) vectorized_phi_0v(3,0)<1> vectorized_phi_0v(3,0)<1;1,0> vector(0,3)<0;1,0> vectorized_phi_0v(3,0)<1;1,0>
; CHECK: mad (M1, 16) vectorized_phi_0v(4,0)<1> vectorized_phi_0v(4,0)<1;1,0> vector(0,4)<0;1,0> vectorized_phi_0v(4,0)<1;1,0>
; CHECK: mad (M1, 16) vectorized_phi_0v(5,0)<1> vectorized_phi_0v(5,0)<1;1,0> vector(0,5)<0;1,0> vectorized_phi_0v(5,0)<1;1,0>
; CHECK: mad (M1, 16) vectorized_phi_0v(6,0)<1> vectorized_phi_0v(6,0)<1;1,0> vector(0,6)<0;1,0> vectorized_phi_0v(6,0)<1;1,0>
; CHECK: mad (M1, 16) vectorized_phi_0v(7,0)<1> vectorized_phi_0v(7,0)<1;1,0> vector(0,7)<0;1,0> vectorized_phi_0v(7,0)<1;1,0>

define spir_kernel void @bitcast_i32_phi_to_float() {
  br label %loop

loop:
  %vectorized_phi = phi <8 x i32> [ zeroinitializer, %0 ], [ %backedge, %loop ]
  %vector = insertelement <8 x float> zeroinitializer, float 0.000000e+00, i64 0
  %vectorized_cast = bitcast <8 x i32> %vectorized_phi to <8 x float>
  %vectorized_binary = fmul fast <8 x float> %vector, %vectorized_cast
  %vectorized_binary_1 = fadd fast <8 x float> %vectorized_binary, %vectorized_cast
  %dpas = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> %vectorized_binary_1, <8 x i16> zeroinitializer, <8 x i32> zeroinitializer, i32 0, i32 0, i32 0, i32 0, i1 false)
  %backedge = bitcast <8 x float> %dpas to <8 x i32>
  br label %loop
}

; CHECK-LABEL: .kernel "bitcast_float_phi_to_i32"
; CHECK: .decl vectorized_phi v_type=G type=f num_elts=128 align=wordx32
; CHECK: .decl vectorized_phi_0v v_type=G type=d num_elts=128 align=wordx32 alias=<vectorized_phi, 0>
; CHECK: .decl vectorized_binary_0v v_type=G type=f num_elts=128 align=wordx32 alias=<vectorized_binary, 0>
; CHECK: add (M1, 16) vectorized_binary(0,0)<1> vector(0,0)<0;1,0> vectorized_phi_0v(0,0)<1;1,0>
; CHECK: add (M1, 16) vectorized_binary(1,0)<1> vector(0,1)<0;1,0> vectorized_phi_0v(1,0)<1;1,0>
; CHECK: add (M1, 16) vectorized_binary(2,0)<1> vector(0,2)<0;1,0> vectorized_phi_0v(2,0)<1;1,0>
; CHECK: add (M1, 16) vectorized_binary(3,0)<1> vector(0,3)<0;1,0> vectorized_phi_0v(3,0)<1;1,0>
; CHECK: add (M1, 16) vectorized_binary(4,0)<1> vector(0,4)<0;1,0> vectorized_phi_0v(4,0)<1;1,0>
; CHECK: add (M1, 16) vectorized_binary(5,0)<1> vector(0,5)<0;1,0> vectorized_phi_0v(5,0)<1;1,0>
; CHECK: add (M1, 16) vectorized_binary(6,0)<1> vector(0,6)<0;1,0> vectorized_phi_0v(6,0)<1;1,0>
; CHECK: add (M1, 16) vectorized_binary(7,0)<1> vector(0,7)<0;1,0> vectorized_phi_0v(7,0)<1;1,0>

define spir_kernel void @bitcast_float_phi_to_i32() {
  br label %loop

loop:
  %vectorized_phi = phi <8 x float> [ zeroinitializer, %0 ], [ %backedge, %loop ]
  %vector = insertelement <8 x i32> zeroinitializer, i32 1, i64 0
  %vectorized_cast = bitcast <8 x float> %vectorized_phi to <8 x i32>
  %vectorized_binary = add <8 x i32> %vector, %vectorized_cast
  %vectorized_cast_back = bitcast <8 x i32> %vectorized_binary to <8 x float>
  %dpas = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> %vectorized_cast_back, <8 x i16> zeroinitializer, <8 x i32> zeroinitializer, i32 0, i32 0, i32 0, i32 0, i1 false)
  %backedge = fadd fast <8 x float> %dpas, %vectorized_phi
  br label %loop
}

; CHECK-LABEL: .kernel "bitcast_shared_by_multiple_uses"
; CHECK: .decl vectorized_phi v_type=G type=d num_elts=128 align=wordx32
; CHECK: .decl vectorized_phi_0v v_type=G type=f num_elts=128 align=wordx32 alias=<vectorized_phi, 0>
; CHECK: mul (M1, 16) mul1(0,0)<1> vectorized_phi_0v(0,0)<1;1,0> vectorized_phi_0v(0,0)<1;1,0>
; CHECK: mad (M1, 16) vectorized_phi_0v(0,0)<1> vectorized_phi_0v(0,0)<1;1,0> vector(0,0)<0;1,0> mul1(0,0)<1;1,0>
; CHECK: mad (M1, 16) vectorized_phi_0v(1,0)<1> vectorized_phi_0v(1,0)<1;1,0> vector(0,1)<0;1,0> mul1(1,0)<1;1,0>
; CHECK: mad (M1, 16) vectorized_phi_0v(2,0)<1> vectorized_phi_0v(2,0)<1;1,0> vector(0,2)<0;1,0> mul1(2,0)<1;1,0>
; CHECK: mad (M1, 16) vectorized_phi_0v(3,0)<1> vectorized_phi_0v(3,0)<1;1,0> vector(0,3)<0;1,0> mul1(3,0)<1;1,0>
; CHECK: mad (M1, 16) vectorized_phi_0v(4,0)<1> vectorized_phi_0v(4,0)<1;1,0> vector(0,4)<0;1,0> mul1(4,0)<1;1,0>
; CHECK: mad (M1, 16) vectorized_phi_0v(5,0)<1> vectorized_phi_0v(5,0)<1;1,0> vector(0,5)<0;1,0> mul1(5,0)<1;1,0>
; CHECK: mad (M1, 16) vectorized_phi_0v(6,0)<1> vectorized_phi_0v(6,0)<1;1,0> vector(0,6)<0;1,0> mul1(6,0)<1;1,0>
; CHECK: mad (M1, 16) vectorized_phi_0v(7,0)<1> vectorized_phi_0v(7,0)<1;1,0> vector(0,7)<0;1,0> mul1(7,0)<1;1,0>

define spir_kernel void @bitcast_shared_by_multiple_uses() {
  br label %loop

loop:
  %vectorized_phi = phi <8 x i32> [ zeroinitializer, %0 ], [ %backedge, %loop ]
  %vector = insertelement <8 x float> zeroinitializer, float 2.000000e+00, i64 0
  %vectorized_cast = bitcast <8 x i32> %vectorized_phi to <8 x float>
  %mul0 = fmul fast <8 x float> %vector, %vectorized_cast
  %mul1 = fmul fast <8 x float> %vectorized_cast, %vectorized_cast
  %sum = fadd fast <8 x float> %mul0, %mul1
  %dpas = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> %sum, <8 x i16> zeroinitializer, <8 x i32> zeroinitializer, i32 0, i32 0, i32 0, i32 0, i1 false)
  %backedge = bitcast <8 x float> %dpas to <8 x i32>
  br label %loop
}

; CHECK-LABEL: .kernel "bitcast_relayout_varying_copy"
; CHECK: .decl dpas_ v_type=G type=f num_elts=128 align=wordx32
; CHECK: .decl relayout v_type=G type=w num_elts=128 align=wordx32
; CHECK: .decl dpas__1v v_type=G type=w num_elts=256 align=wordx32 alias=<dpas_, 0>

; CHECK: mov (M1, 16) relayout(0,0)<1> dpas__1v(0,0)<2;1,0>
; CHECK: mov (M1, 16) relayout(0,16)<1> dpas__1v(1,0)<2;1,0>
; CHECK: mov (M1, 16) relayout(1,0)<1> dpas__1v(2,0)<2;1,0>
; CHECK: mov (M1, 16) relayout(1,16)<1> dpas__1v(3,0)<2;1,0>
; CHECK: mov (M1, 16) relayout(2,0)<1> dpas__1v(4,0)<2;1,0>
; CHECK: mov (M1, 16) relayout(2,16)<1> dpas__1v(5,0)<2;1,0>
; CHECK: mov (M1, 16) relayout(3,0)<1> dpas__1v(6,0)<2;1,0>
; CHECK: mov (M1, 16) relayout(3,16)<1> dpas__1v(7,0)<2;1,0>

define spir_kernel void @bitcast_relayout_varying_copy() {
entry:
  br label %loop

loop:
  %vectorized_phi = phi <8 x i32> [ zeroinitializer, %entry ], [ %backedge, %loop ]
  %phi_f = bitcast <8 x i32> %vectorized_phi to <8 x float>
  %dpas = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> %phi_f, <8 x i16> zeroinitializer, <8 x i32> zeroinitializer, i32 0, i32 0, i32 0, i32 0, i1 false)
  %dpas_i = bitcast <8 x float> %dpas to <8 x i32>
  %relayout = bitcast <8 x i32> %dpas_i to <16 x i16>
  %e0 = extractelement <16 x i16> %relayout, i64 0
  %e1 = extractelement <16 x i16> %relayout, i64 2
  %e2 = extractelement <16 x i16> %relayout, i64 4
  %e3 = extractelement <16 x i16> %relayout, i64 6
  %e4 = extractelement <16 x i16> %relayout, i64 8
  %e5 = extractelement <16 x i16> %relayout, i64 10
  %e6 = extractelement <16 x i16> %relayout, i64 12
  %e7 = extractelement <16 x i16> %relayout, i64 14
  %i0 = insertelement <8 x i16> undef, i16 %e0, i64 0
  %i1 = insertelement <8 x i16> %i0, i16 %e1, i64 1
  %i2 = insertelement <8 x i16> %i1, i16 %e2, i64 2
  %i3 = insertelement <8 x i16> %i2, i16 %e3, i64 3
  %i4 = insertelement <8 x i16> %i3, i16 %e4, i64 4
  %i5 = insertelement <8 x i16> %i4, i16 %e5, i64 5
  %i6 = insertelement <8 x i16> %i5, i16 %e6, i64 6
  %lo = insertelement <8 x i16> %i6, i16 %e7, i64 7
  %widened = zext <8 x i16> %lo to <8 x i32>
  %backedge = add <8 x i32> %widened, %vectorized_phi
  br label %loop
}


; CHECK-LABEL: .kernel "bitcast_relayout_varying_copy_straight"
; CHECK: .decl dpas_ v_type=G type=f num_elts=128 align=wordx32
; CHECK: .decl relayout v_type=G type=w num_elts=128 align=wordx32
; CHECK: .decl dpas__1v v_type=G type=w num_elts=256 align=wordx32 alias=<dpas_, 0>

; CHECK: mov (M1, 16) relayout(0,0)<1> dpas__1v(0,0)<2;1,0>
; CHECK: mov (M1, 16) relayout(0,16)<1> dpas__1v(0,1)<2;1,0>
; CHECK: mov (M1, 16) relayout(1,0)<1> dpas__1v(1,0)<2;1,0>
; CHECK: mov (M1, 16) relayout(1,16)<1> dpas__1v(1,1)<2;1,0>
; CHECK: mov (M1, 16) relayout(2,0)<1> dpas__1v(2,0)<2;1,0>
; CHECK: mov (M1, 16) relayout(2,16)<1> dpas__1v(2,1)<2;1,0>
; CHECK: mov (M1, 16) relayout(3,0)<1> dpas__1v(3,0)<2;1,0>
; CHECK: mov (M1, 16) relayout(3,16)<1> dpas__1v(3,1)<2;1,0>


define spir_kernel void @bitcast_relayout_varying_copy_straight() {
entry:
  br label %loop

loop:
  %vectorized_phi = phi <8 x i32> [ zeroinitializer, %entry ], [ %backedge, %loop ]
  %phi_f = bitcast <8 x i32> %vectorized_phi to <8 x float>
  %dpas = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> %phi_f, <8 x i16> zeroinitializer, <8 x i32> zeroinitializer, i32 0, i32 0, i32 0, i32 0, i1 false)
  %dpas_i = bitcast <8 x float> %dpas to <8 x i32>
  %relayout = bitcast <8 x i32> %dpas_i to <16 x i16>
  %e0 = extractelement <16 x i16> %relayout, i64 0
  %e1 = extractelement <16 x i16> %relayout, i64 1
  %e2 = extractelement <16 x i16> %relayout, i64 2
  %e3 = extractelement <16 x i16> %relayout, i64 3
  %e4 = extractelement <16 x i16> %relayout, i64 4
  %e5 = extractelement <16 x i16> %relayout, i64 5
  %e6 = extractelement <16 x i16> %relayout, i64 6
  %e7 = extractelement <16 x i16> %relayout, i64 7
  %i0 = insertelement <8 x i16> undef, i16 %e0, i64 0
  %i1 = insertelement <8 x i16> %i0, i16 %e1, i64 1
  %i2 = insertelement <8 x i16> %i1, i16 %e2, i64 2
  %i3 = insertelement <8 x i16> %i2, i16 %e3, i64 3
  %i4 = insertelement <8 x i16> %i3, i16 %e4, i64 4
  %i5 = insertelement <8 x i16> %i4, i16 %e5, i64 5
  %i6 = insertelement <8 x i16> %i5, i16 %e6, i64 6
  %lo = insertelement <8 x i16> %i6, i16 %e7, i64 7
  %widened = zext <8 x i16> %lo to <8 x i32>
  %backedge = add <8 x i32> %widened, %vectorized_phi
  br label %loop
}

declare <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float>, <8 x i16>, <8 x i32>, i32, i32, i32, i32, i1)

!igc.functions = !{!0, !4, !6, !8, !10}
!IGCMetadata = !{!100}

!0 = distinct !{void ()* @bitcast_i32_phi_to_float, !1}
!1 = distinct !{!2}
!2 = distinct !{!"function_type", i32 0}
!4 = distinct !{void ()* @bitcast_float_phi_to_i32, !1}
!6 = distinct !{void ()* @bitcast_shared_by_multiple_uses, !1}
!8 = distinct !{void ()* @bitcast_relayout_varying_copy, !1}
!10 = distinct !{void ()* @bitcast_relayout_varying_copy_straight, !1}

!100 = distinct !{!"ModuleMD", !101}
!101 = distinct !{!"FuncMD", !110, !111, !120, !121, !130, !131, !135, !136, !138, !139}
!110 = distinct !{!"FuncMDMap[0]", void ()* @bitcast_i32_phi_to_float}
!111 = distinct !{!"FuncMDValue[0]", !140}
!120 = distinct !{!"FuncMDMap[1]", void ()* @bitcast_float_phi_to_i32}
!121 = distinct !{!"FuncMDValue[1]", !140}
!130 = distinct !{!"FuncMDMap[2]", void ()* @bitcast_shared_by_multiple_uses}
!131 = distinct !{!"FuncMDValue[2]", !140}
!135 = distinct !{!"FuncMDMap[3]", void ()* @bitcast_relayout_varying_copy}
!136 = distinct !{!"FuncMDValue[3]", !140}
!138 = distinct !{!"FuncMDMap[4]", void ()* @bitcast_relayout_varying_copy_straight}
!139 = distinct !{!"FuncMDValue[4]", !140}
!140 = distinct !{!"requiredSubGroupSize", i32 16}

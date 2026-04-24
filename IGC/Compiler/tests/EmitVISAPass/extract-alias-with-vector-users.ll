;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
; REQUIRES: regkeys, llvm-14-plus
; RUN: igc_opt -S %s --opaque-pointers -dce -platformbmg -igc-emit-visa --regkey=DumpVISAASMToConsole=1 -simd-mode 16 &> %t_output.ll
; RUN: FileCheck --input-file %t_output.ll %s

; Test that extractelement can be aliased as a sub-register even when the
; source vector has non-extract users (fdiv), provided those users are
; vector-level read-only operations that consume the vector as a whole via
; GetSymbol().
;
; Pattern:
;   %vec  = select <4 x float>
;   %vdiv = fdiv <4 x float> %num, %vec    ; non-extract user (allowed)
;   %e0   = extractelement %vec, 0         ; aliased to vec(0,0)
;   %bc0  = bitcast float %e0 to i32       ; noop-alias in DeSSA
;   %i0   = add i32 %bc0, 1                ; integer consumer via alias
;
; Expected: no extraction MOVs from vec. Integer adds read vec's typed
; alias directly. Float MADs read vec sub-registers directly.

target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024-n8:16:32"
target triple = "spir64-unknown-unknown"

declare <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float>, <8 x i16>, <8 x i32>, i32, i32, i32, i32, i1)

; Verify no scalar extraction MOVs from vec
; CHECK-NOT: mov (M1_NM, 1) {{.*}}(0,0)<1> vec(0,
; Verify div reads vec sub-registers directly
; CHECK: div (M1_NM, 1) vdiv(0,0)<1> numerator(0,0)<0;1,0> vec(0,0)<0;1,0>
; Verify integer adds read vec's typed alias (vec_0v) directly
; CHECK: add (M1_NM, 1) i0(0,0)<1> vec_0v(0,0)<0;1,0>
; CHECK: add (M1_NM, 1) i1(0,0)<1> vec_0v(0,1)<0;1,0>
; CHECK: add (M1_NM, 1) i2(0,0)<1> vec_0v(0,2)<0;1,0>
; CHECK: add (M1_NM, 1) i3(0,0)<1> vec_0v(0,3)<0;1,0>
; Verify float MADs read vec sub-registers directly
; CHECK-NOT: mov (M1_NM, 1) {{.*}}(0,0)<1> vec(0,
; CHECK: mad (M1_NM, 1) {{.*}} vec(0,0)<0;1,0> vec(0,0)<0;1,0>
; CHECK: mad (M1_NM, 1) {{.*}} vec(0,1)<0;1,0> vec(0,1)<0;1,0>
; CHECK: mad (M1_NM, 1) {{.*}} vec(0,2)<0;1,0> vec(0,2)<0;1,0>
; CHECK: mad (M1_NM, 1) {{.*}} vec(0,3)<0;1,0> vec(0,3)<0;1,0>
define spir_kernel void @test_fdiv_and_extract(
    <4 x float> %true_val, <4 x float> %false_val,
    <4 x float> %numerator, float %cond_val) {
entry:
  br label %loop

loop:
  %acc = phi float [ 0.000000e+00, %entry ], [ %sum, %loop ]

  %cmp = fcmp ogt float %cond_val, %acc
  %vec = select i1 %cmp, <4 x float> %true_val, <4 x float> %false_val

  ; Non-extract user: vector fdiv
  %vdiv = fdiv <4 x float> %numerator, %vec

  ; Extract users with downstream bitcast (noop-alias)
  %e0 = extractelement <4 x float> %vec, i32 0
  %e1 = extractelement <4 x float> %vec, i32 1
  %e2 = extractelement <4 x float> %vec, i32 2
  %e3 = extractelement <4 x float> %vec, i32 3

  %f0 = fmul float %e0, %e0
  %f1 = fmul float %e1, %e1
  %f2 = fmul float %e2, %e2
  %f3 = fmul float %e3, %e3

  %bc0 = bitcast float %e0 to i32
  %bc1 = bitcast float %e1 to i32
  %bc2 = bitcast float %e2 to i32
  %bc3 = bitcast float %e3 to i32

  %i0 = add i32 %bc0, 1
  %i1 = add i32 %bc1, 1
  %i2 = add i32 %bc2, 1
  %i3 = add i32 %bc3, 1

  ; Use fdiv results
  %de0 = extractelement <4 x float> %vdiv, i32 0
  %de1 = extractelement <4 x float> %vdiv, i32 1
  %de2 = extractelement <4 x float> %vdiv, i32 2
  %de3 = extractelement <4 x float> %vdiv, i32 3

  ; Combine into loop accumulator
  %ib0 = bitcast i32 %i0 to float
  %ib1 = bitcast i32 %i1 to float
  %ib2 = bitcast i32 %i2 to float
  %ib3 = bitcast i32 %i3 to float
  %s0 = fadd float %f0, %ib0
  %s1 = fadd float %f1, %ib1
  %s2 = fadd float %f2, %ib2
  %s3 = fadd float %f3, %ib3
  %t0 = fadd float %s0, %s1
  %t1 = fadd float %s2, %s3
  %u0 = fadd float %t0, %t1
  %u1 = fadd float %de0, %de1
  %u2 = fadd float %de2, %de3
  %u3 = fadd float %u1, %u2
  %sum = fadd float %u0, %u3

  %loop_cond = fcmp olt float %sum, 1.000000e+10
  br i1 %loop_cond, label %loop, label %exit

exit:
  ret void
}

; ── DPAS variant (non-uniform select operands; numerator stays as kernel arg) ─
; CHECK-LABEL: .kernel "test_fdiv_and_extract_dpas"
; CHECK-NOT: mov (M1, 16) {{.*}}(0,0)<1> vec(0,
; CHECK: div (M1, 16) vdiv(0,0)<1> numerator(0,0)<0;1,0> vec(0,0)<1;1,0>
; CHECK: add (M1, 16) i0(0,0)<1> vec_0v(0,0)<1;1,0>
; CHECK: add (M1, 16) i1(0,0)<1> vec_0v(1,0)<1;1,0>
; CHECK: add (M1, 16) i2(0,0)<1> vec_0v(2,0)<1;1,0>
; CHECK: add (M1, 16) i3(0,0)<1> vec_0v(3,0)<1;1,0>
; CHECK-NOT: mov (M1, 16) {{.*}}(0,0)<1> vec(0,
; CHECK: mad (M1, 16) {{.*}} vec(0,0)<1;1,0> vec(0,0)<1;1,0>
; CHECK: mad (M1, 16) {{.*}} vec(1,0)<1;1,0> vec(1,0)<1;1,0>
; CHECK: mad (M1, 16) {{.*}} vec(2,0)<1;1,0> vec(2,0)<1;1,0>
; CHECK: mad (M1, 16) {{.*}} vec(3,0)<1;1,0> vec(3,0)<1;1,0>
define spir_kernel void @test_fdiv_and_extract_dpas(
    <4 x float> %numerator, float %cond_val) {
entry:
  br label %loop

loop:
  %acc = phi float [ 0.000000e+00, %entry ], [ %sum, %loop ]
  %cmp = fcmp ogt float %cond_val, %acc
  %dpas_acc = insertelement <8 x float> zeroinitializer, float %acc, i32 0
  %dpas = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> %dpas_acc, <8 x i16> zeroinitializer, <8 x i32> zeroinitializer, i32 12, i32 12, i32 8, i32 8, i1 false)
  %d0 = extractelement <8 x float> %dpas, i32 0
  %d1 = extractelement <8 x float> %dpas, i32 1
  %d2 = extractelement <8 x float> %dpas, i32 2
  %d3 = extractelement <8 x float> %dpas, i32 3
  %d4 = extractelement <8 x float> %dpas, i32 4
  %d5 = extractelement <8 x float> %dpas, i32 5
  %d6 = extractelement <8 x float> %dpas, i32 6
  %d7 = extractelement <8 x float> %dpas, i32 7
  %tv0 = insertelement <4 x float> undef, float %d0, i32 0
  %tv1 = insertelement <4 x float> %tv0, float %d1, i32 1
  %tv2 = insertelement <4 x float> %tv1, float %d2, i32 2
  %true_val_v  = insertelement <4 x float> %tv2, float %d3, i32 3
  %fv0 = insertelement <4 x float> undef, float %d4, i32 0
  %fv1 = insertelement <4 x float> %fv0, float %d5, i32 1
  %fv2 = insertelement <4 x float> %fv1, float %d6, i32 2
  %false_val_v = insertelement <4 x float> %fv2, float %d7, i32 3
  %vec = select i1 %cmp, <4 x float> %true_val_v, <4 x float> %false_val_v

  %vdiv = fdiv <4 x float> %numerator, %vec

  %e0 = extractelement <4 x float> %vec, i32 0
  %e1 = extractelement <4 x float> %vec, i32 1
  %e2 = extractelement <4 x float> %vec, i32 2
  %e3 = extractelement <4 x float> %vec, i32 3

  %f0 = fmul float %e0, %e0
  %f1 = fmul float %e1, %e1
  %f2 = fmul float %e2, %e2
  %f3 = fmul float %e3, %e3

  %bc0 = bitcast float %e0 to i32
  %bc1 = bitcast float %e1 to i32
  %bc2 = bitcast float %e2 to i32
  %bc3 = bitcast float %e3 to i32

  %i0 = add i32 %bc0, 1
  %i1 = add i32 %bc1, 1
  %i2 = add i32 %bc2, 1
  %i3 = add i32 %bc3, 1

  %de0 = extractelement <4 x float> %vdiv, i32 0
  %de1 = extractelement <4 x float> %vdiv, i32 1
  %de2 = extractelement <4 x float> %vdiv, i32 2
  %de3 = extractelement <4 x float> %vdiv, i32 3

  %ib0 = bitcast i32 %i0 to float
  %ib1 = bitcast i32 %i1 to float
  %ib2 = bitcast i32 %i2 to float
  %ib3 = bitcast i32 %i3 to float
  %s0 = fadd float %f0, %ib0
  %s1 = fadd float %f1, %ib1
  %s2 = fadd float %f2, %ib2
  %s3 = fadd float %f3, %ib3
  %t0 = fadd float %s0, %s1
  %t1 = fadd float %s2, %s3
  %u0 = fadd float %t0, %t1
  %u1 = fadd float %de0, %de1
  %u2 = fadd float %de2, %de3
  %u3 = fadd float %u1, %u2
  %sum = fadd float %u0, %u3

  %loop_cond = fcmp olt float %sum, 1.000000e+10
  br i1 %loop_cond, label %loop, label %exit

exit:
  ret void
}

!igc.functions = !{!0, !20}
!IGCMetadata = !{!4}

!0 = !{ptr @test_fdiv_and_extract, !1}
!1 = !{!2, !3}
!2 = !{!"function_type", i32 0}
!3 = !{!"sub_group_size", i32 16}
!4 = !{!"ModuleMD", !5}
!5 = !{!"FuncMD", !6, !7, !11, !12}
!6 = !{!"FuncMDMap[0]", ptr @test_fdiv_and_extract}
!7 = !{!"FuncMDValue[0]", !8}
!8 = !{!"resAllocMD", !9}
!9 = !{!"argAllocMDList", !10}
!10 = !{!"argAllocMDListVec[0]"}
!11 = !{!"FuncMDMap[1]", ptr @test_fdiv_and_extract_dpas}
!12 = !{!"FuncMDValue[1]", !13}
!13 = !{!"resAllocMD", !14}
!14 = !{!"argAllocMDList", !15}
!15 = !{!"argAllocMDListVec[0]"}

!20 = !{ptr @test_fdiv_and_extract_dpas, !1}
